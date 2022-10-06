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
#include <iostream>     // std::cin, std::cout
#include <iomanip>      // std::get_time
#include <ctime>        // struct std::tm
#include <boost/algorithm/string/trim.hpp>
#include "ServiceAnnouncement.h"
#include "Service.h"
#include "Receiver.h"
#include "seamless/SeamlessContentStream.h"
#include "Constants.h"

#include "spdlog/spdlog.h"
#include "gmime/gmime.h"
#include "tinyxml2.h"
#include "cpprest/base_uri.h"


MBMS_RT::ServiceAnnouncement::ServiceAnnouncement(const libconfig::Config &cfg, std::string tmgi,
                                                  const std::string &mcast,
                                                  unsigned long long tsi, std::string iface,
                                                  boost::asio::io_service &io_service, CacheManagement &cache,
                                                  bool seamless_switching,
                                                  get_service_callback_t get_service,
                                                  set_service_callback_t set_service)
    : _cfg(cfg), _tmgi(std::move(tmgi)), _tsi(tsi), _iface(std::move(iface)), _io_service(io_service), _cache(cache),
      _flute_thread{}, _seamless(seamless_switching), _get_service(std::move(get_service)),
      _set_service(std::move(set_service)) {
}

MBMS_RT::ServiceAnnouncement::~ServiceAnnouncement() {
  spdlog::info("Closing service announcement session with TMGI {}", _tmgi);
  _flute_receiver.reset();
  if (_flute_thread.joinable()) {
    _flute_thread.join();
  }
}

/**
 * Starts the FLUTE receiver at the specified multicast address. Includes a callback function that is called once a file has been received via multicast
 * Used for receiving a service announcement file via multicast
 * @param mcast_address
 */
auto MBMS_RT::ServiceAnnouncement::start_flute_receiver(const std::string &mcast_address) -> void {
  size_t delim = mcast_address.find(':');
  if (delim == std::string::npos) {
    spdlog::error("Invalid multicast address {}", mcast_address);
    return;
  }
  _mcast_addr = mcast_address.substr(0, delim);
  _mcast_port = mcast_address.substr(delim + 1);
  spdlog::info("Starting FLUTE receiver on {}:{} for TSI {}", _mcast_addr, _mcast_port, _tsi);
  _flute_thread = std::thread{[&]() {
    _flute_receiver = std::make_unique<LibFlute::Receiver>(_iface, _mcast_addr, atoi(_mcast_port.c_str()), _tsi,
                                                           _io_service);
    _flute_receiver->register_completion_callback(
        [&](std::shared_ptr<LibFlute::File> file) { //NOLINT
          spdlog::info("{} (TOI {}) has been received",
                       file->meta().content_location, file->meta().toi);
          if (file->meta().content_location.find("bootstrap.multipart") != std::string::npos &&
              (!_bootstrapped || _toi != file->meta().toi)) {
            _toi = file->meta().toi;
            _raw_content = std::string(file->buffer());
            parse_bootstrap(file->buffer());
          }
        });
  }};
}

/**
 * Parse the service announcement/bootstrap file
 * @param str
 */
auto
MBMS_RT::ServiceAnnouncement::parse_bootstrap(const std::string &str) -> void {
  std::string bootstrap_format = ServiceAnnouncementFormatConstants::DEFAULT;
  _cfg.lookupValue("mw.bootstrap_format", bootstrap_format);

  // Add all the SA items including their content to _items
  _addServiceAnnouncementItems(str);

  // Parse MBMS envelope: <metadataEnvelope>
  for (const auto &item: _items) {
    if (item.content_type == ContentTypeConstants::MBMS_ENVELOPE) {
      _handleMbmsEnvelope(item);
    }
  }

  // Parse MBMS user service description bundle
  for (const auto &item: _items) {
    if (item.content_type == ContentTypeConstants::MBMS_USER_SERVICE_DESCRIPTION) {
      _handleMbmbsUserServiceDescriptionBundle(item, bootstrap_format);
    }
  }
}

/**
 * Iterates through the service announcement file and adds the different sections/items to the the list of _items
 * @param {std::string} str
 */
