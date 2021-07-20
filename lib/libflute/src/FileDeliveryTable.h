#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "flute_types.h"

namespace LibFlute {
  class FileDeliveryTable {
    public:
      FileDeliveryTable(uint32_t instance_id, char* buffer, size_t len);
      virtual ~FileDeliveryTable() {};

      uint32_t instance_id() { return _instance_id; };

      struct FileEntry {
        uint32_t toi;
        std::string content_location;
        uint32_t content_length;
        std::string content_md5;
        std::string content_type;
        uint32_t expires;
        FecOti fec_oti;
      };

      std::vector<FileEntry> file_entries() { return _file_entries; };
    private:
      uint32_t _instance_id;

      std::string _expires;
      std::vector<FileEntry> _file_entries;
  };
};
