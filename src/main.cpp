// OBECA - Open Broadcast Edge Cache Appliance
// Gateway Process
//
// Copyright (C) 2021 Klaus Kühnhammer (Österreichische Rundfunksender GmbH & Co KG)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

/**
 * @file main.cpp
 * @brief Contains the program entry point, command line parameter handling, and the main runloop for data processing.
 */

/** \mainpage OBECA - Open Broadcast Edge Cache Applicance, gateway process
 *
 * This is the documentation for the OBECA gateway process. Please see main.cpp for for the runloop and main processing logic as a starting point.
 *
 */

#include <argp.h>

#include <cstdlib>
#include <string>
#include <filesystem>
#include <libconfig.h++>

#include "Version.h"

#include "spdlog/async.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/syslog_sink.h"

#include "RpRestClient.h"
#include "Service.h"
#include "File.h"
#include "RestHandler.h"

namespace fcl {
  extern "C" {
#include "flutelib/flute.h"
  }
}


using libconfig::Config;
using libconfig::FileIOException;
using libconfig::ParseException;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

static void print_version(FILE *stream, struct argp_state *state);
void (*argp_program_version_hook)(FILE *, struct argp_state *) = print_version;
const char *argp_program_bug_address = "Austrian Broadcasting Services <obeca@ors.at>";
static char doc[] = "OBECA Gateway Process";  // NOLINT

static struct argp_option options[] = {  // NOLINT
    {"config", 'c', "FILE", 0, "Configuration file (default: /etc/obeca.conf)", 0},
    {"interface", 'i', "IF", 0, "IP address of the interface to bind flute receivers to (default: 192.168.180.10)", 0},
    {"log-level", 'l', "LEVEL", 0,
     "Log verbosity: 0 = trace, 1 = debug, 2 = info, 3 = warn, 4 = error, 5 = "
     "critical, 6 = none. Default: 2.",
     0},
    {nullptr, 0, nullptr, 0, nullptr, 0}};

/**
 * Holds all options passed on the command line
 */
struct gw_arguments {
  const char *config_file = {};  /**< file path of the config file. */
  const char *flute_interface = {};  /**< file path of the config file. */
  unsigned log_level = 2;        /**< log level */
};

/**
 * Parses the command line options into the arguments struct.
 */
