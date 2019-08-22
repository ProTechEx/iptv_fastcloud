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

#include <string>

#include <common/draw/types.h>
#include <common/serializer/json_serializer.h>
#include <common/uri/url.h>
#include <common/value.h>

#include "base/types.h"

namespace fastocloud {

class Logo : public common::serializer::JsonSerializer<Logo> {
 public:
  Logo();
  Logo(const common::uri::Url& path, const common::draw::Point& position, alpha_t alpha);

  bool Equals(const Logo& logo) const;

  common::uri::Url GetPath() const;
  void SetPath(const common::uri::Url& path);

  common::draw::Point GetPosition() const;
  void SetPosition(const common::draw::Point& position);

  alpha_t GetAlpha() const;
  void SetAlpha(alpha_t alpha);

  static common::Optional<Logo> MakeLogo(common::HashValue* value);

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  common::uri::Url path_;
  common::draw::Point position_;
  alpha_t alpha_;
};

}  // namespace fastocloud
