#pragma once

// ===== Default Library =====
#include <string.h>

// ===== SKSE =====
#include <SKSE/Logger.h>

// ===== RE (Game Types) =====
#include <REL/Module.h>

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

        if (ImGuiMCP::SliderFloat("Hide Chance (%)", &Settings::fHideChance, 0.0f, 100.0f, "%.1f")) {
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        }
        HelpMarker("Percentage chance that an item will be hidden. Lower values leave more random loot on bodies. 100% hides everything matching your rules.");

        if (ImGuiMCP::DragFloat("Value Threshold", &Settings::fValueThresholdForLoot, 10.0f, 0.0f, 100000.0f, "%.0f", 0)) {
            if (ImGuiMCP::IsItemDeactivatedAfterEdit()) changed = true;
        }
        HelpMarker("Items with a gold value equal to or higher than this threshold will always be lootable.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Corpse Filters");
        if (ImGuiMCPComponents::ToggleButton("Apply to Player Kills", &Settings::bApplyToPlayerKills)) changed = true;
        HelpMarker("If enabled, loot hiding rules apply to enemies killed by you, your followers or summons.");
        if (ImGuiMCPComponents::ToggleButton("Apply to NPC Kills", &Settings::bApplyToNPCKills)) changed = true;
        HelpMarker("If enabled, loot hiding rules apply to NPCs killed by other NPCs (e.g. World Events, Faction Wars). Disable this if you want free loot from battles you weren't involved in.");
        if (ImGuiMCPComponents::ToggleButton("Apply to Pre-Dead Corpses", &Settings::bApplyToPreDead)) changed = true;
        HelpMarker("If enabled, loot hiding rules apply to corpses that were already dead when you found them (Decoration/Quest corpses).");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Keyword Filters");
        static char keywordBuffer[256];
        if (keywordBuffer[0] == '\0' && !Settings::sHideKeywords.empty()) {
            strncpy_s(keywordBuffer, Settings::sHideKeywords.c_str(), sizeof(keywordBuffer) - 1);
        }
        if (ImGuiMCP::InputText("Blacklisted Keywords (EditorIDs)", keywordBuffer, sizeof(keywordBuffer))) {
            Settings::sHideKeywords = keywordBuffer;
        }
        if (ImGuiMCP::IsItemDeactivatedAfterEdit()) {
            Settings::LoadGameData();
            changed = true;
        }
        HelpMarker("Comma-separated list of EditorIDs (e.g., IsJunk). Items with these keywords will ALWAYS be hidden. Case-sensitive!");

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
        ImGuiMCP::SeparatorText("Pickpocket");
        if (ImGuiMCPComponents::ToggleButton("Apply to Pickpocketing", &Settings::bIncludePickpocket)) changed = true;
        HelpMarker("If enabled, settings apply also while pickpocketing NPCs.");

        if (changed) Settings::Save();
    }

    inline void Install()
    {
        if (REL::Module::IsVR()) {
            logs::info("VR detected. Skipping SKSE Menu Framework integration.");
            return;
        }
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