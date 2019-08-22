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

#include "stream_commands/commands_info/statistic_info.h"

#include <math.h>

#include "stream_commands/commands_info/details/channel_stats_info.h"

#define FIELD_STREAM_ID "id"
#define FIELD_STREAM_TYPE "type"
#define FIELD_STREAM_CPU "cpu"
#define FIELD_STREAM_RSS "rss"
#define FIELD_STREAM_STATUS "status"
#define FIELD_STREAM_LOOP_START_TIME "loop_start_time"
#define FIELD_STREAM_RESTARTS "restarts"
#define FIELD_STREAM_START_TIME "start_time"
#define FIELD_STREAM_TIMESTAMP "timestamp"

#define FIELD_STREAM_INPUT_STREAMS "input_streams"
#define FIELD_STREAM_OUTPUT_STREAMS "output_streams"

namespace fastocloud {

StatisticInfo::StatisticInfo() : stream_struct_(), cpu_load_(), rss_bytes_(), timestamp_(0) {}

StatisticInfo::StatisticInfo(const StreamStruct& str,
                             cpu_load_t cpu_load,
                             rss_t rss_bytes,
                             fastotv::timestamp_t utc_time)
    : stream_struct_(str), cpu_load_(cpu_load), rss_bytes_(rss_bytes), timestamp_(utc_time) {}

StreamStruct StatisticInfo::GetStreamStruct() const {
  return stream_struct_;
}

StatisticInfo::cpu_load_t StatisticInfo::GetCpuLoad() const {
  return cpu_load_;
}

StatisticInfo::rss_t StatisticInfo::GetRssBytes() const {
  return rss_bytes_;
}

fastotv::timestamp_t StatisticInfo::GetTimestamp() const {
  return timestamp_;
}

common::Error StatisticInfo::SerializeFields(json_object* out) const {
  if (!stream_struct_.IsValid()) {
    return common::make_error_inval();
  }

  const auto channel_id = stream_struct_.id;
  json_object_object_add(out, FIELD_STREAM_ID, json_object_new_string(channel_id.c_str()));
  json_object_object_add(out, FIELD_STREAM_TYPE, json_object_new_int(stream_struct_.type));

  input_channels_info_t input_streams = stream_struct_.input;
  json_object* jinput_streams = json_object_new_array();
  for (auto inf : input_streams) {
    json_object* jinf = nullptr;
    details::ChannelStatsInfo sinf(inf);
    common::Error err = sinf.Serialize(&jinf);
    if (err) {
      continue;
    }
    json_object_array_add(jinput_streams, jinf);
  }
  json_object_object_add(out, FIELD_STREAM_INPUT_STREAMS, jinput_streams);

  output_channels_info_t output_streams = stream_struct_.output;
  json_object* joutput_streams = json_object_new_array();
  for (auto inf : output_streams) {
    json_object* jinf = nullptr;
    details::ChannelStatsInfo sinf(inf);
    common::Error err = sinf.Serialize(&jinf);
    if (err) {
      continue;
    }
    json_object_array_add(joutput_streams, jinf);
  }
  json_object_object_add(out, FIELD_STREAM_OUTPUT_STREAMS, joutput_streams);

  json_object_object_add(out, FIELD_STREAM_LOOP_START_TIME, json_object_new_int64(stream_struct_.loop_start_time));
  json_object_object_add(out, FIELD_STREAM_RSS, json_object_new_int64(rss_bytes_));
  json_object_object_add(out, FIELD_STREAM_CPU, json_object_new_double(cpu_load_));
  json_object_object_add(out, FIELD_STREAM_STATUS, json_object_new_int64(stream_struct_.status));
  json_object_object_add(out, FIELD_STREAM_RESTARTS, json_object_new_int64(stream_struct_.restarts));
  json_object_object_add(out, FIELD_STREAM_START_TIME, json_object_new_int64(stream_struct_.start_time));
  json_object_object_add(out, FIELD_STREAM_TIMESTAMP, json_object_new_int64(timestamp_));
  return common::Error();
}

common::Error StatisticInfo::DoDeSerialize(json_object* serialized) {
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, FIELD_STREAM_ID, &jid);
  if (!jid_exists) {
    return common::make_error_inval();
  }
  const auto cid = json_object_get_string(jid);

