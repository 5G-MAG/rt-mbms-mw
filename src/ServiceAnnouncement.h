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

#include <string>
#include <thread>
#include <libconfig.h++>
#include <tinyxml2.h>
#include "cpprest/http_client.h"
#include "File.h"
#include "Receiver.h"
#include "Service.h"
#include "CacheManagement.h"
#include "Constants.h"

namespace MBMS_RT {
  class ServiceAnnouncement {
  public:
    typedef std::function<std::shared_ptr<Service>(const std::string &service_id)> get_service_callback_t;
    typedef std::function<void(const std::string &service_id, std::shared_ptr<Service>)> set_service_callback_t;

    ServiceAnnouncement(const libconfig::Config &cfg, std::string tmgi, const std::string &mcast,
                        unsigned long long tsi,
                        std::string iface, boost::asio::io_service &io_service, CacheManagement &cache,
                        bool seamless_switching,
                        get_service_callback_t get_service, set_service_callback_t set_service);

    virtual ~ServiceAnnouncement();

    struct Item {
      std::string content_type;
      std::string uri;
      time_t valid_from;
      time_t valid_until;
      unsigned version;
      std::string content;
    };

    const std::vector<Item> &items() const { return _items; };

    const std::string &content() const { return _raw_content; };

    uint32_t toi() const { return _toi; };

    void parse_bootstrap(const std::string &str);

    void start_flute_receiver(const std::string &mcast_address);

  private:

    get_service_callback_t _get_service;
    set_service_callback_t _set_service;

    bool _seamless = false;

    std::vector<Item> _items;

    const libconfig::Config &_cfg;

    bool _bootstrapped = false;

    uint32_t _toi = {};
    std::string _raw_content;
    std::string _iface;
    std::string _tmgi;
    std::string _mcast_addr;
    std::string _mcast_port;
    unsigned long long _tsi = 0;
    std::thread _flute_thread;
    std::unique_ptr<LibFlute::Receiver> _flute_receiver;

    boost::asio::io_service &_io_service;
    CacheManagement &_cache;

    void _addServiceAnnouncementItems(const std::string &str);

    void _handleMbmsEnvelope(const Item &item);

    void _handleMbmbsUserServiceDescriptionBundle(const Item &item, const std::string &bootstrap_format);

    std::tuple<std::shared_ptr<MBMS_RT::Service>, bool>
    _registerService(tinyxml2::XMLElement *usd, const std::string &service_id);

    void _handleAppService(tinyxml2::XMLElement *app_service, std::shared_ptr<Service> service);

    bool _setupBroadcastDelivery(tinyxml2::XMLElement *usd, std::string base, std::shared_ptr<ContentStream> cs);

    void
    _setupByAlternativeContentElement(tinyxml2::XMLElement *app_service,
                                      const std::shared_ptr<MBMS_RT::Service> &service,
                                      tinyxml2::XMLElement *usd);

    void
    _setupBy5GMagConfig(tinyxml2::XMLElement *app_service,
                                      const std::shared_ptr<MBMS_RT::Service> &service,
                                      tinyxml2::XMLElement *usd);

    void
    _setupBy5GMagLegacyFormat(tinyxml2::XMLElement *app_service,
                        const std::shared_ptr<MBMS_RT::Service> &service,
                        tinyxml2::XMLElement *usd);
  };
}
