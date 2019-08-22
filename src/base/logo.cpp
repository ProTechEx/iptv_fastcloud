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

#include "base/logo.h"

#include <string>

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include <common/sprintf.h>

#include "base/constants.h"

#define LOGO_PATH_FIELD "path"
#define LOGO_POSITION_FIELD "position"
#define LOGO_ALPHA_FIELD "alpha"

namespace fastocloud {

Logo::Logo() : Logo(common::uri::Url(), common::draw::Point(), alpha_t()) {}

Logo::Logo(const common::uri::Url& path, const common::draw::Point& position, alpha_t alpha)
    : path_(path), position_(position), alpha_(alpha) {}

bool Logo::Equals(const Logo& inf) const {
  return path_ == inf.path_;
}

common::uri::Url Logo::GetPath() const {
  return path_;
}

void Logo::SetPath(const common::uri::Url& path) {
  path_ = path;
}

common::draw::Point Logo::GetPosition() const {
  return position_;
}

void Logo::SetPosition(const common::draw::Point& position) {
  position_ = position;
}

alpha_t Logo::GetAlpha() const {
  return alpha_;
}

void Logo::SetAlpha(alpha_t alpha) {
  alpha_ = alpha;
}

common::Optional<Logo> Logo::MakeLogo(common::HashValue* hash) {
  if (!hash) {
    return common::Optional<Logo>();
  }

  Logo res;
  common::Value* logo_path_field = hash->Find(LOGO_PATH_FIELD);
  std::string logo_path;
  if (logo_path_field && logo_path_field->GetAsBasicString(&logo_path)) {
    res.SetPath(common::uri::Url(logo_path));
  }

  common::Value* logo_pos_field = hash->Find(LOGO_POSITION_FIELD);
  std::string logo_pos_str;
  if (logo_pos_field && logo_pos_field->GetAsBasicString(&logo_pos_str)) {
    common::draw::Point pt;
    if (common::ConvertFromString(logo_pos_str, &pt)) {
      res.SetPosition(pt);
    }
  }

  common::Value* alpha_field = hash->Find(LOGO_ALPHA_FIELD);
  double alpha;
  if (alpha_field && alpha_field->GetAsDouble(&alpha)) {
    res.SetAlpha(alpha);
  }
  return res;
}

common::Error Logo::DoDeSerialize(json_object* serialized) {
  fastocloud::Logo res;
  json_object* jpath = nullptr;
  json_bool jpath_exists = json_object_object_get_ex(serialized, LOGO_PATH_FIELD, &jpath);
  if (jpath_exists) {
    res.SetPath(common::uri::Url(json_object_get_string(jpath)));
  }

  json_object* jposition = nullptr;
  json_bool jposition_exists = json_object_object_get_ex(serialized, LOGO_POSITION_FIELD, &jposition);
  if (jposition_exists) {
    common::draw::Point pt;
    if (common::ConvertFromString(json_object_get_string(jposition), &pt)) {
      res.SetPosition(pt);
    }
  }

  json_object* jalpha = nullptr;
  json_bool jalpha_exists = json_object_object_get_ex(serialized, LOGO_ALPHA_FIELD, &jalpha);
  if (jalpha_exists) {
    res.SetAlpha(json_object_get_double(jalpha));
  }

  *this = res;
  return common::Error();
}

common::Error Logo::SerializeFields(json_object* out) const {
  const std::string logo_path = path_.GetUrl();
  json_object_object_add(out, LOGO_PATH_FIELD, json_object_new_string(logo_path.c_str()));
  const std::string position_str = common::ConvertToString(GetPosition());
  json_object_object_add(out, LOGO_POSITION_FIELD, json_object_new_string(position_str.c_str()));
  json_object_object_add(out, LOGO_ALPHA_FIELD, json_object_new_double(GetAlpha()));

  return common::Error();
}

}  // namespace fastocloud
