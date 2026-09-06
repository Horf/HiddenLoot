#pragma once

// ===== Default Libraries =====
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <system_error>
#include <exception>
#include <string_view>
#include <stdexcept>

// ===== Extern Libraries =====
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// ===== SKSE =====
#include <SKSE/Logger.h>

// ===== RE (Game Types) =====
#include <RE/A/Actor.h>

#include <RE/B/BSCoreTypes.h>
#include <RE/B/BSFixedString.h>
#include <RE/B/BSTEvent.h>
#include <RE/B/BGSKeyword.h>
#include <RE/B/BGSKeywordForm.h>

#include <RE/P/PlayerCharacter.h>

#include <RE/T/TESForm.h>
#include <RE/T/TESDataHandler.h>
#include <RE/T/TESContainerChangedEvent.h>
#include <RE/T/TESBoundObject.h>

// ===== Project =====
#include "Settings.h"

namespace ToolRequirements
{
    // Custom hash function for BSFixedString to allow its use in std::unordered_map
    struct BSFixedStringHash {
        std::size_t operator()(const RE::BSFixedString& a_str) const noexcept {
            return std::hash<std::string_view>{}(a_str.data());
        }
    };

    // Defines a single rule for tool requirements
    struct Rule {
        std::string name;
        std::vector<RE::BSFixedString> lootKeywords;            // Item keywords that trigger this rule
        std::vector<RE::FormID> lootItems;                      // Specific item FormIDs that trigger this rule

        std::vector<RE::BSFixedString> toolKeywords;            // Tool keywords the player needs to have
        std::vector<RE::FormID> toolItems;                      // Specific tool FormIDs the player needs to have
        
        std::vector<RE::BSFixedString> actorKeywords;           // Actor keywords required for this rule to apply
        std::vector<RE::BSFixedString> actorExcludedKeywords;   // Actor keywords that bypass this rule
    };

    class Manager : public RE::BSTEventSink<RE::TESContainerChangedEvent>
    {
    public:
        static Manager* GetSingleton() {
            static Manager singleton;
            return &singleton;
        }

