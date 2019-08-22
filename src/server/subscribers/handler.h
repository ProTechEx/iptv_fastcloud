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

#pragma once

#include <memory>  // for shared_ptr
#include <string>  // for string
#include <unordered_map>
#include <vector>

#include <common/error.h>        // for Error
#include <common/libev/types.h>  // for timer_id_t
#include <common/macros.h>       // for WARN_UNUSED_RESULT
#include <common/net/types.h>

#include <fastotv/protocol/types.h>

#include "server/base/iserver_handler.h"
#include "server/subscribers/commands_info/user_info.h"
#include "server/subscribers/rpc/user_rpc_info.h"

namespace fastocloud {
namespace server {
namespace subscribers {

class SubscriberClient;
class ServerAuthInfo;
class ISubscribeFinder;

class SubscribersHandler : public base::IServerHandler {
 public:
  typedef base::IServerHandler base_class;
  typedef SubscriberClient client_t;
  typedef std::unordered_map<fastotv::user_id_t, std::vector<client_t*>> inner_connections_t;
  enum {
    ping_timeout_clients = 60  // sec
  };

  explicit SubscribersHandler(ISubscribeFinder* finder, const common::net::HostAndPort& bandwidth_host);

  void PreLooped(common::libev::IoLoop* server) override;

  void Accepted(common::libev::IoClient* client) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  void Closed(common::libev::IoClient* client) override;

  void DataReceived(common::libev::IoClient* client) override;
  void DataReadyToWrite(common::libev::IoClient* client) override;
  void PostLooped(common::libev::IoLoop* server) override;
  void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;
#if LIBEV_CHILD_ENABLE
  void Accepted(common::libev::IoChild* child) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoChild* child) override;
  void ChildStatusChanged(common::libev::IoChild* client, int status) override;
#endif

  virtual ~SubscribersHandler();

 private:
  common::Error RegisterInnerConnectionByHost(const ServerAuthInfo& info, SubscriberClient* client) WARN_UNUSED_RESULT;
  common::Error UnRegisterInnerConnectionByHost(SubscriberClient* client) WARN_UNUSED_RESULT;
  std::vector<SubscriberClient*> FindInnerConnectionsByUser(const rpc::UserRpcInfo& user) const;

  common::ErrnoError HandleInnerDataReceived(SubscriberClient* client, const std::string& input_command);
  common::ErrnoError HandleRequestCommand(SubscriberClient* client, fastotv::protocol::request_t* req);
  common::ErrnoError HandleResponceCommand(SubscriberClient* client, fastotv::protocol::response_t* resp);

  common::ErrnoError HandleRequestClientActivate(SubscriberClient* client, fastotv::protocol::request_t* req);
  common::ErrnoError HandleRequestClientPing(SubscriberClient* client, fastotv::protocol::request_t* req);
  common::ErrnoError HandleRequestClientGetServerInfo(SubscriberClient* client, fastotv::protocol::request_t* req);
  common::ErrnoError HandleRequestClientGetChannels(SubscriberClient* client, fastotv::protocol::request_t* req);
  common::ErrnoError HandleRequestClientGetRuntimeChannelInfo(SubscriberClient* client,
                                                              fastotv::protocol::request_t* req);

  common::ErrnoError HandleResponceServerPing(SubscriberClient* client, fastotv::protocol::response_t* resp);
  common::ErrnoError HandleResponceServerGetClientInfo(SubscriberClient* client, fastotv::protocol::response_t* resp);

  size_t GetAndUpdateOnlineUserByStreamID(common::libev::IoLoop* server, fastotv::stream_id sid) const;
  common::Error CheckIsAuthClient(SubscriberClient* client,
                                  subscribers::commands_info::UserInfo* user) const WARN_UNUSED_RESULT;

  ISubscribeFinder* finder_;
  common::libev::timer_id_t ping_client_id_timer_;
  const common::net::HostAndPort bandwidth_host_;
  inner_connections_t connections_;
};

}  // namespace subscribers
}  // namespace server
}  // namespace fastocloud
