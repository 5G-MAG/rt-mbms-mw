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

#include <regex>
#include "Service.h"
#include "Receiver.h"

#include "spdlog/spdlog.h"
#include "gmime/gmime.h" 
#include "tinyxml2.h" 


MBMS_RT::Service::Service(const libconfig::Config& cfg, std::string tmgi, const std::string& mcast, unsigned long long tsi, std::string iface, boost::asio::io_service& io_service)
{
}

MBMS_RT::Service::Service()
{
  spdlog::debug("Service created");
}

MBMS_RT::Service::~Service() 
{
  spdlog::debug("Service destroyed");
}

auto MBMS_RT::Service::add_name(std::string name, std::string lang) -> void
{
  spdlog::debug("Service name added: {} ({})", name, lang);
}
