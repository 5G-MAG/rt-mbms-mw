#pragma once
#include <stddef.h>
#include <stdint.h>
#include <map>
#include "AlcPacket.h"
#include "FileDeliveryTable.h"
#include "EncodingSymbol.h"

namespace LibFlute {
  class File {
    public:
      File(const LibFlute::FileDeliveryTable::FileEntry& entry);
      virtual ~File();

      void put_symbol(const EncodingSymbol& symbol);

      bool complete() const { return _complete; };

      char* buffer() const { return _buffer; };
      size_t length() const { return _fec_oti.transfer_length; };

      const FecOti& fec_oti() const { return _fec_oti; };
      const LibFlute::FileDeliveryTable::FileEntry& meta() const { return _meta; };
      unsigned long received_at() const { return _received_at; };

      void log_access() { _access_count++; };
      unsigned access_count() const { return _access_count; };

    private:
      struct SourceBlock {
        bool complete = false;
        struct Symbol {
          char* data;
          size_t length;
          bool complete = false;
        };
        std::map<uint32_t, Symbol> symbols; 
      };

      void check_source_block_completion(SourceBlock& block);
      void check_file_completion();

      std::map<uint32_t, SourceBlock> _source_blocks; 

      bool _complete = false;;

      uint32_t _nof_source_symbols = 0;
      uint32_t _nof_source_blocks = 0;
      uint32_t _nof_large_source_blocks = 0;
      uint32_t _large_source_block_length = 0;
      uint32_t _small_source_block_length = 0;

      FecOti _fec_oti;

      char* _buffer;

      LibFlute::FileDeliveryTable::FileEntry _meta;
      unsigned long _received_at;
      unsigned _access_count = 0;
  };
};
