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

#include "server/daemon/commands_info/service/server_info.h"

#define STATISTIC_SERVICE_INFO_UPTIME_FIELD "uptime"
#define STATISTIC_SERVICE_INFO_TIMESTAMP_FIELD "timestamp"
#define STATISTIC_SERVICE_INFO_CPU_FIELD "cpu"
#define STATISTIC_SERVICE_INFO_GPU_FIELD "gpu"
#define STATISTIC_SERVICE_INFO_MEMORY_TOTAL_FIELD "memory_total"
#define STATISTIC_SERVICE_INFO_MEMORY_FREE_FIELD "memory_free"

#define STATISTIC_SERVICE_INFO_HDD_TOTAL_FIELD "hdd_total"
#define STATISTIC_SERVICE_INFO_HDD_FREE_FIELD "hdd_free"

#define STATISTIC_SERVICE_INFO_LOAD_AVERAGE_FIELD "load_average"

#define STATISTIC_SERVICE_INFO_BANDWIDTH_IN_FIELD "bandwidth_in"
#define STATISTIC_SERVICE_INFO_BANDWIDTH_OUT_FIELD "bandwidth_out"

#define STATISTIC_SERVICE_INFO_ONLINE_USERS_FIELD "online_users"

#define FULL_SERVICE_INFO_OS_FIELD "os"
#define FULL_SERVICE_INFO_VERSION_FIELD "version"
#define FULL_SERVICE_INFO_HTTP_HOST_FIELD "http_host"
#define FULL_SERVICE_INFO_VODS_HOST_FIELD "vods_host"
#define FULL_SERVICE_INFO_CODS_HOST_FIELD "cods_host"

#define ONLINE_USERS_DAEMON_FIELD "daemon"
#define ONLINE_USERS_HTTP_FIELD "http"
#define ONLINE_USERS_VODS_FIELD "vods"
#define ONLINE_USERS_CODS_FIELD "cods"

namespace fastocloud {
namespace server {
namespace service {

OnlineUsers::OnlineUsers() : OnlineUsers(0, 0, 0, 0) {}

OnlineUsers::OnlineUsers(size_t daemon, size_t http, size_t vods, size_t cods)
    : daemon_(daemon), http_(http), vods_(vods), cods_(cods) {}

common::Error OnlineUsers::DoDeSerialize(json_object* serialized) {
  OnlineUsers inf;
  json_object* jdaemon = nullptr;
  json_bool jdaemon_exists = json_object_object_get_ex(serialized, ONLINE_USERS_DAEMON_FIELD, &jdaemon);
  if (jdaemon_exists) {
    inf.daemon_ = json_object_get_int64(jdaemon);
  }

  json_object* jhttp = nullptr;
  json_bool jhttp_exists = json_object_object_get_ex(serialized, ONLINE_USERS_HTTP_FIELD, &jhttp);
  if (jhttp_exists) {
    inf.http_ = json_object_get_int64(jhttp);
  }

  json_object* jvods = nullptr;
  json_bool jvods_exists = json_object_object_get_ex(serialized, ONLINE_USERS_VODS_FIELD, &jvods);
  if (jvods_exists) {
    inf.vods_ = json_object_get_int64(jvods);
  }

  json_object* jcods = nullptr;
  json_bool jcods_exists = json_object_object_get_ex(serialized, ONLINE_USERS_CODS_FIELD, &jcods);
  if (jcods_exists) {
    inf.cods_ = json_object_get_int64(jcods);
  }

  *this = inf;
  return common::Error();
}

common::Error OnlineUsers::SerializeFields(json_object* out) const {
  json_object_object_add(out, ONLINE_USERS_DAEMON_FIELD, json_object_new_int64(daemon_));
  json_object_object_add(out, ONLINE_USERS_HTTP_FIELD, json_object_new_int64(http_));
  json_object_object_add(out, ONLINE_USERS_VODS_FIELD, json_object_new_int64(vods_));
  json_object_object_add(out, ONLINE_USERS_CODS_FIELD, json_object_new_int64(cods_));
  return common::Error();
}

ServerInfo::ServerInfo()
    : base_class(),
      cpu_load_(),
      gpu_load_(),
      uptime_(),
      mem_shot_(),
      hdd_shot_(),
      net_bytes_recv_(),
      net_bytes_send_(),
      current_ts_(),
      sys_shot_(),
      online_users_() {}

ServerInfo::ServerInfo(cpu_load_t cpu_load,
                       gpu_load_t gpu_load,
                       const std::string& uptime,
                       const MemoryShot& mem_shot,
                       const HddShot& hdd_shot,
                       fastotv::bandwidth_t net_bytes_recv,
                       fastotv::bandwidth_t net_bytes_send,
                       const SysinfoShot& sys,
                       fastotv::timestamp_t timestamp,
                       const OnlineUsers& online_users)
    : base_class(),
      cpu_load_(cpu_load),
      gpu_load_(gpu_load),
      uptime_(uptime),
      mem_shot_(mem_shot),
      hdd_shot_(hdd_shot),
      net_bytes_recv_(net_bytes_recv),
      net_bytes_send_(net_bytes_send),
      current_ts_(timestamp),
      sys_shot_(sys),
      online_users_(online_users) {}

common::Error ServerInfo::SerializeFields(json_object* out) const {
  json_object* obj = nullptr;
  common::Error err = online_users_.Serialize(&obj);
  if (err) {
    return err;
  }

  json_object_object_add(out, STATISTIC_SERVICE_INFO_CPU_FIELD, json_object_new_double(cpu_load_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_GPU_FIELD, json_object_new_double(gpu_load_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_LOAD_AVERAGE_FIELD, json_object_new_string(uptime_.c_str()));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_MEMORY_TOTAL_FIELD,
                         json_object_new_int64(mem_shot_.ram_bytes_total));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_MEMORY_FREE_FIELD,
                         json_object_new_int64(mem_shot_.ram_bytes_free));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_HDD_TOTAL_FIELD, json_object_new_int64(hdd_shot_.hdd_bytes_total));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_HDD_FREE_FIELD, json_object_new_int64(hdd_shot_.hdd_bytes_free));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_BANDWIDTH_IN_FIELD, json_object_new_int64(net_bytes_recv_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_BANDWIDTH_OUT_FIELD, json_object_new_int64(net_bytes_send_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_UPTIME_FIELD, json_object_new_int64(sys_shot_.uptime));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_TIMESTAMP_FIELD, json_object_new_int64(current_ts_));
  json_object_object_add(out, STATISTIC_SERVICE_INFO_ONLINE_USERS_FIELD, obj);
  return common::Error();
}

common::Error ServerInfo::DoDeSerialize(json_object* serialized) {
  ServerInfo inf;

  json_object* jonline = nullptr;
  json_bool jonline_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_ONLINE_USERS_FIELD, &jonline);
  if (jonline_exists) {
    common::Error err = inf.online_users_.DeSerialize(jonline);
    if (err) {
      return err;
    }
  }