        // Parses the JSON configuration file to load all tool requirement rules
        void LoadRules() {
            _rules.clear();
            if (!Settings::bEnableToolRequirements) return;

            std::filesystem::path jsonPath = "Data/SKSE/Plugins/ToolRequiredLoot.json";

            // Create default JSON if it doesn't exist
            if (!std::filesystem::exists(jsonPath)) {
                nlohmann::json defaultJson = {
                    {"lootConditions", {
                        {
                            {"name", "Skinning"},
                            {"description", "Requires a Dagger, Shiv, or a normal Knife to harvest pelts and leather."},
                            {"lootKeywords", {"VendorItemAnimalHide"}},
                            {"lootItems", nlohmann::json::array()},
                            {"toolKeywords", {"WeapTypeDagger"}},
                            {"toolItems", {"Skyrim.esm|0x426C8", "Skyrim.esm|0x104B40"}},
                            {"actorKeywords", {"ActorTypeAnimal", "ActorTypeCreature", "ActorTypeUndead"}},
                            {"actorExcludedKeywords", nlohmann::json::array()}
                        },
                        {
                            {"name", "Bone and Antler Harvesting"},
                            {"description", "Requires a Saw, Hammer, Woodcutter's Axe, or Poacher's Axe to extract bones, tusks, and antlers."},
                            {"lootKeywords", {"VendorItemAnimalPart"}},
                            {"lootItems", nlohmann::json::array()},
                            {"toolKeywords", nlohmann::json::array()},
                            {"toolItems", {"Skyrim.esm|0xF5D0A", "Skyrim.esm|0x5CAE1", "Skyrim.esm|0x2F2F4", "Skyrim.esm|0xAE086"}},
                            {"actorKeywords", {"ActorTypeAnimal", "ActorTypeCreature"}},
                            {"actorExcludedKeywords", {"ActorTypeNPC", "ActorTypeUndead"}}
                        },
                        {
                            {"name", "Ingredient Extraction"},
                            {"description", "Requires a Dagger, Shiv, Embalming Scalpel, or Embalming Knife, or Embalming Scissors to carefully extract alchemy ingredients (e.g. venom, hearts, fat) from monsters."},
                            {"lootKeywords", {"VendorItemIngredient"}},
                            {"lootItems", nlohmann::json::array()},
                            {"toolKeywords", {"WeapTypeDagger"}},
                            {"toolItems", {"Skyrim.esm|0x426C8", "Skyrim.esm|0x34CCE", "Skyrim.esm|0x34CD0", "Skyrim.esm|0x34CD4"}},
                            {"actorKeywords", nlohmann::json::array()},
                            {"actorExcludedKeywords", {"ActorTypeNPC"}}
                        },
                        {
                            {"name", "Daedric Salvage"},
                            {"description", "Requires a Silver or Daedric weapon to safely handle and salvage Daedric gear from fallen Dremora."},
                            {"lootKeywords", {"ArmorMaterialDaedric", "WeapMaterialDaedric"}},
                            {"lootItems", nlohmann::json::array()},
                            {"toolKeywords", {"WeapMaterialDaedric", "WeapMaterialSilver"}},
                            {"toolItems", nlohmann::json::array()},
                            {"actorKeywords", {"ActorTypeDaedra"}},
                            {"actorExcludedKeywords", nlohmann::json::array()}
                        },
                        {
                            {"name", "Dwarven Dismantling"},
                            {"description", "Requires Blacksmith's Tongs, or a Hammer to pry gears, scrap metal, and oil from destroyed constructs (Requires OCF)."},
                            {"lootKeywords", {"OCF_IngrMisc_Oil", "OCF_MatContainsDwarven"}},
                            {"lootItems", nlohmann::json::array()},
                            {"toolKeywords", nlohmann::json::array()},
                            {"toolItems", {"Skyrim.esm|0x5CAE0", "Skyrim.esm|0x5CAE1"}},
                            {"actorKeywords", {"ActorTypeDwarven"}},
                            {"actorExcludedKeywords", nlohmann::json::array()}
                        },
                        {
                            {"name", "Briar Heart Extraction"},
                            {"description", "Requires a dagger to cut the Briar Heart from a Forsworn's chest."},
                            {"lootKeywords", nlohmann::json::array()},
                            {"lootItems", {"Skyrim.esm|0x3AD61"}},
                            {"toolKeywords", {"WeapTypeDagger"}},
                            {"toolItems", nlohmann::json::array()},
                            {"actorKeywords", nlohmann::json::array()},
                            {"actorExcludedKeywords", nlohmann::json::array()}
                        }
                    }}
                };
                std::error_code ec;
                std::filesystem::create_directories(jsonPath.parent_path(), ec);
                std::ofstream file(jsonPath);
                file << defaultJson.dump(4);
                file.close();
            }

            try {
                std::ifstream file(jsonPath);
                nlohmann::json data = nlohmann::json::parse(file);
                auto dataHandler = RE::TESDataHandler::GetSingleton();

                // Helper lambda to parse simple string arrays into BSFixedString vectors
                auto ParseKeywords = [](const nlohmann::json& a_json, const char* a_key, std::vector<RE::BSFixedString>& a_out, const std::string& a_ruleName) {
                    if (a_json.contains(a_key)) {
                        if (!a_json[a_key].is_array()) {
                            logs::warn("ToolRequirements [{}]: Field '{}' is formatted incorrectly. Example: [\"Keyword\"]", a_ruleName, a_key);
                            return;
                        }
                        for (const auto& item : a_json[a_key]) {
                            a_out.push_back(item.get<std::string>());
                        }
                    }
                };

                // Helper lambda to safely parse "ModName.esp|0x123" strings into resolved FormIDs
                auto ParseFormIDs = [&](const nlohmann::json& a_json, const char* a_key, std::vector<RE::FormID>& a_out, const std::string& a_ruleName) {
                    if (a_json.contains(a_key)) {
                        if (!a_json[a_key].is_array()) {
                            logs::warn("ToolRequirements [{}]: Field '{}' is formatted incorrectly. Example: [\"ModName.esp|0x123\"]", a_ruleName, a_key);
                            return;
                        }
                        for (const auto& item : a_json[a_key]) {
                            std::string idString = item.get<std::string>();
                            size_t pipe = idString.find('|');
                            if (pipe != std::string::npos && dataHandler) {
                                std::string modName = idString.substr(0, pipe);
                                std::string hexStr = idString.substr(pipe + 1);
                                try {
                                    uint32_t localID = std::stoul(hexStr, nullptr, 16);
                                    if (auto form = dataHandler->LookupForm(localID, modName)) {
                                        a_out.push_back(form->GetFormID());
                                    }
                                    else {
                                        logs::warn("ToolRequirements [{}]: FormID '{}' not found.", a_ruleName, idString);
                                    }
                                }
                                catch (const std::invalid_argument&) {
                                    logs::warn("ToolRequirements: Invalid Hex format for FormID '{}'", idString);
                                }
                                catch (const std::out_of_range&) {
                                    logs::warn("ToolRequirements: Hex value out of range for FormID '{}'", idString);
                                }
                            }
                            else {
                                logs::warn("ToolRequirements [{}]: Invalid format '{}'. Expected 'ModName.esp|0x123'", a_ruleName, idString);
                            }
                        }
                    }
                };

                if (data.contains("lootConditions") && data["lootConditions"].is_array()) {
                    for (const auto& jRule : data["lootConditions"]) {
                        Rule rule;
                        if (jRule.contains("name")) rule.name = jRule["name"].get<std::string>();

                        ParseKeywords(jRule, "lootKeywords", rule.lootKeywords, rule.name);
                        ParseFormIDs(jRule, "lootItems", rule.lootItems, rule.name);
                        ParseKeywords(jRule, "toolKeywords", rule.toolKeywords, rule.name);
                        ParseFormIDs(jRule, "toolItems", rule.toolItems, rule.name);
                        ParseKeywords(jRule, "actorKeywords", rule.actorKeywords, rule.name);
                        ParseKeywords(jRule, "actorExcludedKeywords", rule.actorExcludedKeywords, rule.name);

                        _rules.push_back(rule);
                    }
                    logs::info("Successfully loaded {} rules from ToolRequiredLoot.json", _rules.size());
                }
            }
            catch (const std::exception& e) {
                logs::warn("Error parsing ToolRequiredLoot.json: {}", e.what());
            }
        }

