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

#include "base/output_uri.h"

#include <string>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include "base/constants.h"

#define OUTPUT_ID_FIELD "id"
#define OUTPUT_URI_FIELD "uri"
#define OUTPUT_HTTP_ROOT_FIELD "http_root"
#define OUTPUT_HLS_TYPE_FIELD "hls_type"

namespace fastocloud {

OutputUri::OutputUri() : OutputUri(0, common::uri::Url()) {}

OutputUri::OutputUri(uri_id_t id, const common::uri::Url& output)
    : base_class(), id_(id), output_(output), http_root_(), hls_type_(HLS_PULL) {}

OutputUri::uri_id_t OutputUri::GetID() const {
  return id_;
}

void OutputUri::SetID(uri_id_t id) {
  id_ = id;
}

common::uri::Url OutputUri::GetOutput() const {
  return output_;
}

void OutputUri::SetOutput(const common::uri::Url& uri) {
  output_ = uri;
}

OutputUri::http_root_t OutputUri::GetHttpRoot() const {
  return http_root_;
}

void OutputUri::SetHttpRoot(const http_root_t& root) {
  http_root_ = root;
}

OutputUri::HlsType OutputUri::GetHlsType() const {
  return hls_type_;
}

void OutputUri::SetHlsType(HlsType type) {
  hls_type_ = type;
}

bool OutputUri::Equals(const OutputUri& inf) const {
  return id_ == inf.id_ && output_ == inf.output_ && http_root_ == inf.http_root_;
}

common::Optional<OutputUri> OutputUri::MakeUrl(common::HashValue* hash) {
  if (!hash) {
    return common::Optional<OutputUri>();
  }

  OutputUri url;
  common::Value* input_id_field = hash->Find(OUTPUT_ID_FIELD);
  int uid;
  if (input_id_field && input_id_field->GetAsInteger(&uid)) {
    url.SetID(uid);
  }

  std::string url_str;
  common::Value* url_str_field = hash->Find(OUTPUT_URI_FIELD);
  if (url_str_field && url_str_field->GetAsBasicString(&url_str)) {
    url.SetOutput(common::uri::Url(url_str));
  }

  std::string http_root_str;
  common::Value* http_root_str_field = hash->Find(OUTPUT_HTTP_ROOT_FIELD);
  if (http_root_str_field && http_root_str_field->GetAsBasicString(&http_root_str)) {
    const common::file_system::ascii_directory_string_path http_root(http_root_str);
    url.SetHttpRoot(http_root);
  }

  int hls_type;
  common::Value* hls_type_field = hash->Find(OUTPUT_HLS_TYPE_FIELD);
  if (hls_type_field && hls_type_field->GetAsInteger(&hls_type)) {
    url.SetHlsType(static_cast<HlsType>(hls_type));
  }

  return url;
}

common::Error OutputUri::DoDeSerialize(json_object* serialized) {
  OutputUri res;
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, OUTPUT_ID_FIELD, &jid);
  if (jid_exists) {
    res.SetID(json_object_get_int64(jid));
  }

  json_object* juri = nullptr;
  json_bool juri_exists = json_object_object_get_ex(serialized, OUTPUT_URI_FIELD, &juri);
  if (juri_exists) {
    res.SetOutput(common::uri::Url(json_object_get_string(juri)));
  }

  json_object* jhttp_root = nullptr;
  json_bool jhttp_root_exists = json_object_object_get_ex(serialized, OUTPUT_HTTP_ROOT_FIELD, &jhttp_root);
  if (jhttp_root_exists) {
    const char* http_root_str = json_object_get_string(jhttp_root);
    const common::file_system::ascii_directory_string_path http_root(http_root_str);
    res.SetHttpRoot(http_root);
  }

  json_object* jhls_type = nullptr;
  json_bool jhls_type_exists = json_object_object_get_ex(serialized, OUTPUT_HLS_TYPE_FIELD, &jhls_type);
  if (jhls_type_exists) {
    res.SetHlsType(static_cast<HlsType>(json_object_get_int(jhls_type)));
  }

  *this = res;
  return common::Error();
}

common::Error OutputUri::SerializeFields(json_object* out) const {
  common::file_system::ascii_directory_string_path ps = GetHttpRoot();
  const std::string http_root_str = ps.GetPath();

  json_object_object_add(out, OUTPUT_ID_FIELD, json_object_new_int64(GetID()));
  std::string url_str = common::ConvertToString(GetOutput());
  json_object_object_add(out, OUTPUT_URI_FIELD, json_object_new_string(url_str.c_str()));
  json_object_object_add(out, OUTPUT_HTTP_ROOT_FIELD, json_object_new_string(http_root_str.c_str()));
  json_object_object_add(out, OUTPUT_HLS_TYPE_FIELD, json_object_new_int(hls_type_));
  return common::Error();
}

bool IsTestOutputUrl(const OutputUri& url) {
  return url.GetOutput() == common::uri::Url(TEST_URL);
}

}  // namespace fastocloud
