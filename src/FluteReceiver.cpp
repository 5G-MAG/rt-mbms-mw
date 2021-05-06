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

#include "spdlog/spdlog.h"
#include "FluteReceiver.h"


OBECA::FluteReceiver::FluteReceiver ( const std::string& iface, const std::string& address, const std::string&  port, unsigned long long tsi, const std::string& target_directory)
{
	char addrs[MAX_CHANNELS_IN_SESSION][INET6_ADDRSTRLEN];  /* Mcast addresses */
	char ports[MAX_CHANNELS_IN_SESSION][MAX_PORT_LENGTH];         /* Local port numbers  */
  memset(addrs[0], 0, INET6_ADDRSTRLEN);
  memset(ports[0], 0, MAX_PORT_LENGTH);

  memcpy(addrs[0], address.c_str(), address.length());
  memcpy(ports[0], port.c_str(), port.length());

  _a.alc_a.tsi = tsi;
  strcpy(_a.alc_a.base_dir, target_directory.c_str());
  _a.alc_a.cc_id = DEF_CC;				/* congestion control */
  _a.alc_a.mode = RECEIVER;				/* sender or receiver? */			
  _a.alc_a.port = DEF_MCAST_PORT;				 /* local port number for base channel */
  _a.alc_a.nb_channel = DEF_NB_CHANNEL;			 /* number of channels */
  _a.alc_a.intface =  iface.c_str();
  _a.alc_a.intface_name = nullptr;				 /* interface name/index for IPv6 multicast join */
  _a.alc_a.addr_family = DEF_ADDR_FAMILY;
  _a.alc_a.addr_type = 0;
  _a.alc_a.es_len = DEF_SYMB_LENGTH;				 /* encoding symbol length */
  _a.alc_a.max_sb_len = DEF_MAX_SB_LEN;			 /* source block length */
  _a.alc_a.tx_rate = DEF_TX_RATE;				 /* transmission rate in kbit/s */
  _a.alc_a.ttl = DEF_TTL;					 /* time to live  */
  _a.alc_a.nb_tx = DEF_TX_NB;					 /* nb of time to send the file/directory */
  _a.alc_a.simul_losses = FALSE;				 /* simulate packet losses */
  _a.alc_a.loss_ratio1 = P_LOSS_WHEN_OK;
  _a.alc_a.loss_ratio2 = P_LOSS_WHEN_LOSS;
  _a.alc_a.fec_ratio = DEF_FEC_RATIO;				 /* FEC ratio percent */	
  _a.alc_a.fec_enc_id = DEF_FEC_ENC_ID;			 /* FEC Encoding */
  _a.alc_a.fec_inst_id = DEF_FEC_INST_ID; 
  _a.alc_a.rx_memory_mode = 0;
  _a.alc_a.verbosity = 0;  
  _a.alc_a.use_fec_oti_ext_hdr = 0;				 /* include fec oti extension header to flute header */
  _a.alc_a.encode_content = 0;				 /* encode content */
  _a.alc_a.src_addr = nullptr;
  _a.alc_a.optimize_tx_rate = FALSE;
  _a.alc_a.calculate_session_size = FALSE;
  _a.alc_a.half_word = FALSE;
  _a.alc_a.accept_expired_fdt_inst = TRUE;
  _a.alc_a.use_ssm = FALSE;					 /* use source specific multicast */

	_a.alc_a.addr_family = PF_INET;

  _alc_session_id = open_alc_session(&_a.alc_a);
	auto ret = fcl::add_alc_channel(_alc_session_id, ports[0], addrs[0], _a.alc_a.intface, _a.alc_a.intface_name);

}

OBECA::FluteReceiver::~FluteReceiver()
{
  spdlog::debug("Closing flute receiver for ALC session {}", _alc_session_id);
  fcl::set_flute_session_state(_alc_session_id, fcl::SExiting);
}

void OBECA::FluteReceiver::start() 
{
  _receiver.fdt = nullptr;
  _receiver.s_id = _alc_session_id;
  _receiver.rx_automatic = TRUE;
  _receiver.accept_expired_fdt_inst = TRUE;
  _receiver.verbosity = 0;

	pthread_t fdt_thread_id;
  if((pthread_create(&fdt_thread_id, nullptr, fcl::fdt_thread, (void*)&_receiver)) != 0) {
    printf("Error: flute_receiver, pthread_create\n");
    fflush(stdout);
    fcl::close_alc_session(_alc_session_id);
    return;        
  }

  spdlog::debug("Starting flute receiver for ALC session {}", _alc_session_id);
  _a.rx_automatic = TRUE;
  fcl::receiver_in_fdt_based_mode(&_a, &_receiver);
}

auto OBECA::FluteReceiver::fileList() -> std::vector<OBECA::File>
{
  std::vector<OBECA::File> files;
  if (!_receiver.fdt) return files;

  auto next = _receiver.fdt->file_list;
  while(next != nullptr) {
    auto file = next;
    next = file->next;
    if (file->status == 2) {
      files.emplace_back( file->filesystem_location, file->content_len, file->received_at);
      file->force_expire = true;
    }
  }
  return files;
}
