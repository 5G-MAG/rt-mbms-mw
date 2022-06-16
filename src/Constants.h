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
}

#endif //MW_CONSTANTS_H
