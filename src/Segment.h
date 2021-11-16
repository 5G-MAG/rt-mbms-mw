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
#include "File.h"
#include "CdnClient.h"
#include "CdnFile.h"
#include "ItemSource.h"

namespace MBMS_RT {
  class Segment {
    public:
      Segment(const std::string& content_location, 
          std::shared_ptr<LibFlute::File> flute_file);
      virtual ~Segment();

      char* buffer();
      uint32_t content_length() const;
      virtual ItemSource data_source() const;

      void set_cdn_client(std::shared_ptr<CdnClient> client) { _cdn_client = client; };
      void fetch_from_cdn();
    private:
      std::string _content_location;
      std::shared_ptr<CdnClient> _cdn_client;

      std::shared_ptr<LibFlute::File> _flute_file;
      std::shared_ptr<CdnFile> _cdn_file;
  };
}
