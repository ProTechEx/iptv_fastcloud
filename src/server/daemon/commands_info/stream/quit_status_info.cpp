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

#include "server/daemon/commands_info/stream/quit_status_info.h"

#define QUIT_STATUS_INFO_SIGNAL_FIELD "signal"
#define QUIT_STATUS_INFO_EXIT_STATUS_FIELD "exit_status"

namespace fastocloud {
namespace server {
namespace stream {

QuitStatusInfo::QuitStatusInfo() : base_class(), exit_status_(), signal_() {}

QuitStatusInfo::QuitStatusInfo(stream_id_t stream_id, int exit_status, int signal)
    : base_class(stream_id), exit_status_(exit_status), signal_(signal) {}

int QuitStatusInfo::GetSignal() const {
  return signal_;
}

int QuitStatusInfo::GetExitStatus() const {
  return exit_status_;
}

common::Error QuitStatusInfo::DoDeSerialize(json_object* serialized) {
  QuitStatusInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jsignal = nullptr;
  json_bool jsignal_exists = json_object_object_get_ex(serialized, QUIT_STATUS_INFO_SIGNAL_FIELD, &jsignal);
  if (!jsignal_exists) {
    return common::make_error_inval();
  }

  json_object* jexit_status = nullptr;
  json_bool jexit_status_exists =
      json_object_object_get_ex(serialized, QUIT_STATUS_INFO_EXIT_STATUS_FIELD, &jexit_status);
  if (!jexit_status_exists) {
    return common::make_error_inval();
  }

  inf.signal_ = json_object_get_int(jsignal);
  inf.exit_status_ = json_object_get_int(jexit_status);
  *this = inf;
  return common::Error();
}

common::Error QuitStatusInfo::SerializeFields(json_object* out) const {
  json_object_object_add(out, QUIT_STATUS_INFO_SIGNAL_FIELD, json_object_new_int(signal_));
  json_object_object_add(out, QUIT_STATUS_INFO_EXIT_STATUS_FIELD, json_object_new_int(exit_status_));
  return base_class::SerializeFields(out);
}

}  // namespace stream
}  // namespace server
}  // namespace fastocloud
