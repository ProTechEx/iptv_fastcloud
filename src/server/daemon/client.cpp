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

#include "server/daemon/client.h"

#include <string>

#include "server/daemon/commands_factory.h"

namespace fastocloud {
namespace server {

DaemonClient::DaemonClient(common::libev::IoLoop* server, const common::net::socket_info& info)
    : base_class(server, info), is_verified_(false) {}

bool DaemonClient::IsVerified() const {
  return is_verified_;
}

void DaemonClient::SetVerified(bool verified) {
  is_verified_ = verified;
}

const char* DaemonClient::ClassName() const {
  return "DaemonClient";
}

ProtocoledDaemonClient::ProtocoledDaemonClient(common::libev::IoLoop* server, const common::net::socket_info& info)
    : base_class(server, info) {}

common::ErrnoError ProtocoledDaemonClient::StopMe(const std::string& license) {
  const service::StopInfo stop_req(license);
  fastotv::protocol::request_t req;
  common::Error err_ser = StopServiceRequest(NextRequestID(), stop_req, &req);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteRequest(req);
}

common::ErrnoError ProtocoledDaemonClient::StopFail(fastotv::protocol::sequance_id_t id, common::Error err) {
  const std::string error_str = err->GetDescription();
  fastotv::protocol::response_t resp;
  common::Error err_ser = StopServiceResponseFail(id, error_str, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::StopSuccess(fastotv::protocol::sequance_id_t id) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = StopServiceResponseSuccess(id, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::Ping() {
  service::ClientPingInfo server_ping_info;
  return Ping(server_ping_info);
}

common::ErrnoError ProtocoledDaemonClient::Ping(const service::ClientPingInfo& server_ping_info) {
  fastotv::protocol::request_t ping_request;
  common::Error err_ser = PingRequest(NextRequestID(), server_ping_info, &ping_request);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteRequest(ping_request);
}

common::ErrnoError ProtocoledDaemonClient::Pong(fastotv::protocol::sequance_id_t id) {
  fastotv::commands_info::ServerPingInfo server_ping_info;
  return Pong(id, server_ping_info);
}

common::ErrnoError ProtocoledDaemonClient::Pong(fastotv::protocol::sequance_id_t id,
                                                const service::ServerPingInfo& pong) {
  std::string ping_server_json;
  common::Error err_ser = pong.SerializeToString(&ping_server_json);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  fastotv::protocol::response_t resp = PingServiceResponce(id, ping_server_json);
  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::ActivateFail(fastotv::protocol::sequance_id_t id, common::Error err) {
  const std::string error_str = err->GetDescription();
  fastotv::protocol::response_t resp;
  common::Error err_ser = ActivateResponseFail(id, error_str, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::ActivateSuccess(fastotv::protocol::sequance_id_t id,
                                                           const std::string& result) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = ActivateResponse(id, result, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::StateServiceSuccess(fastotv::protocol::sequance_id_t id,
                                                               const std::string& result) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = StateServiceResponse(id, result, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::GetLogServiceSuccess(fastotv::protocol::sequance_id_t id) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = GetLogServiceResponseSuccess(id, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::GetLogStreamSuccess(fastotv::protocol::sequance_id_t id) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = GetLogStreamResponseSuccess(id, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::StartStreamFail(fastotv::protocol::sequance_id_t id, common::Error err) {
  const std::string error_str = err->GetDescription();
  fastotv::protocol::response_t resp;
  common::Error err_ser = StartStreamResponseFail(id, error_str, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::StartStreamSuccess(fastotv::protocol::sequance_id_t id) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = StartStreamResponseSuccess(id, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::ReStartStreamFail(fastotv::protocol::sequance_id_t id, common::Error err) {
  const std::string error_str = err->GetDescription();
  fastotv::protocol::response_t resp;
  common::Error err_ser = RestartStreamResponseFail(id, error_str, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::ReStartStreamSuccess(fastotv::protocol::sequance_id_t id) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = RestartStreamResponseSuccess(id, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::StopStreamFail(fastotv::protocol::sequance_id_t id, common::Error err) {
  const std::string error_str = err->GetDescription();
  fastotv::protocol::response_t resp;
  common::Error err_ser = StopStreamResponseFail(id, error_str, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::StopStreamSuccess(fastotv::protocol::sequance_id_t id) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = StopStreamResponseSuccess(id, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

common::ErrnoError ProtocoledDaemonClient::SyncServiceSuccess(fastotv::protocol::sequance_id_t id) {
  fastotv::protocol::response_t resp;
  common::Error err_ser = SyncServiceResponceSuccess(id, &resp);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteResponse(resp);
}

}  // namespace server
}  // namespace fastocloud
