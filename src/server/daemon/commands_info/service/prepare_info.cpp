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

#include "server/daemon/commands_info/service/prepare_info.h"

#include <string>

#include <common/file_system/file_system.h>
#include <common/file_system/file_system_utils.h>
#include <common/file_system/string_path_utils.h>

#include "base/utils.h"

#define OK_RESULT "OK"

#define PREPARE_SERVICE_INFO_FEEDBACK_DIRECTORY_FIELD "feedback_directory"
#define PREPARE_SERVICE_INFO_TIMESHIFTS_DIRECTORY_FIELD "timeshifts_directory"
#define PREPARE_SERVICE_INFO_HLS_DIRECTORY_FIELD "hls_directory"
#define PREPARE_SERVICE_INFO_PLAYLIST_DIRECTORY_FIELD "playlists_directory"
#define PREPARE_SERVICE_INFO_DVB_DIRECTORY_FIELD "dvb_directory"
#define PREPARE_SERVICE_INFO_CAPTURE_CARD_DIRECTORY_FIELD "capture_card_directory"
#define PREPARE_SERVICE_INFO_VODS_IN_DIRECTORY_FIELD "vods_in_directory"
#define PREPARE_SERVICE_INFO_VODS_DIRECTORY_FIELD "vods_directory"
#define PREPARE_SERVICE_INFO_CODS_DIRECTORY_FIELD "cods_directory"

#define SAVE_DIRECTORY_FIELD_PATH "path"
#define SAVE_DIRECTORY_FIELD_CONTENT "content"
#define SAVE_DIRECTORY_FIELD_RESULT "result"
#define SAVE_DIRECTORY_FIELD_ERROR "error"

namespace fastocloud {
namespace server {
namespace service {

namespace {
json_object* MakeDirectoryStateResponce(const DirectoryState& dir) {
  json_object* obj_dir = json_object_new_object();

  json_object* obj = json_object_new_object();
  const std::string path_str = dir.dir.GetPath();
  json_object_object_add(obj, SAVE_DIRECTORY_FIELD_PATH, json_object_new_string(path_str.c_str()));
  if (dir.is_valid) {
    json_object_object_add(obj, SAVE_DIRECTORY_FIELD_RESULT, json_object_new_string(OK_RESULT));
    json_object* jcontent = json_object_new_array();
    if (dir.content) {
      const auto content = *dir.content;
      for (const common::file_system::ascii_file_string_path& file : content) {
        const auto path = file.GetPath();
        json_object_array_add(jcontent, json_object_new_string(path.c_str()));
      }
    }
    json_object_object_add(obj, SAVE_DIRECTORY_FIELD_CONTENT, jcontent);
  } else {
    json_object_object_add(obj, SAVE_DIRECTORY_FIELD_ERROR, json_object_new_string(dir.error_str.c_str()));
  }

  json_object_object_add(obj_dir, dir.key.c_str(), obj);
  return obj_dir;
}
}  // namespace

PrepareInfo::PrepareInfo()
    : base_class(),
      feedback_directory_(),
      timeshifts_directory_(),
      hls_directory_(),
      playlists_directory_(),
      dvb_directory_(),
      capture_card_directory_(),
      vods_in_directory_(),
      vods_directory_(),
      cods_directory_() {}

std::string PrepareInfo::GetFeedbackDirectory() const {
  return feedback_directory_;
}

std::string PrepareInfo::GetTimeshiftsDirectory() const {
  return timeshifts_directory_;
}

std::string PrepareInfo::GetHlsDirectory() const {
  return hls_directory_;
}

std::string PrepareInfo::GetPlaylistsDirectory() const {
  return playlists_directory_;
}

std::string PrepareInfo::GetDvbDirectory() const {
  return dvb_directory_;
}

std::string PrepareInfo::GetCaptureDirectory() const {
  return capture_card_directory_;
}

std::string PrepareInfo::GetVodsInDirectory() const {
  return vods_in_directory_;
}

std::string PrepareInfo::GetVodsDirectory() const {
  return vods_directory_;
}

std::string PrepareInfo::GetCodsDirectory() const {
  return cods_directory_;
}

common::Error PrepareInfo::SerializeFields(json_object* out) const {
  json_object_object_add(out, PREPARE_SERVICE_INFO_FEEDBACK_DIRECTORY_FIELD,
                         json_object_new_string(feedback_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_TIMESHIFTS_DIRECTORY_FIELD,
                         json_object_new_string(timeshifts_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_HLS_DIRECTORY_FIELD, json_object_new_string(hls_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_PLAYLIST_DIRECTORY_FIELD,
                         json_object_new_string(playlists_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_DVB_DIRECTORY_FIELD, json_object_new_string(dvb_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_CAPTURE_CARD_DIRECTORY_FIELD,
                         json_object_new_string(capture_card_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_VODS_IN_DIRECTORY_FIELD,
                         json_object_new_string(vods_in_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_VODS_DIRECTORY_FIELD,
                         json_object_new_string(vods_directory_.c_str()));
  json_object_object_add(out, PREPARE_SERVICE_INFO_CODS_DIRECTORY_FIELD,
                         json_object_new_string(cods_directory_.c_str()));
  return common::Error();
}

common::Error PrepareInfo::DoDeSerialize(json_object* serialized) {
  PrepareInfo inf;
  json_object* jfeedback_directory = nullptr;
  json_bool jfeedback_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_FEEDBACK_DIRECTORY_FIELD, &jfeedback_directory);
  if (jfeedback_directory_exists) {
    inf.feedback_directory_ = json_object_get_string(jfeedback_directory);
  }

  json_object* jtimeshifts_directory = nullptr;
  json_bool jtimeshifts_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_TIMESHIFTS_DIRECTORY_FIELD, &jtimeshifts_directory);
  if (jtimeshifts_directory_exists) {
    inf.timeshifts_directory_ = json_object_get_string(jtimeshifts_directory);
  }

  json_object* jhls_directory = nullptr;
  json_bool jhls_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_HLS_DIRECTORY_FIELD, &jhls_directory);
  if (jhls_directory_exists) {
    inf.hls_directory_ = json_object_get_string(jhls_directory);
  }

  json_object* jplaylists_directory = nullptr;
  json_bool jplaylists_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_PLAYLIST_DIRECTORY_FIELD, &jplaylists_directory);
  if (jplaylists_directory_exists) {
    inf.playlists_directory_ = json_object_get_string(jplaylists_directory);
  }

  json_object* jdvb_directory = nullptr;
  json_bool jdvb_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_DVB_DIRECTORY_FIELD, &jdvb_directory);
  if (jdvb_directory_exists) {
    inf.dvb_directory_ = json_object_get_string(jdvb_directory);
  }

  json_object* jcapture_card_directory = nullptr;
  json_bool jcapture_card_directory_exists = json_object_object_get_ex(
      serialized, PREPARE_SERVICE_INFO_CAPTURE_CARD_DIRECTORY_FIELD, &jcapture_card_directory);
  if (jcapture_card_directory_exists) {
    inf.capture_card_directory_ = json_object_get_string(jcapture_card_directory);
  }

  json_object* jvods_in_directory = nullptr;
  json_bool jvods_in_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_VODS_IN_DIRECTORY_FIELD, &jvods_in_directory);
  if (jvods_in_directory_exists) {
    inf.vods_in_directory_ = json_object_get_string(jvods_in_directory);
  }

  json_object* jvods_directory = nullptr;
  json_bool jvods_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_VODS_DIRECTORY_FIELD, &jvods_directory);
  if (jvods_directory_exists) {
    inf.vods_directory_ = json_object_get_string(jvods_directory);
  }

  json_object* jcods_directory = nullptr;
  json_bool jcods_directory_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_CODS_DIRECTORY_FIELD, &jcods_directory);
  if (jcods_directory_exists) {
    inf.cods_directory_ = json_object_get_string(jcods_directory);
  }

