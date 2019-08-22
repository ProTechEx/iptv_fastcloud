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

#include "base/inputs_outputs.h"

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#include <common/convert2string.h>

#include "base/config_fields.h"

#define FIELD_OUTPUT_URLS "urls"
#define FIELD_INPUT_URLS "urls"

namespace fastocloud {
namespace {

bool ConvertFromString(common::HashValue* output_urls, fastocloud::output_t* out) {
  if (!output_urls || !out) {
    return false;
  }

  common::Value* urls = output_urls->Find(FIELD_OUTPUT_URLS);
  common::ArrayValue* arr = nullptr;
  if (!urls || !urls->GetAsList(&arr)) {
    return false;
  }

  fastocloud::output_t output;
  for (size_t i = 0; i < arr->GetSize(); ++i) {
    common::Value* url = nullptr;
    common::HashValue* url_hash = nullptr;
    if (arr->Get(i, &url) && url->GetAsHash(&url_hash)) {
      const auto murl = OutputUri::MakeUrl(url_hash);
      if (murl) {
        output.push_back(*murl);
      }
    }
  }
  *out = output;
  return true;
}

bool ConvertFromString(common::HashValue* input_urls, fastocloud::input_t* out) {
  if (!input_urls || !out) {
    return false;
  }

  common::Value* urls = input_urls->Find(FIELD_INPUT_URLS);
  common::ArrayValue* arr = nullptr;
  if (!urls || !urls->GetAsList(&arr)) {
    return false;
  }

  fastocloud::input_t input;
  for (size_t i = 0; i < arr->GetSize(); ++i) {
    common::Value* url = nullptr;
    common::HashValue* url_hash = nullptr;
    if (arr->Get(i, &url) && url->GetAsHash(&url_hash)) {
      const auto murl = InputUri::MakeUrl(url_hash);
      if (murl) {
        input.push_back(*murl);
      }
    }
  }
  *out = input;
  return true;
}

}  // namespace

bool read_input(const StreamConfig& config, input_t* input) {
  if (!config || !input) {
    return false;
  }

  common::Value* input_field = config->Find(INPUT_FIELD);
  common::HashValue* input_hash = nullptr;
  if (!input_field || !input_field->GetAsHash(&input_hash)) {
    return false;
  }

  input_t linput;
  if (!ConvertFromString(input_hash, &linput)) {
    return false;
  }

  *input = linput;
  return true;
}

bool read_output(const StreamConfig& config, output_t* output) {
  if (!config || !output) {
    return false;
  }

  common::Value* output_field = config->Find(OUTPUT_FIELD);
  common::HashValue* output_hash = nullptr;
  if (!output_field || !output_field->GetAsHash(&output_hash)) {
    return false;
  }

  output_t loutput;
  if (!ConvertFromString(output_hash, &loutput)) {
    return false;
  }

  *output = loutput;
  return true;
}

}  // namespace fastocloud
