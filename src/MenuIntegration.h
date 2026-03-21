#pragma once

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
        ImGuiMCP::SeparatorText("Armor & Shields");
        if (ImGuiMCPComponents::ToggleButton("Hide Armor", &Settings::bUnlootableArmor)) changed = true;
        HelpMarker("If disabled you can define bodyslot options (head, chest, arms and legs) seperately.");

        if (!Settings::bUnlootableArmor) {
            ImGuiMCP::Indent(15.0f);
            if (ImGuiMCPComponents::ToggleButton("Hide Head", &Settings::bUnlootableArmorHead)) changed = true;
            HelpMarker("Includes Head, Hair and Circlets.");
            if (ImGuiMCPComponents::ToggleButton("Hide Chest", &Settings::bUnlootableArmorChest)) changed = true;
            HelpMarker("Includes Body, Chest and Back.");
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
        ImGuiMCP::SeparatorText("Weapons");
        if (ImGuiMCPComponents::ToggleButton("Hide Weapons", &Settings::bUnlootableWeapons)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Weapons", &Settings::bWeaponsWornOnly)) changed = true;
        HelpMarker("If enabled, only hides the weapon types the NPC is currently wearing.");

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Pickpocket");
        if (ImGuiMCPComponents::ToggleButton("Apply to Pickpocketing", &Settings::bIncludePickpocket)) changed = true;
        HelpMarker("If enabled, settings apply also while pickpocketing NPCs.");

        if (changed) {
            Settings::Save();
        }
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