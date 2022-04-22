/*
// Copyright (c) 2018 Intel Corporation
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
/// \file PerformProbe.cpp
#include "PerformProbe.hpp"
#include "Probe.hpp"

PerformProbe::PerformProbe(
    nlohmann::json& recordRef,
    const std::vector<std::string>& probeCommand,
    std::string probeName,
    std::shared_ptr<PerformScan>& scanPtr) :
    recordRef(recordRef),
    _probeCommand(probeCommand),
    probeName(probeName),
    scan(scanPtr)
{}
PerformProbe::~PerformProbe()
{
    FoundDevices foundDevs;
    if (probe(_probeCommand, scan, foundDevs))
    {
        scan->updateSystemConfiguration(recordRef, probeName, foundDevs);
    }
}
