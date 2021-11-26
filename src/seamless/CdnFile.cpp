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

#include "CdnFile.h"
#include "spdlog/spdlog.h"

MBMS_RT::CdnFile::CdnFile(size_t length)
  : _length( length )
{
  spdlog::debug("CdnFile with size {} created", length);
  _buffer = (char*)malloc(length);
  if (_buffer == nullptr) {
    throw "Failed to allocate CDN file buffer";
  }
}

MBMS_RT::CdnFile::~CdnFile() {
  spdlog::debug("CdnFile destroyed");
  if (_buffer != nullptr) {
    free(_buffer);
  }
}
