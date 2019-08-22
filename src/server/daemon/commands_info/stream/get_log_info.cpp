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

#include "server/daemon/commands_info/stream/get_log_info.h"

#include <string>

#define GET_LOG_INFO_PATH_FIELD "path"
#define GET_LOG_INFO_FEEDBACK_DIR_FIELD "feedback_directory"

namespace fastocloud {
namespace server {
namespace stream {

GetLogInfo::GetLogInfo() : base_class(), feedback_dir_(), path_() {}

GetLogInfo::GetLogInfo(stream_id_t stream_id, const std::string& feedback_dir, const url_t& path)
    : base_class(stream_id), feedback_dir_(feedback_dir), path_(path) {}

GetLogInfo::url_t GetLogInfo::GetLogPath() const {
  return path_;
}

std::string GetLogInfo::GetFeedbackDir() const {
  return feedback_dir_;
}

common::Error GetLogInfo::DoDeSerialize(json_object* serialized) {
  GetLogInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jfeedback_dir = nullptr;
  json_bool jfeedback_dir_exists =
      json_object_object_get_ex(serialized, GET_LOG_INFO_FEEDBACK_DIR_FIELD, &jfeedback_dir);
  if (!jfeedback_dir_exists) {
    return common::make_error_inval();
  }
  inf.feedback_dir_ = json_object_get_string(jfeedback_dir);

  json_object* jpath = nullptr;
  json_bool jpath_exists = json_object_object_get_ex(serialized, GET_LOG_INFO_PATH_FIELD, &jpath);
  if (jpath_exists) {
    inf.path_ = url_t(json_object_get_string(jpath));
  }

  *this = inf;
  return common::Error();
}

common::Error GetLogInfo::SerializeFields(json_object* out) const {
  const std::string path_str = path_.GetUrl();
  json_object_object_add(out, GET_LOG_INFO_PATH_FIELD, json_object_new_string(path_str.c_str()));
  json_object_object_add(out, GET_LOG_INFO_FEEDBACK_DIR_FIELD, json_object_new_string(feedback_dir_.c_str()));
  return base_class::SerializeFields(out);
}

}  // namespace stream
}  // namespace server
}  // namespace fastocloud
