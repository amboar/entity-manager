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
/// \file PerformScan.hpp

#pragma once

#include "DBusData.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <functional>
#include <list>
#include <memory>
#include <string>

struct DBusInterfaceInstance;

struct PerformScan : std::enable_shared_from_this<PerformScan>
{
    PerformScan(nlohmann::json& systemConfiguration,
                nlohmann::json& missingConfigurations,
                std::list<nlohmann::json>& configurations,
                sdbusplus::asio::object_server& objServer,
                std::function<void()>&& callback);
    void updateSystemConfiguration(const nlohmann::json& recordRef,
                                   const std::string& probeName,
                                   FoundDevices& foundDevices);
    void registerInterface(const DBusInterfaceInstance& instance, const DBusInterface& interface);
    void run(void);
    virtual ~PerformScan();
    nlohmann::json& _systemConfiguration;
    nlohmann::json& _missingConfigurations;
    std::list<nlohmann::json> _configurations;
    sdbusplus::asio::object_server& objServer;
    std::function<void()> _callback;
    bool _passed = false;
    MapperGetSubTreeResponse dbusProbeObjects;
    std::vector<std::string> passedProbes;
};
