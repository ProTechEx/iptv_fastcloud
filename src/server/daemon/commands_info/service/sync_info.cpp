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

#include "server/daemon/commands_info/service/sync_info.h"

#include "base/stream_config_parse.h"

#define SYNC_INFO_STREAMS_FIELD "streams"

namespace fastocloud {
namespace server {
namespace service {

SyncInfo::SyncInfo() : base_class(), streams_() {}

SyncInfo::streams_t SyncInfo::GetStreams() const {
  return streams_;
}

common::Error SyncInfo::SerializeFields(json_object*) const {
  NOTREACHED() << "Not need";
  return common::Error();
}

common::Error SyncInfo::DoDeSerialize(json_object* serialized) {
  SyncInfo inf;
  json_object* jstreams = nullptr;
  json_bool jstreams_exists = json_object_object_get_ex(serialized, SYNC_INFO_STREAMS_FIELD, &jstreams);
  if (jstreams_exists) {
    size_t len = json_object_array_length(jstreams);
    streams_t streams;
    for (size_t i = 0; i < len; ++i) {
      json_object* jstream = json_object_array_get_idx(jstreams, i);
      config_t conf = MakeConfigFromJson(jstream);
      if (conf) {
        streams.push_back(conf);
      }
    }
    inf.streams_ = streams;
  }

  *this = inf;
  return common::Error();
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
