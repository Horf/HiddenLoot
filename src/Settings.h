#pragma once

// ===== Default Library =====
#define WIN32_LEAN_AND_MEAN

#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <locale>
#include <fstream>
#include <ranges>
#include <cctype>
#include <windows.h>

// ===== SKSE =====
#include <SKSE/Logger.h>

// ===== RE (Game Types) =====
#include <RE/B/BGSKeyword.h>
#include <RE/B/BSFixedString.h>
#include <RE/B/BSCoreTypes.h>
#include <RE/T/TESDataHandler.h>

namespace Settings
{
    // General
    inline bool bEnableMod = true;
    inline bool bAlwaysShowEnchanted = false;
    inline float fHideChance = 100.0f;
    inline float fValueThresholdForLoot = 1000.0f;

    // Armor & Shields
    inline bool bUnlootableArmor = true;
    inline bool bUnlootableArmorShield = true;
    inline bool bArmorWornOnly = true;

    // Specific armor (takes effect if bUnlootableArmor = false)
    inline bool bUnlootableArmorHead = false;
    inline bool bUnlootableArmorChest = false;
    inline bool bUnlootableArmorArms = false;
    inline bool bUnlootableArmorLegs = false;

    // Clothing
    inline bool bUnlootableClothing = false;
    inline bool bClothingWornOnly = true;

    // Jewelry
    inline bool bUnlootableJewelry = false;
    inline bool bJewelryWornOnly = true;

    // Backpacks
    inline bool bUnlootableBackpacks = false;
    inline bool bBackpacksWornOnly = true;

    // Weapons
    inline bool bUnlootableWeapons = false;
    inline bool bWeaponsWornOnly = true;

    // Pickpocket
    inline bool bIncludePickpocket = false;

    // Keyword Filters (fixed)
    inline std::vector<RE::BGSKeyword*> uniqueKeywords;

    // User defined keyword blacklist
    inline std::string sHideKeywords = "";
    inline std::vector<RE::BSFixedString> hideKeywordsList;

    // Misc items defined blacklist(experimental)
    inline std::string sMiscHideKeywords = "";
    inline std::vector<RE::BSFixedString> miscHideKeywordsList;
    inline float fMiscHideChance = 100.0f;

    // Corpse Filters
    inline bool bApplyToPlayerKills = true;
    inline bool bApplyToNPCKills = true;
    inline bool bApplyToPreDead = true;

    // Dynamic compatibility flag
    inline bool bIgnoreHealthExtraData = false;

    // Excluded NPC inventories (Base FormIDs)
    inline std::vector<RE::FormID> excludedNPCBaseIDs = {
        // Gunjar (Tutorial - Unbound) - Prevents quest progression blocker
        0x0009B0AD
    };

    // Helper function to remove leading/trailing whitespace
    inline std::string Trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

	// Helper function to safely parse floats from INI values, with support for both dot and comma as decimal separators
    inline float ParseFloatSafe(const std::string& val, float fallback) {
        std::string cleanVal = val;
        std::replace(cleanVal.begin(), cleanVal.end(), ',', '.');
        std::istringstream stream(cleanVal);
        stream.imbue(std::locale::classic());
        float result = fallback;
        stream >> result;
        if (stream.fail()) {
            return fallback;
        }
        return result;
    }

    inline void Save();