void MBMS_RT::ServiceAnnouncement::_addServiceAnnouncementItems(const std::string &str) {
  g_mime_init();
  auto stream = g_mime_stream_mem_new_with_buffer(str.c_str(), str.length());
  auto parser = g_mime_parser_new_with_stream(stream);
  g_object_unref(stream);

  auto mpart = g_mime_parser_construct_part(parser, nullptr);
  g_object_unref(parser);

  auto iter = g_mime_part_iter_new(mpart);
  do {
    GMimeObject *current = g_mime_part_iter_get_current(iter);
    GMimeObject *parent = g_mime_part_iter_get_parent(iter);

    if (GMIME_IS_PART (current)) {
      auto type = std::string(g_mime_content_type_get_mime_type(g_mime_object_get_content_type(current)));
      std::string location = "";
      if (g_mime_object_get_header(current, "Content-Location")) {
        location = std::string(g_mime_object_get_header(current, "Content-Location"));
      }
      auto options = g_mime_format_options_new();
      g_mime_format_options_add_hidden_header(options, "Content-Type");
      g_mime_format_options_add_hidden_header(options, "Content-Transfer-Encoding");
      g_mime_format_options_add_hidden_header(options, "Content-Location");
      std::string content = g_mime_object_to_string(current, options);
      boost::algorithm::trim_left(content);

      if (location != "") {
        _items.emplace_back(Item{
            type,
            location,
            0, 0, 0,
            content
        });
      }
    }
  } while (g_mime_part_iter_next(iter));

}

/**
 * Parses the MBMS envelope
 * @param {MBMS_RT::ServiceAnnouncement::Item} item
 */
void MBMS_RT::ServiceAnnouncement::_handleMbmsEnvelope(const MBMS_RT::ServiceAnnouncement::Item &item) {
  try {
    tinyxml2::XMLDocument doc;
    doc.Parse(item.content.c_str());
    auto envelope = doc.FirstChildElement(ServiceAnnouncementXmlElements::METADATA_ENVELOPE);
    for (auto *i = envelope->FirstChildElement(ServiceAnnouncementXmlElements::ITEM);
         i != nullptr; i = i->NextSiblingElement(ServiceAnnouncementXmlElements::ITEM)) {
      spdlog::debug("uri: {}", i->Attribute(ServiceAnnouncementXmlElements::METADATA_URI));
      for (auto &ir: _items) {
        if (ir.uri == i->Attribute(ServiceAnnouncementXmlElements::METADATA_URI)) {
          std::stringstream ss_from(i->Attribute(ServiceAnnouncementXmlElements::VALID_FROM));
          struct std::tm from;
          ss_from >> std::get_time(&from, "%Y-%m-%dT%H:%M:%S.%fZ");
          ir.valid_from = mktime(&from);
          std::stringstream ss_until(i->Attribute(ServiceAnnouncementXmlElements::VALID_UNTIL));
          struct std::tm until;
          ss_until >> std::get_time(&until, "%Y-%m-%dT%H:%M:%S.%fZ");
          ir.valid_until = mktime(&until);
          ir.version = atoi(i->Attribute(ServiceAnnouncementXmlElements::VERSION));
        }
      }
    }
  } catch (std::exception e) {
    spdlog::warn("MBMS envelope parsing failed: {}", e.what());
  }
}

/**
 * Parses the MBMS USD
 * @param {MBMS_RT::ServiceAnnouncement::Item} item
 */
