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

#include <regex>
#include "Service.h"
#include "File.h"
#include "Receiver.h"

#include "spdlog/spdlog.h"
#include "gmime/gmime.h" 
#include "tinyxml2.h"

struct FileEntryFlute : MBMS_RT::IFile::IEntry {
  FileEntryFlute(const LibFlute::FileDeliveryTable::FileEntry &entry)
    : delegate(entry)
  {
  }
  std::string content_location() const override {
    return delegate.content_location;
  }
  uint32_t content_length() const override {
    return delegate.content_length;
  }

private:
  const LibFlute::FileDeliveryTable::FileEntry &delegate;
};

struct FileFlute : MBMS_RT::IFile {
  FileFlute(std::shared_ptr<LibFlute::File> &file)
    : delegate(file)
  {
  }
  unsigned access_count() const override {
    return delegate->access_count();
  }
  const MBMS_RT::IFile::IEntry& meta() const override {
    fileEntry = std::make_unique<const FileEntryFlute>(delegate->meta());
    return *fileEntry;
  }
  unsigned long received_at() const override {
    return delegate->received_at();
  }
  void log_access() const override {
    return delegate->log_access();
  }
  char *buffer() const override {
    return delegate->buffer();
  }

private:
  std::shared_ptr<LibFlute::File> delegate;
  mutable std::unique_ptr<const FileEntryFlute> fileEntry;
};

MBMS_RT::Service::Service(const libconfig::Config& cfg, std::string tmgi, const std::string& mcast, unsigned long long tsi, std::string iface, boost::asio::io_service& io_service)
  : _cfg(cfg)
  , _tmgi(std::move(tmgi))
  , _tsi(tsi)
  , _iface(std::move(iface))
  , _flute_thread{}
{
  size_t delim = mcast.find(':');
  if (delim == std::string::npos) {
    spdlog::error("Invalid multicast address {}", mcast);
    return;
  }
  _mcast_addr = mcast.substr(0, delim);
  _mcast_port = mcast.substr(delim + 1);
  spdlog::info("Starting FLUTE receiver on {}:{} for TSI {}", _mcast_addr, _mcast_port, _tsi); 
  _flute_thread = std::thread{[&](){
    _flute_receiver = std::make_unique<LibFlute::Receiver>(_iface, _mcast_addr, atoi(_mcast_port.c_str()), _tsi, io_service) ;
  }};
}

MBMS_RT::Service::~Service() {
  spdlog::info("Closing service with TMGI {}", _tmgi);
  _flute_receiver.reset();
  if (_flute_thread.joinable()) {
    _flute_thread.join();
  }
}

auto MBMS_RT::Service::fileList() -> std::vector<std::shared_ptr<IFile>>
{
  std::vector<std::shared_ptr<IFile>> fileList;
  if (_flute_receiver) {
    for (auto &file : _flute_receiver->file_list()) {
      fileList.push_back(std::make_shared<FileFlute>(file));
    }
  }
  return fileList;
}

auto MBMS_RT::Service::tryParseBootstrapFile(std::string str) -> void
{
  g_mime_init();

  // R&S header has no newlines, fix that
  str = std::regex_replace(str, std::regex(" Content-Type:"), "\nContent-Type:");
  str = std::regex_replace(str, std::regex(" Content-Description:"), "\nContent-Description:");

  auto stream = g_mime_stream_mem_new_with_buffer(str.c_str(), str.length());
  auto parser = g_mime_parser_new_with_stream(stream);
  g_object_unref(stream);

  auto mpart = g_mime_parser_construct_part(parser, nullptr);
  g_object_unref(parser);

  auto iter = g_mime_part_iter_new(mpart);
  auto ct = g_mime_object_get_content_type_parameter(mpart, "type");
  while (g_mime_part_iter_next(iter)) {
    GMimeObject *current = g_mime_part_iter_get_current (iter);
    GMimeObject *parent = g_mime_part_iter_get_parent (iter);

    if (GMIME_IS_MULTIPART (parent) && GMIME_IS_PART (current)) {
      auto type = std::string(g_mime_content_type_get_mime_type(g_mime_object_get_content_type(current)));
      auto options = g_mime_format_options_new();
      g_mime_format_options_add_hidden_header(options, "Content-Type");
      g_mime_format_options_add_hidden_header(options, "Content-Transfer-Encoding");
      g_mime_format_options_add_hidden_header(options, "Content-Location");

      if (type == "application/mbms-user-service-description+xml") {
        _service_description = g_mime_object_to_string(current, options);
      } else if (type == "application/sdp" && _sdp.empty()) {
        _sdp = g_mime_object_to_string(current, options);
        _sdp = std::regex_replace(_sdp, std::regex("^\n"), "");
      } else if (type == "application/vnd.apple.mpegurl") {
        _m3u = g_mime_object_to_string(current, options);
      }
    }
  }

  if (!_service_description.empty()) {
    tinyxml2::XMLDocument doc;
    doc.Parse(_service_description.c_str());
    auto bundle = doc.FirstChildElement("bundleDescription");
    if (bundle) {
      auto usd = bundle->FirstChildElement("userServiceDescription");
      if (usd) {
        _service_name = usd->FirstChildElement("name")->GetText();
        spdlog::debug("Service Name: {}", _service_name);
      }
    }
  }

  if (!_sdp.empty()) {
    std::istringstream iss(_sdp);
    for (std::string line; std::getline(iss, line); )
    {
      const std::regex sdp_line_regex("^([a-z])\\=(.+)$");
      std::smatch match;
      if (std::regex_match(line, match, sdp_line_regex)) {
        if (match.size() == 3) {
          auto field = match[1].str();
          auto value = match[2].str();
          spdlog::debug("{}: {}", field, value);

          if (field == "c") {
            const std::regex value_regex("^IN (IP.) ([0-9\\.]+).*$");
            std::smatch cmatch;
            if (std::regex_match(value, cmatch, value_regex)) {
              if (cmatch.size() == 3) {
                _stream_mcast_addr = cmatch[2].str();
              }
            }
          } else if (field == "m") {
            const std::regex value_regex("^application (.+) (.+)$");
            std::smatch cmatch;
            if (std::regex_match(value, cmatch, value_regex)) {
              if (cmatch.size() == 3) {
                _stream_mcast_port = cmatch[1].str();
                _stream_type = cmatch[2].str();
              }
            }
            const std::regex value_regex2("^application (.+) (.+) (.+)$");
            if (std::regex_match(value, cmatch, value_regex2)) {
              if (cmatch.size() == 4) {
                _stream_mcast_port = cmatch[1].str();
                _stream_type = cmatch[2].str();
              }
            }
          } else if (field == "a") {
            const std::regex value_regex("^flute-tsi:(.+)$");
            std::smatch cmatch;
            if (std::regex_match(value, cmatch, value_regex)) {
              if (cmatch.size() == 2) {
                _stream_flute_tsi = stoul(cmatch[1].str());
              }
            }
          }
        }
      }
    }

    if (_stream_type != "none") {
      _bootstrap_file_parsed = true;
    }

  }
}
auto MBMS_RT::Service::remove_expired_files(unsigned max_age) -> void
{
  if (_flute_receiver) {
    _flute_receiver->remove_expired_files(max_age);
  }
}
