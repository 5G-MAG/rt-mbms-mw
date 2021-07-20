#include <cstdio>
#include <cstring>
#include <iostream>
#include <cmath>
#include <arpa/inet.h>
#include "EncodingSymbol.h"

auto LibFlute::EncodingSymbol::initialize_from_payload(char* encoded_data, size_t data_len, const FecOti& fec_oti) -> std::vector<EncodingSymbol> 
{
  auto source_block_number = 0;
  auto encoding_symbol_id = 0;
  std::vector<EncodingSymbol> symbols;
  if (fec_oti.encoding_id == FecScheme::CompactNoCode) {
    source_block_number = ntohs(*(uint16_t*)encoded_data);
    encoded_data += 2;
    encoding_symbol_id = ntohs(*(uint16_t*)encoded_data);
    encoded_data += 2;
    data_len -= 4;
  }

  int nof_symbols = std::ceil((float)data_len / (float)fec_oti.encoding_symbol_length);
  for (int i = 0; i < nof_symbols; i++) {
    if (fec_oti.encoding_id == FecScheme::CompactNoCode) {
      symbols.emplace_back(encoding_symbol_id, source_block_number, encoded_data, std::min(data_len, (size_t)fec_oti.encoding_symbol_length), fec_oti.encoding_id);
    }
    encoded_data += fec_oti.encoding_symbol_length;
    encoding_symbol_id++;
  }

  return symbols;
}

auto LibFlute::EncodingSymbol::decode_to(char* buffer, size_t max_length) const -> void {
  if (_fec_scheme == FecScheme::CompactNoCode) {
    if (_data_len <= max_length) {
      memcpy(buffer, _encoded_data, _data_len);
    }
  }
}
