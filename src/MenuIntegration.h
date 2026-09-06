#pragma once

// ===== Default Library =====
#include <string.h>

// ===== SKSE =====
#include <SKSE/Logger.h>

// ===== Project =====
#include "SKSEMenuFramework.h"
#include "Settings.h"
#include "Translation.h"

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

        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_General", "General").c_str());

        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_EnableMod", "Enable Mod").c_str(), &Settings::bEnableMod)) changed = true;

        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_EnableToolReq", "Enable Tool Requirements").c_str(), &Settings::bEnableToolRequirements)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_EnableToolReq", "If enabled, requires specific tools in your inventory to loot certain items (e.g., a Dagger to harvest animal hides). Configure rules in 'ToolRequiredLoot.json'.").c_str());

        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_AlwaysEnchanted", "Always Show Enchanted").c_str(), &Settings::bAlwaysShowEnchanted)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_AlwaysEnchanted", "If enabled, magically enchanted items are never hidden.").c_str());

        ImGuiMCP::SliderFloat(Translation::GetImGui("$HL_Opt_HideChance", "Hide Chance (%)").c_str(), &Settings::fHideChance, 0.0f, 100.0f, "%.1f");
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_HideChance", "Percentage chance that an item will be hidden. Lower values leave more random loot on bodies. 100% hides everything matching your rules.").c_str());

        ImGuiMCP::DragFloat(Translation::GetImGui("$HL_Opt_ValThresh", "Value Threshold").c_str(), &Settings::fValueThresholdForLoot, 10.0f, 0.0f, 100000.0f, "%.0f", 0);
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_ValThresh", "Items with a gold value equal to or higher than this threshold will always be lootable.").c_str());

        ImGuiMCP::DragFloat(Translation::GetImGui("$HL_Opt_ValWeightThresh", "Value/Weight Threshold").c_str(), &Settings::fValueWeightThresholdForLoot, 1.0f, 0.0f, 10000.0f, "%.1f", 0);
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_ValWeightThresh", "Items with a Gold/Weight ratio equal to or higher than this will always be lootable. Set to 0 to disable.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_AdvHide", "Advanced Hide Chances").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_UseCatChances", "Use Category Specific Chances").c_str(), &Settings::bUseCategoryHideChances)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_UseCatChances", "If enabled, the specific chances below will override the global Hide Chance.").c_str());

        if (Settings::bUseCategoryHideChances) {
            ImGuiMCP::Indent(15.0f);
            ImGuiMCP::SliderFloat(Translation::GetImGui("$HL_Opt_HC_Armor", "Armor & Shields (%)").c_str(), &Settings::fHideChanceArmor, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            ImGuiMCP::SliderFloat(Translation::GetImGui("$HL_Opt_HC_Weap", "Weapons & Ammo (%)").c_str(), &Settings::fHideChanceWeapons, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            ImGuiMCP::SliderFloat(Translation::GetImGui("$HL_Opt_HC_Cloth", "Clothing & Jewelry (%)").c_str(), &Settings::fHideChanceClothing, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            ImGuiMCP::Unindent(15.0f);
        }

        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_SkillScale", "Enable Skill Scaling").c_str(), &Settings::bEnableSkillScaling)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_SkillScale", "Dynamically reduces the hide chance based on the player's skill associated with the item (e.g. Heavy Armor skill protects heavy armors).").c_str());

        if (Settings::bEnableSkillScaling) {
            ImGuiMCP::Indent(15.0f);
            ImGuiMCP::SliderFloat(Translation::GetImGui("$HL_Opt_MaxSkillRed", "Max Skill Reduction (%)").c_str(), &Settings::fMaxSkillHideReduction, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_MaxSkillRed", "The maximum percentage the hide chance is reduced when the corresponding skill is at level 100.").c_str());

            ImGuiMCP::SliderFloat(Translation::GetImGui("$HL_Opt_MaxSmithRed", "Max Smithing Reduction (%)").c_str(), &Settings::fMaxSmithingHideReduction, 0.0f, 100.0f, "%.1f");
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_MaxSmithRed", "The maximum percentage the hide chance is reduced when the Smithing skill is at level 100. This stacks with the combat skill reduction but only applies to weapons, armors and shields.").c_str());
            ImGuiMCP::Unindent(15.0f);
        }

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Hotkey", "Hotkey Toggle").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_EnableHotkey", "Enable Hotkey").c_str(), &Settings::bEnableHotkey)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_EnableHotkey", "Allows you to toggle the mod on/off in-game using a key combination. (Re-open the loot menu or look away and back at the corpse to refresh it).").c_str());

        if (Settings::bEnableHotkey) {
            ImGuiMCP::Indent(15.0f);
            if (ImGuiMCP::InputInt(Translation::GetImGui("$HL_Opt_KeyTog", "Toggle Key (DXScanCode)").c_str(), &Settings::iToggleHotkey)) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_KeyTog", "DXScanCode for the main key. Default is 45 (X).").c_str());
            if (ImGuiMCP::InputInt(Translation::GetImGui("$HL_Opt_KeyMod", "Modifier Key (DXScanCode)").c_str(), &Settings::iToggleModifierKey)) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_KeyMod", "DXScanCode for the modifier key. Default is 56 (Left Alt). Set to 0 to require no modifier key.").c_str());
            ImGuiMCP::Unindent(15.0f);
        }

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Filters", "Corpse Filters").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_F_Player", "Apply to Player Kills").c_str(), &Settings::bApplyToPlayerKills)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_F_Player", "If enabled, loot hiding rules apply to enemies killed by you, your followers or summons.").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_F_NPC", "Apply to NPC Kills").c_str(), &Settings::bApplyToNPCKills)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_F_NPC", "If enabled, loot hiding rules apply to NPCs killed by other NPCs (e.g. World Events, Faction Wars). Disable this if you want free loot from battles you weren't involved in.").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_F_PreDead", "Apply to Pre-Dead Corpses").c_str(), &Settings::bApplyToPreDead)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_F_PreDead", "If enabled, loot hiding rules apply to corpses that were already dead when you found them (Decoration/Quest corpses).").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Integration", "Mod Integration").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_JunkIt", "Hide 'Junk It' Items").c_str(), &Settings::bHideJunkItItems)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_JunkIt", "If enabled, automatically hides any items you have marked as junk. (Requires Junk It version 2.0.8 or newer).").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Safety", "Compatibility & Safety Nets").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_ProtPlayer", "Protect Player-Modified Gear").c_str(), &Settings::bProtectPlayerModifiedGear)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_ProtPlayer", "If enabled, items that have been tempered, custom enchanted, or renamed by the player are permanently protected and will never be hidden. Disable this if mods like 'Vibrant Weapons' cause the hiding system to fail.").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_IgnHealth", "Ignore Health/Durability Data").c_str(), &Settings::bIgnoreHealthExtraData)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_IgnHealth", "Enable this manually if you use a Durability/Degradation mod that isn't automatically detected. It prevents the mod from thinking all degraded items are tempered player-gear.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Blacklist", "Blacklist").c_str());
        static char keywordBuffer[256];
        if (keywordBuffer[0] == '\0' && !Settings::sHideKeywords.empty()) {
            strncpy_s(keywordBuffer, Settings::sHideKeywords.c_str(), sizeof(keywordBuffer) - 1);
        }
        if (ImGuiMCP::InputText(Translation::GetImGui("$HL_Opt_BL_Keys", "Blacklisted Keywords & Mods").c_str(), keywordBuffer, sizeof(keywordBuffer))) {
            Settings::sHideKeywords = keywordBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker(Translation::Get("$HL_Desc_BL_Keys", "Comma-separated list of EditorID keywords or mod filenames (e.g., WeapMaterialIron, MyMod.esp). Items with these keywords or from these mods will ALWAYS be hidden. Case-sensitive!").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_NPCWL", "NPC Whitelist").c_str());
        static char npcBuffer[256];
        if (npcBuffer[0] == '\0' && !Settings::sExcludedNPCs.empty()) {
            strncpy_s(npcBuffer, Settings::sExcludedNPCs.c_str(), sizeof(npcBuffer) - 1);
        }
        if (ImGuiMCP::InputText(Translation::GetImGui("$HL_Opt_WL_NPC", "Excluded NPCs").c_str(), npcBuffer, sizeof(npcBuffer))) {
            Settings::sExcludedNPCs = npcBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker(Translation::Get("$HL_Desc_WL_NPC", "Comma-separated list of NPC EditorIDs (e.g., Ulfric, Tullius). Items on these specific NPCs will NEVER be hidden. Case-sensitive! Gunjar is permanently protected.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_ItemWL", "Whitelist").c_str());
        static char itemBuffer[256];
        if (itemBuffer[0] == '\0' && !Settings::sWhitelistedItems.empty()) {
            strncpy_s(itemBuffer, Settings::sWhitelistedItems.c_str(), sizeof(itemBuffer) - 1);
        }
        if (ImGuiMCP::InputText(Translation::GetImGui("$HL_Opt_WL_Item", "Whitelisted Items & Mods").c_str(), itemBuffer, sizeof(itemBuffer))) {
            Settings::sWhitelistedItems = itemBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker(Translation::Get("$HL_Desc_WL_Item", "Comma-separated list of Item EditorIDs or mod filenames (e.g., IronSword, MyMod.esp). These items will NEVER be hidden, bypassing all rules. Case-sensitive!").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Armor", "Armor & Shields").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideArmor", "Hide Armor").c_str(), &Settings::bUnlootableArmor)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_HideArmor", "If disabled you can define bodyslot options (head, chest, arms and legs) seperately.").c_str());

        if (!Settings::bUnlootableArmor) {
            ImGuiMCP::Indent(15.0f);
            if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideHead", "Hide Head").c_str(), &Settings::bUnlootableArmorHead)) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_HideHead", "Includes Head, Hair and Circlets.").c_str());
            if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideChest", "Hide Chest").c_str(), &Settings::bUnlootableArmorChest)) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_HideChest", "Includes Body and Chest.").c_str());
            if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideArms", "Hide Arms").c_str(), &Settings::bUnlootableArmorArms)) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_HideArms", "Includes Hands, Arms, Forearms and Shoulder.").c_str());
            if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideLegs", "Hide Legs").c_str(), &Settings::bUnlootableArmorLegs)) changed = true;
            HelpMarker(Translation::Get("$HL_Desc_HideLegs", "Includes Feet, Leg, Calves and Pelvis.").c_str());
            ImGuiMCP::Unindent(15.0f);
        }

        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideShield", "Hide Shields").c_str(), &Settings::bUnlootableArmorShield)) changed = true;
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_ArmorWorn", "Only Hide Worn Armor").c_str(), &Settings::bArmorWornOnly)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_ArmorWorn", "If enabled, only hides the armor/shield types the NPC is currently wearing.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Clothing", "Clothing").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideCloth", "Hide Clothing").c_str(), &Settings::bUnlootableClothing)) changed = true;
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_ClothWorn", "Only Hide Worn Clothing").c_str(), &Settings::bClothingWornOnly)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_ClothWorn", "If enabled, only hides the clothing types the NPC is currently wearing.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Jewelry", "Jewelry").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideJewelry", "Hide Jewelry").c_str(), &Settings::bUnlootableJewelry)) changed = true;
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_JewelryWorn", "Only Hide Worn Jewelry").c_str(), &Settings::bJewelryWornOnly)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_JewelryWorn", "If enabled, only hides the jewelry types the NPC is currently wearing.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Backpacks", "Backpacks").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideBackpack", "Hide Backpacks").c_str(), &Settings::bUnlootableBackpacks)) changed = true;
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_BackpackWorn", "Only Hide Worn Backpacks").c_str(), &Settings::bBackpacksWornOnly)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_BackpackWorn", "If enabled, only hides backpacks the NPC is currently wearing.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Weapons", "Weapons").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideWeap", "Hide Weapons").c_str(), &Settings::bUnlootableWeapons)) changed = true;
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_WeapWorn", "Only Hide Worn Weapons").c_str(), &Settings::bWeaponsWornOnly)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_WeapWorn", "If enabled, only hides the weapon types the NPC is currently wearing.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Ammo", "Ammunition").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_HideAmmo", "Hide Ammunition").c_str(), &Settings::bUnlootableAmmo)) changed = true;
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_AmmoWorn", "Only Hide Equipped Ammo").c_str(), &Settings::bAmmoWornOnly)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_AmmoWorn", "If enabled, only hides the ammunition the NPC currently has equipped (e.g. arrows in quiver).").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Pickpocket", "Pickpocket").c_str());
        if (ImGuiMCPComponents::ToggleButton(Translation::GetImGui("$HL_Opt_Pickpocket", "Apply to Pickpocketing").c_str(), &Settings::bIncludePickpocket)) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_Pickpocket", "If enabled, settings apply also while pickpocketing NPCs.").c_str());

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText(Translation::Get("$HL_Sec_Clutter", "Clutter Items (Misc, Food, Books)").c_str());
        static char miscKeywordBuffer[256];
        if (miscKeywordBuffer[0] == '\0' && !Settings::sMiscHideKeywords.empty()) {
            strncpy_s(miscKeywordBuffer, Settings::sMiscHideKeywords.c_str(), sizeof(miscKeywordBuffer) - 1);
        }
        if (ImGuiMCP::InputText(Translation::GetImGui("$HL_Opt_ClutterBL", "Clutter Item Blacklist").c_str(), miscKeywordBuffer, sizeof(miscKeywordBuffer))) {
            Settings::sMiscHideKeywords = miscKeywordBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker(Translation::Get("$HL_Desc_ClutterBL", "Comma-separated list of EditorID keywords (e.g., VendorItemClutter). Applies to MISC, ALCH, SCRL and BOOK. Quest items, gold, lockpicks, and gems are always protected.").c_str());

        ImGuiMCP::SliderFloat(Translation::GetImGui("$HL_Opt_ClutterChance", "Clutter Hide Chance (%)").c_str(), &Settings::fHideChanceMisc, 0.0f, 100.0f, "%.1f");
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        HelpMarker(Translation::Get("$HL_Desc_ClutterChance", "Percentage chance that a blacklisted clutter item will be hidden. Completely separate from the global hide chance.").c_str());

        if (changed) Settings::Save();
    }

    inline void Install()
    {
        if (SKSEMenuFramework::IsInstalled()) {
            SKSEMenuFramework::SetSection("Hidden Loot");
            SKSEMenuFramework::AddSectionItem(Translation::Get("$HL_Settings", "Settings").c_str(), RenderMenu);
            logs::info("SKSE Menu Framework detected. In-game menu registered.");
        }
        else {
            logs::info("SKSE Menu Framework not found. Skipping in-game menu integration.");
        }
    }
}