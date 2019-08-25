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

#pragma once

#include <string>
#include <vector>

#include <common/macros.h>

#include "base/channel_stats.h"
#include "base/types.h"

namespace fastocloud {

typedef std::vector<ChannelStats> input_channels_info_t;
typedef std::vector<ChannelStats> output_channels_info_t;

struct StreamInfo {
  stream_id_t id;
  StreamType type;
  std::vector<channel_id_t> input;
  std::vector<channel_id_t> output;
};

}  // namespace fastocloud
