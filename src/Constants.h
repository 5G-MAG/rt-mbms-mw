// 5G-MAG Reference Tools
// MBMS Middleware Process
//
// Copyright (C) 2022 Daniel Silhavy (Fraunhofer FOKUS)
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

#ifndef MW_CONSTANTS_H
#define MW_CONSTANTS_H

namespace ContentTypeConstants {
  const std::string MBMS_ENVELOPE = "application/mbms-envelope+xml";
  const std::string MBMS_USER_SERVICE_DESCRIPTION = "application/mbms-user-service-description+xml";
  const std::string HLS = "application/vnd.apple.mpegurl";
  const std::string DASH = "application/dash+xml";
  const std::string SDP = "application/sdp";
  const std::string HLS_MANIFEST = "m3u8";
  const std::string DASH_MANIFEST = "mpd";
}

namespace ServiceAnnouncementFormatConstants {
  const std::string DEFAULT = "default";
  const std::string FIVEG_MAG_BC_UC = "5gmag_bc_uc";
  const std::string FIVEG_MAG_LEGACY = "5gmag_legacy";
}


namespace ServiceAnnouncementXmlElements {
  const char *const BUNDLE_DESCRIPTION = "bundleDescription";
  const char *const NAME = "name";
  const char *const LANG = "lang";
  const char *const USER_SERVICE_DESCRIPTION = "userServiceDescription";
  const char *const SERVICE_ID = "serviceId";
  const char *const APP_SERVICE = "r12:appService";
  const char *const DELIVERY_METHOD = "deliveryMethod";
  const char *const SESSION_DESCRIPTION_URI = "sessionDescriptionURI";
  const char *const BASE_PATTERN = "r12:basePattern";
  const char *const BROADCAST_APP_SERVICE = "r12:broadcastAppService";
  const char *const UNICAST_APP_SERVICE = "r12:unicastAppService";
  const char *const ALTERNATIVE_CONTENT = "r12:alternativeContent";
  const char *const IDENTICAL_CONTENT = "r12:identicalContent";
  const char *const MIME_TYPE = "mimeType";
  const char *const APP_SERVICE_DESCRIPTION_URI = "appServiceDescriptionURI";
  const char *const METADATA_ENVELOPE = "metadataEnvelope";
  const char *const METADATA_URI = "metadataURI";
  const char *const ITEM = "item";
  const char *const VERSION = "version";
  const char *const VALID_FROM = "validFrom";
  const char *const VALID_UNTIL = "validUntil";
}

#endif //MW_CONSTANTS_H