    inline void LoadINI()
    {
        std::filesystem::path iniPath = "Data/SKSE/Plugins/HiddenLoot.ini";
        int keysFound = 0;

        if (std::filesystem::exists(iniPath)) {
            std::ifstream file(iniPath);
            std::string line;

            while (std::getline(file, line)) {
                std::string trimmedLine = Trim(line);
                // Ignore empty lines and comments
                if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#' || trimmedLine[0] == '[') {
                    continue;
                }

                auto delimiterPos = trimmedLine.find('=');
                if (delimiterPos != std::string::npos) {
                    std::string key = Trim(trimmedLine.substr(0, delimiterPos));
                    std::string value = Trim(trimmedLine.substr(delimiterPos + 1));
                    
                    // Strip inline comments
                    auto commentPos = value.find_first_of(";#");
                    if (commentPos != std::string::npos) {
                        value = value.substr(0, commentPos);
                    }
                    value = Trim(value);
                    std::string originalValue = value;
                    if (key != "sHideKeywords") std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    bool isTrue = (value == "true" || value == "1");
                    bool keyMatched = true;

                    if (key == "bEnableMod") bEnableMod = isTrue;
                    else if (key == "bAlwaysShowEnchanted") bAlwaysShowEnchanted = isTrue;
                    else if (key == "fValueThresholdForLoot") {
                        fValueThresholdForLoot = ParseFloatSafe(value, 1000.0f);
                    }
                    else if (key == "fHideChance") {
                        fHideChance = std::clamp(ParseFloatSafe(value, 100.0f), 0.0f, 100.0f);
                    }
                    else if (key == "bUnlootableArmor") bUnlootableArmor = isTrue;
                    else if (key == "bArmorWornOnly") bArmorWornOnly = isTrue;
                    else if (key == "bUnlootableArmorHead") bUnlootableArmorHead = isTrue;
                    else if (key == "bUnlootableArmorChest") bUnlootableArmorChest = isTrue;
                    else if (key == "bUnlootableArmorArms") bUnlootableArmorArms = isTrue;
                    else if (key == "bUnlootableArmorLegs") bUnlootableArmorLegs = isTrue;
                    else if (key == "bUnlootableArmorShield") bUnlootableArmorShield = isTrue;
                    else if (key == "bUnlootableClothing") bUnlootableClothing = isTrue;
                    else if (key == "bClothingWornOnly") bClothingWornOnly = isTrue;
                    else if (key == "bUnlootableJewelry") bUnlootableJewelry = isTrue;
                    else if (key == "bJewelryWornOnly") bJewelryWornOnly = isTrue;
                    else if (key == "bUnlootableBackpacks") bUnlootableBackpacks = isTrue;
                    else if (key == "bBackpacksWornOnly") bBackpacksWornOnly = isTrue;
                    else if (key == "bUnlootableWeapons") bUnlootableWeapons = isTrue;
                    else if (key == "bWeaponsWornOnly") bWeaponsWornOnly = isTrue;
					else if (key == "bIncludePickpocket") bIncludePickpocket = isTrue;
                    else if (key == "bApplyToPlayerKills") bApplyToPlayerKills = isTrue;
                    else if (key == "bApplyToNPCKills") bApplyToNPCKills = isTrue;
                    else if (key == "bApplyToPreDead") bApplyToPreDead = isTrue;
                    else if (key == "sHideKeywords") sHideKeywords = originalValue;
                    else if (key == "sMiscHideKeywords") sMiscHideKeywords = originalValue;
                    else if (key == "fMiscHideChance") {
                        fMiscHideChance = std::clamp(ParseFloatSafe(value, 100.0f), 0.0f, 100.0f);
                    }
                    else keyMatched = false;
                    if (keyMatched) keysFound++;
                }
            }
			file.close();
        }
        if (!std::filesystem::exists(iniPath) || keysFound < 26) Save();
    }

	// Helper fuction to process comma-separated keyword strings into lists, with safety checks against essential keywords
    inline void ProcessKeywords(std::vector<RE::BSFixedString> &keywordList, std::string &keywordString) {
        keywordList.clear();
        if (!keywordString.empty()) {
            std::stringstream ss(keywordString);
            std::string token;
            while (std::getline(ss, token, ',')) {
                token = Trim(token);
                // Lookup the keyword by EditorID across all loaded plugins,
                // using strings so dynamic keywords can be used as well
                if (!token.empty()) {
                    bool isSafe = true;
                    for (auto* uniqueKw : uniqueKeywords) {
                        // Essential keywords (Quest items, Artifacts) are hardcoded for protection
                        // This prevents players from accidentally bricking their game by blacklisting items required for progression
                        const char* edid = uniqueKw->GetFormEditorID();
                        if (edid && std::string(edid) == token) {
                            logs::info("Safety Override: Prevented use of whitelisted essential keyword '{}' in blacklist.", token);
                            isSafe = false;
                            break;
                        }
                    }

                    if (isSafe) keywordList.push_back(token);
                }
            }
        }
    }

    inline void LoadGameData() {
        // Cache unique keywords using FormIDs in Vanilla Skyrim.
        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (dataHandler) {
            uniqueKeywords.clear();
            // Keywords: VendorNoSale, MagicDisallowEnchanting, DaedricArtifact, MQ201ThalmorDisguise
            std::vector<RE::FormID> ids = { 0xA8668, 0x10F5E2, 0xFF9FB, 0xC27BD };
            for (RE::FormID id : ids) {
                // Ensure the forms are specifically pulled from Skyrim.esm
                auto kw = dataHandler->LookupForm<RE::BGSKeyword>(id, "Skyrim.esm");
                if (kw) uniqueKeywords.push_back(kw);
            }
        }

        // Process user-defined hide keywords
        ProcessKeywords(hideKeywordsList, sHideKeywords);

        // Process misc hide keywords (experimental)
        ProcessKeywords(miscHideKeywordsList, sMiscHideKeywords);

		// Check for known durability SKSE plugins and set compatibility flag
        if (GetModuleHandleA("EquipmentDurabilitySystem-NG.dll") ||
            GetModuleHandleA("EquipmentDurability.dll") ||
            GetModuleHandleA("ItemDurability.dll")) {
            bIgnoreHealthExtraData = true;
            logs::info("Durability SKSE plugin detected! Disabling 'kHealth' protection to ensure hiding rules function correctly.");
        }
    }

    inline void Save()
    {
        std::filesystem::path iniPath = "Data/SKSE/Plugins/HiddenLoot.ini";
        std::ofstream file(iniPath);
        if (file.is_open()) {
            file << "[General]\n";
            file << "bEnableMod=" << (bEnableMod ? "true" : "false") << "\n\n";

            file << "; If true, magically enchanted items will always be lootable.\n";
            file << "bAlwaysShowEnchanted=" << (bAlwaysShowEnchanted ? "true" : "false") << "\n\n";
            
            file << "; Chance in percent (0.0 to 100.0) that an item gets hidden.\n";
            file << "fHideChance=" << fHideChance << "\n\n";

            file << "; Items with a gold value equal to or higher than this threshold will always be lootable.\n";
            file << "fValueThresholdForLoot=" << fValueThresholdForLoot << "\n\n\n";


            file << "[CorpseFilters]\n";
            file << "; Choose which corpses the hiding rules apply to.\n";
            file << "bApplyToPlayerKills=" << (bApplyToPlayerKills ? "true" : "false") << "\t; Killed by Player, Followers or Summons\n";
            file << "bApplyToNPCKills=" << (bApplyToNPCKills ? "true" : "false") << "\t\t; Killed by other NPCs, Creatures, etc\n";
            file << "bApplyToPreDead=" << (bApplyToPreDead ? "true" : "false") << "\t\t; Corpses that were already dead when you found them\n\n\n";


            file << "[Keywords]\n";
            file << "; Comma-separated list of EditorIDs for keywords that should ALWAYS hide an item.\n";
            file << "; Example: sHideKeywords=IsJunk, FollowerArrowKeyword\n";
            file << "sHideKeywords=" << sHideKeywords << "\n\n\n";


            file << "[Armor]\n";
            file << "bUnlootableArmor=" << (bUnlootableArmor ? "true" : "false") << "\n\n";
            
            file << "; Specific body slots only (Takes effect if bUnlootableArmor=false)\n";
            file << "bUnlootableArmorHead=" << (bUnlootableArmorHead ? "true" : "false") << "\t; Includes Head, Hair and Circlets\n";
            file << "bUnlootableArmorChest=" << (bUnlootableArmorChest ? "true" : "false") << "\t; Includes Body and Chest\n";
            file << "bUnlootableArmorArms=" << (bUnlootableArmorArms ? "true" : "false") << "\t; Includes Hands, Arms, Forearms and Shoulder\n";
            file << "bUnlootableArmorLegs=" << (bUnlootableArmorLegs ? "true" : "false") << "\t; Includes Feet, Leg, Calves and Pelvis\n\n";

            file << "; Hides shields\n";
            file << "bUnlootableArmorShield=" << (bUnlootableArmorShield ? "true" : "false") << "\n\n";

            file << "; If true, only hides the armor/shield types the NPC is currently wearing.\n";
            file << "; Same goes for every other WornOnly option.\n";
            file << "bArmorWornOnly=" << (bArmorWornOnly ? "true" : "false") << "\n\n\n";


            file << "[Clothing]\n";
            file << "bUnlootableClothing=" << (bUnlootableClothing ? "true" : "false") << "\n";
            file << "bClothingWornOnly=" << (bClothingWornOnly ? "true" : "false") << "\n\n\n";


            file << "[Jewelry]\n";
            file << "bUnlootableJewelry=" << (bUnlootableJewelry ? "true" : "false") << "\n";
            file << "bJewelryWornOnly=" << (bJewelryWornOnly ? "true" : "false") << "\n\n\n";


            file << "[Backpacks]\n";
            file << "bUnlootableBackpacks=" << (bUnlootableBackpacks ? "true" : "false") << "\n";
            file << "bBackpacksWornOnly=" << (bBackpacksWornOnly ? "true" : "false") << "\n\n\n";


            file << "[Weapons]\n";
            file << "bUnlootableWeapons=" << (bUnlootableWeapons ? "true" : "false") << "\n";
            file << "bWeaponsWornOnly=" << (bWeaponsWornOnly ? "true" : "false") << "\n\n\n";


            file << "[Pickpocket]\n";
            file << "; If true, settings apply also while pickpocketing NPCs.\n";
            file << "bIncludePickpocket=" << (bIncludePickpocket ? "true" : "false") << "\n\n\n";


            file << "[Clutter]\n";
            file << "; Comma-separated list of EditorIDs for clutter items you want to hide (e.g., VendorItemClutter).\n";
			file << "; Applies to MISC, ALCH, SCRL and BOOK item types.\n";
            file << "; Note: Quest items, gold, lockpicks and gems are hardcoded to never be hidden.\n";
            file << "sMiscHideKeywords=" << sMiscHideKeywords << "\n\n";

            file << "; Chance in percent (0.0 to 100.0) that a blacklisted misc item gets hidden.\n";
            file << "fMiscHideChance=" << fMiscHideChance << "\n";
        }
    }
}