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

#include "stream/streams/mosaic_options.h"

namespace fastocloud {
namespace stream {
namespace streams {

SoundInfo::SoundInfo() : channels() {}

MosaicImageOptions::MosaicImageOptions() : screen_size(), right_padding(0), sreams() {}

bool MosaicImageOptions::isValid() const {
  return screen_size.width != 0 && screen_size.height != 0;
}

}  // namespace streams
}  // namespace stream
}  // namespace fastocloud
