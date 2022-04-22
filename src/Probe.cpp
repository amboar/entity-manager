/*
// Copyright (c) 2018 Intel Corporation
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
/// \file Probe.cpp

#include "Probe.hpp"
#include "Utils.hpp"

#include <boost/algorithm/string/replace.hpp>

#include <iostream>
#include <regex>

constexpr const bool debug = false;

const boost::container::flat_map<const char*, probe_type_codes, CmpStr>
    probeTypes{{{"FALSE", probe_type_codes::FALSE_T},
                {"TRUE", probe_type_codes::TRUE_T},
                {"AND", probe_type_codes::AND},
                {"OR", probe_type_codes::OR},
                {"FOUND", probe_type_codes::FOUND},
                {"MATCH_ONE", probe_type_codes::MATCH_ONE}}};

FoundProbeTypeT findProbeType(const std::string& probe)
{
    boost::container::flat_map<const char*, probe_type_codes,
                               CmpStr>::const_iterator probeType;
    for (probeType = probeTypes.begin(); probeType != probeTypes.end();
         ++probeType)
    {
        if (probe.find(probeType->first) != std::string::npos)
        {
            return probeType;
        }
    }

    return std::nullopt;
}

/// \brief JSON/DBus matching Callable for std::variant (visitor)
///
/// Default match JSON/DBus match implementation
/// \tparam T The concrete DBus value type from DBusValueVariant
template <typename T>
struct MatchProbe
{
    /// \param probe the probe statement to match against
    /// \param value the property value being matched to a probe
    /// \return true if the dbusValue matched the probe otherwise false
    static bool match(const nlohmann::json& probe, const T& value)
    {
        return probe == value;
    }
};

/// \brief JSON/DBus matching Callable for std::variant (visitor)
///
/// std::string specialization of MatchProbe enabling regex matching
template <>
struct MatchProbe<std::string>
{
    /// \param probe the probe statement to match against
    /// \param value the string value being matched to a probe
    /// \return true if the dbusValue matched the probe otherwise false
    static bool match(const nlohmann::json& probe, const std::string& value)
    {
        if (probe.is_string())
        {
            try
            {
                std::regex search(probe);
                std::smatch regMatch;
                return std::regex_search(value, regMatch, search);
            }
            catch (const std::regex_error&)
            {
                std::cerr << "Syntax error in regular expression: " << probe
                          << " will never match";
            }
        }

        // Skip calling nlohmann here, since it will never match a non-string
        // to a std::string
        return false;
    }
};

/// \brief Forwarding JSON/DBus matching Callable for std::variant (visitor)
///
/// Forward calls to the correct template instantiation of MatchProbe
struct MatchProbeForwarder
{
    explicit MatchProbeForwarder(const nlohmann::json& probe) : probeRef(probe)
    {}
    const nlohmann::json& probeRef;

    template <typename T>
    bool operator()(const T& dbusValue) const
    {
        return MatchProbe<T>::match(probeRef, dbusValue);
    }
};

bool matchProbe(const nlohmann::json& probe, const DBusValueVariant& dbusValue)
{
    return std::visit(MatchProbeForwarder(probe), dbusValue);
}

// probes dbus interface dictionary for a key with a value that matches a regex
// When an interface passes a probe, also save its D-Bus path with it.
static bool probeDbus(const std::string& interfaceName,
               const std::map<std::string, nlohmann::json>& matches,
               FoundDevices& devices, const std::shared_ptr<PerformScan>& scan,
               bool& foundProbe)
{
    bool foundMatch = false;
    foundProbe = false;

    for (const auto& [path, interfaces] : scan->dbusProbeObjects)
    {
        auto it = interfaces.find(interfaceName);
        if (it == interfaces.end())
        {
            continue;
        }

        foundProbe = true;

        bool deviceMatches = true;
        const DBusInterface& interface = it->second;

        for (const auto& [matchProp, matchJSON] : matches)
        {
            auto deviceValue = interface.find(matchProp);
            if (deviceValue != interface.end())
            {
                deviceMatches =
                    deviceMatches && matchProbe(matchJSON, deviceValue->second);
            }
            else
            {
                // Move on to the next DBus path
                deviceMatches = false;
                break;
            }
        }
        if (deviceMatches)
        {
            if constexpr (debug)
            {
                std::cerr << "probeDBus: Found probe match on " << path << " "
                          << interfaceName << "\n";
            }
            // Use emplace back when clang implements
            // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html
            //
            // https://en.cppreference.com/w/cpp/compiler_support/20
            devices.push_back({interface, path});
            foundMatch = true;
        }
    }
    return foundMatch;
}

// default probe entry point, iterates a list looking for specific types to
// call specific probe functions
bool probe(const std::vector<std::string>& probeCommand,
           const std::shared_ptr<PerformScan>& scan, FoundDevices& foundDevs)
{
    const static std::regex command(R"(\((.*)\))");
    std::smatch match;
    bool ret = false;
    bool matchOne = false;
    bool cur = true;
    probe_type_codes lastCommand = probe_type_codes::FALSE_T;
    bool first = true;

    for (auto& probe : probeCommand)
    {
        FoundProbeTypeT probeType = findProbeType(probe);
        if (probeType)
        {
            switch ((*probeType)->second)
            {
                case probe_type_codes::FALSE_T:
                {
                    cur = false;
                    break;
                }
                case probe_type_codes::TRUE_T:
                {
                    cur = true;
                    break;
                }
                case probe_type_codes::MATCH_ONE:
                {
                    // set current value to last, this probe type shouldn't
                    // affect the outcome
                    cur = ret;
                    matchOne = true;
                    break;
                }
                /*case probe_type_codes::AND:
                  break;
                case probe_type_codes::OR:
                  break;
                  // these are no-ops until the last command switch
                  */
                case probe_type_codes::FOUND:
                {
                    if (!std::regex_search(probe, match, command))
                    {
                        std::cerr << "found probe syntax error " << probe
                                  << "\n";
                        return false;
                    }
                    std::string commandStr = *(match.begin() + 1);
                    boost::replace_all(commandStr, "'", "");
                    cur = (std::find(scan->passedProbes.begin(),
                                     scan->passedProbes.end(),
                                     commandStr) != scan->passedProbes.end());
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        // look on dbus for object
        else
        {
            if (!std::regex_search(probe, match, command))
            {
                std::cerr << "dbus probe syntax error " << probe << "\n";
                return false;
            }
            std::string commandStr = *(match.begin() + 1);
            // convert single ticks and single slashes into legal json
            boost::replace_all(commandStr, "'", "\"");
            boost::replace_all(commandStr, R"(\)", R"(\\)");
            auto json = nlohmann::json::parse(commandStr, nullptr, false);
            if (json.is_discarded())
            {
                std::cerr << "dbus command syntax error " << commandStr << "\n";
                return false;
            }
            // we can match any (string, variant) property. (string, string)
            // does a regex
            std::map<std::string, nlohmann::json> dbusProbeMap =
                json.get<std::map<std::string, nlohmann::json>>();
            auto findStart = probe.find('(');
            if (findStart == std::string::npos)
            {
                return false;
            }
            bool foundProbe = !!probeType;
            std::string probeInterface = probe.substr(0, findStart);
            cur = probeDbus(probeInterface, dbusProbeMap, foundDevs, scan,
                            foundProbe);
        }

        // some functions like AND and OR only take affect after the
        // fact
        if (lastCommand == probe_type_codes::AND)
        {
            ret = cur && ret;
        }
        else if (lastCommand == probe_type_codes::OR)
        {
            ret = cur || ret;
        }

        if (first)
        {
            ret = cur;
            first = false;
        }
        lastCommand =
            probeType ? (*probeType)->second : probe_type_codes::FALSE_T;
    }

    // probe passed, but empty device
    if (ret && foundDevs.size() == 0)
    {
        // Use emplace back when clang implements
        // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0960r3.html
        //
        // https://en.cppreference.com/w/cpp/compiler_support/20
        foundDevs.push_back(
            {boost::container::flat_map<std::string, DBusValueVariant>{},
             std::string{}});
    }
    if (matchOne && ret)
    {
        // match the last one
        auto last = foundDevs.back();
        foundDevs.clear();

        foundDevs.emplace_back(std::move(last));
    }
    return ret;
}
