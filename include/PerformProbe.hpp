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
/// \file Utils.hpp

#pragma once

#include "PerformScan.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

// this class finds the needed dbus fields and on destruction runs the probe
struct PerformProbe : std::enable_shared_from_this<PerformProbe>
{
    PerformProbe(
        nlohmann::json& recordRef,
        const std::vector<std::string>& probeCommand,
        std::string probeName,
        std::shared_ptr<PerformScan>& scanPtr);
    virtual ~PerformProbe();

    nlohmann::json& recordRef;
    std::vector<std::string> _probeCommand;
    std::string probeName;
    std::shared_ptr<PerformScan> scan;
};