static auto parse_opt(int key, char *arg, struct argp_state *state) -> error_t {
  auto arguments = static_cast<struct gw_arguments *>(state->input);
  switch (key) {
    case 'c':
      arguments->config_file = arg;
      break;
    case 'i':
      arguments->flute_interface = arg;
      break;
    case 'l':
      arguments->log_level = static_cast<unsigned>(strtoul(arg, nullptr, 10));
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, nullptr, doc,
                           nullptr, nullptr,   nullptr};

/**
 * Print the program version in MAJOR.MINOR.PATCH format.
 */
void print_version(FILE *stream, struct argp_state * /*state*/) {
  fprintf(stream, "%s.%s.%s\n", std::to_string(VERSION_MAJOR).c_str(),
          std::to_string(VERSION_MINOR).c_str(),
          std::to_string(VERSION_PATCH).c_str());
}

static Config cfg;  /**< Global configuration object. */

static std::map<std::string, OBECA::File> downloaded_files;
/**
 *  Main entry point for the program.
 *  
 * @param argc  Command line agument count
 * @param argv  Command line arguments
 * @return 0 on clean exit, -1 on failure
 */
auto main(int argc, char **argv) -> int {
  struct gw_arguments arguments;
  /* Default values */
  arguments.config_file = "/etc/obeca.conf";
  arguments.flute_interface= "192.168.180.10";

  argp_parse(&argp, argc, argv, 0, nullptr, &arguments);

  // Read and parse the configuration file
  try {
    cfg.readFile(arguments.config_file);
  } catch(const FileIOException &fioex) {
    spdlog::error("I/O error while reading config file at {}. Exiting.", arguments.config_file);
    exit(1);
  } catch(const ParseException &pex) {
    spdlog::error("Config parse error at {}:{} - {}. Exiting.",
        pex.getFile(), pex.getLine(), pex.getError());
    exit(1);
  }

  // Set up logging
  std::string ident = "gw";
  auto syslog_logger = spdlog::syslog_logger_mt("syslog", ident, LOG_PID | LOG_PERROR | LOG_CONS );

  spdlog::set_level(
      static_cast<spdlog::level::level_enum>(arguments.log_level));
  spdlog::set_pattern("[%H:%M:%S.%f %z] [%^%l%$] [thr %t] %v");

  spdlog::set_default_logger(syslog_logger);
  spdlog::info("OBECA gw v{}.{}.{} starting up", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

  if(fcl::start_up_flute() != 0) {
    spdlog::error("Failed to start FLUTE library");
    return -1;
  }

  unsigned max_cache_size = 512;
  unsigned total_cache_size = 0;
  cfg.lookupValue("gw.cache.max_total_size", max_cache_size);
  max_cache_size *= 1024 * 1024;

  unsigned max_cache_file_age = 30;
  cfg.lookupValue("gw.cache.max_file_age", max_cache_file_age);


  OBECA::RpRestClient rp(cfg);
  std::map<std::string, std::string> available_services;
  std::map<std::string, std::unique_ptr<OBECA::Service>> services;
  std::map<std::string, std::unique_ptr<OBECA::Service>> payload_flute_streams;

  std::string uri = "http://0.0.0.0:3020/gw-api/";
  cfg.lookupValue("gw.restful_api.uri", uri);
  spdlog::info("Starting RESTful API handler at {}", uri);
  RestHandler api(cfg, uri, downloaded_files, total_cache_size, services);

  for (;;) {
    sleep(1);
    auto mchs = rp.getMchInfo();
    available_services.clear();
    for (auto const &mch : mchs.as_array()) {
      for (auto const &mtch : mch.at("mtchs").as_array()) {
        auto tmgi = mtch.at("tmgi").as_string();
        auto dest = mtch.at("dest").as_string();
        available_services[tmgi] = dest;
        auto is_service_announcement = std::stoul(tmgi.substr(0,6), nullptr, 16) < 0xF;
        if (!dest.empty() && is_service_announcement && services.find(tmgi) == services.end()) {
          // automatically start receiving the service announcement
          // 26.346 5.2.3.1.1 : the pre-defined TSI value shall be "0". 
          services[tmgi] = std::make_unique<OBECA::Service>(cfg, tmgi, dest, 0 /*TSI*/, arguments.flute_interface);
          services[tmgi]->setIsServiceAnnouncement(true);
        }
      }
    }

    for (auto it = services.cbegin(); it != services.cend();)
    {
      if (available_services.find(it->first) == available_services.end()) {
        it = services.erase(it);
      } else {
        ++it;
      }
    }

    for (auto const& [tmgi, service] : services) {
    // read file lists
      auto files = service->fileList();
      for (auto const & file : files) {
        downloaded_files.insert_or_assign(file.location(), file);
        if (file.location().filename() == "bootstrap.multipart" && service->isServiceAnnouncement()) {
          if (!service->bootstrapped()) {
            service->tryParseBootstrapFile();
          }
        }
      }

      if (service->bootstrapped() && service->streamType() == "FLUTE/UDP") {
        if (payload_flute_streams.count(service->streamMcast()) == 0) {
          auto mcast_target = service->streamMcast();
          auto service_available = std::find_if(available_services.begin(), available_services.end(),
                        [&mcast_target](const std::pair<std::string, std::string> &p) {
                            return p.second == mcast_target;
                        });
          
          if (service_available != available_services.end()) {
            if (services.find(service_available->first) == services.end()) {
              service->setStreamTmgi(service_available->first);
              services[service_available->first] = std::make_unique<OBECA::Service>(cfg, service_available->first, service->streamMcast(), service->streamFluteTsi(), arguments.flute_interface);
            }
          }
        }
      }
    }

    std::multimap<unsigned, OBECA::File, std::greater<>> file_ages;
    unsigned long long total_size = 0;
    for (auto it = downloaded_files.cbegin(); it != downloaded_files.cend();)
    {
        time_t ts = it->second.received_at();
        auto age = time(nullptr) - it->second.received_at();
        total_size += it->second.content_length();
        file_ages.insert(std::make_pair(age, it->second));
       if ( it->second.location().filename() != "bootstrap.multipart"  && age > max_cache_file_age) {
         remove(it->second.location());
        it = downloaded_files.erase(it);
      } else {
        ++it;
      }
    }
    for (auto const& [age, file] : file_ages) {
      if (total_size > max_cache_size) {
        downloaded_files.erase(file.location());
        remove(file.location());
        total_size -= file.content_length();
      }
    }
    total_cache_size = total_size;

  }

exit:
  return 0;
}
