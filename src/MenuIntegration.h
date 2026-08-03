#pragma once

// ===== Default Library =====
#include <string.h>

// ===== SKSE =====
#include <SKSE/Logger.h>

// ===== Project =====
#include "SKSEMenuFramework.h"
#include "Settings.h"

namespace MenuIntegration
{
    inline void HelpMarker(const char* desc)
    {
        ImGuiMCP::SameLine();
        ImGuiMCP::TextDisabled("(?)");
        if (ImGuiMCP::IsItemHovered(ImGuiMCP::ImGuiHoveredFlags_DelayNormal))
        {
            ImGuiMCP::BeginTooltip();
            ImGuiMCP::PushTextWrapPos(ImGuiMCP::GetFontSize() * 35.0f);
            ImGuiMCP::TextUnformatted(desc);
            ImGuiMCP::PopTextWrapPos();
            ImGuiMCP::EndTooltip();
        }
    }

    inline void __stdcall RenderMenu()
    {
        bool changed = false;

        ImGuiMCP::SeparatorText("General");

        if (ImGuiMCPComponents::ToggleButton("Enable Mod", &Settings::bEnableMod)) changed = true;

        if (ImGuiMCPComponents::ToggleButton("Always Show Enchanted", &Settings::bAlwaysShowEnchanted)) changed = true;
        HelpMarker("If enabled, magically enchanted items are never hidden.");

        ImGuiMCP::SliderFloat("Hide Chance (%)", &Settings::fHideChance, 0.0f, 100.0f, "%.1f");
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker("Percentage chance that an item will be hidden. Lower values leave more random loot on bodies. 100% hides everything matching your rules.");

        ImGuiMCP::DragFloat("Value Threshold", &Settings::fValueThresholdForLoot, 10.0f, 0.0f, 100000.0f, "%.0f", 0);
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker("Items with a gold value equal to or higher than this threshold will always be lootable.");

        ImGuiMCP::DragFloat("Value/Weight Threshold", &Settings::fValueWeightThresholdForLoot, 1.0f, 0.0f, 10000.0f, "%.1f", 0);
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker("Items with a Gold/Weight ratio equal to or higher than this will always be lootable. Set to 0 to disable.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Advanced Hide Chances");
        if (ImGuiMCPComponents::ToggleButton("Use Category Specific Chances", &Settings::bUseCategoryHideChances)) changed = true;
        HelpMarker("If enabled, the specific chances below will override the global Hide Chance.");

        if (Settings::bUseCategoryHideChances) {
            ImGuiMCP::Indent(15.0f);
            ImGuiMCP::SliderFloat("Armor & Shields (%)", &Settings::fHideChanceArmor, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            ImGuiMCP::SliderFloat("Weapons & Ammo (%)", &Settings::fHideChanceWeapons, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            ImGuiMCP::SliderFloat("Clothing & Jewelry (%)", &Settings::fHideChanceClothing, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            ImGuiMCP::Unindent(15.0f);
        }

        if (ImGuiMCPComponents::ToggleButton("Enable Skill Scaling", &Settings::bEnableSkillScaling)) changed = true;
        HelpMarker("Dynamically reduces the hide chance based on the player's skill associated with the item (e.g. Heavy Armor skill protects heavy armors).");

        if (Settings::bEnableSkillScaling) {
            ImGuiMCP::Indent(15.0f);
            ImGuiMCP::SliderFloat("Max Skill Reduction (%)", &Settings::fMaxSkillHideReduction, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            HelpMarker("The maximum percentage the hide chance is reduced when the corresponding skill is at level 100.");
            ImGuiMCP::Unindent(15.0f);
        }

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Hotkey Toggle");
        if (ImGuiMCPComponents::ToggleButton("Enable Hotkey", &Settings::bEnableHotkey)) changed = true;
        HelpMarker("Allows you to toggle the mod on/off in-game using a key combination. (Re-open the loot menu or look away and back at the corpse to refresh it).");

        if (Settings::bEnableHotkey) {
            ImGuiMCP::Indent(15.0f);
            if (ImGuiMCP::InputInt("Toggle Key (DXScanCode)", &Settings::iToggleHotkey)) changed = true;
            HelpMarker("DXScanCode for the main key. Default is 45 (X).");
            if (ImGuiMCP::InputInt("Modifier Key (DXScanCode)", &Settings::iToggleModifierKey)) changed = true;
            HelpMarker("DXScanCode for the modifier key. Default is 56 (Left Alt). Set to 0 to require no modifier key.");
            ImGuiMCP::Unindent(15.0f);
        }

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Corpse Filters");
        if (ImGuiMCPComponents::ToggleButton("Apply to Player Kills", &Settings::bApplyToPlayerKills)) changed = true;
        HelpMarker("If enabled, loot hiding rules apply to enemies killed by you, your followers or summons.");
        if (ImGuiMCPComponents::ToggleButton("Apply to NPC Kills", &Settings::bApplyToNPCKills)) changed = true;
        HelpMarker("If enabled, loot hiding rules apply to NPCs killed by other NPCs (e.g. World Events, Faction Wars). Disable this if you want free loot from battles you weren't involved in.");
        if (ImGuiMCPComponents::ToggleButton("Apply to Pre-Dead Corpses", &Settings::bApplyToPreDead)) changed = true;
        HelpMarker("If enabled, loot hiding rules apply to corpses that were already dead when you found them (Decoration/Quest corpses).");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Compatibility & Safety Nets");
        if (ImGuiMCPComponents::ToggleButton("Protect Player-Modified Gear", &Settings::bProtectPlayerModifiedGear)) changed = true;
        HelpMarker("If enabled, items that have been tempered, custom enchanted, or renamed by the player are permanently protected and will never be hidden. Disable this if mods like 'Vibrant Weapons' cause the hiding system to fail.");
        if (ImGuiMCPComponents::ToggleButton("Ignore Health/Durability Data", &Settings::bIgnoreHealthExtraData)) changed = true;
        HelpMarker("Enable this manually if you use a Durability/Degradation mod that isn't automatically detected. It prevents the mod from thinking all degraded items are tempered player-gear.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Blacklist");
        static char keywordBuffer[256];
        if (keywordBuffer[0] == '\0' && !Settings::sHideKeywords.empty()) {
            strncpy_s(keywordBuffer, Settings::sHideKeywords.c_str(), sizeof(keywordBuffer) - 1);
        }
        if (ImGuiMCP::InputText("Blacklisted Keywords & Mods", keywordBuffer, sizeof(keywordBuffer))) {
            Settings::sHideKeywords = keywordBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker("Comma-separated list of EditorID keywords or mod filenames (e.g., IsJunk, MyMod.esp). Items with these keywords or from these mods will ALWAYS be hidden. Case-sensitive!");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("NPC Whitelist");
        static char npcBuffer[256];
        if (npcBuffer[0] == '\0' && !Settings::sExcludedNPCs.empty()) {
            strncpy_s(npcBuffer, Settings::sExcludedNPCs.c_str(), sizeof(npcBuffer) - 1);
        }
        if (ImGuiMCP::InputText("Excluded NPCs", npcBuffer, sizeof(npcBuffer))) {
            Settings::sExcludedNPCs = npcBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker("Comma-separated list of NPC EditorIDs (e.g., Ulfric, Tullius). Items on these specific NPCs will NEVER be hidden. Case-sensitive! Gunjar is permanently protected.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Whitelist");
        static char itemBuffer[256];
        if (itemBuffer[0] == '\0' && !Settings::sWhitelistedItems.empty()) {
            strncpy_s(itemBuffer, Settings::sWhitelistedItems.c_str(), sizeof(itemBuffer) - 1);
        }
        if (ImGuiMCP::InputText("Whitelisted Items & Mods", itemBuffer, sizeof(itemBuffer))) {
            Settings::sWhitelistedItems = itemBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker("Comma-separated list of Item EditorIDs or mod filenames (e.g., IronSword, MyMod.esp). These items will NEVER be hidden, bypassing all rules. Case-sensitive!");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Armor & Shields");
        if (ImGuiMCPComponents::ToggleButton("Hide Armor", &Settings::bUnlootableArmor)) changed = true;
        HelpMarker("If disabled you can define bodyslot options (head, chest, arms and legs) seperately.");

        if (!Settings::bUnlootableArmor) {
            ImGuiMCP::Indent(15.0f);
            if (ImGuiMCPComponents::ToggleButton("Hide Head", &Settings::bUnlootableArmorHead)) changed = true;
            HelpMarker("Includes Head, Hair and Circlets.");
            if (ImGuiMCPComponents::ToggleButton("Hide Chest", &Settings::bUnlootableArmorChest)) changed = true;
            HelpMarker("Includes Body and Chest.");
            if (ImGuiMCPComponents::ToggleButton("Hide Arms", &Settings::bUnlootableArmorArms)) changed = true;
            HelpMarker("Includes Hands, Arms, Forearms and Shoulder.");
            if (ImGuiMCPComponents::ToggleButton("Hide Legs", &Settings::bUnlootableArmorLegs)) changed = true;
            HelpMarker("Includes Feet, Leg, Calves and Pelvis.");
            ImGuiMCP::Unindent(15.0f);
        }

        if (ImGuiMCPComponents::ToggleButton("Hide Shields", &Settings::bUnlootableArmorShield)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Armor", &Settings::bArmorWornOnly)) changed = true;
        HelpMarker("If enabled, only hides the armor/shield types the NPC is currently wearing.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Clothing");
        if (ImGuiMCPComponents::ToggleButton("Hide Clothing", &Settings::bUnlootableClothing)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Clothing", &Settings::bClothingWornOnly)) changed = true;
        HelpMarker("If enabled, only hides the clothing types the NPC is currently wearing.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Jewelry");
        if (ImGuiMCPComponents::ToggleButton("Hide Jewelry", &Settings::bUnlootableJewelry)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Jewelry", &Settings::bJewelryWornOnly)) changed = true;
        HelpMarker("If enabled, only hides the jewelry types the NPC is currently wearing.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Backpacks");
        if (ImGuiMCPComponents::ToggleButton("Hide Backpacks", &Settings::bUnlootableBackpacks)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Backpacks", &Settings::bBackpacksWornOnly)) changed = true;
        HelpMarker("If enabled, only hides backpacks the NPC is currently wearing.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Weapons");
        if (ImGuiMCPComponents::ToggleButton("Hide Weapons", &Settings::bUnlootableWeapons)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Weapons", &Settings::bWeaponsWornOnly)) changed = true;
        HelpMarker("If enabled, only hides the weapon types the NPC is currently wearing.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Ammunition");
        if (ImGuiMCPComponents::ToggleButton("Hide Ammunition", &Settings::bUnlootableAmmo)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Equipped Ammo", &Settings::bAmmoWornOnly)) changed = true;
        HelpMarker("If enabled, only hides the ammunition the NPC currently has equipped (e.g. arrows in quiver).");
        
        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Pickpocket");
        if (ImGuiMCPComponents::ToggleButton("Apply to Pickpocketing", &Settings::bIncludePickpocket)) changed = true;
        HelpMarker("If enabled, settings apply also while pickpocketing NPCs.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Clutter Items (Misc, Food, Books)");
        static char miscKeywordBuffer[256];
        if (miscKeywordBuffer[0] == '\0' && !Settings::sMiscHideKeywords.empty()) {
            strncpy_s(miscKeywordBuffer, Settings::sMiscHideKeywords.c_str(), sizeof(miscKeywordBuffer) - 1);
        }
        if (ImGuiMCP::InputText("Clutter Item Blacklist", miscKeywordBuffer, sizeof(miscKeywordBuffer))) {
            Settings::sMiscHideKeywords = miscKeywordBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker("Comma-separated list of EditorID keywords (e.g., VendorItemClutter). Applies to MISC, ALCH, SCRL and BOOK. Quest items, gold, lockpicks, and gems are always protected.");

        ImGuiMCP::SliderFloat("Clutter Hide Chance (%)", &Settings::fHideChanceMisc, 0.0f, 100.0f, "%.1f");
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker("Percentage chance that a blacklisted clutter item will be hidden. Completely separate from the global hide chance.");

        if (changed) Settings::Save();
    }

    inline void Install()
    {
        if (SKSEMenuFramework::IsInstalled()) {
            SKSEMenuFramework::SetSection("Hidden Loot");
            SKSEMenuFramework::AddSectionItem("Settings", RenderMenu);
            logs::info("SKSE Menu Framework detected. In-game menu registered.");
        }
        else {
            logs::info("SKSE Menu Framework not found. Skipping in-game menu integration.");
        }
    }
}