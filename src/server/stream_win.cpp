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

#include <windows.h>

#include <stdlib.h>

#include <iostream>
#include <string>

#include <common/file_system/string_path_utils.h>
#include <common/sprintf.h>

#include "stream/stream_start_info.hpp"

int main(int argc, char** argv, char** envp) {
  const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
  const std::string lib_full_path = common::file_system::make_path(absolute_source_dir, CORE_LIBRARY);
  HINSTANCE dll = LoadLibrary(lib_full_path.c_str());
  if (!dll) {
    std::cerr << "Failed to load " CORE_LIBRARY " path: " << lib_full_path << std::endl;
    return EXIT_FAILURE;
  }

  typedef int (*stream_exec_t)(const char* process_name, const void* args, void* command_client);
  stream_exec_t stream_exec_func = reinterpret_cast<stream_exec_t>(GetProcAddress(dll, "stream_exec"));
  if (!stream_exec_func) {
    std::cerr << "Failed to load start stream function error: " << GetLastError();
    FreeLibrary(dll);
    return EXIT_FAILURE;
  }

  const char* hid = argv[1];
#ifdef _WIN64
  HANDLE param_handle = reinterpret_cast<HANDLE>(_atoi64(hid));
#else
  HANDLE param_handle = reinterpret_cast<HANDLE>(atol(hid));
#endif
  fastocloud::StreamStartInfo* params =
      static_cast<fastocloud::StreamStartInfo*>(MapViewOfFile(param_handle, FILE_MAP_READ, 0, 0, 0));
  if (!params) {
    std::cerr << "Can't load shared settings: " << GetLastError();
    FreeLibrary(dll);
    return EXIT_FAILURE;
  }

  const std::string new_process_name = common::MemSPrintf(STREAMER_NAME "_%s", params->sha.id);
  const char* new_name = new_process_name.c_str();
  int res = stream_exec_func(new_name, params, nullptr);
  UnmapViewOfFile(params);
  FreeLibrary(dll);
  return res;
}
