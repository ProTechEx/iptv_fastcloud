/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <vector>

#include <fastotv/commands_info/channels_info.h>  // for ChannelsInfo

namespace fastocloud {
namespace server {
namespace subscribers {
namespace commands_info {

enum Status { NO_ACTIVE = 0, ACTIVE = 1, BANNED = 2 };

class DeviceInfo : public common::serializer::JsonSerializer<DeviceInfo> {
 public:
  DeviceInfo();
  explicit DeviceInfo(fastotv::device_id_t did, size_t connections);

  bool IsValid() const;
  fastotv::device_id_t GetDeviceID() const;
  size_t GetConnections() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  fastotv::device_id_t id_;
  size_t connections_;
};

class UserInfo : public common::serializer::JsonSerializer<UserInfo> {
 public:
  typedef std::vector<DeviceInfo> devices_t;

  UserInfo();
  explicit UserInfo(const fastotv::user_id_t& uid,
                    const fastotv::login_t& login,
                    const std::string& password,
                    const fastotv::commands_info::ChannelsInfo& ch,
                    const devices_t& devices,
                    Status state);

  bool IsValid() const;
  bool IsBanned() const;

  common::Error FindDevice(fastotv::device_id_t id, DeviceInfo* dev) const;
  devices_t GetDevices() const;
  fastotv::login_t GetLogin() const;
  std::string GetPassword() const;
  fastotv::commands_info::ChannelsInfo GetChannelInfo() const;
  fastotv::user_id_t GetUserID() const;

  bool Equals(const UserInfo& inf) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  fastotv::user_id_t uid_;
  fastotv::login_t login_;  // unique
  std::string password_;    // hash
  fastotv::commands_info::ChannelsInfo ch_;
  devices_t devices_;
  Status status_;
};

}  // namespace commands_info
}  // namespace subscribers
}  // namespace server
}  // namespace fastocloud
