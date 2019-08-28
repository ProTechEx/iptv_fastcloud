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

#include <stdlib.h>
#include <windows.h>

#include <iostream>
#include <string>

#include <common/file_system/string_path_utils.h>
#include <common/sprintf.h>

#include <common/libev/tcp/tcp_client.h>
#include <fastotv/protocol/protocol.h>

#include "base/config_fields.h"
#include "base/stream_config_parse.h"

#include "server/tcp/client.h"

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Must be 4 arguments";
    return EXIT_FAILURE;
  }

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
  const char* sz = argv[2];
  const char* csock = argv[3];
#if defined(_WIN64)
  HANDLE param_handle = reinterpret_cast<HANDLE>(_atoi64(hid));
  size_t size = _atoi64(sz);
  common::net::socket_descr_t cfd = _atoi64(csock);
#else
  HANDLE param_handle = reinterpret_cast<HANDLE>(atol(hid));
  size_t size = atol(sz);
  common::net::socket_descr_t cfd = atol(csock);
#endif

  const char* params_json = static_cast<const char*>(MapViewOfFile(param_handle, FILE_MAP_READ, 0, 0, 0));
  if (!params_json) {
    std::cerr << "Can't load shared settings: " << GetLastError();
    FreeLibrary(dll);
    return EXIT_FAILURE;
  }

  const auto params = fastocloud::MakeConfigFromJson(std::string(params_json, size));
  UnmapViewOfFile(params_json);
  if (!params) {
    std::cerr << "Invalid config json";
    FreeLibrary(dll);
    return EXIT_FAILURE;
  }

  common::Value* id_field = params->Find(ID_FIELD);
  std::string sid;
  if (!id_field || !id_field->GetAsBasicString(&sid)) {
    std::cerr << "Define " ID_FIELD " variable and make it valid";
    FreeLibrary(dll);
    return EXIT_FAILURE;
  }

  const std::string new_process_name = common::MemSPrintf(STREAMER_NAME "_%s", sid);
  const char* new_name = new_process_name.c_str();
  int res = stream_exec_func(new_name, params.get(),
                             new fastocloud::server::tcp::Client(nullptr, common::net::socket_info(cfd)));
  FreeLibrary(dll);
  return res;
}
