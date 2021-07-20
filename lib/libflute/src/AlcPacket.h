#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include "flute_types.h"

namespace LibFlute {
  class AlcPacket {
    public:
      AlcPacket(char* data, size_t len);
      virtual ~AlcPacket() {};

      uint64_t tsi() const { return _tsi; };
      uint64_t toi() const { return _toi; };

      FecScheme fec_scheme() const { return _fec_oti.encoding_id; };
      const FecOti& fec_oti() const { return _fec_oti; };

      size_t header_length() const  { return _lct_header.lct_header_len * 4; };

      uint32_t fdt_instance_id() const { return _fdt_instance_id; };

    private:
      uint64_t _tsi = 0;
      uint64_t _toi = 0;

      uint32_t _fdt_instance_id = 0;

      uint32_t _source_block_number = 0;
      uint32_t _encoding_symbol_id = 0;

      ContentEncoding _content_encoding = ContentEncoding::NONE;
      FecOti _fec_oti = {};

      // RFC5651 5.1 - LCT Header Format
      struct __attribute__((packed)) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint8_t res1:1;
        uint8_t source_packet_indicator:1;
        uint8_t congestion_control_flag:2;
        uint8_t version:4;

        uint8_t close_object_flag:1;
        uint8_t close_session_flag:1;
        uint8_t res:2;
        uint8_t half_word_flag:1;
        uint8_t toi_flag:2;
        uint8_t tsi_flag:1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint8_t version:4;
        uint8_t congestion_control_flag:2;
        uint8_t source_packet_indicator:1;
        uint8_t res1:1;

        uint8_t tsi_flag:1;
        uint8_t toi_flag:2;
        uint8_t half_word_flag:1;
        uint8_t res2:2;
        uint8_t close_session_flag:1;
        uint8_t close_object_flag:1;
#else
#error "Endianness can not be determined"
#endif
        uint8_t lct_header_len;
        uint8_t codepoint;
      } _lct_header;
      static_assert(sizeof(_lct_header) == 4);

      enum HeaderExtension { 
        EXT_NOP  =   0,
        EXT_AUTH =   1,
        EXT_TIME =   2,
        EXT_FTI  =  64,
        EXT_FDT  = 192,
        EXT_CENC = 193
      };

  };
};

