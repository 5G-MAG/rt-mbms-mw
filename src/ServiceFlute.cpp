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
#include "ServiceFlute.h"
#include "File.h"
#include "Receiver.h"

struct FileEntryFlute : MBMS_RT::IFile::IEntry {
  FileEntryFlute(const LibFlute::FileDeliveryTable::FileEntry &entry)
    : delegate(entry)
  {
  }
  [[nodiscard]] auto content_location() const -> std::string override {
    return delegate.content_location;
  }
  [[nodiscard]] auto content_length() const -> uint32_t override {
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
  auto access_count() const -> unsigned override {
    return delegate->access_count();
  }
  auto meta() const -> const MBMS_RT::IFile::IEntry& override {
    fileEntry = std::make_unique<const FileEntryFlute>(delegate->meta());
    return *fileEntry;
  }
  auto received_at() const -> unsigned long override {
    return delegate->received_at();
  }
  void log_access() const override {
    return delegate->log_access();
  }
  auto buffer() const -> char* override {
    return delegate->buffer();
  }

private:
  std::shared_ptr<LibFlute::File> delegate;
  mutable std::unique_ptr<const FileEntryFlute> fileEntry;
};

ReceiverFlute::ReceiverFlute(const std::string& iface, const std::string& address,
    short port, uint64_t tsi,
    boost::asio::io_service& io_service)
  : delegate(std::make_shared<LibFlute::Receiver>(iface, address, port, tsi, io_service))
{
}

auto ReceiverFlute::file_list() -> std::vector<std::shared_ptr<MBMS_RT::IFile>> {
  std::vector<std::shared_ptr<MBMS_RT::IFile>> fileList;
  for (auto &file : delegate->file_list()) {
    fileList.push_back(std::make_shared<FileFlute>(file));
  }
  return fileList;
}

void ReceiverFlute::remove_expired_files(unsigned max_age) {
  delegate->remove_expired_files(max_age);
}
