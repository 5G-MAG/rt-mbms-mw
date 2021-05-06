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
#include <string>
#include "File.h"

namespace fcl {
  extern "C" {
#include "flutelib/flute.h"
#include "flutelib/receiver.h"
  }
}


#pragma once
namespace OBECA {
class FluteReceiver {
  public:
    FluteReceiver( const std::string& iface, const std::string& address, 
        const std::string& port, unsigned long long tsi, const std::string& target_directory);

    virtual ~FluteReceiver();
    void start();

    std::vector<OBECA::File> fileList();

  private:
    int _alc_session_id = {}; 
    fcl::arguments_t _a = {};
    fcl::flute_receiver_t _receiver = {};
};
}
