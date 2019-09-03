/*  Copyright (C) 2014-2019 FastoGT. All right reserved.
    This file is part of fastocloud.
    fastocloud is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    fastocloud is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with fastocloud.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "server/process_slave_wrapper.h"

#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <common/convert2string.h>
#include <common/file_system/file_system.h>
#include <common/file_system/string_path_utils.h>
#include <common/net/http_client.h>
#include <common/net/net.h>

#include "base/config_fields.h"
#include "base/constants.h"
#include "base/inputs_outputs.h"
#include "base/utils.h"

#include "gpu_stats/perf_monitor.h"

#include "server/child_stream.h"
#include "server/daemon/client.h"
#include "server/daemon/commands.h"
#include "server/daemon/commands_info/service/activate_info.h"
#include "server/daemon/commands_info/service/get_log_info.h"
#include "server/daemon/commands_info/service/prepare_info.h"
#include "server/daemon/commands_info/service/server_info.h"
#include "server/daemon/commands_info/service/stop_info.h"
#include "server/daemon/commands_info/service/sync_info.h"
#include "server/daemon/commands_info/stream/get_log_info.h"
#include "server/daemon/commands_info/stream/restart_info.h"
#include "server/daemon/commands_info/stream/start_info.h"
#include "server/daemon/commands_info/stream/stop_info.h"
#include "server/daemon/server.h"
#include "server/http/handler.h"
#include "server/http/server.h"
#include "server/options/options.h"
#include "server/vods/handler.h"
#include "server/vods/server.h"

#include "stream_commands/commands.h"

#include "utils/m3u8_reader.h"

#if defined(OS_WIN)
#undef SetPort
#endif

namespace {

bool GetHttpHostAndPort(const std::string& host, common::net::HostAndPort* out) {
  if (host.empty() || !out) {
    return false;
  }

  common::net::HostAndPort http_server;
  size_t del = host.find_last_of(':');
  if (del != std::string::npos) {
    http_server.SetHost(host.substr(0, del));
    std::string port_str = host.substr(del + 1);
    uint16_t lport;
    if (common::ConvertFromString(port_str, &lport)) {
      http_server.SetPort(lport);
    }
  } else {
    http_server.SetHost(host);
    http_server.SetPort(80);
  }
  *out = http_server;
  return true;
}

bool GetPostServerFromUrl(const common::uri::Url& url, common::net::HostAndPort* out) {
  if (!url.IsValid() || !out) {
    return false;
  }

  const std::string host_str = url.GetHost();
  return GetHttpHostAndPort(host_str, out);
}

common::Optional<common::file_system::ascii_file_string_path> MakeStreamLogPath(const std::string& feedback_dir) {
  common::file_system::ascii_directory_string_path dir(feedback_dir);
  return dir.MakeFileStringPath(LOGS_FILE_NAME);
}

common::Optional<common::file_system::ascii_file_string_path> MakeStreamPipelinePath(const std::string& feedback_dir) {
  common::file_system::ascii_directory_string_path dir(feedback_dir);
  return dir.MakeFileStringPath(DUMP_FILE_NAME);
}

common::Error PostHttpFile(const common::file_system::ascii_file_string_path& file_path, const common::uri::Url& url) {
  common::net::HostAndPort http_server_address;
  if (!GetPostServerFromUrl(url, &http_server_address)) {
    return common::make_error_inval();
  }

  common::net::HttpClient cl(http_server_address);
  common::ErrnoError errn = cl.Connect();
  if (errn) {
    return common::make_error_from_errno(errn);
  }

  const auto path = url.GetPath();
  common::Error err = cl.PostFile(path, file_path);
  if (err) {
    cl.Disconnect();
    return err;
  }

  common::http::HttpResponse lresp;
  err = cl.ReadResponse(&lresp);
  if (err) {
    cl.Disconnect();
    return err;
  }

  if (lresp.IsEmptyBody()) {
    cl.Disconnect();
    return common::make_error("Empty body");
  }

  cl.Disconnect();
  return common::Error();
}

}  // namespace

namespace fastocloud {
namespace server {
namespace {
typedef VodsHandler CodsHandler;
typedef VodsServer CodsServer;

bool CheckIsFullVod(const common::file_system::ascii_file_string_path& file) {
  utils::M3u8Reader reader;
  if (!reader.Parse(file)) {
    return false;
  }

  common::file_system::ascii_directory_string_path dir(file.GetDirectory());
  const auto chunks = reader.GetChunks();
  for (const utils::ChunkInfo& chunk : chunks) {
    const auto chunk_path = dir.MakeFileStringPath(chunk.path);
    if (!chunk_path) {
      return false;
    }

    if (!common::file_system::is_file_exist(chunk_path->GetPath())) {
      return false;
    }
  }

  return true;
}
}  // namespace

struct ProcessSlaveWrapper::NodeStats {
  NodeStats() : prev(), prev_nshot(), gpu_load(0), timestamp(common::time::current_utc_mstime()) {}

  service::CpuShot prev;
  service::NetShot prev_nshot;
  int gpu_load;
  fastotv::timestamp_t timestamp;
};

ProcessSlaveWrapper::ProcessSlaveWrapper(const std::string& license_key, const Config& config)
    : config_(config),
      license_key_(license_key),
      process_argc_(0),
      process_argv_(nullptr),
      loop_(nullptr),
      http_server_(nullptr),
      http_handler_(nullptr),
      vods_server_(nullptr),
      vods_handler_(nullptr),
      cods_server_(nullptr),
      cods_handler_(nullptr),
      ping_client_timer_(INVALID_TIMER_ID),
      node_stats_timer_(INVALID_TIMER_ID),
      cleanup_files_timer_(INVALID_TIMER_ID),
      quit_cleanup_timer_(INVALID_TIMER_ID),
      node_stats_(new NodeStats),
      vods_links_(),
      cods_links_() {
  loop_ = new DaemonServer(config.host, this);
  loop_->SetName("client_server");

  http_handler_ = new HttpHandler(this);
  http_server_ = new HttpServer(config.http_host, http_handler_);
  http_server_->SetName("http_server");

  vods_handler_ = new VodsHandler(this);
  vods_server_ = new VodsServer(config.vods_host, vods_handler_);
  vods_server_->SetName("vods_server");

  cods_handler_ = new CodsHandler(this);
  cods_server_ = new CodsServer(config.cods_host, cods_handler_);
  cods_server_->SetName("cods_server");
}

int ProcessSlaveWrapper::SendStopDaemonRequest(const std::string& license) {
  if (license.empty()) {
    return EXIT_FAILURE;
  }

  const common::net::HostAndPort host = Config::GetDefaultHost();
  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(host, common::net::ST_SOCK_STREAM, nullptr, &client_info);
  if (err) {
    return EXIT_FAILURE;
  }

  std::unique_ptr<ProtocoledDaemonClient> connection(new ProtocoledDaemonClient(nullptr, client_info));
  err = connection->StopMe(license);
  ignore_result(connection->Close());
  if (err) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

ProcessSlaveWrapper::~ProcessSlaveWrapper() {
  destroy(&cods_server_);
  destroy(&cods_handler_);
  destroy(&vods_server_);
  destroy(&vods_handler_);
  destroy(&http_server_);
  destroy(&http_handler_);
  destroy(&loop_);
  destroy(&node_stats_);
}

int ProcessSlaveWrapper::Exec(int argc, char** argv) {
  process_argc_ = argc;
  process_argv_ = argv;

  // gpu statistic monitor
  std::thread perf_thread;
  gpu_stats::IPerfMonitor* perf_monitor = gpu_stats::CreatePerfMonitor(&node_stats_->gpu_load);
  if (perf_monitor) {
    perf_thread = std::thread([perf_monitor] { perf_monitor->Exec(); });
  }

  HttpServer* http_server = static_cast<HttpServer*>(http_server_);
  std::thread http_thread = std::thread([http_server] {
    common::ErrnoError err = http_server->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    err = http_server->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    int res = http_server->Exec();
    UNUSED(res);
  });

  VodsServer* vods_server = static_cast<VodsServer*>(vods_server_);
  std::thread vods_thread = std::thread([vods_server] {
    common::ErrnoError err = vods_server->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    err = vods_server->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    int res = vods_server->Exec();
    UNUSED(res);
  });

  CodsServer* cods_server = static_cast<CodsServer*>(cods_server_);
  std::thread cods_thread = std::thread([cods_server] {
    common::ErrnoError err = cods_server->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    err = cods_server->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    int res = cods_server->Exec();
    UNUSED(res);
  });

  int res = EXIT_FAILURE;
  DaemonServer* server = static_cast<DaemonServer*>(loop_);
  common::ErrnoError err = server->Bind(true);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    goto finished;
  }

  err = server->Listen(5);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    goto finished;
  }

  node_stats_->prev = service::GetMachineCpuShot();
  node_stats_->prev_nshot = service::GetMachineNetShot();
  node_stats_->timestamp = common::time::current_utc_mstime();

  res = server->Exec();

finished:
  vods_thread.join();
  cods_thread.join();
  http_thread.join();
  if (perf_monitor) {
    perf_monitor->Stop();
  }
  if (perf_thread.joinable()) {
    perf_thread.join();
  }
  delete perf_monitor;
  return res;
}

void ProcessSlaveWrapper::PreLooped(common::libev::IoLoop* server) {
  ping_client_timer_ = server->CreateTimer(ping_timeout_clients_seconds, true);
  node_stats_timer_ = server->CreateTimer(node_stats_send_seconds, true);
  cleanup_files_timer_ = server->CreateTimer(config_.ttl_files, true);
}

void ProcessSlaveWrapper::Accepted(common::libev::IoClient* client) {
  UNUSED(client);
}

void ProcessSlaveWrapper::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(server);
  UNUSED(client);
}

void ProcessSlaveWrapper::Closed(common::libev::IoClient* client) {
  UNUSED(client);
}

void ProcessSlaveWrapper::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  if (ping_client_timer_ == id) {
    std::vector<common::libev::IoClient*> online_clients = server->GetClients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      common::libev::IoClient* client = online_clients[i];
      ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(client);
      if (dclient && dclient->IsVerified()) {
        common::ErrnoError err = dclient->Ping();
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
          ignore_result(dclient->Close());
          delete dclient;
        } else {
          INFO_LOG() << "Sent ping to client[" << client->GetFormatedName() << "], from server["
                     << server->GetFormatedName() << "], " << online_clients.size() << " client(s) connected.";
        }
      }
    }
  } else if (node_stats_timer_ == id) {
    const std::string node_stats = MakeServiceStats(false);
    fastotv::protocol::request_t req;
    common::Error err_ser = StatisitcServiceBroadcast(node_stats, &req);
    if (err_ser) {
      return;
    }

    BroadcastClients(req);
  } else if (cleanup_files_timer_ == id) {
    for (auto it = vods_links_.begin(); it != vods_links_.end(); ++it) {
      RemoveFilesByExtension((*it).first, CHUNK_EXT);
    }
  } else if (quit_cleanup_timer_ == id) {
    vods_server_->Stop();
    cods_server_->Stop();
    http_server_->Stop();
    loop_->Stop();
  }
}

void ProcessSlaveWrapper::Accepted(common::libev::IoChild* child) {
  UNUSED(child);
}

void ProcessSlaveWrapper::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  UNUSED(server);
  UNUSED(child);
}

void ProcessSlaveWrapper::ChildStatusChanged(common::libev::IoChild* child, int status, int signal) {
  ChildStream* channel = static_cast<ChildStream*>(child);
  const auto sid = channel->GetStreamID();

  INFO_LOG() << "Successful finished children id: " << sid << "\nStream id: " << sid
             << ", exit with status: " << (status ? "FAILURE" : "SUCCESS") << ", signal: " << signal;

  loop_->UnRegisterChild(child);

  delete channel;

  stream::QuitStatusInfo ch_status_info(sid, status, signal);
  fastotv::protocol::request_t req;
  common::Error err_ser = QuitStatusStreamBroadcast(ch_status_info, &req);
  if (err_ser) {
    return;
  }

  BroadcastClients(req);
}

Child* ProcessSlaveWrapper::FindChildByID(stream_id_t cid) const {
  DaemonServer* server = static_cast<DaemonServer*>(loop_);
  auto childs = server->GetChilds();
  for (auto* child : childs) {
    Child* channel = static_cast<Child*>(child);
    if (channel->GetStreamID() == cid) {
      return channel;
    }
  }

  return nullptr;
}

void ProcessSlaveWrapper::BroadcastClients(const fastotv::protocol::request_t& req) {
  std::vector<common::libev::IoClient*> clients = loop_->GetClients();
  for (size_t i = 0; i < clients.size(); ++i) {
    ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(clients[i]);
    if (dclient && dclient->IsVerified()) {
      common::ErrnoError err = dclient->WriteRequest(req);
      if (err) {
        WARNING_LOG() << "BroadcastClients error: " << err->GetDescription();
      }
    }
  }
}

common::ErrnoError ProcessSlaveWrapper::DaemonDataReceived(ProtocoledDaemonClient* dclient) {
  CHECK(loop_->IsLoopThread());
  std::string input_command;
  common::ErrnoError err = dclient->ReadCommand(&input_command);
  if (err) {
    return err;  // i don't want handle spam, comand must be foramated according
                 // protocol
  }

  fastotv::protocol::request_t* req = nullptr;
  fastotv::protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    DEBUG_LOG() << "Received daemon request: " << input_command;
    err = HandleRequestServiceCommand(dclient, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    DEBUG_LOG() << "Received daemon responce: " << input_command;
    err = HandleResponceServiceCommand(dclient, resp);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete resp;
  } else {
    DNOTREACHED();
    return common::make_errno_error("Invalid command type.", EINVAL);
  }

  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::StreamDataReceived(stream_client_t* pipe_client) {
  CHECK(loop_->IsLoopThread());
  std::string input_command;
  common::ErrnoError err = pipe_client->ReadCommand(&input_command);
  if (err) {
    return err;  // i don't want to handle spam, command must be formated according protocol
  }

  fastotv::protocol::request_t* req = nullptr;
  fastotv::protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    INFO_LOG() << "Received stream responce: " << input_command;
    err = HandleRequestStreamsCommand(pipe_client, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    INFO_LOG() << "Received stream responce: " << input_command;
    err = HandleResponceStreamsCommand(pipe_client, resp);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete resp;
  } else {
    NOTREACHED();
    return common::make_errno_error("Invalid command type.", EINVAL);
  }
  return common::ErrnoError();
}

void ProcessSlaveWrapper::DataReceived(common::libev::IoClient* client) {
  if (ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(client)) {
    common::ErrnoError err = DaemonDataReceived(dclient);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      ignore_result(dclient->Close());
      delete dclient;
    }
  } else if (stream_client_t* pipe_client = dynamic_cast<stream_client_t*>(client)) {
    common::ErrnoError err = StreamDataReceived(pipe_client);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      DaemonServer* server = static_cast<DaemonServer*>(loop_);
      auto childs = server->GetChilds();
      for (auto* child : childs) {
        ChildStream* channel = static_cast<ChildStream*>(child);
        if (pipe_client == channel->GetClient()) {
          channel->SetClient(nullptr);
          break;
        }
      }

      ignore_result(pipe_client->Close());
      delete pipe_client;
    }
  } else {
    NOTREACHED();
  }
}

void ProcessSlaveWrapper::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);
}

void ProcessSlaveWrapper::PostLooped(common::libev::IoLoop* server) {
  if (cleanup_files_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(cleanup_files_timer_);
    cleanup_files_timer_ = INVALID_TIMER_ID;
  }

  if (ping_client_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(ping_client_timer_);
    ping_client_timer_ = INVALID_TIMER_ID;
  }

  if (node_stats_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(node_stats_timer_);
    node_stats_timer_ = INVALID_TIMER_ID;
  }
}

void ProcessSlaveWrapper::OnHttpRequest(common::libev::http::HttpClient* client, const file_path_t& file) {
  if (client->GetServer() == vods_server_) {
    const std::string ext = file.GetExtension();
    if (common::EqualsASCII(ext, M3U8_EXTENSION, false)) {
      loop_->ExecInLoopThread([this, file]() {
        const common::file_system::ascii_directory_string_path http_root(file.GetDirectory());
        auto it = vods_links_.find(http_root);
        if (it == vods_links_.end()) {
          return;
        }

        bool is_full_vod = CheckIsFullVod(file);
        if (!is_full_vod) {
          const serialized_stream_t config = it->second;
          CreateChildStream(config);
        }
      });
    }
  } else if (client->GetServer() == cods_server_) {
    const std::string ext = file.GetExtension();
    if (common::EqualsASCII(ext, M3U8_EXTENSION, false)) {
      loop_->ExecInLoopThread([this, file]() {
        const common::file_system::ascii_directory_string_path http_root(file.GetDirectory());
        auto it = cods_links_.find(http_root);
        if (it == cods_links_.end()) {
          return;
        }

        const serialized_stream_t config = it->second;
        CreateChildStream(config);
      });

      if (!common::file_system::is_file_exist(file.GetPath())) {
        sleep(5);  // may be fix :)
      }
    }
  }
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientStopService(ProtocoledDaemonClient* dclient,
                                                                       fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    service::StopInfo stop_info;
    common::Error err_des = stop_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    bool is_verified_request = stop_info.GetLicense() == license_key_ || dclient->IsVerified();
    if (!is_verified_request) {
      return common::make_errno_error_inval();
    }

    if (quit_cleanup_timer_ != INVALID_TIMER_ID) {
      // in progress
      return dclient->StopFail(req->id, common::make_error("Stop service in progress..."));
    }

    DaemonServer* server = static_cast<DaemonServer*>(loop_);
    auto childs = server->GetChilds();
    for (auto* child : childs) {
      ChildStream* channel = static_cast<ChildStream*>(child);
      ignore_result(channel->Stop());
    }

    quit_cleanup_timer_ = loop_->CreateTimer(cleanup_seconds, false);
    return dclient->StopSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponcePingService(ProtocoledDaemonClient* dclient,
                                                                  fastotv::protocol::response_t* resp) {
  UNUSED(dclient);
  CHECK(loop_->IsLoopThread());
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_ping = json_tokener_parse(params_ptr);
    if (!jclient_ping) {
      return common::make_errno_error_inval();
    }

    service::ClientPingInfo client_ping_info;
    common::Error err_des = client_ping_info.DeSerialize(jclient_ping);
    json_object_put(jclient_ping);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::CreateChildStream(const serialized_stream_t& config_args) {
  CHECK(loop_->IsLoopThread());
  common::ErrnoError err = options::ValidateConfig(config_args);
  if (err) {
    return err;
  }

  StreamInfo sha;
  std::string feedback_dir;
  common::logging::LOG_LEVEL logs_level;
  err = MakeStreamInfo(config_args, true, &sha, &feedback_dir, &logs_level);
  if (err) {
    return err;
  }

  Child* stream = FindChildByID(sha.id);
  if (stream) {
    NOTICE_LOG() << "Skip request to start stream id: " << sha.id;
    return common::make_errno_error(common::MemSPrintf("Stream with id: %s exist, skip request.", sha.id), EINVAL);
  }

  config_args->Insert(STREAM_LINK_PATH, common::Value::CreateStringValueFromBasicString(config_.streamlink_path));
  return CreateChildStreamImpl(config_args, sha.id);
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestChangedSourcesStream(stream_client_t* pclient,
                                                                          fastotv::protocol::request_t* req) {
  UNUSED(pclient);
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrequest_changed_sources = json_tokener_parse(params_ptr);
    if (!jrequest_changed_sources) {
      return common::make_errno_error_inval();
    }

    ChangedSouresInfo ch_sources_info;
    common::Error err_des = ch_sources_info.DeSerialize(jrequest_changed_sources);
    json_object_put(jrequest_changed_sources);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    fastotv::protocol::request_t req;
    common::Error err_ser = ChangedSourcesStreamBroadcast(ch_sources_info, &req);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    BroadcastClients(req);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestStatisticStream(stream_client_t* pclient,
                                                                     fastotv::protocol::request_t* req) {
  UNUSED(pclient);
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrequest_stat = json_tokener_parse(params_ptr);
    if (!jrequest_stat) {
      return common::make_errno_error_inval();
    }

    StatisticInfo stat;
    common::Error err_des = stat.DeSerialize(jrequest_stat);
    json_object_put(jrequest_stat);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    fastotv::protocol::request_t req;
    common::Error err_ser = StatisitcStreamBroadcast(stat, &req);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    BroadcastClients(req);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientStartStream(ProtocoledDaemonClient* dclient,
                                                                       fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstart_info = json_tokener_parse(params_ptr);
    if (!jstart_info) {
      return common::make_errno_error_inval();
    }

    stream::StartInfo start_info;
    common::Error err_des = start_info.DeSerialize(jstart_info);
    json_object_put(jstart_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    common::ErrnoError err = CreateChildStream(start_info.GetConfig());
    if (err) {
      ignore_result(dclient->StartStreamFail(req->id, common::make_error_from_errno(err)));
      return err;
    }

    return dclient->StartStreamSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientStopStream(ProtocoledDaemonClient* dclient,
                                                                      fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop_info = json_tokener_parse(params_ptr);
    if (!jstop_info) {
      return common::make_errno_error_inval();
    }

    stream::StopInfo stop_info;
    common::Error err_des = stop_info.DeSerialize(jstop_info);
    json_object_put(jstop_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    Child* chan = FindChildByID(stop_info.GetStreamID());
    if (!chan) {
      return dclient->StopFail(req->id, common::make_error("Stream not found"));
    }

    ignore_result(chan->Stop());
    return dclient->StopStreamSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientRestartStream(ProtocoledDaemonClient* dclient,
                                                                         fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrestart_info = json_tokener_parse(params_ptr);
    if (!jrestart_info) {
      return common::make_errno_error_inval();
    }

    stream::RestartInfo restart_info;
    common::Error err_des = restart_info.DeSerialize(jrestart_info);
    json_object_put(jrestart_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    Child* chan = FindChildByID(restart_info.GetStreamID());
    if (!chan) {
      return dclient->ReStartStreamFail(req->id, common::make_error("Stream not found"));
    }

    ignore_result(chan->Restart());
    return dclient->ReStartStreamSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientGetLogStream(ProtocoledDaemonClient* dclient,
                                                                        fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jgetlog_info = json_tokener_parse(params_ptr);
    if (!jgetlog_info) {
      return common::make_errno_error_inval();
    }

    stream::GetLogInfo log_info;
    common::Error err_des = log_info.DeSerialize(jgetlog_info);
    json_object_put(jgetlog_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    const auto remote_log_path = log_info.GetLogPath();
    if (remote_log_path.GetScheme() == common::uri::Url::http) {
      const auto stream_log_file = MakeStreamLogPath(log_info.GetFeedbackDir());
      if (stream_log_file) {
        PostHttpFile(*stream_log_file, remote_log_path);
      }
    } else if (remote_log_path.GetScheme() == common::uri::Url::https) {
    }
    return dclient->GetLogStreamSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientGetPipelineStream(ProtocoledDaemonClient* dclient,
                                                                             fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jgetpipe_info = json_tokener_parse(params_ptr);
    if (!jgetpipe_info) {
      return common::make_errno_error_inval();
    }

    stream::GetLogInfo pipeline_info;
    common::Error err_des = pipeline_info.DeSerialize(jgetpipe_info);
    json_object_put(jgetpipe_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    const auto remote_log_path = pipeline_info.GetLogPath();
    if (remote_log_path.GetScheme() == common::uri::Url::http) {
      const auto stream_log_file = MakeStreamPipelinePath(pipeline_info.GetFeedbackDir());
      if (stream_log_file) {
        PostHttpFile(*stream_log_file, remote_log_path);
      }
    } else if (remote_log_path.GetScheme() == common::uri::Url::https) {
    }
    return dclient->GetLogStreamSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientPrepareService(ProtocoledDaemonClient* dclient,
                                                                          fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jservice_state = json_tokener_parse(params_ptr);
    if (!jservice_state) {
      return common::make_errno_error_inval();
    }

    service::PrepareInfo state_info;
    common::Error err_des = state_info.DeSerialize(jservice_state);
    json_object_put(jservice_state);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    const auto http_root = HttpHandler::http_directory_path_t(state_info.GetHlsDirectory());
    static_cast<HttpHandler*>(http_handler_)->SetHttpRoot(http_root);

    const auto vods_root = VodsHandler::http_directory_path_t(state_info.GetVodsDirectory());
    static_cast<VodsHandler*>(vods_handler_)->SetHttpRoot(vods_root);

    const auto cods_root = CodsHandler::http_directory_path_t(state_info.GetCodsDirectory());
    static_cast<CodsHandler*>(cods_handler_)->SetHttpRoot(cods_root);

    service::Directories dirs(state_info);
    std::string resp_str = service::MakeDirectoryResponce(dirs);
    return dclient->StateServiceSuccess(req->id, resp_str);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientSyncService(ProtocoledDaemonClient* dclient,
                                                                       fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jservice_state = json_tokener_parse(params_ptr);
    if (!jservice_state) {
      return common::make_errno_error_inval();
    }

    service::SyncInfo sync_info;
    common::Error err_des = sync_info.DeSerialize(jservice_state);
    json_object_put(jservice_state);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    // refresh vods
    vods_links_.clear();
    cods_links_.clear();
    for (StreamConfig config : sync_info.GetStreams()) {
      AddStreamLine(config);
    }

    return dclient->SyncServiceSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

void ProcessSlaveWrapper::AddStreamLine(const serialized_stream_t& config_args) {
  CHECK(loop_->IsLoopThread());
  common::ErrnoError err = options::ValidateConfig(config_args);
  if (err) {
    return;
  }

  StreamInfo sha;
  std::string feedback_dir;
  common::logging::LOG_LEVEL logs_level;
  err = MakeStreamInfo(config_args, false, &sha, &feedback_dir, &logs_level);
  if (err) {
    return;
  }

  if (sha.type == VOD_ENCODE || sha.type == VOD_RELAY) {
    output_t output;
    if (read_output(config_args, &output)) {
      for (const OutputUri& out_uri : output) {
        common::uri::Url ouri = out_uri.GetOutput();
        if (ouri.GetScheme() == common::uri::Url::http) {
          const common::file_system::ascii_directory_string_path http_root = out_uri.GetHttpRoot();
          config_args->Insert(CLEANUP_TS_FIELD, common::Value::CreateBooleanValue(false));
          vods_links_[http_root] = config_args;
        }
      }
    }
  } else if (sha.type == COD_ENCODE || sha.type == COD_RELAY) {
    output_t output;
    if (read_output(config_args, &output)) {
      for (const OutputUri& out_uri : output) {
        common::uri::Url ouri = out_uri.GetOutput();
        if (ouri.GetScheme() == common::uri::Url::http) {
          const common::file_system::ascii_directory_string_path http_root = out_uri.GetHttpRoot();
          cods_links_[http_root] = config_args;
        }
      }
    }
  }
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientActivate(ProtocoledDaemonClient* dclient,
                                                                    fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jactivate = json_tokener_parse(params_ptr);
    if (!jactivate) {
      return common::make_errno_error_inval();
    }

    service::ActivateInfo activate_info;
    common::Error err_des = activate_info.DeSerialize(jactivate);
    json_object_put(jactivate);
    if (err_des) {
      ignore_result(dclient->ActivateFail(req->id, err_des));
      return common::make_errno_error(err_des->GetDescription(), EAGAIN);
    }

    bool is_active = activate_info.GetLicense() == license_key_;
    if (!is_active) {
      common::Error err = common::make_error("Wrong license key");
      ignore_result(dclient->ActivateFail(req->id, err));
      return common::make_errno_error(err->GetDescription(), EINVAL);
    }

    const std::string node_stats = MakeServiceStats(true);
    common::ErrnoError err_ser = dclient->ActivateSuccess(req->id, node_stats);
    if (err_ser) {
      return err_ser;
    }

    dclient->SetVerified(true);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientPingService(ProtocoledDaemonClient* dclient,
                                                                       fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    service::ClientPingInfo client_ping_info;
    common::Error err_des = client_ping_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    return dclient->Pong(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientGetLogService(ProtocoledDaemonClient* dclient,
                                                                         fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jlog = json_tokener_parse(params_ptr);
    if (!jlog) {
      return common::make_errno_error_inval();
    }

    service::GetLogInfo get_log_info;
    common::Error err_des = get_log_info.DeSerialize(jlog);
    json_object_put(jlog);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    const auto remote_log_path = get_log_info.GetLogPath();
    if (remote_log_path.GetScheme() == common::uri::Url::http) {
      PostHttpFile(common::file_system::ascii_file_string_path(config_.log_path), remote_log_path);
    } else if (remote_log_path.GetScheme() == common::uri::Url::https) {
    }

    return dclient->GetLogServiceSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestServiceCommand(ProtocoledDaemonClient* dclient,
                                                                    fastotv::protocol::request_t* req) {
  if (req->method == DAEMON_START_STREAM) {
    return HandleRequestClientStartStream(dclient, req);
  } else if (req->method == DAEMON_STOP_STREAM) {
    return HandleRequestClientStopStream(dclient, req);
  } else if (req->method == DAEMON_RESTART_STREAM) {
    return HandleRequestClientRestartStream(dclient, req);
  } else if (req->method == DAEMON_GET_LOG_STREAM) {
    return HandleRequestClientGetLogStream(dclient, req);
  } else if (req->method == DAEMON_GET_PIPELINE_STREAM) {
    return HandleRequestClientGetPipelineStream(dclient, req);
  } else if (req->method == DAEMON_PREPARE_SERVICE) {
    return HandleRequestClientPrepareService(dclient, req);
  } else if (req->method == DAEMON_SYNC_SERVICE) {
    return HandleRequestClientSyncService(dclient, req);
  } else if (req->method == DAEMON_STOP_SERVICE) {
    return HandleRequestClientStopService(dclient, req);
  } else if (req->method == DAEMON_ACTIVATE) {
    return HandleRequestClientActivate(dclient, req);
  } else if (req->method == DAEMON_PING_SERVICE) {
    return HandleRequestClientPingService(dclient, req);
  } else if (req->method == DAEMON_GET_LOG_SERVICE) {
    return HandleRequestClientGetLogService(dclient, req);
  }

  WARNING_LOG() << "Received unknown method: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponceServiceCommand(ProtocoledDaemonClient* dclient,
                                                                     fastotv::protocol::response_t* resp) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  fastotv::protocol::request_t req;
  if (dclient->PopRequestByID(resp->id, &req)) {
    if (req.method == DAEMON_SERVER_PING) {
      ignore_result(HandleResponcePingService(dclient, resp));
    } else {
      WARNING_LOG() << "HandleResponceServiceCommand not handled command: " << req.method;
    }
  }

  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestStreamsCommand(stream_client_t* pclient,
                                                                    fastotv::protocol::request_t* req) {
  if (req->method == CHANGED_SOURCES_STREAM) {
    return HandleRequestChangedSourcesStream(pclient, req);
  } else if (req->method == STATISTIC_STREAM) {
    return HandleRequestStatisticStream(pclient, req);
  }

  WARNING_LOG() << "Received unknown command: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponceStreamsCommand(stream_client_t* pclient,
                                                                     fastotv::protocol::response_t* resp) {
  fastotv::protocol::request_t req;
  if (pclient->PopRequestByID(resp->id, &req)) {
    if (req.method == STOP_STREAM) {
    } else if (req.method == RESTART_STREAM) {
    } else {
      WARNING_LOG() << "HandleResponceStreamsCommand not handled command: " << req.method;
    }
  }
  return common::ErrnoError();
}

std::string ProcessSlaveWrapper::MakeServiceStats(bool full_stat) const {
  service::CpuShot next = service::GetMachineCpuShot();
  double cpu_load = service::GetCpuMachineLoad(node_stats_->prev, next);
  node_stats_->prev = next;

  service::NetShot next_nshot = service::GetMachineNetShot();
  uint64_t bytes_recv = (next_nshot.bytes_recv - node_stats_->prev_nshot.bytes_recv);
  uint64_t bytes_send = (next_nshot.bytes_send - node_stats_->prev_nshot.bytes_send);
  node_stats_->prev_nshot = next_nshot;

  service::MemoryShot mem_shot = service::GetMachineMemoryShot();
  service::HddShot hdd_shot = service::GetMachineHddShot();
  service::SysinfoShot sshot = service::GetMachineSysinfoShot();
  std::string uptime_str = common::MemSPrintf("%lu %lu %lu", sshot.loads[0], sshot.loads[1], sshot.loads[2]);
  fastotv::timestamp_t current_time = common::time::current_utc_mstime();
  fastotv::timestamp_t ts_diff = (current_time - node_stats_->timestamp) / 1000;
  if (ts_diff == 0) {
    ts_diff = 1;  // divide by zero
  }
  node_stats_->timestamp = current_time;

  size_t daemons_client_count = 0;
  std::vector<common::libev::IoClient*> clients = loop_->GetClients();
  for (size_t i = 0; i < clients.size(); ++i) {
    ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(clients[i]);
    if (dclient && dclient->IsVerified()) {
      daemons_client_count++;
    }
  }
  service::OnlineUsers online(daemons_client_count, static_cast<HttpHandler*>(http_handler_)->GetOnlineClients(),
                              static_cast<HttpHandler*>(vods_handler_)->GetOnlineClients(),
                              static_cast<HttpHandler*>(cods_handler_)->GetOnlineClients());
  service::ServerInfo stat(cpu_load, node_stats_->gpu_load, uptime_str, mem_shot, hdd_shot, bytes_recv / ts_diff,
                           bytes_send / ts_diff, sshot, current_time, online);

  std::string node_stats;
  if (full_stat) {
    service::FullServiceInfo fstat(config_.http_host, config_.vods_host, config_.cods_host, stat);
    common::Error err_ser = fstat.SerializeToString(&node_stats);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      WARNING_LOG() << "Failed to generate node full statistic: " << err_str;
    }
  } else {
    common::Error err_ser = stat.SerializeToString(&node_stats);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      WARNING_LOG() << "Failed to generate node statistic: " << err_str;
    }
  }
  return node_stats;
}

}  // namespace server
}  // namespace fastocloud
