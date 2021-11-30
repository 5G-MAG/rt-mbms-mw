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
#include "File.h"
#include "seamless/CdnClient.h"
#include "seamless/CdnFile.h"
#include "ItemSource.h"
#include "Segment.h"

namespace MBMS_RT {
  class Segment {
    public:
      Segment(std::string content_location, 
          int seq, double extinf);
      virtual ~Segment();

      char* buffer();
      uint32_t content_length() const;
      virtual ItemSource data_source() const;

      void set_cdn_client(std::shared_ptr<CdnClient> client) { _cdn_client = client; };
      void fetch_from_cdn();

      void set_flute_file(std::shared_ptr<LibFlute::File> file) { _flute_file = file; _content_received_at = file->received_at(); };

      std::string uri() const { return _content_location; };
      int seq() const { return _seq; };
      double extinf() const { return _extinf; };

      unsigned long received_at() const { return _content_received_at; };
    private:
      std::string _content_location;
      std::shared_ptr<CdnClient> _cdn_client;

      std::shared_ptr<LibFlute::File> _flute_file;
      std::shared_ptr<CdnFile> _cdn_file;
      int _seq;
      double _extinf;

      unsigned long _content_received_at = 0;

  };
}
