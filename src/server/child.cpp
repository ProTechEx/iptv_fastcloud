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

#include "server/child.h"

#include "stream_commands/commands_factory.h"

namespace fastocloud {
namespace server {

Child::Child(common::libev::IoLoop* server) : IoChild(server), client_(nullptr), id_(0) {}

Child::~Child() {}

Child::client_t* Child::GetClient() const {
  return client_;
}

void Child::SetClient(client_t* pipe) {
  client_ = pipe;
}

common::ErrnoError Child::Stop() {
  if (!client_) {
    return common::make_errno_error_inval();
  }

  fastotv::protocol::request_t req = StopStreamRequest(NextRequestID());
  return client_->WriteRequest(req);
}

common::ErrnoError Child::Restart() {
  if (!client_) {
    return common::make_errno_error_inval();
  }

  fastotv::protocol::request_t req = RestartStreamRequest(NextRequestID());
  return client_->WriteRequest(req);
}

fastotv::protocol::sequance_id_t Child::NextRequestID() {
  const fastotv::protocol::seq_id_t next_id = id_++;
  return common::protocols::json_rpc::MakeRequestID(next_id);
}

}  // namespace server
}  // namespace fastocloud
