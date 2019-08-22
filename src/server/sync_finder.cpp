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

#include "server/sync_finder.h"

namespace fastocloud {
namespace server {

SyncFinder::SyncFinder() : users_() {}

common::Error SyncFinder::FindUser(const fastotv::commands_info::AuthInfo& user, user_t* uinf) const {
  if (!user.IsValid() || !uinf) {
    return common::make_error_inval();
  }

  std::unique_lock<std::mutex> lock(users_mutex_);
  const auto it = users_.find(user.GetLogin());
  if (it == users_.end()) {
    return common::make_error("User not found");
  }

  user_t founded_user = it->second;
  if (user.GetPassword() != founded_user.GetPassword()) {
    return common::make_error("Invalid password");
  }

  *uinf = founded_user;
  return common::Error();
}

void SyncFinder::Clear() {
  std::unique_lock<std::mutex> lock(users_mutex_);
  users_.clear();
}

void SyncFinder::AddUser(const user_t& user) {
  std::unique_lock<std::mutex> lock(users_mutex_);
  users_.insert(std::make_pair(user.GetLogin(), user));
}

}  // namespace server
}  // namespace fastocloud
