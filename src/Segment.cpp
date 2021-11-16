// OBECA - Open Broadcast Edge Cache Appliance
// Gateway Process
//
// Copyright (C) 2021 Klaus Kühnhammer (Österreichische Rundfunksender GmbH & Co KG)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "Segment.h"

#include "spdlog/spdlog.h"

MBMS_RT::Segment::Segment(const std::string& content_location,
          std::shared_ptr<LibFlute::File> flute_file)
  : _content_location( content_location )
  , _flute_file( flute_file )
{
  spdlog::debug("Segment at {} created", _content_location);
  int rnd = rand() % 100;
  if (rnd < 50) {
    _flute_file = nullptr;
  }
}

MBMS_RT::Segment::~Segment() {
  spdlog::debug("Segment at {} destroyed", _content_location);
}

auto MBMS_RT::Segment::fetch_from_cdn() -> void
{
  if (_cdn_client) {
    _cdn_client->get(_content_location,
        [&](std::shared_ptr<CdnFile> file) -> void {
        spdlog::debug("Segment at {} received data from CDN", _content_location);
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
