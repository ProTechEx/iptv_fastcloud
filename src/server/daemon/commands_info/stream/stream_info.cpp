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

#include "server/daemon/commands_info/stream/stream_info.h"

#define STREAM_INFO_ID_FIELD "id"

namespace fastocloud {
namespace server {
namespace stream {

StreamInfo::StreamInfo() : base_class(), stream_id_() {}

StreamInfo::StreamInfo(stream_id_t stream_id) : stream_id_(stream_id) {}

stream_id_t StreamInfo::GetStreamID() const {
  return stream_id_;
}

common::Error StreamInfo::DoDeSerialize(json_object* serialized) {
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, STREAM_INFO_ID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_error_inval();
  }

  StreamInfo inf;
  inf.stream_id_ = json_object_get_string(jid);
  *this = inf;
  return common::Error();
}

common::Error StreamInfo::SerializeFields(json_object* out) const {
  json_object_object_add(out, STREAM_INFO_ID_FIELD, json_object_new_string(stream_id_.c_str()));
  return common::Error();
}

}  // namespace stream
}  // namespace server
}  // namespace fastocloud