        // Performs an initial scan of the player's inventory upon loading a save
        // Populates the internal cache tracking which tools the player currently holds
        void ScanPlayerInventory() {
            if (!Settings::bEnableToolRequirements || _rules.empty()) return;

            std::lock_guard<std::shared_mutex> writeLock(_mutex);
            _keywordCounts.clear();
            _formIDCounts.clear();

            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return;

            auto inventory = player->GetInventory();
            for (const auto& [object, data] : inventory) {
                if (data.first > 0) {
                    UpdateItemCounts(object, data.first);
                }
            }
        }

        // Fast check to see if an item is targeted by ANY rule, ignoring context.
        // Used by the LootHook to prevent early-outs for tool candidates.
        bool IsLootCandidate(RE::TESBoundObject* a_lootItem) const {
            if (!Settings::bEnableToolRequirements || _rules.empty() || !a_lootItem) return false;
            auto keywordForm = a_lootItem->As<RE::BGSKeywordForm>();

            for (const auto& rule : _rules) {
                if (keywordForm) {
                    for (const auto& keyword : rule.lootKeywords) {
                        if (keywordForm->HasKeywordString(keyword)) return true;
                    }
                }
                for (const auto& id : rule.lootItems) {
                    if (a_lootItem->GetFormID() == id) return true;
                }
            }
            return false;
        }

