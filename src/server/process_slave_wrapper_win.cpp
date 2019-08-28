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

#include "server/process_slave_wrapper.h"

#include <common/libev/io_loop.h>
#include <common/net/net.h>

#include "base/stream_config_parse.h"

#include "server/child_stream.h"
#include "server/tcp/client.h"
#include "server/utils/utils.h"

namespace fastocloud {
namespace server {

common::ErrnoError ProcessSlaveWrapper::CreateChildStreamImpl(const serialized_stream_t& config_args, stream_id_t sid) {
  common::net::socket_descr_t parent_sock;
  common::net::socket_descr_t child_sock;
  common::ErrnoError err = CreateSocketPair(&parent_sock, &child_sock);
  if (err) {
    return err;
  }

  SECURITY_ATTRIBUTES sa;
  memset(&sa, 0, sizeof(sa));
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  std::string json;
  if (!MakeJsonFromConfig(config_args, &json)) {
    return common::make_errno_error(EINTR);
  }

  HANDLE args_handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, json.size(), sid.c_str());
  if (args_handle == INVALID_HANDLE_VALUE) {
    return common::make_errno_error(errno);
  }

  void* param = MapViewOfFile(args_handle, FILE_MAP_WRITE, 0, 0, json.size());
  if (!param) {
    CloseHandle(args_handle);
    return common::make_errno_error(errno);
  }

  memcpy(param, json.c_str(), json.size());

#define CMD_LINE_SIZE 512
  char cmd_line[CMD_LINE_SIZE] = {0};
#if defined(_WIN64)
  common::SNPrintf(cmd_line, CMD_LINE_SIZE, STREAMER_EXE_NAME ".exe %llu %llu %llu",
                   reinterpret_cast<UINT_PTR>(args_handle), json.size(), child_sock);
#else
  common::SNPrintf(cmd_line, CMD_LINE_SIZE, STREAMER_EXE_NAME ".exe %lu %lu %lu", reinterpret_cast<DWORD>(args_handle),
                   json.size(), child_sock);
#endif

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));
  memset(&si, 0, sizeof(si));
  if (!CreateProcess(nullptr, cmd_line, nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
    CloseHandle(args_handle);
    return common::make_errno_error(errno);
  }

  UnmapViewOfFile(param);
  CloseHandle(args_handle);

  if (ResumeThread(pi.hThread) == -1) {
    if (!TerminateProcess(pi.hProcess, EXIT_FAILURE)) {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
      return common::make_errno_error(errno);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return common::make_errno_error(errno);
  }

  tcp::Client* sock_client = new tcp::Client(loop_, common::net::socket_info(parent_sock));
  sock_client->SetName(sid);
  loop_->RegisterClient(sock_client);
  ChildStream* child = new ChildStream(loop_, sid);
  child->SetClient(sock_client);
  loop_->RegisterChild(child, pi.hProcess);
  CloseHandle(pi.hThread);
  return common::ErrnoError();
}

}  // namespace server
}  // namespace fastocloud
