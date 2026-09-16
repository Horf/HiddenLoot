#pragma once

// ===== Default Library =====
#include <Windows.h>

// ===== SKSE =====
#include <SKSE/API.h>
#include <SKSE/Interfaces.h>

// ===== RE (Game Types) =====
#include <RE/C/ContainerMenu.h>

#include <RE/I/ItemList.h>

#include <RE/N/NiSmartPointer.h>

#include <RE/P/PlayerCharacter.h>

#include <RE/T/TESObjectREFR.h>

#include <RE/U/UI.h>

// ===== Project =====
#include "QuickLootAPI.h"

namespace MenuRefresh
{
    // Forces both QuickLoot and the Vanilla ContainerMenu to rebuild their item lists
    // Should be called via SKSE Task to ensure thread safety
    inline void RefreshOpenLootMenus()
    {
        if (auto tasks = SKSE::GetTaskInterface()) {
            tasks->AddTask([]() {

                // Refresh QuickLoot
                if (GetModuleHandleA("QuickLootIE.dll")) {
                    if (QuickLoot::API::QuickLootAPI::IsReady(QuickLoot::API::ApiVersion::kV20)) {
                        QuickLoot::API::QuickLootAPI::RefreshLootMenu();
                    }
                }

                // Refresh Vanilla ContainerMenu
                auto ui = RE::UI::GetSingleton();
                if (ui && ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
                    auto containerMenu = static_cast<RE::ContainerMenu*>(ui->GetMenu(RE::ContainerMenu::MENU_NAME).get());

                    if (containerMenu) {
                        auto itemList = containerMenu->GetRuntimeData().itemList;
                        if (itemList) {
                            RE::TESObjectREFR* activeTarget = nullptr;

                            // Check which inventory the player is currently viewing
                            if (containerMenu->GetContainerMode() == RE::ContainerMenu::ContainerMode::kLoot) {
                                // The player is viewing the corpse's inventory
                                RE::NiPointer<RE::TESObjectREFR> refPtr;
                                if (RE::TESObjectREFR::LookupByHandle(containerMenu->GetTargetRefHandle(), refPtr)) {
                                    activeTarget = refPtr.get();
                                }
                            }
                            else {
                                // The player is viewing their own inventory
                                activeTarget = RE::PlayerCharacter::GetSingleton();
                            }

                            // Force the ItemList to rebuild and redraw its UI
                            if (activeTarget) {
                                itemList->Update(activeTarget);
                            }
                            else {
                                // Failsafe
                                itemList->Update();
                            }
                        }
                    }
                }
            });
        }
    }
}