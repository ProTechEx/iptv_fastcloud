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

#include "stream_commands/commands_info/changed_sources_info.h"

#include <string>

#define CHANGE_SOURCES_ID_FIELD "id"
#define CHANGE_SOURCES_URL_FIELD "url"

namespace fastocloud {

ChangedSouresInfo::ChangedSouresInfo(stream_id_t sid, const url_t& url) : base_class(), id_(sid), url_(url) {}

ChangedSouresInfo::ChangedSouresInfo() : base_class(), id_(), url_() {}

common::Error ChangedSouresInfo::SerializeFields(json_object* out) const {
  json_object* url_obj = nullptr;
  common::Error err = url_.Serialize(&url_obj);
  if (err) {
    return err;
  }

  json_object_object_add(out, CHANGE_SOURCES_ID_FIELD, json_object_new_string(id_.c_str()));
  json_object_object_add(out, CHANGE_SOURCES_URL_FIELD, url_obj);
  return common::Error();
}

ChangedSouresInfo::url_t ChangedSouresInfo::GetUrl() const {
  return url_;
}

stream_id_t ChangedSouresInfo::GetStreamID() const {
  return id_;
}

common::Error ChangedSouresInfo::DoDeSerialize(json_object* serialized) {
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, CHANGE_SOURCES_ID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_error_inval();
  }

  ChangedSouresInfo inf;
  inf.id_ = json_object_get_string(jid);

  json_object* jurl = nullptr;
  json_bool jurl_exists = json_object_object_get_ex(serialized, CHANGE_SOURCES_URL_FIELD, &jurl);
  if (jurl_exists) {
    common::Error err = inf.url_.DeSerialize(jurl);
    if (err) {
      return err;
    }
  }

  *this = inf;
  return common::Error();
}

}  // namespace fastocloud
