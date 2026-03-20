#pragma once

#include "Settings.h"

namespace LootHook
{
    using GetPlayable_t = bool(*)(RE::TESBoundObject*);
    inline REL::Relocation<GetPlayable_t> original_ARMO_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_WEAP_GetPlayable;

    // Helper to check if a specific item exists in a reference's inventory (Base or Dynamic)
    bool ContainerHasItem(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_item)
    {
        if (!a_ref || !a_item) return false;

        // Check dynamic inventory changes (worn, added, or moved items)
        auto changes = a_ref->GetInventoryChanges();
        if (changes && changes->entryList) {
            for (auto* entry : *changes->entryList) {
                if (entry && entry->object && entry->object->GetFormID() == a_item->GetFormID()) {
                    return true;
                }
            }
        }

        // Check base container (untouched default loot)
        auto base = a_ref->GetBaseObject();
        auto container = base ? base->As<RE::TESContainer>() : nullptr;
        if (container) {
            bool found = false;
            container->ForEachContainerObject([&](RE::ContainerObject& a_obj) {
                if (a_obj.obj && a_obj.obj->GetFormID() == a_item->GetFormID()) {
                    found = true;
                    return RE::BSContainer::ForEachResult::kStop;
                }
                return RE::BSContainer::ForEachResult::kContinue;
            });
            return found;
        }
        return false;
    }

    // Improved target retrieval with a history buffer to sync fast crosshair movement with slow UI threads
    RE::TESObjectREFR* GetTargetRef(RE::TESBoundObject* a_item)
    {
        // Storing the last 5 unique references the crosshair has touched.
        static std::deque<RE::ObjectRefHandle> s_targetHistory;

        auto crosshair = RE::CrosshairPickData::GetSingleton();
        if (!crosshair) return nullptr;

        RE::NiPointer<RE::TESObjectREFR> refPtr;

        // Skyrim VR specific crosshair/hand targeting
        if (REL::Module::IsVR()) {
            const auto player = RE::PlayerCharacter::GetSingleton();
            if (player) {
                const auto& vrData = player->GetVRPlayerRuntimeData();
                const uint32_t hand = vrData.isRightHandMainHand ? 1 : 0;

                if (crosshair->grabPickRef[hand]) refPtr = crosshair->grabPickRef[hand].get();
                else if (crosshair->targetActor[hand]) refPtr = crosshair->targetActor[hand].get();
                else if (crosshair->target[hand]) refPtr = crosshair->target[hand].get();
            }
        }
        else {
            // Skyrim SE/AE targeting
            if (crosshair->target[0]) refPtr = crosshair->target[0].get();
        }

        // If a new target is under the crosshair, push it to our history.
        if (refPtr) {
            auto currentHandle = refPtr->CreateRefHandle();
            // Only add if it's not already the newest entry
            if (s_targetHistory.empty() || s_targetHistory.front() != currentHandle) {
                s_targetHistory.push_front(currentHandle);
                if (s_targetHistory.size() > 5) s_targetHistory.pop_back();
            }
        }

        // Which of the recent targets actually owns the item the UI is asking for?
        // Iterating from newest to oldest to find the most likely match.
        for (auto& handle : s_targetHistory) {
            auto ref = handle.get().get();
            if (ref && ContainerHasItem(ref, a_item)) {
                return ref;
            }
        }

        // Item found nowhere (it was just looted or crosshair moved to empty space).
        return nullptr;
    }

