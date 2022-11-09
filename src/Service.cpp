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

#include <regex>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Service.h"
#include "Receiver.h"
#include "HlsPrimaryPlaylist.h"
#include "DashManifest.h"
#include "Constants.h"

#include "spdlog/spdlog.h"
#include "gmime/gmime.h"
#include "tinyxml2.h"

auto MBMS_RT::Service::add_name(const std::string &name, const std::string &lang) -> void {
  spdlog::debug("Service name added: {} ({})", name, lang);
  _names[lang] = name;
}

auto MBMS_RT::Service::read_master_manifest(const std::string &manifest, const std::string &base_path) -> void {
  spdlog::debug("service: master manifest contents:\n{}, base path {}", manifest, base_path);
  if (_delivery_protocol == DeliveryProtocol::HLS) {
    _hls_primary_playlist = HlsPrimaryPlaylist(manifest, base_path);
  } else if(_delivery_protocol == DeliveryProtocol::DASH) {
    _dash_manifest = DashManifest(manifest, base_path);
  }

  _manifest_path =
      _delivery_protocol == DeliveryProtocol::HLS ? base_path + "manifest.m3u8" : base_path + "manifest.mpd",
      _cache.add_item(std::make_shared<CachedPlaylist>(
          _manifest_path,
          0,
          [&]() -> const std::string & {
            spdlog::debug("Service: master manifest requested");
            return _manifest;
          }
      ));
}

auto MBMS_RT::Service::add_and_start_content_stream(std::shared_ptr<ContentStream> s) -> void // NOLINT
{
  spdlog::debug("adding stream with playlist path {}", s->playlist_path());
  if (_delivery_protocol == DeliveryProtocol::HLS) {
    for (const auto &stream: _hls_primary_playlist.streams()) {
      if (stream.uri == s->playlist_path()) {
        spdlog::debug("matched hls entry with uri {}", stream.uri);
        s->set_resolution(stream.resolution);
        s->set_codecs(stream.codecs);
        s->set_bandwidth(stream.bandwidth);
        s->set_frame_rate(stream.frame_rate);
      }
    }
  }
  _content_streams[s->playlist_path()] = s;
  s->start();

  if (_delivery_protocol == DeliveryProtocol::HLS) {
    // recreate the manifest
    HlsPrimaryPlaylist pl;
    for (const auto &stream: _content_streams) {
      HlsPrimaryPlaylist::Stream s{
          "/" + stream.second->playlist_path(),
          stream.second->resolution(),
          stream.second->codecs(),
          stream.second->bandwidth(),
          stream.second->frame_rate()
      };
      pl.add_stream(s);
    }
    _manifest = pl.to_string();
  } else if(_delivery_protocol == DeliveryProtocol::DASH) {
    _manifest = _dash_manifest.content;
  }
}

auto MBMS_RT::Service::set_delivery_protocol_from_mime_type(const std::string &mime_type) -> void {
  // Need to remove potential profile from mimeType, example:application/dash+xml;profiles=urn:3GPP:PSS:profile:DASH10
  std::vector<std::string> strs;
  boost::split(strs, mime_type, boost::is_any_of(";"));
  std::string adjusted_mime_type = strs.front();
  _delivery_protocol =
      (adjusted_mime_type == ContentTypeConstants::HLS ? DeliveryProtocol::HLS :
       (adjusted_mime_type == ContentTypeConstants::DASH ? DeliveryProtocol::DASH : DeliveryProtocol::RTP));
  spdlog::info("Setting delivery type {} from MIME type {}", _delivery_protocol == DeliveryProtocol::HLS ? "HLS" : (
      _delivery_protocol == DeliveryProtocol::DASH ? "DASH" : "RTP"), mime_type);
};

