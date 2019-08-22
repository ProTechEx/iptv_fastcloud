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

#include "stream/elements/sink/http.h"

#include <string>

#include <common/time.h>

namespace fastocloud {
namespace stream {
namespace elements {
namespace sink {

HlsOutput MakeHlsOutput(const common::uri::Url& uri,
                        const common::file_system::ascii_directory_string_path& http_root,
                        const std::string& filename) {
  elements::sink::HlsOutput hout;
  const std::string http_root_str = http_root.GetPath();
  fastotv::timestamp_t t = common::time::current_utc_mstime();
  hout.location = http_root_str + GenHttpTsTemplate(t);
  hout.play_locataion = http_root_str + filename;
  hout.playlist_root = uri.GetHpath();
  hout.paylist_length = 5;
  hout.max_files = 10;
  return hout;
}

HlsOutput MakeVodHlsOutput(const common::uri::Url& uri,
                           const common::file_system::ascii_directory_string_path& http_root,
                           const std::string& filename) {
  elements::sink::HlsOutput hout;
  const std::string http_root_str = http_root.GetPath();
  hout.location = http_root_str + GenVodHttpTsTemplate();
  hout.play_locataion = http_root_str + filename;
  hout.playlist_root = uri.GetHpath();
  hout.paylist_length = 0;
  hout.max_files = 0;
  return hout;
}

void ElementHLSSink::SetLocation(const std::string& location) {
  SetProperty("location", location);
}

std::string ElementHLSSink::GetLocation() const {
  return GetStringProperty("location");
}

void ElementHLSSink::SetPlayLocation(const std::string& play_location) {
  SetProperty("playlist-location", play_location);
}

void ElementHLSSink::SetTargetDuration(guint duration) {
  SetProperty("target-duration", duration);
}

void ElementHLSSink::SetPlayListRoot(const std::string& play_root) {
  SetProperty("playlist-root", play_root);
}

void ElementHLSSink::SetPlaylistLenght(guint duration) {
  SetProperty("playlist-length", duration);
}

void ElementHLSSink::SetMaxFiles(guint max_files) {
  SetProperty("max-files", max_files);
}

void ElementSoupHttpSink::SetLocation(const std::string& location) {
  SetProperty("location", location);
}

ElementSoupHttpSink* make_http_soup_sink(element_id_t sink_id, const std::string& location) {
  ElementSoupHttpSink* hls_out = make_sink<ElementSoupHttpSink>(sink_id);
  hls_out->SetLocation(location);
  return hls_out;
}

ElementHLSSink* make_http_sink(element_id_t sink_id, const HlsOutput& output) {
  ElementHLSSink* hls_out = make_sink<ElementHLSSink>(sink_id);
  hls_out->SetLocation(output.location);
  hls_out->SetPlayLocation(output.play_locataion);
  hls_out->SetTargetDuration(TS_DURATION);
  hls_out->SetPlaylistLenght(output.paylist_length);
  hls_out->SetMaxFiles(output.max_files);
  return hls_out;
}

}  // namespace sink
}  // namespace elements
}  // namespace stream
}  // namespace fastocloud
