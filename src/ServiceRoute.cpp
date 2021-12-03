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

#include "ServiceRoute.h"
#include "File.h"
#include "Receiver.h"
#include "spdlog/spdlog.h"
#include <unordered_map>

extern "C" {
#include <gpac/route.h>
#include <gpac/cache.h>
}

struct FileRoute : MBMS_RT::IFile, MBMS_RT::IFile::IEntry {
  FileRoute(const char *filename, const char *buffer, const uint32_t size, const unsigned long received_at)
  : _filename(filename), _size(size), _buffer(buffer), _received_at(received_at)
  {
  }
  FileRoute(const FileRoute& f)
: _filename(f._filename),
    _buffer(f._buffer),
    _size(f._size),
    _received_at(f._received_at)
  {
  }

  // IFile
  [[nodiscard]]
  auto access_count() const -> unsigned override {
    return _nb_access;
  }
  [[nodiscard]]
  auto meta() const -> const MBMS_RT::IFile::IEntry& override {
    return *this;
  }
  [[nodiscard]]
  auto received_at() const -> unsigned long override {
    return _received_at;
  }
  void log_access() override {
    ++_nb_access;
  }
  [[nodiscard]]
  auto buffer() const -> const char * const override {
    return _buffer;
  }

  // IEntry
  [[nodiscard]]
  auto content_location() const -> std::string override {
    return _filename;
  }
  [[nodiscard]]
  auto content_length() const -> uint32_t override {
    return _size;
  }

private:
  // IEntry
  std::string _filename;
  uint32_t _size;

  // IFile
  const char * _buffer;
  unsigned long _received_at;
  unsigned _nb_access = 0;
};

struct RouteSession {
  ~RouteSession() {
    if (executor.joinable())
      executor.join();

    gf_route_dmx_del(processor);
  }

  struct TSI_Output {
    u32 sid;
    u32 tsi;
  };

  GF_ROUTEDmx *processor = nullptr;
  std::thread executor;

  std::mutex fileEntriesMutex;
  std::unordered_map<std::string, FileRoute/*MBMS_RT::IFile::IEntry*/> fileEntries;
};

namespace {

void sendFile(RouteSession *session, u32 service_id, GF_ROUTEEventFileInfo *finfo, u32 evt_type) {
  if (finfo->tsi) {
    if ((evt_type == GF_ROUTE_EVT_FILE) || (evt_type == GF_ROUTE_EVT_MPD)) {
      if (!finfo->updated)
        // repeated file: don't resend
        return;
    }
  }

  spdlog::info("Service {} got file {} (TSI {} TOI {}) size {} in {} ms ({} fragments)",
               service_id, finfo->filename, finfo->tsi, finfo->toi, finfo->total_size, finfo->download_ms, finfo->nb_frags);

  // write to disk
  auto containsSubfolder = strrchr(finfo->filename, GF_PATH_SEPARATOR);
  if (containsSubfolder) {
    auto folder = std::string(finfo->filename).substr(0, containsSubfolder - finfo->filename);
    if (!gf_dir_exists(folder.c_str())) {
      GF_Err e = gf_mkdir(folder.c_str());
      if (e != GF_OK)
        spdlog::error("Could not create subfolder \"{}\".", folder);
    }
  }
  auto fd = fopen(finfo->filename, "wb");
  if (fd) {
    fwrite(finfo->blob->data, 1, finfo->blob->size, fd);
    fclose(fd);
  } else {
    spdlog::error("Could not write to file \"{}\".", finfo->filename);
  }

  // store or update entry
  {
    std::unique_lock<std::mutex> lock(session->fileEntriesMutex);
    session->fileEntries.insert_or_assign(finfo->filename, FileRoute(finfo->filename, (const char*)finfo->blob->data, finfo->blob->size, time(nullptr)));
  }

  while (gf_route_dmx_get_object_count(session->processor, service_id) > 1) {
    if (!gf_route_dmx_remove_first_object(session->processor, service_id))
      break;
  }
}

void repairSegment(GF_ROUTEEventFileInfo *finfo)
{
  //if (finfo->blob->mx)
  //  gf_mx_p(finfo->blob->mx);

  spdlog::warn("Not implemented: \"{}\" needs repair", finfo->filename);

  //if (finfo->blob->mx)
  //  gf_mx_v(finfo->blob->mx);
}

void onEvent(void *udta, GF_ROUTEEventType evt, u32 evt_param, GF_ROUTEEventFileInfo *finfo)
{
  auto session = (RouteSession*)udta;

  //events without finfo
  if (evt == GF_ROUTE_EVT_SERVICE_FOUND) {
    return;
  }
  if (evt == GF_ROUTE_EVT_SERVICE_SCAN) {
    return;
  }
  if (!finfo)
    return;

  //events without finfo->blob
  if (evt == GF_ROUTE_EVT_FILE_DELETE) {
    return;
  }
  if (!finfo->blob)
    return;

  switch (evt) {
  case GF_ROUTE_EVT_MPD:
    sendFile(session, evt_param, finfo, evt);
    break;
  case GF_ROUTE_EVT_DYN_SEG:
    //corrupted file, try to repair
    if (finfo->blob->flags & GF_BLOB_CORRUPTED) {
      repairSegment(finfo);
    }
    sendFile(session, evt_param, finfo, evt);
    break;
  case GF_ROUTE_EVT_DYN_SEG_FRAG:
    //discard, we only push complete files
    break;
  case GF_ROUTE_EVT_FILE:
    sendFile(session, evt_param, finfo, evt);
    break;
  default:
    break;
  }
}

} /*anonymous namespace*/

