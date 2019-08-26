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

#include <common/libev/io_loop.h>

#include <fastotv/protocol/protocol.h>
#include <fastotv/protocol/types.h>

#include "stream_commands/commands_info/changed_sources_info.h"
#include "stream_commands/commands_info/statistic_info.h"

namespace fastocloud {
namespace stream {

class StreamServer : public common::libev::IoLoop {
 public:
  typedef common::libev::IoLoop base_class;
  explicit StreamServer(fastotv::protocol::protocol_client_t* command_client,
                        common::libev::IoLoopObserver* observer = nullptr);

  void WriteRequest(const fastotv::protocol::request_t& request) WARN_UNUSED_RESULT;

  const char* ClassName() const override;

  void SendChangeSourcesBroadcast(const ChangedSouresInfo& change) WARN_UNUSED_RESULT;
  void SendStatisticBroadcast(const StatisticInfo& statistic) WARN_UNUSED_RESULT;

  common::libev::IoChild* CreateChild() override;
  common::libev::IoClient* CreateClient(const common::net::socket_info& info) override;
  void Started(common::libev::LibEvLoop* loop) override;
  void Stopped(common::libev::LibEvLoop* loop) override;

 private:
  fastotv::protocol::protocol_client_t* const command_client_;
};

}  // namespace stream
}  // namespace fastocloud
