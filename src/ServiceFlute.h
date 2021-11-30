// 5G-MAG Reference Tools
// MBMS Middleware Process
//
// Author: Romain Bouqueau
// Copyright Motion Spell / GPAC Licensing
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

#include "Service.h"

namespace LibFlute {
class Receiver;
}

struct ReceiverFlute : MBMS_RT::IReceiver {
  ReceiverFlute(const std::string& iface, const std::string& address,
      short port, uint64_t tsi,
      boost::asio::io_service& io_service);
  std::vector<std::shared_ptr<MBMS_RT::IFile>> file_list() override;
  void remove_expired_files(unsigned max_age) override;

private:
  std::shared_ptr<LibFlute::Receiver> delegate;
};
