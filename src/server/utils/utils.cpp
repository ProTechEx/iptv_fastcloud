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

#include "server/utils/utils.h"

#include <unistd.h>

namespace fastocloud {
namespace server {

#if defined(OS_POSIX)
common::ErrnoError CreatePipe(common::net::socket_descr_t* read_client_fd,
                              common::net::socket_descr_t* write_client_fd) {
  if (!read_client_fd || !write_client_fd) {
    return common::make_errno_error_inval();
  }

  int pipefd[2] = {INVALID_DESCRIPTOR, INVALID_DESCRIPTOR};
  int res = pipe(pipefd);
  if (res == ERROR_RESULT_VALUE) {
    return common::make_errno_error(errno);
  }

  *read_client_fd = pipefd[0];
  *write_client_fd = pipefd[1];
  return common::ErrnoError();
}
#endif

common::ErrnoError CreateSocketPair(common::net::socket_descr_t* parent_sock, common::net::socket_descr_t* child_sock) {
  if (!parent_sock || !child_sock) {
    return common::make_errno_error_inval();
  }

  int socks[2] = {INVALID_DESCRIPTOR, INVALID_DESCRIPTOR};
  int res = socketpair(AF_LOCAL, SOCK_STREAM, 0, socks);
  if (res == ERROR_RESULT_VALUE) {
    return common::make_errno_error(errno);
  }

  *parent_sock = socks[1];
  *child_sock = socks[0];
  return common::ErrnoError();
}

}  // namespace server
}  // namespace fastocloud
