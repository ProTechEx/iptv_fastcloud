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

#if defined(OS_LINUX)
#include <sys/prctl.h>
#endif

#include <dlfcn.h>
#include <sys/wait.h>

#include <unistd.h>

#include <common/file_system/file_system.h>
#include <common/file_system/string_path_utils.h>

#include "base/stream_struct.h"

#include "server/child_stream.h"
#include "server/daemon/server.h"

#include "stream/stream_wrapper.h"

#include "pipe/pipe_client.h"

namespace {
common::ErrnoError CreatePipe(int* read_client_fd, int* write_client_fd) {
  if (!read_client_fd || !write_client_fd) {
    return common::make_errno_error_inval();
  }

  int pipefd[2] = {0};
  int res = pipe(pipefd);
  if (res == ERROR_RESULT_VALUE) {
    return common::make_errno_error(errno);
  }

  *read_client_fd = pipefd[0];
  *write_client_fd = pipefd[1];
  return common::ErrnoError();
}
}  // namespace

namespace fastocloud {
namespace server {

ProcessSlaveWrapper::stream_exec_t ProcessSlaveWrapper::GetStartStreamFunction(const std::string& lib_full_path) {
  void* handle = dlopen(lib_full_path.c_str(), RTLD_LAZY);
  if (!handle) {
    ERROR_LOG() << "Failed to load " CORE_LIBRARY " path: " << lib_full_path
                << ", error: " << common::common_strerror(errno);
    return nullptr;
  }

  stream_exec_t stream_exec_func = reinterpret_cast<stream_exec_t>(dlsym(handle, "stream_exec"));
  char* error = dlerror();
  if (error) {
    ERROR_LOG() << "Failed to load start stream function error: " << error;
    dlclose(handle);
    return nullptr;
  }

  dlclose(handle);
  return stream_exec_func;
}

common::ErrnoError ProcessSlaveWrapper::CreateChildStreamImpl(const serialized_stream_t& config_args,
                                                              const StreamInfo& sha,
                                                              const std::string& feedback_dir,
                                                              common::logging::LOG_LEVEL logs_level) {
  int read_command_client = 0;
  int write_requests_client = 0;
  common::ErrnoError err = CreatePipe(&read_command_client, &write_requests_client);
  if (err) {
    return err;
  }

  int read_responce_client = 0;
  int write_responce_client = 0;
  err = CreatePipe(&read_responce_client, &write_responce_client);
  if (err) {
    return err;
  }

#if !defined(TEST)
  pid_t pid = fork();
#else
  pid_t pid = 0;
#endif
  if (pid == 0) {  // child
    const std::string new_process_name = common::MemSPrintf(STREAMER_NAME "_%s", sha.id);
    const char* new_name = new_process_name.c_str();
#if defined(OS_LINUX)
    for (int i = 0; i < process_argc_; ++i) {
      memset(process_argv_[i], 0, strlen(process_argv_[i]));
    }
    char* app_name = process_argv_[0];
    strncpy(app_name, new_name, new_process_name.length());
    app_name[new_process_name.length()] = 0;
    prctl(PR_SET_NAME, new_name);
#elif defined(OS_FREEBSD)
    setproctitle(new_name);
#else
#pragma message "Please implement"
#endif

#if !defined(TEST)
    // close not needed pipes
    common::ErrnoError errn = common::file_system::close_descriptor(read_responce_client);
    if (errn) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }
    errn = common::file_system::close_descriptor(write_requests_client);
    if (errn) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }
#endif

    const struct cmd_args client_args = {feedback_dir.c_str(), logs_level, config_.streamlink_path.c_str()};
    pipe::ProtocoledPipeClient* client =
        new pipe::ProtocoledPipeClient(nullptr, read_command_client, write_responce_client);
    client->SetName(sha.id);
    StreamStruct* mem = new StreamStruct(sha);
    int res = stream_exec_func_(new_name, &client_args, &config_args, client, mem);
    delete mem;
    client->Close();
    delete client;
    _exit(res);
  } else if (pid < 0) {
    ERROR_LOG() << "Failed to start children!";
  } else {
    // close not needed pipes
    common::ErrnoError errn = common::file_system::close_descriptor(read_command_client);
    if (errn) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }
    errn = common::file_system::close_descriptor(write_responce_client);
    if (err) {
      DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_WARNING);
    }

    pipe::ProtocoledPipeClient* pipe_client =
        new pipe::ProtocoledPipeClient(loop_, read_responce_client, write_requests_client);
    pipe_client->SetName(sha.id);
    loop_->RegisterClient(pipe_client);
    ChildStream* new_channel = new ChildStream(loop_, sha.id);
    new_channel->SetClient(pipe_client);
    loop_->RegisterChild(new_channel, pid);
  }

  return common::ErrnoError();
}

}  // namespace server
}  // namespace fastocloud
