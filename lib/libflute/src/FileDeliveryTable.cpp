#include "FileDeliveryTable.h"
#include "tinyxml2.h" 
#include <iostream>
#include <string>
#include "spdlog/spdlog.h"

LibFlute::FileDeliveryTable::FileDeliveryTable(uint32_t instance_id, char* buffer, size_t len) 
  : _instance_id( instance_id )
{
  tinyxml2::XMLDocument doc(true, tinyxml2::COLLAPSE_WHITESPACE);
  doc.Parse(buffer, len);
  auto fdt_instance = doc.FirstChildElement("FDT-Instance");
  _expires = fdt_instance->Attribute("Expires");

  spdlog::debug("Received new FDT with instance ID {}", instance_id);

  uint8_t def_fec_encoding_id = 0;
  auto val = fdt_instance->Attribute("FEC-OTI-FEC-Encoding-ID");
  if (val != nullptr) {
    def_fec_encoding_id = strtoul(val, nullptr, 0);
  }

  uint32_t def_fec_max_source_block_length = 0;
  val = fdt_instance->Attribute("FEC-OTI-Maximum-Source-Block-Length");
  if (val != nullptr) {
    def_fec_max_source_block_length = strtoul(val, nullptr, 0);
  }

  uint32_t def_fec_encoding_symbol_length = 0;
  val = fdt_instance->Attribute("FEC-OTI-Encoding-Symbol-Length");
  if (val != nullptr) {
    def_fec_encoding_symbol_length = strtoul(val, nullptr, 0);
  }

  for (auto file = fdt_instance->FirstChildElement("File"); 
      file != nullptr; file = file->NextSiblingElement("File")) {

    // required attributes
    auto toi_str = file->Attribute("TOI");
    if (toi_str == nullptr) {
      throw "Missing TOI attribute on File element";
    }
    uint32_t toi = strtoull(toi_str, nullptr, 0);

    auto content_location = file->Attribute("Content-Location");
    if (content_location == nullptr) {
      throw "Missing Content-Location attribute on File element";
    }

    uint32_t content_length = 0;
    val = file->Attribute("Content-Length");
    if (val != nullptr) {
      content_length = strtoull(val, nullptr, 0);
    }

    uint32_t transfer_length = 0;
    val = file->Attribute("Transfer-Length");
    if (val != nullptr) {
      transfer_length = strtoull(val, nullptr, 0);
    }

    auto content_md5 = file->Attribute("Content-MD5");
    if (!content_md5) {
      content_md5 = "";
    }

    auto content_type = file->Attribute("Content-Type");
    if (!content_type) {
      content_type = "";
    }

    auto encoding_id = def_fec_encoding_id;
    val = file->Attribute("FEC-OTI-FEC-Encoding-ID");
    if (val != nullptr) {
      encoding_id = strtoul(val, nullptr, 0);
    }

    auto max_source_block_length = def_fec_max_source_block_length;
    val = file->Attribute("FEC-OTI-Maximum-Source-Block-Length");
    if (val != nullptr) {
      max_source_block_length = strtoul(val, nullptr, 0);
    }

    auto encoding_symbol_length = def_fec_encoding_symbol_length;
    val = file->Attribute("FEC-OTI-Encoding-Symbol-Length");
    if (val != nullptr) {
      encoding_symbol_length = strtoul(val, nullptr, 0);
    }
    uint32_t expires = 0;
    auto cc = file->FirstChildElement("mbms2007:Cache-Control");
    if (cc) {
      auto expires_elem = cc->FirstChildElement("mbms2007:Expires");
      if (expires_elem) {
        expires = strtoul(expires_elem->GetText(), nullptr, 0);
      }
    }

    FecOti fec_oti{
      (FecScheme)encoding_id,
        transfer_length,
        encoding_symbol_length,
        max_source_block_length
    };

    FileEntry fe{
      toi,
        std::string(content_location),
        content_length,
        std::string(content_md5),
        std::string(content_type),
        expires,
        fec_oti
    };
    _file_entries.push_back(fe);
  }
}
