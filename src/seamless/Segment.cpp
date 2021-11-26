// 5G-MAG Reference Tools
// MBMS Middleware Process
//
// Copyright (C) 2021 Klaus Kühnhammer (Österreichische Rundfunksender GmbH & Co KG)
//
// Licensed under the License terms and conditions for use, reproduction, and
// distribution of 5G-MAG software (the “License”).  You may not use this file
// except in compliance with the License.  You may obtain a copy of the License at
// https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
// agreed to in writing, software distributed under the License is distributed on
// an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.
// 
// See the License for the specific language governing permissions and limitations
// under the License.
//

#include "Segment.h"
#include <future>

#include "spdlog/spdlog.h"

MBMS_RT::Segment::Segment(std::string content_location,
    int seq, double extinf)
  : _content_location( std::move(content_location) )
  , _seq(seq)
  , _extinf(extinf)
{
  spdlog::debug(" Segment at {} created", _content_location);
}

MBMS_RT::Segment::~Segment() {
  spdlog::debug(" Segment at {} destroyed", _content_location);
}

auto MBMS_RT::Segment::fetch_from_cdn() -> void
{
  if (_cdn_client) {
    spdlog::debug("Requesting segment from CDN at {}", _content_location);
    _cdn_client->get(_content_location,
        [&](std::shared_ptr<CdnFile> file) -> void {
        spdlog::debug("Segment at {} received data from CDN", _content_location);
        _content_received_at = time(nullptr);
        _cdn_file = std::move(file);
        });
  }
}

auto MBMS_RT::Segment::buffer() -> char*
{
  if (_flute_file && _flute_file->complete()) {
    spdlog::debug("Segment at {} returning FLUTE file buffer ptr", _content_location);
    return _flute_file->buffer();
  } else if (_cdn_file) {
    spdlog::debug("Segment at {} returning CDN file buffer ptr", _content_location);
    return _cdn_file->buffer();
  } else {
    fetch_from_cdn();
    spdlog::debug("Segment at {} has no data", _content_location);
    return nullptr;
  };
}

auto MBMS_RT::Segment::content_length() const -> uint32_t
{
  if (_flute_file &&_flute_file->complete()) {
    return _flute_file->length();
  } else if (_cdn_file) {
    return _cdn_file->length();
  } else {
    return 0;
  };
}

auto MBMS_RT::Segment::data_source() const -> MBMS_RT::ItemSource
{
  if (_flute_file &&_flute_file->complete()) {
    return MBMS_RT::ItemSource::Broadcast;
  } else if (_cdn_file) {
    return MBMS_RT::ItemSource::CDN;
  } else {
    return MBMS_RT::ItemSource::Unavailable;
  }
}