void
MBMS_RT::ServiceAnnouncement::_handleMbmbsUserServiceDescriptionBundle(const MBMS_RT::ServiceAnnouncement::Item &item,
                                                                       const std::string &bootstrap_format) {
  try {
    tinyxml2::XMLDocument doc;
    doc.Parse(item.content.c_str());

    auto bundle = doc.FirstChildElement(ServiceAnnouncementXmlElements::BUNDLE_DESCRIPTION);
    for (auto *usd = bundle->FirstChildElement(ServiceAnnouncementXmlElements::USER_SERVICE_DESCRIPTION);
         usd != nullptr;
         usd = usd->NextSiblingElement(ServiceAnnouncementXmlElements::USER_SERVICE_DESCRIPTION)) {

      // Create a new service
      auto service_id = usd->Attribute(ServiceAnnouncementXmlElements::SERVICE_ID);
      auto[service, is_new_service] = _registerService(usd, service_id);

      // Handle the app service element. Will read the master manifest as provided in the SA
      auto app_service = usd->FirstChildElement(ServiceAnnouncementXmlElements::APP_SERVICE);
      _handleAppService(app_service, service);

      // For the default format we need an alternativeContent attribute to setup the service
      if (bootstrap_format == ServiceAnnouncementFormatConstants::FIVEG_MAG_BC_UC) {
        _setupBy5GMagConfig(app_service, service, usd);
      } else if (bootstrap_format == ServiceAnnouncementFormatConstants::FIVEG_MAG_LEGACY) {
        _setupBy5GMagLegacyFormat(app_service, service, usd);
      } else {
        _setupByAlternativeContentElement(app_service, service, usd);
      }

      if (is_new_service && service->content_streams().size() > 0) {
        _set_service(service_id, service);
      }
    }
  } catch (std::exception e) {
    spdlog::warn("MBMS user service desription parsing failed: {}", e.what());
  }
}

/**
 * Creates a new service or finds an existing service for the specified service id
 * @param usd
 * @param service_id
 * @return
 */
std::tuple<std::shared_ptr<MBMS_RT::Service>, bool>
MBMS_RT::ServiceAnnouncement::_registerService(tinyxml2::XMLElement *usd, const std::string &service_id) {
  // Register a new service if we have not seen this service id before

  bool is_new_service = false;
  auto service = _get_service(service_id);
  if (service == nullptr) {
    service = std::make_shared<Service>(_cache);
    is_new_service = true;
  }

  // read the names
  for (auto *name = usd->FirstChildElement(ServiceAnnouncementXmlElements::NAME);
       name != nullptr; name = name->NextSiblingElement(ServiceAnnouncementXmlElements::NAME)) {
    auto lang = name->Attribute(ServiceAnnouncementXmlElements::LANG);
    auto namestr = name->GetText();
    if (lang && namestr) {
      service->add_name(namestr, lang);
    }
  }

  return {service, is_new_service};
}

/**
 * Parse the appService element
 * Spec: Presence of the r12:appService child element of userServiceDescription indicates that the associated MBMS User Service is an application service explicitly linked to the
 * r12:broadcastAppService and r12:unicastAppService elements under deliveryMethod
 * @param usd
 * @param app_service
 * @param service
 */
void MBMS_RT::ServiceAnnouncement::_handleAppService(tinyxml2::XMLElement *app_service,
                                                     std::shared_ptr<MBMS_RT::Service> service) {

  service->set_delivery_protocol_from_mime_type(app_service->Attribute(ServiceAnnouncementXmlElements::MIME_TYPE));

  // Now search for the content that corresponds to appServiceDescriptionURI. For instance appServiceDescriptionURI="http://localhost/watchfolder/manifest.m3u8"
  // The attribute appServiceDescriptionURI of r12:appService references an Application Service Description which may be a Media Presentation Description fragment corresponding to a unified MPD.
  for (const auto &item: _items) {
    // item.uri is derived from the Content-Location of each entry in the bootstrap file. For HLS we are looking for the content of the master manifest in the bootstrap file:
    if (item.uri == app_service->Attribute(ServiceAnnouncementXmlElements::APP_SERVICE_DESCRIPTION_URI)) {
      web::uri uri(item.uri);

      // remove file, leave only dir
      const std::string &path = uri.path();
      size_t spos = path.rfind('/');
      auto base_path = path.substr(0, spos + 1);

      // make relative path: remove leading /
      if (base_path[0] == '/') {
        base_path.erase(0, 1);
      }
      service->read_master_manifest(item.content, base_path);
    }
  }
}

