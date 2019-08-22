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

#include "server/pipe/pipe_client.h"

namespace fastocloud {
namespace server {
namespace pipe {

ProtocoledPipeClient::ProtocoledPipeClient(common::libev::IoLoop* server, descriptor_t read_fd, descriptor_t write_fd)
    : base_class(server),
      pipe_read_client_(new common::libev::PipeReadClient(nullptr, read_fd)),
      pipe_write_client_(new common::libev::PipeWriteClient(nullptr, write_fd)),
      read_fd_(read_fd) {}

common::ErrnoError ProtocoledPipeClient::SingleWrite(const void* data, size_t size, size_t* nwrite_out) {
  return pipe_write_client_->SingleWrite(data, size, nwrite_out);
}

common::ErrnoError ProtocoledPipeClient::SingleRead(void* out, size_t max_size, size_t* nread) {
  return pipe_read_client_->SingleRead(out, max_size, nread);
}

descriptor_t ProtocoledPipeClient::GetFd() const {
  return read_fd_;
}

common::ErrnoError ProtocoledPipeClient::DoClose() {
  ignore_result(pipe_write_client_->Close());
  ignore_result(pipe_read_client_->Close());
  return common::ErrnoError();
}

const char* ProtocoledPipeClient::ClassName() const {
  return "ProtocoledPipeClient";
}

ProtocoledPipeClient::~ProtocoledPipeClient() {
  destroy(&pipe_write_client_);
  destroy(&pipe_read_client_);
}

}  // namespace pipe
}  // namespace server
}  // namespace fastocloud