ReceiverRoute::ReceiverRoute(const std::string& iface, const std::string& address,
    short port, uint64_t tsi, boost::asio::io_service& io_service)
  : session(std::make_shared<RouteSession>())
{
  session->processor = gf_route_dmx_new(address.c_str(), port, iface.c_str(),
                                        0x80000/*sock_buffer_size*/, onEvent, (void*)this->session.get());
  if (!session->processor) {
    spdlog::error("Could not create ROUTE demuxer:");
    spdlog::error("  Interface: {}",    iface);
    spdlog::error("  Address  : {}:{}", address, port);
    spdlog::error("  TSI      : {}",    tsi);
    throw std::runtime_error("Could not create ROUTE demuxer. See log messages for more information.");
  }

  gf_route_set_allow_progressive_dispatch(session->processor, GF_FALSE /*TODO: set to true and append file writing to activate low latency*/);
  gf_route_set_reorder(session->processor, GF_FALSE, 5000);

  session->executor = std::thread([&]() {
    while (!exiting) {
      auto e = gf_route_dmx_process(session->processor);
      if (e == GF_IP_NETWORK_EMPTY) {
        spdlog::debug("ROUTE demuxer: no network reception.");
      } else if (e != GF_OK) {
        spdlog::warn("ROUTE demuxer: error \"{}\".", gf_error_to_string(e));
      }
    }
  });
}

ReceiverRoute::~ReceiverRoute() {
  exiting = true;
}

auto ReceiverRoute::file_list() -> std::vector<std::shared_ptr<MBMS_RT::IFile>> {
  std::vector<std::shared_ptr<MBMS_RT::IFile>> fileList;
  {
    std::unique_lock<std::mutex> lock(session->fileEntriesMutex);
    for (auto &f : session->fileEntries) {
      fileList.push_back(std::make_shared<FileRoute>(f.second));
    }
  }
  return fileList;
}

void ReceiverRoute::remove_expired_files(unsigned max_age) {
  auto const now = time(nullptr);

  std::unique_lock<std::mutex> lock(session->fileEntriesMutex);
  for (auto it=session->fileEntries.cbegin(); it!=session->fileEntries.cend(); ) {
    if (strstr(it->first.c_str(), "_init.mp4") || strstr(it->first.c_str(), ".mpd"))
      continue; // init segments and manifest never expire (but can be updated)
    
    if (it->second.received_at() + max_age < now) {
      session->fileEntries.erase(it->first);
    } else {
      ++it;
    }
  }
}
