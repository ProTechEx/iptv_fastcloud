/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#include "server/subscribers/handler.h"

#include <common/libev/io_loop.h>  // for IoLoop

#include <fastotv/commands/commands.h>
#include <fastotv/commands_info/client_info.h>

#include "server/subscribers/client.h"
#include "server/subscribers/commands_info/user_info.h"
#include "server/subscribers/isubscribe_finder.h"

namespace fastocloud {
namespace server {
namespace subscribers {

SubscribersHandler::SubscribersHandler(ISubscribeFinder* finder, const common::net::HostAndPort& bandwidth_host)
    : base_class(),
      finder_(finder),
      ping_client_id_timer_(INVALID_TIMER_ID),
      bandwidth_host_(bandwidth_host),
      connections_() {}

SubscribersHandler::~SubscribersHandler() {}

void SubscribersHandler::PreLooped(common::libev::IoLoop* server) {
  ping_client_id_timer_ = server->CreateTimer(ping_timeout_clients, true);
  base_class::PostLooped(server);
}

void SubscribersHandler::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  base_class::Moved(server, client);
}

void SubscribersHandler::PostLooped(common::libev::IoLoop* server) {
  if (ping_client_id_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(ping_client_id_timer_);
    ping_client_id_timer_ = INVALID_TIMER_ID;
  }
  base_class::PostLooped(server);
}

void SubscribersHandler::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  if (ping_client_id_timer_ == id) {
    std::vector<common::libev::IoClient*> online_clients = server->GetClients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      common::libev::IoClient* client = online_clients[i];
      SubscriberClient* iclient = static_cast<SubscriberClient*>(client);
      if (iclient) {
        common::ErrnoError err = iclient->Ping();
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
          ignore_result(client->Close());
          delete client;
        } else {
          INFO_LOG() << "Sent ping to client[" << client->GetFormatedName() << "], from server["
                     << server->GetFormatedName() << "], " << online_clients.size() << " client(s) connected.";
        }
      }
    }
  }
  base_class::TimerEmited(server, id);
}

void SubscribersHandler::Accepted(common::libev::IoChild* child) {
  base_class::Accepted(child);
}

void SubscribersHandler::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  base_class::Moved(server, child);
}

void SubscribersHandler::ChildStatusChanged(common::libev::IoChild* client, int status, int signal) {
  base_class::ChildStatusChanged(client, status, signal);
}

void SubscribersHandler::Accepted(common::libev::IoClient* client) {
  base_class::Accepted(client);
}

void SubscribersHandler::Closed(common::libev::IoClient* client) {
  SubscriberClient* iclient = static_cast<SubscriberClient*>(client);
  const ServerAuthInfo server_user_auth = iclient->GetServerHostInfo();
  common::Error unreg_err = UnRegisterInnerConnectionByHost(iclient);
  if (unreg_err) {
    return base_class::Closed(client);
  }

  INFO_LOG() << "Byu registered user: " << server_user_auth.GetLogin();
  base_class::Closed(client);
}

void SubscribersHandler::DataReceived(common::libev::IoClient* client) {
  std::string buff;
  SubscriberClient* iclient = static_cast<SubscriberClient*>(client);
  common::ErrnoError err = iclient->ReadCommand(&buff);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    ignore_result(client->Close());
    delete client;
    return base_class::DataReceived(client);
  }

  HandleInnerDataReceived(iclient, buff);
  base_class::DataReceived(client);
}

void SubscribersHandler::DataReadyToWrite(common::libev::IoClient* client) {
  base_class::DataReadyToWrite(client);
}

common::Error SubscribersHandler::RegisterInnerConnectionByHost(const ServerAuthInfo& info, SubscriberClient* client) {
  CHECK(info.IsValid());
  if (!client) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  client->SetServerHostInfo(info);
  connections_[info.GetUserID()].push_back(client);
  return common::Error();
}

common::Error SubscribersHandler::UnRegisterInnerConnectionByHost(SubscriberClient* client) {
  if (!client) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  const auto sinf = client->GetServerHostInfo();
  if (!sinf.IsValid()) {
    return common::make_error_inval();
  }

  auto hs = connections_.find(sinf.GetUserID());
  if (hs == connections_.end()) {
    return common::Error();
  }

  for (auto it = hs->second.begin(); it != hs->second.end(); ++it) {
    if (*it == client) {
      hs->second.erase(it);
      break;
    }
  }

  if (hs->second.empty()) {
    connections_.erase(hs);
  }
  return common::Error();
}