  json_object* jtype = nullptr;
  json_bool jtype_exists = json_object_object_get_ex(serialized, FIELD_STREAM_TYPE, &jtype);
  if (!jtype_exists) {
    return common::make_error_inval();
  }
  StreamType type = static_cast<StreamType>(json_object_get_int(jtype));

  input_channels_info_t input;
  json_object* jinput = nullptr;
  json_bool jinput_exists = json_object_object_get_ex(serialized, FIELD_STREAM_INPUT_STREAMS, &jinput);
  if (jinput_exists) {
    size_t len = json_object_array_length(jinput);
    for (size_t i = 0; i < len; ++i) {
      json_object* jin = json_object_array_get_idx(jinput, i);
      details::ChannelStatsInfo sinf;
      common::Error err = sinf.DeSerialize(jin);
      if (err) {
        continue;
      }

      input.push_back(ChannelStats(sinf.GetChannelStats()));
    }
  }

  output_channels_info_t output;
  json_object* joutput = nullptr;
  json_bool joutput_exists = json_object_object_get_ex(serialized, FIELD_STREAM_OUTPUT_STREAMS, &joutput);
  if (joutput_exists) {
    size_t len = json_object_array_length(joutput);
    for (size_t i = 0; i < len; ++i) {
      json_object* jin = json_object_array_get_idx(joutput, i);
      details::ChannelStatsInfo sinf;
      common::Error err = sinf.DeSerialize(jin);
      if (err) {
        continue;
      }

      output.push_back(ChannelStats(sinf.GetChannelStats()));
    }
  }

  StreamStatus st = NEW;
  json_object* jstatus = nullptr;
  json_bool jstatus_exists = json_object_object_get_ex(serialized, FIELD_STREAM_STATUS, &jstatus);
  if (jstatus_exists) {
    st = static_cast<StreamStatus>(json_object_get_int(jstatus));
  }

  cpu_load_t cpu_load = 0;
  json_object* jcpu_load = nullptr;
  json_bool jcpu_load_exists = json_object_object_get_ex(serialized, FIELD_STREAM_CPU, &jcpu_load);
  if (jcpu_load_exists) {
    cpu_load = json_object_get_double(jcpu_load);
  }

  rss_t rss = 0;
  json_object* jrss = nullptr;
  json_bool jrss_exists = json_object_object_get_ex(serialized, FIELD_STREAM_RSS, &jrss);
  if (jrss_exists) {
    rss = json_object_get_int(jrss);
  }

  fastotv::timestamp_t time = 0;
  json_object* jtime = nullptr;
  json_bool jtime_exists = json_object_object_get_ex(serialized, FIELD_STREAM_TIMESTAMP, &jtime);
  if (jtime_exists) {
    time = json_object_get_int64(jtime);
  }

  fastotv::timestamp_t start_time = 0;
  json_object* jstart_time = nullptr;
  json_bool jstart_time_exists = json_object_object_get_ex(serialized, FIELD_STREAM_START_TIME, &jstart_time);
  if (jstart_time_exists) {
    start_time = json_object_get_int64(jstart_time);
  }

  size_t restarts = 0;
  json_object* jrestarts = nullptr;
  json_bool jrestarts_exists = json_object_object_get_ex(serialized, FIELD_STREAM_RESTARTS, &jrestarts);
  if (jrestarts_exists) {
    restarts = json_object_get_int64(jrestarts);
  }

  fastotv::timestamp_t loop_start_time = 0;
  json_object* jloop_start_time = nullptr;
  json_bool jloop_start_time_exists =
      json_object_object_get_ex(serialized, FIELD_STREAM_LOOP_START_TIME, &jloop_start_time);
  if (jloop_start_time_exists) {
    loop_start_time = json_object_get_int64(jloop_start_time);
  }

  StreamStruct strct(cid, type, st, input, output, start_time, loop_start_time, restarts);
  *this = StatisticInfo(strct, cpu_load, rss, time);
  return common::Error();
}

}  // namespace fastocloud