  *this = inf;
  return common::Error();
}

DirectoryState::DirectoryState(const std::string& dir_str, const char* k)
    : key(k), dir(), content(), is_valid(false), error_str() {
  if (dir_str.empty()) {
    error_str = "Invalid input.";
    return;
  }

  dir = common::file_system::ascii_directory_string_path(dir_str);
  const std::string dir_path = dir.GetPath();
  common::ErrnoError errn = CreateAndCheckDir(dir_path);
  if (errn) {
    error_str = errn->GetDescription();
    return;
  }

  is_valid = true;
}

void DirectoryState::LoadContent() {
  if (is_valid) {
    content = common::file_system::ScanFolder(dir, "*.*", false);
  }
}

Directories::Directories(const PrepareInfo& sinf)
    : feedback_dir(sinf.GetFeedbackDirectory(), PREPARE_SERVICE_INFO_FEEDBACK_DIRECTORY_FIELD),
      timeshift_dir(sinf.GetTimeshiftsDirectory(), PREPARE_SERVICE_INFO_TIMESHIFTS_DIRECTORY_FIELD),
      hls_dir(sinf.GetHlsDirectory(), PREPARE_SERVICE_INFO_HLS_DIRECTORY_FIELD),
      playlist_dir(sinf.GetPlaylistsDirectory(), PREPARE_SERVICE_INFO_PLAYLIST_DIRECTORY_FIELD),
      dvb_dir(sinf.GetDvbDirectory(), PREPARE_SERVICE_INFO_DVB_DIRECTORY_FIELD),
      capture_card_dir(sinf.GetCaptureDirectory(), PREPARE_SERVICE_INFO_CAPTURE_CARD_DIRECTORY_FIELD),
      vods_in_dir(sinf.GetVodsInDirectory(), PREPARE_SERVICE_INFO_VODS_IN_DIRECTORY_FIELD),
      vods_dir(sinf.GetVodsDirectory(), PREPARE_SERVICE_INFO_VODS_DIRECTORY_FIELD),
      cods_dir(sinf.GetCodsDirectory(), PREPARE_SERVICE_INFO_CODS_DIRECTORY_FIELD) {
  vods_in_dir.LoadContent();
}

std::string MakeDirectoryResponce(const Directories& dirs) {
  json_object* obj = json_object_new_array();
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.feedback_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.timeshift_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.hls_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.playlist_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.dvb_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.capture_card_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.vods_in_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.vods_dir));
  json_object_array_add(obj, MakeDirectoryStateResponce(dirs.cods_dir));
  std::string obj_str = json_object_get_string(obj);
  json_object_put(obj);
  return obj_str;
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
