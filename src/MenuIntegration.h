#pragma once

#include "SKSEMenuFramework.h"
#include "Settings.h"

namespace MenuIntegration
{
    inline void __stdcall RenderMenu()
    {
        bool changed = false;

        ImGuiMCP::SeparatorText("General");

        if (ImGuiMCPComponents::ToggleButton("Enable Mod", &Settings::bEnableMod)) changed = true;

        float threshold = Settings::fValueThresholdForLoot;
        if (ImGuiMCP::DragFloat("Value Threshold", &threshold, 10.0f, 0.0f, 100000.0f, "%.0f", 0)) {
            Settings::fValueThresholdForLoot = threshold;
            changed = true;
        }

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Armor & Shields");
        if (ImGuiMCPComponents::ToggleButton("Hide Armor", &Settings::bUnlootableArmor)) changed = true;

        if (!Settings::bUnlootableArmor) {
            ImGuiMCP::Indent(15.0f);
            if (ImGuiMCPComponents::ToggleButton("Hide Head", &Settings::bUnlootableArmorHead)) changed = true;
            if (ImGuiMCPComponents::ToggleButton("Hide Chest", &Settings::bUnlootableArmorChest)) changed = true;
            if (ImGuiMCPComponents::ToggleButton("Hide Arms", &Settings::bUnlootableArmorArms)) changed = true;
            if (ImGuiMCPComponents::ToggleButton("Hide Legs", &Settings::bUnlootableArmorLegs)) changed = true;
            ImGuiMCP::Unindent(15.0f);
        }

        if (ImGuiMCPComponents::ToggleButton("Hide Shields", &Settings::bUnlootableArmorShield)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Armor", &Settings::bArmorWornOnly)) changed = true;

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Clothing");
        if (ImGuiMCPComponents::ToggleButton("Hide Clothing", &Settings::bUnlootableClothing)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Clothing", &Settings::bClothingWornOnly)) changed = true;

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Weapons");
        if (ImGuiMCPComponents::ToggleButton("Hide Weapons", &Settings::bUnlootableWeapons)) changed = true;
        if (ImGuiMCPComponents::ToggleButton("Only Hide Worn Weapons", &Settings::bWeaponsWornOnly)) changed = true;

        ImGuiMCP::Spacing();
        ImGuiMCP::SeparatorText("Pickpocket");
        if (ImGuiMCPComponents::ToggleButton("Apply to Pickpocketing", &Settings::bIncludePickpocket)) changed = true;

        if (changed) {
            Settings::Save();
        }
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