        // Evaluates if the player is missing the required tools to loot a specific item from a given actor
        // Returns true if a tool is required but missing, false otherwise
        bool IsToolMissing(RE::TESBoundObject* a_lootItem, RE::Actor* a_actor) {
            if (!Settings::bEnableToolRequirements || _rules.empty() || !a_lootItem) return false;

            auto keywordForm = a_lootItem->As<RE::BGSKeywordForm>();

            for (const auto& rule : _rules) {
                bool isTargetedItem = false;
                
                if (keywordForm) {
                    for (const auto& keyword : rule.lootKeywords) {
                        if (keywordForm->HasKeywordString(keyword)) {
                            isTargetedItem = true;
                            break;
                        }
                    }
                }
                if (!isTargetedItem) {
                    for (const auto& id : rule.lootItems) {
                        if (a_lootItem->GetFormID() == id) {
                            isTargetedItem = true;
                            break;
                        }
                    }
                }
                if (!isTargetedItem) continue;

                // Actor Inclusion Check
                if (!rule.actorKeywords.empty()) {
                    // If the rule demands an actor keyword, but we don't have an actor, the rule cannot apply
                    if (!a_actor) continue;

                    bool actorMatch = false;
                    auto race = a_actor->GetRace();
                    for (const auto& actorKeyword : rule.actorKeywords) {
                        // Check both the Actor (for dynamic/NPC keywords) and the Race (for ActorType keywords)
                        if (a_actor->HasKeywordString(actorKeyword) || (race && race->HasKeywordString(actorKeyword))) {
                            actorMatch = true;
                            break;
                        }
                    }
                    if (!actorMatch) continue;
                }

                // Actor Exclusion Check
                if (a_actor && !rule.actorExcludedKeywords.empty()) {
                    bool actorExcluded = false;
                    auto race = a_actor->GetRace();
                    for (const auto& excludeKeyword : rule.actorExcludedKeywords) {
                        if (a_actor->HasKeywordString(excludeKeyword) || (race && race->HasKeywordString(excludeKeyword))) {
                            actorExcluded = true;
                            break;
                        }
                    }
                    if (actorExcluded) continue;
                }

                bool hasTool = false;
                {
                    std::shared_lock<std::shared_mutex> readLock(_mutex);

                    for (const auto& reqKeyword : rule.toolKeywords) {
                        auto it = _keywordCounts.find(reqKeyword);
                        if (it != _keywordCounts.end() && it->second > 0) { hasTool = true; break; }
                    }
                    if (!hasTool) {
                        for (const auto& reqId : rule.toolItems) {
                            auto it = _formIDCounts.find(reqId);
                            if (it != _formIDCounts.end() && it->second > 0) { hasTool = true; break; }
                        }
                    }
                }

                if (!hasTool) return true;
            }
            return false;
        }

        // Listens for inventory changes (items added/removed) to dynamically update the player's tool cache without needing to rescan the entire inventory
        virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override {
            if (!Settings::bEnableToolRequirements || !a_event) return RE::BSEventNotifyControl::kContinue;

            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return RE::BSEventNotifyControl::kContinue;

            bool isPlayerReceiver = (a_event->newContainer == player->GetFormID());
            bool isPlayerSender = (a_event->oldContainer == player->GetFormID());

            if (isPlayerReceiver || isPlayerSender) {
                auto form = RE::TESForm::LookupByID(a_event->baseObj);
                auto object = form ? form->As<RE::TESBoundObject>() : nullptr;
                if (object) {
                    std::unique_lock<std::shared_mutex> writeLock(_mutex);
                    int change = a_event->itemCount;
                    if (isPlayerSender && !isPlayerReceiver) change = -change;

                    UpdateItemCounts(object, change);
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        Manager() = default;
        std::vector<Rule> _rules;
        std::unordered_map<RE::BSFixedString, int, BSFixedStringHash> _keywordCounts;
        std::unordered_map<RE::FormID, int> _formIDCounts;
        mutable std::shared_mutex _mutex;

        // Safely increments or decrements the tracked amount of a specific item and its keywords
        void UpdateItemCounts(RE::TESBoundObject* obj, int countChange) {
            _formIDCounts[obj->GetFormID()] += countChange;
            if (_formIDCounts[obj->GetFormID()] < 0) _formIDCounts[obj->GetFormID()] = 0;

            auto keywordForm = obj->As<RE::BGSKeywordForm>();
            if (keywordForm && keywordForm->keywords) {
                for (uint32_t i = 0; i < keywordForm->numKeywords; ++i) {
                    if (keywordForm->keywords[i]) {
                        RE::BSFixedString keywordString = keywordForm->keywords[i]->formEditorID;
                        _keywordCounts[keywordString] += countChange;
                        if (_keywordCounts[keywordString] < 0) _keywordCounts[keywordString] = 0;
                    }
                }
            }
        }
    };
}