void MBMS_RT::ServiceAnnouncement::_setupBy5GMagConfig(tinyxml2::XMLElement *app_service,
                                                       const std::shared_ptr<MBMS_RT::Service> &service,
                                                       tinyxml2::XMLElement *usd) {

  std::vector<std::shared_ptr<ContentStream>> broadcastContentStreams;
  std::vector<std::shared_ptr<SeamlessContentStream>> unicastContentStreams;
// Create content stream objects for each broadcastAppService::basePattern element.
  for (auto *delivery_method = usd->FirstChildElement(ServiceAnnouncementXmlElements::DELIVERY_METHOD);
       delivery_method != nullptr;
       delivery_method = delivery_method->NextSiblingElement(ServiceAnnouncementXmlElements::DELIVERY_METHOD)) {
    auto sdp_uri = delivery_method->Attribute(ServiceAnnouncementXmlElements::SESSION_DESCRIPTION_URI);
    // We assume that the master manifest is signaled in the SA and that we can simply replace the .sdp ending with .m3u8 to find the element with the right Content-Location
    auto manifest_url = std::regex_replace(sdp_uri, std::regex(".sdp"), ".m3u8");
    auto broadcast_app_service = delivery_method->FirstChildElement(
        ServiceAnnouncementXmlElements::BROADCAST_APP_SERVICE);

    if (broadcast_app_service != nullptr) {
      for (auto *base_pattern = broadcast_app_service->FirstChildElement(ServiceAnnouncementXmlElements::BASE_PATTERN);
           base_pattern != nullptr;
           base_pattern = base_pattern->NextSiblingElement(ServiceAnnouncementXmlElements::BASE_PATTERN)) {

        std::string broadcast_url = base_pattern->GetText();

        // create a content stream
        std::shared_ptr<ContentStream> cs;
        if (_seamless) {
          cs = std::make_shared<SeamlessContentStream>(broadcast_url, _iface, _io_service, _cache,
                                                       service->delivery_protocol(), _cfg);
        } else {
          cs = std::make_shared<ContentStream>(broadcast_url, _iface, _io_service, _cache, service->delivery_protocol(),
                                               _cfg);
        }

        for (const auto &item: _items) {
          if (item.uri == manifest_url) {
            cs->read_master_manifest(item.content);
          }
          if (item.content_type == ContentTypeConstants::SDP &&
              item.uri == sdp_uri) {
            cs->configure_5gbc_delivery_from_sdp(item.content);
          }
        }

        broadcastContentStreams.push_back(cs);
      }
    }

    if (_seamless) {
      // Iterate through unicastAppService elements. If we find a match in identical content we add the CDN information to the existing broadcast content stream
      // If not we create a new content stream object pointing to the CDN url
      auto unicast_app_service = delivery_method->FirstChildElement(
          ServiceAnnouncementXmlElements::UNICAST_APP_SERVICE);
      if (unicast_app_service != nullptr) {
        for (auto *base_pattern = unicast_app_service->FirstChildElement(
            ServiceAnnouncementXmlElements::BASE_PATTERN);
             base_pattern != nullptr;
             base_pattern = base_pattern->NextSiblingElement(ServiceAnnouncementXmlElements::BASE_PATTERN)) {
          // For HLS streams base_pattern now holds the url to the media manifest
          std::string unicast_url = base_pattern->GetText();
          // Check for identical content entries that contain this base pattern.
          for (auto *identical_content = app_service->FirstChildElement(
              ServiceAnnouncementXmlElements::IDENTICAL_CONTENT);
               identical_content != nullptr;
               identical_content = identical_content->NextSiblingElement(
                   ServiceAnnouncementXmlElements::IDENTICAL_CONTENT)) {
            std::shared_ptr<SeamlessContentStream> broadcast_content_stream;
            bool found_identical_element = false;
            for (auto *base_pattern_ic = identical_content->FirstChildElement(
                ServiceAnnouncementXmlElements::BASE_PATTERN);
                 base_pattern_ic != nullptr;
                 base_pattern_ic = base_pattern_ic->NextSiblingElement(ServiceAnnouncementXmlElements::BASE_PATTERN)) {
              // For HLS streams base_pattern_ic now holds either an url that points to a BC media playlist or to a UC playlist
              std::string identical_content_url = base_pattern_ic->GetText();
              // Check if we have a match of our current unicast url with the url in the identical content element
              // Otherwise check if the current identical content url points to an existing BC element
              if (unicast_url == identical_content_url) {
                found_identical_element = true;
              } else {
                for (auto &element: broadcastContentStreams) {
                  if (element->base() == identical_content_url) {
                    broadcast_content_stream = std::dynamic_pointer_cast<SeamlessContentStream>(element);
                  }
                }
              }
            }
            // If we found a matching broadcast stream we add the CDN url to this one. Otherwise, we create a new SeamlessContentStream element
            if (broadcast_content_stream != nullptr && found_identical_element) {
              broadcast_content_stream->set_cdn_endpoint(unicast_url);
            } else {
              std::shared_ptr<SeamlessContentStream> cs = std::make_shared<SeamlessContentStream>(manifest_url, _iface,
                                                                                                  _io_service, _cache,
                                                                                                  service->delivery_protocol(),
                                                                                                  _cfg);
              cs->set_cdn_endpoint(unicast_url);
              unicastContentStreams.push_back(cs);
            }
          }
        }
      }
    }

    for (auto &element: unicastContentStreams) {
      service->add_and_start_content_stream(element);
    }

    for (auto &element: broadcastContentStreams) {
      service->add_and_start_content_stream(element);
    }

    spdlog::info("Finished SA setup with 5G-MAG Format");
  }

}

