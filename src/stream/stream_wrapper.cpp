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

#include "stream/stream_wrapper.h"

#include <string>

#include <common/file_system/string_path_utils.h>

#include "base/config_fields.h"

#include "stream/stream_controller.h"

namespace {

const size_t kMaxSizeLogFile = 1024 * 1024;

int start_stream(const std::string& process_name,
                 const common::file_system::ascii_directory_string_path& feedback_dir,
                 const common::file_system::ascii_file_string_path& streamlink_path,
                 common::logging::LOG_LEVEL logs_level,
                 const fastocloud::StreamConfig& config_args,
                 fastotv::protocol::protocol_client_t* command_client,
                 fastocloud::StreamStruct* mem) {
  auto log_file = feedback_dir.MakeFileStringPath(LOGS_FILE_NAME);
  if (log_file) {
    common::logging::INIT_LOGGER(process_name, log_file->GetPath(), logs_level,
                                 kMaxSizeLogFile);  // initialization of logging system
  }
  NOTICE_LOG() << "Running " PROJECT_VERSION_HUMAN;

  fastocloud::stream::StreamController proc(feedback_dir, streamlink_path, command_client, mem);
  common::Error err = proc.Init(config_args);
  if (err) {
    WARNING_LOG() << err->GetDescription();
    NOTICE_LOG() << "Quiting " PROJECT_VERSION_HUMAN;
    return EXIT_FAILURE;
  }

  int res = proc.Exec();
  NOTICE_LOG() << "Quiting " PROJECT_VERSION_HUMAN;
  return res;
}

}  // namespace

int stream_exec(const char* process_name,
                const cmd_args* args,
                const void* config_args,
                void* command_client,
                void* mem) {
  if (!process_name || !args || !config_args || !command_client || !mem) {
    CRITICAL_LOG() << "Invalid arguments.";
    return EXIT_FAILURE;
  }

  const char* feedback_dir_ptr = args->feedback_dir;
  if (!feedback_dir_ptr) {
    CRITICAL_LOG() << "Define " FEEDBACK_DIR_FIELD " variable and make it valid";
    return EXIT_FAILURE;
  }

  const char* streamlink_path_ptr = args->streamlink_path;
  if (!streamlink_path_ptr) {
    CRITICAL_LOG() << "Define streamlink path variable and make it valid";
    return EXIT_FAILURE;
  }

  const fastocloud::StreamConfig* config_args_map = static_cast<const fastocloud::StreamConfig*>(config_args);
  common::logging::LOG_LEVEL logs_level = static_cast<common::logging::LOG_LEVEL>(args->log_level);
  fastotv::protocol::protocol_client_t* client = static_cast<fastotv::protocol::protocol_client_t*>(command_client);
  fastocloud::StreamStruct* smem = static_cast<fastocloud::StreamStruct*>(mem);
  return start_stream(process_name, common::file_system::ascii_directory_string_path(feedback_dir_ptr),
                      common::file_system::ascii_file_string_path(streamlink_path_ptr), logs_level, *config_args_map,
                      client, smem);
}
