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
      virtual ~SeamlessContentStream();

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
      
      bool _running = true;
  };
}
