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
#include "Service.h"
#include "Receiver.h"
#include "HlsPrimaryPlaylist.h"

#include "spdlog/spdlog.h"
#include "gmime/gmime.h" 
#include "tinyxml2.h" 

auto MBMS_RT::Service::add_name(const std::string& name, const std::string& lang) -> void
{
  spdlog::debug("Service name added: {} ({})", name, lang);
  _names[lang] = name;
}

auto MBMS_RT::Service::read_master_manifest(const std::string& manifest, const std::string& base_path) -> void
{
  spdlog::debug("service: master manifest contents:\n{}, base path {}", manifest, base_path);
  if (_delivery_protocol == DeliveryProtocol::HLS) {
    _hls_primary_playlist = HlsPrimaryPlaylist(manifest, base_path);
  }

  _manifest_path = base_path + "manifest.m3u8",
  _cache.add_item( std::make_shared<CachedPlaylist>(
        _manifest_path,
        0,
        [&]() -> const std::string& {
          spdlog::info("Service: master manifest requested");
            return _manifest;
          }
        ));
}

auto MBMS_RT::Service::add_and_start_content_stream(std::shared_ptr<ContentStream> s) -> void // NOLINT
{
  spdlog::debug("adding stream with playlist path {}", s->playlist_path());
  if (_delivery_protocol == DeliveryProtocol::HLS) {
    for (const auto& stream : _hls_primary_playlist.streams()) {
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

  // recreate the manifest
  HlsPrimaryPlaylist pl;
  for (const auto& stream : _content_streams) {
    HlsPrimaryPlaylist::Stream s{
      "/"+stream.second->playlist_path(),
      stream.second->resolution(),
      stream.second->codecs(),
      stream.second->bandwidth(),
      stream.second->frame_rate()
    };
    pl.add_stream(s);
  }
  _manifest = pl.to_string();

}

auto MBMS_RT::Service::set_delivery_protocol_from_mime_type(const std::string& mime_type) -> void
{
  _delivery_protocol = 
    ( mime_type == "application/vnd.apple.mpegurl" ? DeliveryProtocol::HLS : 
      ( mime_type == "application/dash+xml" ? DeliveryProtocol::DASH : DeliveryProtocol::RTP ) );
  spdlog::debug("Setting delivery type {} from MIME type {}", _delivery_protocol == DeliveryProtocol::HLS ? "HLS" : (
        _delivery_protocol == DeliveryProtocol::DASH ? "DASH" : "RTP"), mime_type);
};

