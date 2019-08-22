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

#include <map>
#include <mutex>

#include "server/subscribers/isubscribe_finder.h"

namespace fastocloud {
namespace server {

class SyncFinder : public subscribers::ISubscribeFinder {
 public:
  typedef subscribers::commands_info::UserInfo user_t;
  typedef std::map<fastotv::login_t, user_t> users_t;

  SyncFinder();
  common::Error FindUser(const fastotv::commands_info::AuthInfo& user, user_t* uinf) const override;
  void Clear();
  void AddUser(const user_t& user);

 private:
  users_t users_;
  mutable std::mutex users_mutex_;
};

}  // namespace server
}  // namespace fastocloud
