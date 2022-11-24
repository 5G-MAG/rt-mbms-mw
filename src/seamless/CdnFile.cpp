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