  json_object* jcpu_load = nullptr;
  json_bool jcpu_load_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_CPU_FIELD, &jcpu_load);
  if (jcpu_load_exists) {
    inf.cpu_load_ = json_object_get_double(jcpu_load);
  }

  json_object* jgpu_load = nullptr;
  json_bool jgpu_load_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_GPU_FIELD, &jgpu_load);
  if (jgpu_load_exists) {
    inf.gpu_load_ = json_object_get_double(jgpu_load);
  }

  json_object* juptime = nullptr;
  json_bool juptime_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_LOAD_AVERAGE_FIELD, &juptime);
  if (juptime_exists) {
    inf.uptime_ = json_object_get_string(juptime);
  }

  json_object* jmemory_total = nullptr;
  json_bool jmemory_total_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_MEMORY_TOTAL_FIELD, &jmemory_total);
  if (jmemory_total_exists) {
    inf.mem_shot_.ram_bytes_total = json_object_get_int64(jmemory_total);
  }

  json_object* jmemory_avail = nullptr;
  json_bool jmemory_avail_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_MEMORY_FREE_FIELD, &jmemory_avail);
  if (jmemory_avail_exists) {
    inf.mem_shot_.ram_bytes_free = json_object_get_int64(jmemory_avail);
  }

  json_object* jhdd_total = nullptr;
  json_bool jhdd_total_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_HDD_TOTAL_FIELD, &jhdd_total);
  if (jhdd_total_exists) {
    inf.hdd_shot_.hdd_bytes_total = json_object_get_int64(jhdd_total);
  }

  json_object* jhdd_free = nullptr;
  json_bool jhdd_free_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_HDD_FREE_FIELD, &jhdd_free);
  if (jhdd_free_exists) {
    inf.hdd_shot_.hdd_bytes_free = json_object_get_int64(jhdd_free);
  }

  json_object* jnet_bytes_recv = nullptr;
  json_bool jnet_bytes_recv_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_BANDWIDTH_IN_FIELD, &jnet_bytes_recv);
  if (jnet_bytes_recv_exists) {
    inf.net_bytes_recv_ = json_object_get_int64(jnet_bytes_recv);
  }

  json_object* jnet_bytes_send = nullptr;
  json_bool jnet_bytes_send_exists =
      json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_BANDWIDTH_OUT_FIELD, &jnet_bytes_send);
  if (jnet_bytes_send_exists) {
    inf.net_bytes_send_ = json_object_get_int64(jnet_bytes_send);
  }

  json_object* jsys_stamp = nullptr;
  json_bool jsys_stamp_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_UPTIME_FIELD, &jsys_stamp);
  if (jsys_stamp_exists) {
    inf.sys_shot_.uptime = json_object_get_int64(jsys_stamp);
  }

  json_object* jcur_ts = nullptr;
  json_bool jcur_ts_exists = json_object_object_get_ex(serialized, STATISTIC_SERVICE_INFO_TIMESTAMP_FIELD, &jcur_ts);
  if (jcur_ts_exists) {
    inf.current_ts_ = json_object_get_int64(jcur_ts);
  }

  *this = inf;
  return common::Error();
}

