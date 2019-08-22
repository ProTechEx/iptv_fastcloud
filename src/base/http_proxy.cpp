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

#include "base/http_proxy.h"

#include <string>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include <common/sprintf.h>

#include "base/constants.h"

#define HTTP_PROXY_URL_FIELD "url"
#define HTTP_PROXY_USER_FIELD "user"
#define HTTP_PROXY_PASSWORD_FIELD "password"

namespace fastocloud {

HttpProxy::HttpProxy() : HttpProxy(common::uri::Url()) {}

HttpProxy::HttpProxy(const common::uri::Url& url) : url_(url), user_(), password_() {}

bool HttpProxy::Equals(const HttpProxy& proxy) const {
  return url_ == proxy.url_;
}

common::uri::Url HttpProxy::GetUrl() const {
  return url_;
}

void HttpProxy::SetUrl(const common::uri::Url& path) {
  url_ = path;
}

HttpProxy::user_id_t HttpProxy::GetUserID() const {
  return user_;
}

void HttpProxy::SetUserID(const user_id_t& sid) {
  user_ = sid;
}

HttpProxy::password_t HttpProxy::GetPassword() const {
  return password_;
}

void HttpProxy::SetPassword(const password_t& password) {
  password_ = password;
}

common::Optional<HttpProxy> HttpProxy::MakeHttpProxy(common::HashValue* hash) {
  if (!hash) {
    return common::Optional<HttpProxy>();
  }

  HttpProxy res;
  common::Value* url_field = hash->Find(HTTP_PROXY_URL_FIELD);
  std::string url_str;
  if (!url_field || !url_field->GetAsBasicString(&url_str) || url_str.empty()) {
    return common::Optional<HttpProxy>();
  }
  const auto url = common::uri::Url(url_str);
  res.SetUrl(url);

  common::Value* user_field = hash->Find(HTTP_PROXY_USER_FIELD);
  std::string user_str;
  if (user_field && user_field->GetAsBasicString(&user_str)) {
    res.SetUserID(user_str);
  }

  common::Value* password_field = hash->Find(HTTP_PROXY_PASSWORD_FIELD);
  std::string password_str;
  if (password_field && password_field->GetAsBasicString(&password_str)) {
    res.SetPassword(password_str);
  }
  return res;
}

common::Error HttpProxy::DoDeSerialize(json_object* serialized) {
  fastocloud::HttpProxy res;
  json_object* jurl = nullptr;
  json_bool jurl_exists = json_object_object_get_ex(serialized, HTTP_PROXY_URL_FIELD, &jurl);
  if (!jurl_exists) {
    return common::make_error_inval();
  }
  res.SetUrl(common::uri::Url(json_object_get_string(jurl)));

  json_object* juser = nullptr;
  json_bool juser_exists = json_object_object_get_ex(serialized, HTTP_PROXY_USER_FIELD, &juser);
  if (juser_exists) {
    std::string user = json_object_get_string(juser);
    res.SetUserID(user);
  }

  json_object* jpassword = nullptr;
  json_bool jpassword_exists = json_object_object_get_ex(serialized, HTTP_PROXY_USER_FIELD, &jpassword);
  if (jpassword_exists) {
    std::string password = json_object_get_string(jpassword);
    res.SetPassword(password);
  }
  *this = res;
  return common::Error();
}

common::Error HttpProxy::SerializeFields(json_object* out) const {
  const std::string logo_path = url_.GetUrl();
  json_object_object_add(out, HTTP_PROXY_URL_FIELD, json_object_new_string(logo_path.c_str()));
  return common::Error();
}

}  // namespace fastocloud