/**
 * Setup according to the format that was used for the first 5G-MAG sample recordings
 * @param app_service
 * @param service
 * @param usd
 */
void MBMS_RT::ServiceAnnouncement::_setupBy5GMagLegacyFormat(tinyxml2::XMLElement *app_service,
                                                             const std::shared_ptr<MBMS_RT::Service> &service,
                                                             tinyxml2::XMLElement *usd) {

  std::vector<std::shared_ptr<ContentStream>> broadcastContentStreams;
// Create content stream objects for each broadcastAppService::basePattern element.
  for (auto *delivery_method = usd->FirstChildElement(ServiceAnnouncementXmlElements::DELIVERY_METHOD);
       delivery_method != nullptr;
       delivery_method = delivery_method->NextSiblingElement(ServiceAnnouncementXmlElements::DELIVERY_METHOD)) {
    auto sdp_uri = delivery_method->Attribute(ServiceAnnouncementXmlElements::SESSION_DESCRIPTION_URI);
    // We assume that the master manifest is signaled in the SA and that we can simply replace the .sdp ending with .m3u8 to find the element with the right Content-Location
    auto manifest_url = std::regex_replace(sdp_uri, std::regex(".sdp"), ".m3u8");
    auto broadcast_app_service = delivery_method->FirstChildElement(
        ServiceAnnouncementXmlElements::BROADCAST_APP_SERVICE);

    if (broadcast_app_service != nullptr) {
      for (auto *base_pattern = broadcast_app_service->FirstChildElement(ServiceAnnouncementXmlElements::BASE_PATTERN);
           base_pattern != nullptr;
           base_pattern = base_pattern->NextSiblingElement(ServiceAnnouncementXmlElements::BASE_PATTERN)) {

        std::string broadcast_url = base_pattern->GetText();

        // create a content stream if this is not a base pattern that points to file://
        if (broadcast_url.find("file://") == std::string::npos) {

          std::shared_ptr<ContentStream> cs;
          cs = std::make_shared<ContentStream>(broadcast_url, _iface, _io_service, _cache, service->delivery_protocol(),
                                               _cfg);


          for (const auto &item: _items) {
            if (item.uri == manifest_url) {
              cs->read_master_manifest(item.content);
            }
            if (item.content_type == ContentTypeConstants::SDP &&
                item.uri == sdp_uri) {
              cs->configure_5gbc_delivery_from_sdp(item.content);
            }
          }

          broadcastContentStreams.push_back(cs);
        }
      }
    }

    for (auto &element: broadcastContentStreams) {
      service->add_and_start_content_stream(element);
    }

    spdlog::info("Finished SA setup with 5G-MAG Legacy Format");

  }
}


/**
 * Setup according to original SA format with an alternativeContentElement required to indicate that the stream is available via BC and UC
 * @param app_service
 * @param service
 * @param usd
 */
