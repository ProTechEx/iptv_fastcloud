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

#include "server/child_stream.h"

#include "stream/stream_start_info.hpp"

namespace fastocloud {
namespace server {

common::ErrnoError ProcessSlaveWrapper::CreateChildStreamImpl(const serialized_stream_t& config_args,
                                                              const StreamInfo& sha,
                                                              const std::string& feedback_dir,
                                                              common::logging::LOG_LEVEL logs_level) {
  SECURITY_ATTRIBUTES sa;
  memset(&sa, 0, sizeof(sa));
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  HANDLE args_handle =
      CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, sizeof(StreamStartInfo), nullptr);
  if (args_handle == INVALID_HANDLE_VALUE) {
    return common::make_errno_error(errno);
  }

  StreamStartInfo* param =
      static_cast<StreamStartInfo*>(MapViewOfFile(args_handle, FILE_MAP_WRITE, 0, 0, sizeof(StreamStartInfo)));
  if (!param) {
    CloseHandle(args_handle);
    return common::make_errno_error(errno);
  }

  const serialized_stream_t copy(config_args->DeepCopy());
  param->feedback_dir = feedback_dir;
  param->log_level = logs_level;
  param->streamlink_path = config_.streamlink_path;
  param->config_args = copy;
  param->sha = sha;

#define CMD_LINE_SIZE 512
  char cmd_line[CMD_LINE_SIZE] = {0};
#if defined(_WIN64)
  common::SNPrintf(cmd_line, CMD_LINE_SIZE, STREAMER_EXE_NAME ".exe %llu", reinterpret_cast<LONG_PTR>(args_handle));
#else
  common::SNPrintf(cmd_line, CMD_LINE_SIZE, STREAMER_EXE_NAME ".exe %lu", reinterpret_cast<DWORD>(args_handle));
#endif

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  if (!CreateProcess(nullptr, cmd_line, nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
    CloseHandle(args_handle);
    return common::make_errno_error(errno);
  }

  if (!UnmapViewOfFile(param)) {
  }
  if (!CloseHandle(args_handle)) {
  }

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

  ChildStream* child = new ChildStream(loop_, sha.id);
  loop_->RegisterChild(child, pi.hProcess);
  CloseHandle(pi.hThread);
  return common::ErrnoError();
}

}  // namespace server
}  // namespace fastocloud
