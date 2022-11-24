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

#pragma once

#include <libconfig.h++>
#include <boost/asio.hpp>
#include "seamless/Segment.h"
#include "ItemSource.h"

namespace MBMS_RT {
  class CacheItem {
    public:
      CacheItem(const std::string& content_location, unsigned long received_at)
        : _content_location( content_location )
        , _received_at( received_at ) {}

      virtual ~CacheItem() = default;
      
      enum class ItemType {
        File,
        Segment,
        Playlist,
        Manifest
      };
      virtual ItemType item_type() const = 0;
      virtual char* buffer() const = 0;
      virtual uint32_t content_length() const = 0;
      virtual ItemSource item_source() const  = 0;

      std::string item_source_as_string() const {
        switch (item_source()) {
          case ItemSource::Broadcast:
            return "5G-BC";
          case ItemSource::CDN:
            return "CDN";
          case ItemSource::Generated:
            return "GEN";
          case ItemSource::Unavailable:
          default:
            return "-";
        }
      }

      std::string content_location() const { return _content_location; };
      virtual unsigned long received_at() const { return _received_at; };

    private:
      std::string _content_location;
      unsigned long _received_at;
  };

  class CachedFile : public CacheItem {
    public:
      CachedFile(const std::string& content_location, unsigned long received_at,
          std::shared_ptr<LibFlute::File> file)
        : CacheItem( content_location, received_at ) 
        , _file( file ) 
        {}
      virtual ~CachedFile() = default;

      virtual ItemType item_type() const { return ItemType::File; };
      virtual char* buffer() const { return _file->buffer(); };
      virtual uint32_t content_length() const { return _file->length(); };
      virtual ItemSource item_source() const { return ItemSource::Broadcast; };

    private:
      std::shared_ptr<LibFlute::File> _file;
  };
  class CachedSegment : public CacheItem {
    public:
      CachedSegment(const std::string& content_location, unsigned long received_at,
          std::shared_ptr<MBMS_RT::Segment> segment)
        : CacheItem( content_location, received_at ) 
        , _segment( segment ) 
        {}
      virtual ~CachedSegment() = default;

      virtual ItemType item_type() const { return ItemType::Segment; };
      virtual char* buffer() const { return _segment->buffer(); };
      virtual uint32_t content_length() const { return _segment->content_length(); };
      virtual ItemSource item_source() const { return _segment->data_source(); };

      virtual unsigned long received_at() const { return _segment->received_at(); };

    private:
      std::shared_ptr<Segment> _segment;
  };

  class CachedPlaylist : public CacheItem {
    public:
      CachedPlaylist(const std::string& content_location, unsigned long received_at,
          std::function<const std::string&(void)> playlist_cb)
        : CacheItem( content_location, received_at ) 
        , _playlist_cb( playlist_cb ) 
        {}
      virtual ~CachedPlaylist() = default;

      virtual ItemType item_type() const { return ItemType::Playlist; };
      virtual char* buffer() const { return (char*)_playlist_cb().c_str(); };
      virtual uint32_t content_length() const { return _playlist_cb().size(); };
      virtual ItemSource item_source() const { return ItemSource::Generated; };
      virtual unsigned long received_at() const { return time(nullptr); };

    private:
      std::function<const std::string&(void)> _playlist_cb;
  };

  class CachedManifest : public CacheItem {
    public:
      CachedManifest(const std::string& content_location, unsigned long received_at,
          std::function<const std::string&(void)> playlist_cb)
        : CacheItem( content_location, received_at ) 
        , _playlist_cb( playlist_cb ) 
        {}
      virtual ~CachedManifest() = default;

      virtual ItemType item_type() const { return ItemType::Manifest; };
      virtual char* buffer() const { return (char*)_playlist_cb().c_str(); };
      virtual uint32_t content_length() const { return _playlist_cb().size(); };
      virtual ItemSource item_source() const { return ItemSource::Generated; };
      virtual unsigned long received_at() const { return time(nullptr); };

    private:
      std::function<const std::string&(void)> _playlist_cb;
  };
}
