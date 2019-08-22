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

#include "stream/config.h"

namespace fastocloud {
namespace stream {
namespace streams {

class AudioVideoConfig : public Config {
 public:
  typedef Config base_class;
  typedef common::Optional<int> audio_select_t;
  typedef bool loop_t;
  typedef bool avformat_t;
  typedef bool have_stream_t;
  explicit AudioVideoConfig(const base_class& config);

  have_stream_t HaveVideo() const;  // relay, encoding
  void SetHaveVideo(have_stream_t have_video);

  have_stream_t HaveAudio() const;  // relay, encoding
  void SetHaveAudio(have_stream_t have_audio);

  audio_select_t GetAudioSelect() const;
  void SetAudioSelect(audio_select_t sel);

  avformat_t IsAvFormat() const;
  void SetIsAvFormat(avformat_t av);

  loop_t GetLoop() const;
  void SetLoop(loop_t loop);

  bool IsVod() const;

  AudioVideoConfig* Clone() const override;

 private:
  have_stream_t have_video_;
  have_stream_t have_audio_;
  audio_select_t audio_select_;
  avformat_t avformat_;
  loop_t loop_;
};

}  // namespace streams
}  // namespace stream
}  // namespace fastocloud
