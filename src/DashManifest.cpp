// 5G-MAG Reference Tools
// MBMS Middleware Process
//
// Copyright (C) 2021 Daniel Silhavy (Fraunhofer FOKUS)
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

#include "DashManifest.h"

MBMS_RT::DashManifest::DashManifest(const std::string &content, const std::string &base_path)
    : content(content), base_path(base_path) {}