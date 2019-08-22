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
    along with fastocloud. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <common/optional.h>

#include <fastotv/types.h>

#define TS_EXTENSION "ts"
#define M3U8_EXTENSION "m3u8"
#define M3U8_CHUNK_MARKER "#EXTINF"
#define CHUNK_EXT "." TS_EXTENSION

#define DUMP_FILE_NAME "dump.html"

namespace fastocloud {

typedef uint64_t channel_id_t;           // channel id
typedef fastotv::stream_id stream_id_t;  // db id
typedef common::Optional<double> volume_t;
typedef double alpha_t;
typedef common::Optional<int> bit_rate_t;

enum StreamType : int {
  PROXY = 0,
  RELAY = 1,
  ENCODE = 2,
  TIMESHIFT_PLAYER = 3,
  TIMESHIFT_RECORDER = 4,
  CATCHUP = 5,
  TEST_LIFE = 6,
  VOD_RELAY = 7,
  VOD_ENCODE = 8,
  COD_RELAY = 9,
  COD_ENCODE = 10,
  SCREEN  // for inner use
};

}  // namespace fastocloud