void MBMS_RT::ServiceAnnouncement::_setupByAlternativeContentElement(tinyxml2::XMLElement *app_service,
                                                                     const std::shared_ptr<MBMS_RT::Service> &service,
                                                                     tinyxml2::XMLElement *usd) {
  auto alternative_content = app_service->FirstChildElement(ServiceAnnouncementXmlElements::ALTERNATIVE_CONTENT);
  if (alternative_content != nullptr) {
    for (auto *base_pattern = alternative_content->FirstChildElement(ServiceAnnouncementXmlElements::BASE_PATTERN);
         base_pattern != nullptr;
         base_pattern = base_pattern->NextSiblingElement(ServiceAnnouncementXmlElements::BASE_PATTERN)) {
      std::string base = base_pattern->GetText();

      // create a content stream
      std::shared_ptr<ContentStream> cs;
      if (_seamless) {
        cs = std::make_shared<SeamlessContentStream>(base, _iface, _io_service, _cache,
                                                     service->delivery_protocol(), _cfg);
      } else {
        cs = std::make_shared<ContentStream>(base, _iface, _io_service, _cache, service->delivery_protocol(),
                                             _cfg);
      }


      // Check for 5GBC delivery method elements
      bool broadcast_delivery_available = false;
      for (auto *delivery_method = usd->FirstChildElement(ServiceAnnouncementXmlElements::DELIVERY_METHOD);
           delivery_method != nullptr;
           delivery_method = delivery_method->NextSiblingElement(ServiceAnnouncementXmlElements::DELIVERY_METHOD)) {
        auto sdp_uri = delivery_method->Attribute(ServiceAnnouncementXmlElements::SESSION_DESCRIPTION_URI);
        auto broadcast_app_service = delivery_method->FirstChildElement(
            ServiceAnnouncementXmlElements::BROADCAST_APP_SERVICE);
        std::string broadcast_base_pattern = broadcast_app_service->FirstChildElement(
            ServiceAnnouncementXmlElements::BASE_PATTERN)->GetText();

        if (broadcast_base_pattern == base) {
          for (const auto &item: _items) {
            if (item.uri == broadcast_base_pattern) {
              cs->read_master_manifest(item.content);
            }
            if (item.content_type == ContentTypeConstants::SDP &&
                item.uri == sdp_uri) {
              broadcast_delivery_available = cs->configure_5gbc_delivery_from_sdp(item.content);
            }
          }
        }
      }
      bool unicast_delivery_available = false;

      // When seamless switching is enabled we check for unicast endpoints as well
      if (_seamless) {
        if (!broadcast_delivery_available) {
          // No 5G broadcast available. Assume the base pattern is a CDN endpoint.
          std::dynamic_pointer_cast<SeamlessContentStream>(cs)->set_cdn_endpoint(base);
          unicast_delivery_available = true;
        } else {
          // Check for identical content entries to find a CDN base pattern
          for (auto *identical_content = app_service->FirstChildElement(
              ServiceAnnouncementXmlElements::IDENTICAL_CONTENT);
               identical_content != nullptr;
               identical_content = identical_content->NextSiblingElement(
                   ServiceAnnouncementXmlElements::IDENTICAL_CONTENT)) {

            bool base_matched = false;
            std::string found_identical_base;
            for (auto *base_pattern = identical_content->FirstChildElement(
                ServiceAnnouncementXmlElements::BASE_PATTERN);
                 base_pattern != nullptr;
                 base_pattern = base_pattern->NextSiblingElement(ServiceAnnouncementXmlElements::BASE_PATTERN)) {
              std::string identical_base = base_pattern->GetText();
              if (base == identical_base) {
                base_matched = true;
              } else {
                found_identical_base = identical_base;
              }
            }

            if (base_matched && found_identical_base.length()) {
              std::dynamic_pointer_cast<SeamlessContentStream>(cs)->set_cdn_endpoint(found_identical_base);
            }
          }
        }
      }

      if (unicast_delivery_available || broadcast_delivery_available) {
        service->add_and_start_content_stream(cs);
      }
    }
  }
}
