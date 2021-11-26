// 5G-MAG Reference Tools
// MBMS Middleware Process
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

#pragma once

#include <string>
#include <thread>
#include <libconfig.h++>
#include "File.h"
#include "Receiver.h"
#include "CacheManagement.h"
#include "CdnClient.h"
#include "seamless/Segment.h"
#include "ContentStream.h"
#include <mutex>

namespace MBMS_RT {
  class SeamlessContentStream : public ContentStream{
    public:
      SeamlessContentStream(std::string base, std::string flute_if, boost::asio::io_service& io_service, CacheManagement& cache, DeliveryProtocol protocol, const libconfig::Config& cfg);
      virtual ~SeamlessContentStream() = default;

      virtual StreamType stream_type() const { return StreamType::SeamlessSwitching; };
      virtual std::string stream_type_string() const { return "Seamless Switching"; };

      void set_cdn_endpoint(const std::string& cdn_ept);
      virtual void flute_file_received(std::shared_ptr<LibFlute::File> file);

      std::string cdn_endpoint() const { return _cdn_endpoint + _playlist_path; };
    private:
      void handle_playlist( const std::string& content, ItemSource source);
      void tick_handler();

      std::string _cdn_endpoint = "none";
      std::shared_ptr<CdnClient> _cdn_client;
      std::string _playlist_dir;
      std::string _playlist = "none";
      std::string _manifest;

      std::map<int, std::shared_ptr<Segment>> _segments;
      std::map<std::string, std::shared_ptr<LibFlute::File>> _flute_files;
      std::mutex _segments_mutex;

      boost::posix_time::seconds _tick_interval;
      boost::asio::deadline_timer _timer;

      int _segments_to_keep = 10;
      int _truncate_cdn_playlist_segments = 7;
  };
}
