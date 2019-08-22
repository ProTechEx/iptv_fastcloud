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

#include "server/daemon/commands_info/service/stop_info.h"

#define STOP_SERVICE_INFO_DELAY_FIELD "delay"

namespace fastocloud {
namespace server {
namespace service {

StopInfo::StopInfo() : base_class(), delay_(0) {}

StopInfo::StopInfo(const std::string& license, common::time64_t delay) : base_class(license), delay_(delay) {}

common::Error StopInfo::DoDeSerialize(json_object* serialized) {
  StopInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jlicense = nullptr;
  json_bool jdelay_exists = json_object_object_get_ex(serialized, STOP_SERVICE_INFO_DELAY_FIELD, &jlicense);
  if (jdelay_exists) {
    inf.delay_ = json_object_get_int64(jlicense);
  }

  *this = inf;
  return common::Error();
}

common::Error StopInfo::SerializeFields(json_object* out) const {
  json_object_object_add(out, STOP_SERVICE_INFO_DELAY_FIELD, json_object_new_int64(delay_));
  return base_class::SerializeFields(out);
}

common::time64_t StopInfo::GetDelay() const {
  return delay_;
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
