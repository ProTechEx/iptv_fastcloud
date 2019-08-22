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

#include "stream/streams_factory.h"

#include <string>

#include "base/config_fields.h"

#include "stream/link_generator/ilink_generator.h"
#include "stream/streams/encoding/device_stream.h"
#include "stream/streams/encoding/encoding_only_audio_stream.h"
#include "stream/streams/encoding/encoding_only_video_stream.h"
#include "stream/streams/encoding/playlist_encoding_stream.h"
#include "stream/streams/encoding/rtsp_encoding_stream.h"
#include "stream/streams/mosaic_stream.h"
#include "stream/streams/relay/playlist_relay_stream.h"
#include "stream/streams/relay/rtsp_relay_stream.h"
#include "stream/streams/test/test_life_stream.h"
#include "stream/streams/test/test_stream.h"
#include "stream/streams/timeshift/catchup_stream.h"
#include "stream/streams/timeshift/timeshift_player_stream.h"
#include "stream/streams/timeshift/timeshift_recorder_stream.h"
#include "stream/streams/vod/vod_encoding_stream.h"
#include "stream/streams/vod/vod_relay_stream.h"

namespace fastocloud {
namespace stream {

IBaseStream* StreamsFactory::CreateStream(const Config* config,
                                          IBaseStream::IStreamClient* client,
                                          StreamStruct* stats,
                                          const TimeShiftInfo& tinfo,
                                          chunk_index_t start_chunk_index) {
  input_t input = config->GetInput();
  StreamType type = config->GetType();
  if (type == PROXY) {
    NOTREACHED();
    return nullptr;
  } else if (type == RELAY || type == COD_RELAY) {
    const streams::RelayConfig* rconfig = static_cast<const streams::RelayConfig*>(config);
    if (input.size() > 1) {
      bool is_playlist = true;
      for (InputUri iuri : input) {
        common::uri::Url input_uri = iuri.GetInput();
        is_playlist &= input_uri.GetScheme() == common::uri::Url::file;
      }
      if (is_playlist) {  // playlist
        const streams::PlaylistRelayConfig* prconfig = static_cast<const streams::PlaylistRelayConfig*>(config);
        return new streams::PlaylistRelayStream(prconfig, client, stats);
      }

      NOTREACHED();
      return nullptr;  // not supported
      // return new streams::MosaicStream(rconfig, client, stats);
    }

    InputUri iuri = input[0];
    if (iuri.GetInput().GetScheme() == common::uri::Url::rtsp) {
      return new streams::RtspRelayStream(rconfig, client, stats);
    }

    return new streams::RelayStream(rconfig, client, stats);
  } else if (type == ENCODE || type == COD_ENCODE) {
    const streams::EncodeConfig* econfig = static_cast<const streams::EncodeConfig*>(config);
    if (input.size() > 1) {
      bool is_playlist = true;
      for (InputUri iuri : input) {
        common::uri::Url input_uri = iuri.GetInput();
        is_playlist &= input_uri.GetScheme() == common::uri::Url::file;
      }
      if (is_playlist) {  // playlist
        return new streams::PlaylistEncodingStream(econfig, client, stats);
      }

      return new streams::MosaicStream(econfig, client, stats);
    }

    InputUri iuri = input[0];
    if (IsTestInputUrl(iuri)) {
      return new streams::TestInputStream(econfig, client, stats);
    }

    if (iuri.GetInput().GetScheme() == common::uri::Url::dev) {
      return new streams::encoding::DeviceStream(econfig, client, stats);
    } else if (iuri.GetInput().GetScheme() == common::uri::Url::rtsp) {
      return new streams::RtspEncodingStream(econfig, client, stats);
    }

    if (econfig->GetRelayVideo()) {
      return new streams::EncodingOnlyAudioStream(econfig, client, stats);
    } else if (econfig->GetRelayAudio()) {
      return new streams::EncodingOnlyVideoStream(econfig, client, stats);
    }

    return new streams::EncodingStream(econfig, client, stats);
  } else if (type == TIMESHIFT_PLAYER) {
    const streams::RelayConfig* tconfig = static_cast<const streams::RelayConfig*>(config);
    return new streams::TimeShiftPlayerStream(tconfig, tinfo, client, stats, start_chunk_index);
  } else if (type == TIMESHIFT_RECORDER) {
    const streams::TimeshiftConfig* tconfig = static_cast<const streams::TimeshiftConfig*>(config);
    return new streams::TimeShiftRecorderStream(tconfig, tinfo, client, stats);
  } else if (type == CATCHUP) {
    const streams::TimeshiftConfig* tconfig = static_cast<const streams::TimeshiftConfig*>(config);
    return new streams::CatchupStream(tconfig, tinfo, client, stats);
  } else if (type == TEST_LIFE) {
    const streams::RelayConfig* rconfig = static_cast<const streams::RelayConfig*>(config);
    return new streams::test::TestLifeStream(rconfig, client, stats);
  } else if (type == VOD_RELAY) {
    const streams::VodRelayConfig* vconfig = static_cast<const streams::VodRelayConfig*>(config);
    if (vconfig->IsVod()) {
      return new streams::VodRelayStream(vconfig, client, stats);
    }
  } else if (type == VOD_ENCODE) {
    const streams::VodEncodeConfig* vconfig = static_cast<const streams::VodEncodeConfig*>(config);
    if (vconfig->IsVod()) {
      return new streams::VodEncodeStream(vconfig, client, stats);
    }
  }

  NOTREACHED();
  return nullptr;
}

}  // namespace stream
}  // namespace fastocloud
