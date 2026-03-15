#pragma once

namespace Settings
{
    // general
    inline bool bEnableMod = true;

    // armor & shields
    inline bool bUnlootableArmor = true;
    inline bool bUnlootableArmorShield = true;
    inline bool bArmorWornOnly = true;

    // specific armor if bUnlootableArmor = false
    inline bool bUnlootableArmorHead = false;
    inline bool bUnlootableArmorChest = false;
    inline bool bUnlootableArmorArms = false;
    inline bool bUnlootableArmorLegs = false;

    // clothing
    inline bool bUnlootableClothing = false;
    inline bool bClothingWornOnly = true;

    // weapons
    inline bool bUnlootableWeapons = false;
    inline bool bWeaponsWornOnly = true;

    // pickpocket
    inline bool bIncludePickpocket = false;

    inline std::string Trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    inline void Load()
    {
        std::filesystem::path iniPath = "Data/SKSE/Plugins/HiddenLoot.ini";

        if (std::filesystem::exists(iniPath)) {
            std::ifstream file(iniPath);
            std::string line;

            while (std::getline(file, line)) {
                std::string trimmedLine = Trim(line);
                if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#' || trimmedLine[0] == '[') {
                    continue;
                }

                auto delimiterPos = trimmedLine.find('=');
                if (delimiterPos != std::string::npos) {
                    std::string key = Trim(trimmedLine.substr(0, delimiterPos));
                    std::string value = Trim(trimmedLine.substr(delimiterPos + 1));
                    auto commentPos = value.find_first_of("; #");
                    if (commentPos != std::string::npos) {
                        value = value.substr(0, commentPos);
                    }
                    value = Trim(value);
                    std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    bool isTrue = (value == "true" || value == "1");

                    if (key == "bEnableMod") bEnableMod = isTrue;
                    else if (key == "bUnlootableArmor") bUnlootableArmor = isTrue;
                    else if (key == "bArmorWornOnly") bArmorWornOnly = isTrue;
                    else if (key == "bUnlootableArmorHead") bUnlootableArmorHead = isTrue;
                    else if (key == "bUnlootableArmorChest") bUnlootableArmorChest = isTrue;
                    else if (key == "bUnlootableArmorArms") bUnlootableArmorArms = isTrue;
                    else if (key == "bUnlootableArmorLegs") bUnlootableArmorLegs = isTrue;
                    else if (key == "bUnlootableArmorShield") bUnlootableArmorShield = isTrue;
                    else if (key == "bUnlootableClothing") bUnlootableClothing = isTrue;
                    else if (key == "bClothingWornOnly") bClothingWornOnly = isTrue;
                    else if (key == "bUnlootableWeapons") bUnlootableWeapons = isTrue;
                    else if (key == "bWeaponsWornOnly") bWeaponsWornOnly = isTrue;
					else if (key == "bIncludePickpocket") bIncludePickpocket = isTrue;
                }
            }
        }
        else {
            std::ofstream file(iniPath);
            if (file.is_open()) {
                file << "[General]\n";
                file << "bEnableMod=true\n\n\n";


                file << "[Armor]\n";
                file << "bUnlootableArmor=true\n\n";

                file << "; Specific body slots only (Takes effect if bUnlootableArmor=false)\n";
                file << "bUnlootableArmorHead=false\t; Includes Head, Hair and Circlets\n";
                file << "bUnlootableArmorChest=false\t; Includes Body, Chest and Back\n";
                file << "bUnlootableArmorArms=false\t; Includes Hands, Arms, Forearms and Shoulder\n";
                file << "bUnlootableArmorLegs=false\t; Includes Feet, Leg, Calves and Pelvis\n\n";

                file << "; Hides shields\n";
                file << "bUnlootableArmorShield=true\n\n";

                file << "; If true, only hides the armor/shield types the NPC is currently wearing.\n";
                file << "; Same goes for every other WornOnly option.\n";
                file << "bArmorWornOnly=true\n\n\n";


                file << "[Clothing]\n";
                file << "bUnlootableClothing=false\n";
                file << "bClothingWornOnly=true\n\n\n";


                file << "[Weapons]\n";
                file << "bUnlootableWeapons=false\n";
                file << "bWeaponsWornOnly=true\n\n\n";


                file << "[Pickpocket]\n";
                file << "; If true, settings apply also while pickpocketing NPCs.\n";
                file << "bIncludePickpocket=false\n";
            }
            else {
                logs::error("Error during generating INI file!");
            }
        }
    }
}