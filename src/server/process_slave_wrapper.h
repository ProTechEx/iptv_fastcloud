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

#pragma once

#include <map>
#include <string>

#include <common/libev/io_loop_observer.h>
#include <common/net/types.h>

#include <fastotv/protocol/protocol.h>
#include <fastotv/protocol/types.h>

#include "base/stream_config.h"
#include "base/stream_info.h"
#include "base/types.h"

#include "server/base/ihttp_requests_observer.h"
#include "server/config.h"

namespace fastocloud {
namespace server {

class Child;
class ProtocoledDaemonClient;

class ProcessSlaveWrapper : public common::libev::IoLoopObserver, public server::base::IHttpRequestsObserver {
 public:
  enum { node_stats_send_seconds = 10, ping_timeout_clients_seconds = 60, cleanup_seconds = 3 };
  typedef StreamConfig serialized_stream_t;
  typedef fastotv::protocol::protocol_client_t stream_client_t;

  explicit ProcessSlaveWrapper(const std::string& licensy_key, const Config& config);
  ~ProcessSlaveWrapper() override;

  static int SendStopDaemonRequest(const std::string& license);
  common::net::HostAndPort GetServerHostAndPort();

  int Exec(int argc, char** argv) WARN_UNUSED_RESULT;

 protected:
  void PreLooped(common::libev::IoLoop* server) override;
  void Accepted(common::libev::IoClient* client) override;
  void Moved(common::libev::IoLoop* server,
             common::libev::IoClient* client) override;  // owner server, now client is orphan
  void Closed(common::libev::IoClient* client) override;
  void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

  void Accepted(common::libev::IoChild* child) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoChild* child) override;
  void ChildStatusChanged(common::libev::IoChild* child, int status, int signal) override;

  void DataReceived(common::libev::IoClient* client) override;
  void DataReadyToWrite(common::libev::IoClient* client) override;
  void PostLooped(common::libev::IoLoop* server) override;

  void OnHttpRequest(common::libev::http::HttpClient* client, const file_path_t& file) override;

  virtual common::ErrnoError HandleRequestServiceCommand(ProtocoledDaemonClient* dclient,
                                                         fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  virtual common::ErrnoError HandleResponceServiceCommand(ProtocoledDaemonClient* dclient,
                                                          fastotv::protocol::response_t* resp) WARN_UNUSED_RESULT;

  virtual common::ErrnoError HandleRequestStreamsCommand(stream_client_t* pclient,
                                                         fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  virtual common::ErrnoError HandleResponceStreamsCommand(stream_client_t* pclient,
                                                          fastotv::protocol::response_t* resp) WARN_UNUSED_RESULT;

 private:
  Child* FindChildByID(stream_id_t cid) const;
  void BroadcastClients(const fastotv::protocol::request_t& req);

  common::ErrnoError DaemonDataReceived(ProtocoledDaemonClient* dclient) WARN_UNUSED_RESULT;
  common::ErrnoError StreamDataReceived(stream_client_t* pclient) WARN_UNUSED_RESULT;

  common::ErrnoError CreateChildStream(const serialized_stream_t& config_args);
  common::ErrnoError CreateChildStreamImpl(const serialized_stream_t& config_args, stream_id_t sid);

  // stream
  common::ErrnoError HandleRequestChangedSourcesStream(stream_client_t* pclient,
                                                       fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;

  common::ErrnoError HandleRequestStatisticStream(stream_client_t* pclient,
                                                  fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;

  common::ErrnoError HandleRequestClientStartStream(ProtocoledDaemonClient* dclient,
                                                    fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientStopStream(ProtocoledDaemonClient* dclient,
                                                   fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientRestartStream(ProtocoledDaemonClient* dclient,
                                                      fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientGetLogStream(ProtocoledDaemonClient* dclient,
                                                     fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientGetPipelineStream(ProtocoledDaemonClient* dclient,
                                                          fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;

  // service
  common::ErrnoError HandleRequestClientPrepareService(ProtocoledDaemonClient* dclient,
                                                       fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientSyncService(ProtocoledDaemonClient* dclient,
                                                    fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientActivate(ProtocoledDaemonClient* dclient,
                                                 fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientPingService(ProtocoledDaemonClient* dclient,
                                                    fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientGetLogService(ProtocoledDaemonClient* dclient,
                                                      fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestClientStopService(ProtocoledDaemonClient* dclient,
                                                    fastotv::protocol::request_t* req) WARN_UNUSED_RESULT;

  common::ErrnoError HandleResponcePingService(ProtocoledDaemonClient* dclient,
                                               fastotv::protocol::response_t* resp) WARN_UNUSED_RESULT;

  std::string MakeServiceStats(bool full_stat) const;
  void AddStreamLine(const serialized_stream_t& config_args);

  struct NodeStats;

  const Config config_;
  const std::string license_key_;

  int process_argc_;
  char** process_argv_;

  common::libev::IoLoop* loop_;
  // http
  common::libev::IoLoop* http_server_;
  common::libev::IoLoopObserver* http_handler_;
  // vods (video on demand)
  common::libev::IoLoop* vods_server_;
  common::libev::IoLoopObserver* vods_handler_;
  // cods (channel on demand)
  common::libev::IoLoop* cods_server_;
  common::libev::IoLoopObserver* cods_handler_;

  common::libev::timer_id_t ping_client_timer_;
  common::libev::timer_id_t node_stats_timer_;
  common::libev::timer_id_t cleanup_files_timer_;
  common::libev::timer_id_t quit_cleanup_timer_;
  NodeStats* node_stats_;

  std::map<common::file_system::ascii_directory_string_path, serialized_stream_t> vods_links_;
  std::map<common::file_system::ascii_directory_string_path, serialized_stream_t> cods_links_;
};

}  // namespace server
}  // namespace fastocloud
