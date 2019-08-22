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

#include "stream/elements/sources/build_input.h"

#include <string>

#include "stream/elements/sources/filesrc.h"
#include "stream/elements/sources/httpsrc.h"
#include "stream/elements/sources/rtmpsrc.h"
#include "stream/elements/sources/tcpsrc.h"
#include "stream/elements/sources/udpsrc.h"

namespace {
const char kFFmpegUA[] = "Lavf/58.27.103";
const char kVLC[] = "VLC/3.0.1 LibVLC/3.0.1";
const char kWinkUA[] = "WINK";
}  // namespace

namespace fastocloud {
namespace stream {
namespace elements {
namespace sources {

Element* make_src(const InputUri& uri, element_id_t input_id, gint timeout_secs) {
  common::uri::Url url = uri.GetInput();
  common::uri::Url::scheme scheme = url.GetScheme();
  if (scheme == common::uri::Url::file) {
    const common::uri::Upath upath = url.GetPath();
    return make_file_src(upath.GetPath(), input_id);
  } else if (scheme == common::uri::Url::http || scheme == common::uri::Url::https) {
    common::Optional<std::string> agent;
    if (uri.GetUserAgent() == InputUri::VLC) {
      agent = std::string(kVLC);
    } else if (uri.GetUserAgent() == InputUri::FFMPEG) {
      agent = std::string(kFFmpegUA);
    } else if (uri.GetUserAgent() == InputUri::WINK) {
      agent = std::string(kWinkUA);
    }

    return make_http_src(url.GetUrl(), agent, uri.GetHttpProxyUrl(), timeout_secs, input_id);
  } else if (scheme == common::uri::Url::udp) {
    // udp://localhost:8080
    std::string host_str = url.GetHost();
    common::net::HostAndPort host;
    if (!common::ConvertFromString(host_str, &host)) {
      NOTREACHED() << "Unknown input url: " << host_str;
      return nullptr;
    }
    return make_udp_src(host, input_id);
  } else if (scheme == common::uri::Url::rtmp) {
    return make_rtmp_src(url.GetUrl(), timeout_secs, input_id);
  } else if (scheme == common::uri::Url::tcp) {
    // tcp://localhost:8080
    std::string host_str = url.GetHost();
    common::net::HostAndPort host;
    if (!common::ConvertFromString(host_str, &host)) {
      NOTREACHED() << "Unknown input url: " << host_str;
      return nullptr;
    }
    return make_tcp_server_src(host, input_id);
  }

  NOTREACHED() << "Unknown input url: " << url.GetUrl();
  return nullptr;
}

}  // namespace sources
}  // namespace elements
}  // namespace stream
}  // namespace fastocloud
