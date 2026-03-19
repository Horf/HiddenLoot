#pragma once

#include "Settings.h"

namespace LootHook
{
    using GetPlayable_t = bool(*)(RE::TESBoundObject*);
    inline REL::Relocation<GetPlayable_t> original_ARMO_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_WEAP_GetPlayable;

    // Retrieves the current actor in the crosshair.
    // Includes a fallback mechanism to handle asynchronous UI updates (like QuickLoot (IE)).
    RE::Actor* GetTargetActor(RE::TESBoundObject* a_item)
    {
        // Cache the last valid actor to prevent items from flickering when 
        // looking away from a corpse while asynchronous loot menus are still updating.
        static RE::ObjectRefHandle s_lastActor;

        auto crosshair = RE::CrosshairPickData::GetSingleton();
        if (!crosshair) return nullptr;

        RE::NiPointer<RE::TESObjectREFR> refPtr;

        // Skyrim VR specific crosshair/hand targeting
        if (REL::Module::IsVR()) {
            const auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return nullptr;
            const auto& vrData = player->GetVRPlayerRuntimeData();
            const uint32_t hand = vrData.isRightHandMainHand ? 1 : 0;

            if (crosshair->grabPickRef[hand]) refPtr = crosshair->grabPickRef[hand].get();
            else if (crosshair->targetActor[hand]) refPtr = crosshair->targetActor[hand].get();
            else if (crosshair->target[hand]) refPtr = crosshair->target[hand].get();
        }
        else {
            // Skyrim SE/AE targeting
            if (crosshair->target[0]) refPtr = crosshair->target[0].get();
        }

        if (refPtr) {
            auto actor = refPtr->As<RE::Actor>();
            if (actor) {
                // Target is a valid actor, update fallback cache
                s_lastActor = actor->CreateRefHandle();
                return actor;
            }
            else {
                // Target is not an actor (e.g. a chest). Ccheck if the UI is actually querying
                // an item inside this container, or if this is a delayed asynchronous call
                // from a loot menu still processing the previous NPC.
                bool belongsToContainer = false;
                auto changes = refPtr->GetInventoryChanges();
                if (changes && changes->entryList) {
                    for (auto* entry : *changes->entryList) {
                        if (entry && entry->object && entry->object->GetFormID() == a_item->GetFormID()) {
                            belongsToContainer = true;
                            break;
                        }
                    }
                }

                if (belongsToContainer) {
                    // Item belongs to the chest/container. Ignore it.
                    return nullptr;
                }
                else if (s_lastActor) {
                    // Item is NOT in the chest. This is a delayed UI call. Return the cached NPC.
                    auto last = s_lastActor.get();
                    if (last) return last->As<RE::Actor>();
                }
            }
        }
        else {
            if (s_lastActor) {
                // Crosshair is entirely empty. If a menu is still requesting item data, use the cached actor.
                auto last = s_lastActor.get();
                if (last) return last->As<RE::Actor>();
            }
        }

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
        bool isBaseEnchanted = false;
        if (Settings::bAlwaysShowEnchanted) {
            auto enchantable = a_this->As<RE::TESEnchantableForm>();
            if (enchantable && enchantable->formEnchanting) {
                isBaseEnchanted = true;
            }
        }
        if (isBaseEnchanted) return true;

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

        // Always show items when interacting with specific UI menus (e.g. trading, crafting)
        auto ui = RE::UI::GetSingleton();
        if (ui) {
            if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::MagicMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::FavoritesMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::BarterMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::GiftMenu::MENU_NAME))
            {
                return true;
            }
        }

        auto actor = GetTargetActor(a_this);
        if (!actor) return true;

        // Whitelist check: Always show inventory of specific NPCs to prevent progression blockers
        auto baseObj = actor->GetBaseObject();
        if (baseObj) {
            auto formID = baseObj->GetFormID();
            if (std::find(Settings::excludedNPCBaseIDs.begin(), Settings::excludedNPCBaseIDs.end(), formID) != Settings::excludedNPCBaseIDs.end()) {
                return true;
            }
        }

        // Determine context (Looting a corpse vs. Pickpocketing)
        bool isPickpocketing = false;
        if (ui && ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
            if (!actor->IsDead()) {
                isPickpocketing = true;
            }
        }

        bool isValidTarget = actor->IsDead() || (Settings::bIncludePickpocket && isPickpocketing);

        // If it's a valid looting scenario, scan the inventory
        if (isValidTarget) {
            bool isQuestObject = false;
            bool isWorn = false;
            bool isExtraEnchanted = false;
            bool foundInNPCInventory = false;

            // Fetch live inventory data to accurately determine 'worn' and 'quest' status
            auto invMap = actor->GetInventory();
            for (auto& [item, data] : invMap) {
                if (item && item->GetFormID() == a_this->GetFormID()) {
                    foundInNPCInventory = true;

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

            // If the item isn't in their direct inventory or is a quest item, keep it visible
            if (!foundInNPCInventory || isQuestObject || isExtraEnchanted)  return true;

            // if fHideChance is set to less than 100%, apply deterministic 'random' hiding
            if (Settings::fHideChance < 100.0f) {
				// Same seed value for the same actor-item pair to ensure consistent hiding across inventory updates
                uint32_t seed = actor->GetFormID() ^ a_this->GetFormID();

                seed = (seed ^ 61) ^ (seed >> 16);
                seed = seed + (seed << 3);
                seed = seed ^ (seed >> 4);
                seed = seed * 0x27d4eb2d;
                seed = seed ^ (seed >> 15);

                float randomVal = static_cast<float>(seed % 10000) / 100.0f;

				// If the random value exceeds the hide chance, show the item
                if (randomVal >= Settings::fHideChance) {
                    return true;
                }
            }

            // Final decision based on 'WornOnly' setting
            if (requireWorn) {
                // Hide only if worn
                if (isWorn) return false;
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