    // Core logic to determine if an item should be shown or hidden
    bool ProcessItem(RE::TESBoundObject* a_this, bool originalResult)
    {
        // Abort if mod is disabled or the item is natively unplayable
        if (!Settings::bEnableMod) return originalResult;
        if (!originalResult) return false;

        // Exclude valuable items and 'unique' artifacts from being hidden
        if (a_this->GetGoldValue() >= Settings::fValueThresholdForLoot) return true;
        if (a_this->HasKeywordInArray(Settings::uniqueKeywords, false)) return true;

        // Check if the base item is already enchanted and the user wants to see it
        if (Settings::bAlwaysShowEnchanted) {
            auto enchantable = a_this->As<RE::TESEnchantableForm>();
            if (enchantable && enchantable->formEnchanting) return true;
        }

        // Category detection (Armor, Weapon, Clothing, Jewelry)
        bool isWeapon = a_this->IsWeapon();
        bool isArmor = false, isClothing = false, isJewelry = false;
        bool isHead = false, isChest = false, isArms = false, isLegs = false, isShield = false;

        // Categorize armor types and body slots
        if (a_this->IsArmor()) {
			auto armor = static_cast<RE::TESObjectARMO*>(a_this);
            auto CheckSlot = [&](RE::BIPED_MODEL::BipedObjectSlot a_slot) -> bool {
                return armor->GetSlotMask().any(a_slot);
            };

            if (CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kAmulet) ||
                CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kRing)) {
                isJewelry = true;
            }
            else {
                isHead = CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kHead) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kHair) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kCirclet);

                isChest = CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kBody) ||
                          CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModChestPrimary) ||
                          CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModChestSecondary) ||
                          CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModBack);

                isArms = CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kHands) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kForearms) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModShoulder) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModArmLeft) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModArmRight);

                isLegs = CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kFeet) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kCalves) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModPelvisPrimary) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModPelvisSecondary) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModLegLeft) ||
                         CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModLegRight);

                isShield = CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kShield);

                if (armor->GetArmorType() == RE::BIPED_MODEL::ArmorType::kClothing) {
                    isClothing = true;
                }
                else {
                    isArmor = true;
                }
            }
        }

        bool shouldHide = false;
        bool requireWorn = true;

        // Apply user settings based on item category
        if (isWeapon) {
            shouldHide = Settings::bUnlootableWeapons;
            requireWorn = Settings::bWeaponsWornOnly;
        }
        else if (isClothing) {
            shouldHide = Settings::bUnlootableClothing;
            requireWorn = Settings::bClothingWornOnly;
        }
        else if (isJewelry) {
            shouldHide = Settings::bUnlootableJewelry;
            requireWorn = Settings::bJewelryWornOnly;
        }
        else if (isArmor) {
            if (isShield) {
                shouldHide = Settings::bUnlootableArmorShield;
            }
            else {
                if (Settings::bUnlootableArmor) {
                    // Master toggle hides all regular armor
                    shouldHide = true;
                }
                else {
                    // Check individual body slots if master toggle is off
                    if (isHead && Settings::bUnlootableArmorHead) shouldHide = true;
                    if (isChest && Settings::bUnlootableArmorChest) shouldHide = true;
                    if (isArms && Settings::bUnlootableArmorArms) shouldHide = true;
                    if (isLegs && Settings::bUnlootableArmorLegs) shouldHide = true;
                }
            }
            // WornOnly applies to both shields and regular armor
            requireWorn = Settings::bArmorWornOnly;
        }
        // If the item type isn't configured to be hidden, allow it
        if (!shouldHide) return true;

        // UI Context check
        auto ui = RE::UI::GetSingleton();
        bool isLootMenuOpen = ui && ui->IsMenuOpen("LootMenu");
        bool isContainerOpen = ui && ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME);

        // Always show in regular menus (Player Inventory, Trading, Barter, Crafting)
        if (ui && !isLootMenuOpen && !isContainerOpen) {
            if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) || ui->IsMenuOpen(RE::MagicMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::FavoritesMenu::MENU_NAME) || ui->IsMenuOpen(RE::BarterMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME) || ui->IsMenuOpen(RE::GiftMenu::MENU_NAME))
            {
                return true;
            }
        }

        auto targetRef = GetTargetRef(a_this);
        bool inPlayer = ContainerHasItem(RE::PlayerCharacter::GetSingleton(), a_this);

        // If in QuickLoot (LootMenu):
        if (isLootMenuOpen) {
            // QuickLoot ONLY shows the NPC side. If no NPC owner is found -> Hide (Ghost).
            if (!targetRef) return false;
        }
        // If full Container Interface:
        else if (isContainerOpen) {
            // If the player owns the item, always show it
            if (inPlayer) return true;
            // If player doesn't have it and no NPC owner found -> Hide (Ghost).
            if (!targetRef) return false;
        }
        // Fallback for safety (e.g. looking at a corpse without menu open)
        else {
            if (!targetRef) return true;
        }

        // From here on, 'targetRef' is guaranteed to be a valid owner from history.
        auto actor = targetRef->As<RE::Actor>();
        bool isFECAshGhostContainer = false;

        if (actor) {
            // Whitelist check for specific NPCs to prevent progression blockers
            auto baseObj = actor->GetBaseObject();
            if (baseObj) {
                auto formID = baseObj->GetFormID();
                if (std::find(Settings::excludedNPCBaseIDs.begin(), Settings::excludedNPCBaseIDs.end(), formID) != Settings::excludedNPCBaseIDs.end()) {
                    return true;
                }
            }
        }
        else {
            // Detect if the target are Ash Piles, Ghost Remains or a custom FEC corpse container
            auto baseObj = targetRef->GetBaseObject();
            if (baseObj) {
                auto formType = baseObj->GetFormType();
                // Ash Piles are Activators. FEC uses containers or activators.
                if (formType == RE::FormType::Activator || formType == RE::FormType::Container) {
                    if (targetRef->extraList.HasType(RE::ExtraDataType::kAshPileRef) ||
                        targetRef->extraList.HasType(RE::ExtraDataType::kTextDisplayData)) {
                        isFECAshGhostContainer = true;
                    }
                }
            }
        }

        bool isPickpocketing = actor && !actor->IsDead() && isContainerOpen;

        // Valid targets: Dead Actors, Pickpocketing (if enabled), or FEC/Ash Piles
        bool isValidTarget = (actor && actor->IsDead()) || isFECAshGhostContainer || (Settings::bIncludePickpocket && isPickpocketing);

        // If it's a valid looting scenario, scan the inventory
        if (isValidTarget) {
            bool isQuestObject = false;
            bool isWorn = false;
            bool isExtraEnchanted = false;

            // USE targetRef->GetInventory() - This works for Actors and FEC Containers/Ashpiles
            auto invMap = targetRef->GetInventory();
            for (auto& [item, data] : invMap) {
                if (item && item->GetFormID() == a_this->GetFormID()) {
                    auto* entryData = data.second.get();
                    if (entryData) {
                        if (entryData->IsQuestObject()) isQuestObject = true;
                        if (entryData->IsWorn()) isWorn = true;
						// Check for individual enchanted items in the inventory if the setting is enabled
                        if (Settings::bAlwaysShowEnchanted && entryData->IsEnchanted()) isExtraEnchanted = true;
                    }
                    break;
                }
            }

            // If the item is a quest item or showEnchanted enabled, keep it visible
            if (isQuestObject || isExtraEnchanted)  return true;

            // if fHideChance is set to less than 100%, apply deterministic 'random' hiding
            if (Settings::fHideChance < 100.0f) {
				// Same seed value for the same actor-item pair to ensure consistent hiding across inventory updates
                uint32_t seed = targetRef->GetFormID() ^ a_this->GetFormID();

                seed = (seed ^ 61) ^ (seed >> 16);
                seed = seed + (seed << 3);
                seed = seed ^ (seed >> 4);
                seed = seed * 0x27d4eb2d;
                seed = seed ^ (seed >> 15);

                float randomVal = static_cast<float>(seed % 10000) / 100.0f;

				// If the random value exceeds the hide chance, show the item
                if (randomVal >= Settings::fHideChance) return true;
            }

            // Final decision based on 'WornOnly' setting
            if (actor && requireWorn) {
                // Hide only if worn
                if (isWorn) return false;
            }
            else if (isFECAshGhostContainer) {
                // FEC / Ash Piles lose the 'isWorn' data entirely. 
                // To maintain the mod's core functionality the items get forcefully hidden.
                return false;
            }
            // Hide regardless of worn status
            else return false;
        }
        return true;
    }

    bool Hook_ARMO_GetPlayable(RE::TESObjectARMO* a_this) {
        return ProcessItem(a_this, original_ARMO_GetPlayable(a_this));
    }

    bool Hook_WEAP_GetPlayable(RE::TESObjectWEAP* a_this) {
        return ProcessItem(a_this, original_WEAP_GetPlayable(a_this));
    }

    void InstallHooks()
    {
        // Hook GetPlayable for Armors
        REL::Relocation<std::uintptr_t> armoVTable(RE::VTABLE_TESObjectARMO[0]);
        original_ARMO_GetPlayable = armoVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_ARMO_GetPlayable));

        // Hook GetPlayable for Weapons
        REL::Relocation<std::uintptr_t> weapVTable(RE::VTABLE_TESObjectWEAP[0]);
        original_WEAP_GetPlayable = weapVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_WEAP_GetPlayable));

        logs::info("VTable hooks applied successfully.");
    }
}