std::vector<SubscriberClient*> SubscribersHandler::FindInnerConnectionsByUser(const rpc::UserRpcInfo& user) const {
  const auto hs = connections_.find(user.GetUserID());
  if (hs == connections_.end()) {
    return std::vector<SubscriberClient*>();
  }

  std::vector<SubscriberClient*> result;
  auto devices = hs->second;
  for (client_t* connected_device : devices) {
    auto uinf = connected_device->GetServerHostInfo();
    if (user.Equals(uinf.MakeUserRpc())) {
      result.push_back(connected_device);
    }
  }
  return result;
}

size_t SubscribersHandler::GetAndUpdateOnlineUserByStreamID(common::libev::IoLoop* server,
                                                            fastotv::stream_id sid) const {
  size_t total = 0;
  std::vector<common::libev::IoClient*> online_clients = server->GetClients();
  for (size_t i = 0; i < online_clients.size(); ++i) {
    common::libev::IoClient* client = online_clients[i];
    SubscriberClient* iclient = static_cast<SubscriberClient*>(client);
    if (iclient && iclient->GetCurrentStreamID() == sid) {
      total++;
    }
  }

  return total;
}

common::ErrnoError SubscribersHandler::HandleRequestClientActivate(SubscriberClient* client,
                                                                   fastotv::protocol::request_t* req) {
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jauth = json_tokener_parse(params_ptr);
    if (!jauth) {
      return common::make_errno_error_inval();
    }

    fastotv::commands_info::AuthInfo uauth;
    common::Error err_des = uauth.DeSerialize(jauth);
    json_object_put(jauth);
    if (err_des) {
      client->ActivateFail(req->id, err_des);
      return common::make_errno_error(err_des->GetDescription(), EINVAL);
    }

    subscribers::commands_info::UserInfo registered_user;
    common::Error err_find = finder_->FindUser(uauth, &registered_user);
    if (err_find) {
      client->ActivateFail(req->id, err_find);
      return common::make_errno_error(err_find->GetDescription(), EINVAL);
    }

    if (registered_user.IsBanned()) {
      const common::Error err = common::make_error("Banned user");
      client->ActivateFail(req->id, err);
      return common::make_errno_error(err->GetDescription(), EINVAL);
    }

    const fastotv::device_id_t did = uauth.GetDeviceID();
    commands_info::DeviceInfo dev;
    common::Error dev_find = registered_user.FindDevice(did, &dev);
    if (dev_find) {
      client->ActivateFail(req->id, dev_find);
      return common::make_errno_error(dev_find->GetDescription(), EINVAL);
    }

    const ServerAuthInfo server_user_auth(registered_user.GetUserID(), uauth);
    const rpc::UserRpcInfo user_rpc = server_user_auth.MakeUserRpc();
    auto fconnections = FindInnerConnectionsByUser(user_rpc);
    if (fconnections.size() >= dev.GetConnections()) {
      const common::Error err = common::make_error("Limit connection reject");
      client->ActivateFail(req->id, err);
      return common::make_errno_error(err->GetDescription(), EINVAL);
    }

    common::ErrnoError errn = client->ActivateSuccess(req->id);
    if (errn) {
      return errn;
    }

    common::Error err = RegisterInnerConnectionByHost(server_user_auth, client);
    CHECK(!err) << "Register inner connection error: " << err->GetDescription();
    INFO_LOG() << "Welcome registered user: " << uauth.GetLogin() << ", connection: " << fconnections.size() + 1;
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError SubscribersHandler::HandleRequestClientPing(SubscriberClient* client,
                                                               fastotv::protocol::request_t* req) {
  subscribers::commands_info::UserInfo user;
  common::Error err = CheckIsAuthClient(client, &user);
  if (err) {
    ignore_result(client->CheckActivateFail(req->id, err));
    ignore_result(client->Close());
    delete client;
    return common::make_errno_error(err->GetDescription(), EINVAL);
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    fastotv::commands_info::ClientPingInfo ping_info;
    common::Error err_des = ping_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    return client->Pong(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError SubscribersHandler::HandleRequestClientGetServerInfo(SubscriberClient* client,
                                                                        fastotv::protocol::request_t* req) {
  subscribers::commands_info::UserInfo user;
  common::Error err = CheckIsAuthClient(client, &user);
  if (err) {
    ignore_result(client->CheckActivateFail(req->id, err));
    ignore_result(client->Close());
    delete client;
    return common::make_errno_error(err->GetDescription(), EINVAL);
  }

  return client->GetServerInfoSuccess(req->id, bandwidth_host_);
}

common::Error SubscribersHandler::CheckIsAuthClient(SubscriberClient* client,
                                                    subscribers::commands_info::UserInfo* user) const {
  if (!user) {
    return common::make_error_inval();
  }

  fastotv::commands_info::AuthInfo hinf = client->GetServerHostInfo();
  return finder_->FindUser(hinf, user);
}

common::ErrnoError SubscribersHandler::HandleRequestClientGetChannels(SubscriberClient* client,
                                                                      fastotv::protocol::request_t* req) {
  subscribers::commands_info::UserInfo user;
  common::Error err = CheckIsAuthClient(client, &user);
  if (err) {
    ignore_result(client->CheckActivateFail(req->id, err));
    ignore_result(client->Close());
    delete client;
    return common::make_errno_error(err->GetDescription(), EINVAL);
  }

  fastotv::commands_info::ChannelsInfo chan = user.GetChannelInfo();
  return client->GetChannelsSuccess(req->id, chan);
}

common::ErrnoError SubscribersHandler::HandleRequestClientGetRuntimeChannelInfo(SubscriberClient* client,
                                                                                fastotv::protocol::request_t* req) {
  subscribers::commands_info::UserInfo user;
  common::Error err = CheckIsAuthClient(client, &user);
  if (err) {
    ignore_result(client->CheckActivateFail(req->id, err));
    ignore_result(client->Close());
    delete client;
    return common::make_errno_error(err->GetDescription(), EINVAL);
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrun = json_tokener_parse(params_ptr);
    if (!jrun) {
      return common::make_errno_error_inval();
    }

    fastotv::commands_info::RuntimeChannelLiteInfo run;
    common::Error err_des = run.DeSerialize(jrun);
    json_object_put(jrun);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    common::libev::IoLoop* server = client->GetServer();
    const fastotv::stream_id sid = run.GetStreamID();

    size_t watchers = GetAndUpdateOnlineUserByStreamID(server, sid);  // calc watchers
    client->SetCurrentStreamID(sid);                                  // add to watcher

    return client->GetRuntimeChannelInfoSuccess(req->id, sid, watchers);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError SubscribersHandler::HandleInnerDataReceived(SubscriberClient* client,
                                                               const std::string& input_command) {
  fastotv::protocol::request_t* req = nullptr;
  fastotv::protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    INFO_LOG() << "Received request: " << input_command;
    common::ErrnoError err = HandleRequestCommand(client, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    INFO_LOG() << "Received responce: " << input_command;
    common::ErrnoError err = HandleResponceCommand(client, resp);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete resp;
  } else {
    DNOTREACHED();
    return common::make_errno_error("Invalid command type", EINVAL);
  }

  return common::ErrnoError();
}

common::ErrnoError SubscribersHandler::HandleRequestCommand(SubscriberClient* client,
                                                            fastotv::protocol::request_t* req) {
  SubscriberClient* iclient = static_cast<SubscriberClient*>(client);
  if (req->method == CLIENT_ACTIVATE) {
    return HandleRequestClientActivate(iclient, req);
  } else if (req->method == CLIENT_PING) {
    return HandleRequestClientPing(iclient, req);
  } else if (req->method == CLIENT_GET_SERVER_INFO) {
    return HandleRequestClientGetServerInfo(iclient, req);
  } else if (req->method == CLIENT_GET_CHANNELS) {
    return HandleRequestClientGetChannels(iclient, req);
  } else if (req->method == CLIENT_GET_RUNTIME_CHANNEL_INFO) {
    return HandleRequestClientGetRuntimeChannelInfo(iclient, req);
  }

  WARNING_LOG() << "Received unknown command: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError SubscribersHandler::HandleResponceServerPing(SubscriberClient* client,
                                                                fastotv::protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_ping = json_tokener_parse(params_ptr);
    if (!jclient_ping) {
      return common::make_errno_error_inval();
    }

    fastotv::commands_info::ClientPingInfo client_ping_info;
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

common::ErrnoError SubscribersHandler::HandleResponceServerGetClientInfo(SubscriberClient* client,
                                                                         fastotv::protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_info = json_tokener_parse(params_ptr);
    if (!jclient_info) {
      return common::make_errno_error_inval();
    }

    fastotv::commands_info::ClientInfo cinf;
    common::Error err_des = cinf.DeSerialize(jclient_info);
    json_object_put(jclient_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError SubscribersHandler::HandleResponceCommand(SubscriberClient* client,
                                                             fastotv::protocol::response_t* resp) {
  fastotv::protocol::request_t req;
  SubscriberClient* sclient = static_cast<SubscriberClient*>(client);
  SubscriberClient::callback_t cb;
  if (sclient->PopRequestByID(resp->id, &req, &cb)) {
    if (cb) {
      cb(resp);
    }
    if (req.method == SERVER_PING) {
      return HandleResponceServerPing(sclient, resp);
    } else if (req.method == SERVER_GET_CLIENT_INFO) {
      return HandleResponceServerGetClientInfo(sclient, resp);
    } else {
      WARNING_LOG() << "HandleResponceServiceCommand not handled command: " << req.method;
    }
  }

  return common::ErrnoError();
}

}  // namespace subscribers
}  // namespace server
}  // namespace fastocloud
