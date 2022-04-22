/*
// Copyright (c) 2017 Intel Corporation
// Copyright (c) 2022 IBM Corp.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
/// \file Probe.hpp
#include "DBusData.hpp"
#include "PerformScan.hpp"

#include <memory>
#include <string>
#include <vector>

struct CmpStr
{
    bool operator()(const char* a, const char* b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

// underscore T for collison with dbus c api
enum class probe_type_codes
{
    FALSE_T,
    TRUE_T,
    AND,
    OR,
    FOUND,
    MATCH_ONE
};

using FoundProbeTypeT =
    std::optional<boost::container::flat_map<const char*, probe_type_codes,
                                             CmpStr>::const_iterator>;
FoundProbeTypeT findProbeType(const std::string& probe);

/// \brief Match a Dbus property against a probe statement.
/// \param probe the probe statement to match against.
/// \param dbusValue the property value being matched to a probe.
/// \return true if the dbusValue matched the probe otherwise false.
bool matchProbe(const nlohmann::json& probe, const DBusValueVariant& dbusValue);

bool probe(const std::vector<std::string>& probeCommand,
           const std::shared_ptr<PerformScan>& scan, FoundDevices& foundDevs);