ServerInfo::cpu_load_t ServerInfo::GetCpuLoad() const {
  return cpu_load_;
}

ServerInfo::gpu_load_t ServerInfo::GetGpuLoad() const {
  return gpu_load_;
}

std::string ServerInfo::GetUptime() const {
  return uptime_;
}

MemoryShot ServerInfo::GetMemShot() const {
  return mem_shot_;
}

HddShot ServerInfo::GetHddShot() const {
  return hdd_shot_;
}

fastotv::bandwidth_t ServerInfo::GetNetBytesRecv() const {
  return net_bytes_recv_;
}

fastotv::bandwidth_t ServerInfo::GetNetBytesSend() const {
  return net_bytes_send_;
}

fastotv::timestamp_t ServerInfo::GetTimestamp() const {
  return current_ts_;
}

OnlineUsers ServerInfo::GetOnlineUsers() const {
  return online_users_;
}

FullServiceInfo::FullServiceInfo()
    : base_class(),
      http_host_(),
      proj_ver_(PROJECT_VERSION_HUMAN),
      os_(fastotv::commands_info::OperationSystemInfo::MakeOSSnapshot()) {}

FullServiceInfo::FullServiceInfo(const common::net::HostAndPort& http_host,
                                 const common::net::HostAndPort& vods_host,
                                 const common::net::HostAndPort& cods_host,
                                 const base_class& base)
    : base_class(base),
      http_host_(http_host),
      vods_host_(vods_host),
      cods_host_(cods_host),
      proj_ver_(PROJECT_VERSION_HUMAN),
      os_(fastotv::commands_info::OperationSystemInfo::MakeOSSnapshot()) {}

common::net::HostAndPort FullServiceInfo::GetHttpHost() const {
  return http_host_;
}

common::net::HostAndPort FullServiceInfo::GetVodsHost() const {
  return vods_host_;
}

common::net::HostAndPort FullServiceInfo::GetCodsHost() const {
  return cods_host_;
}

std::string FullServiceInfo::GetProjectVersion() const {
  return proj_ver_;
}

common::Error FullServiceInfo::DoDeSerialize(json_object* serialized) {
  FullServiceInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jos = nullptr;
  json_bool jos_exists = json_object_object_get_ex(serialized, FULL_SERVICE_INFO_OS_FIELD, &jos);
  if (jos_exists) {
    common::Error err = inf.os_.DeSerialize(jos);
    if (err) {
      return err;
    }
  }

  json_object* jhttp_host = nullptr;
  json_bool jhttp_host_exists = json_object_object_get_ex(serialized, FULL_SERVICE_INFO_HTTP_HOST_FIELD, &jhttp_host);
  if (jhttp_host_exists) {
    common::net::HostAndPort host;
    if (common::ConvertFromString(json_object_get_string(jhttp_host), &host)) {
      inf.http_host_ = host;
    }
  }

  json_object* jvods_host = nullptr;
  json_bool jvods_host_exists = json_object_object_get_ex(serialized, FULL_SERVICE_INFO_VODS_HOST_FIELD, &jvods_host);
  if (jvods_host_exists) {
    common::net::HostAndPort host;
    if (common::ConvertFromString(json_object_get_string(jvods_host), &host)) {
      inf.vods_host_ = host;
    }
  }

  json_object* jcods_host = nullptr;
  json_bool jcods_host_exists = json_object_object_get_ex(serialized, FULL_SERVICE_INFO_CODS_HOST_FIELD, &jcods_host);
  if (jcods_host_exists) {
    common::net::HostAndPort host;
    if (common::ConvertFromString(json_object_get_string(jcods_host), &host)) {
      inf.cods_host_ = host;
    }
  }

  json_object* jproj_ver = nullptr;
  json_bool jproj_ver_exists = json_object_object_get_ex(serialized, FULL_SERVICE_INFO_VERSION_FIELD, &jproj_ver);
  if (jproj_ver_exists) {
    inf.proj_ver_ = json_object_get_string(jproj_ver);
  }

  *this = inf;
  return common::Error();
}

common::Error FullServiceInfo::SerializeFields(json_object* out) const {
  json_object* jos = nullptr;
  common::Error err = os_.Serialize(&jos);
  if (err) {
    return err;
  }

  std::string http_host_str = common::ConvertToString(http_host_);
  std::string vods_host_str = common::ConvertToString(vods_host_);
  std::string cods_host_str = common::ConvertToString(cods_host_);
  json_object_object_add(out, FULL_SERVICE_INFO_HTTP_HOST_FIELD, json_object_new_string(http_host_str.c_str()));
  json_object_object_add(out, FULL_SERVICE_INFO_VODS_HOST_FIELD, json_object_new_string(vods_host_str.c_str()));
  json_object_object_add(out, FULL_SERVICE_INFO_CODS_HOST_FIELD, json_object_new_string(cods_host_str.c_str()));
  json_object_object_add(out, FULL_SERVICE_INFO_VERSION_FIELD, json_object_new_string(proj_ver_.c_str()));
  json_object_object_add(out, FULL_SERVICE_INFO_OS_FIELD, jos);
  return base_class::SerializeFields(out);
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
