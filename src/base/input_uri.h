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

#include <common/serializer/json_serializer.h>
#include <common/uri/url.h>
#include <common/value.h>

#include "base/http_proxy.h"
#include "base/types.h"

namespace fastocloud {

class InputUri : public common::serializer::JsonSerializer<InputUri> {
 public:
  typedef JsonSerializer<InputUri> base_class;
  typedef channel_id_t uri_id_t;
  typedef common::Optional<HttpProxy> http_proxy_url_t;
  enum UserAgent : int { GSTREAMER = 0, VLC = 1, FFMPEG = 2, WINK = 3 };
  typedef UserAgent user_agent_t;

  InputUri();
  explicit InputUri(uri_id_t id, const common::uri::Url& input, user_agent_t ua = GSTREAMER);

  uri_id_t GetID() const;
  void SetID(uri_id_t id);

  common::uri::Url GetInput() const;
  void SetInput(const common::uri::Url& uri);

  user_agent_t GetUserAgent() const;
  void SetUserAgent(user_agent_t agent);

  bool GetStreamLink() const;
  void SetStreamLink(bool stream);

  http_proxy_url_t GetHttpProxyUrl() const;
  void SetHttpProxyUrl(const http_proxy_url_t& url);

  bool Equals(const InputUri& inf) const;

  static common::Optional<InputUri> MakeUrl(common::HashValue* hash);

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  uri_id_t id_;
  common::uri::Url input_;
  user_agent_t user_agent_;
  bool stream_url_;
  http_proxy_url_t http_proxy_url_;
};

bool IsTestInputUrl(const InputUri& url);

}  // namespace fastocloud
