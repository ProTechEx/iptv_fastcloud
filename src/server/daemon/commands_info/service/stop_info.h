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

#include <common/time.h>

#include "server/daemon/commands_info/service/license_info.h"

namespace fastocloud {
namespace server {
namespace service {

class StopInfo : public LicenseInfo {
 public:
  typedef LicenseInfo base_class;
  StopInfo();
  explicit StopInfo(const std::string& license, common::time64_t delay = 0);

  common::time64_t GetDelay() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  common::time64_t delay_;
};

}  // namespace service
}  // namespace server
}  // namespace fastocloud
