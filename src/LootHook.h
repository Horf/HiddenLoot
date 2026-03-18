#pragma once

#include "Settings.h"

namespace LootHook
{
    using GetPlayable_t = bool(*)(RE::TESBoundObject*);
    inline REL::Relocation<GetPlayable_t> original_ARMO_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_WEAP_GetPlayable;

    RE::Actor* GetTargetActor()
    {
        auto crosshair = RE::CrosshairPickData::GetSingleton();
        if (!crosshair) return nullptr;

        RE::NiPointer<RE::TESObjectREFR> refPtr;

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
            if (crosshair->target[0]) refPtr = crosshair->target[0].get();
        }

        if (refPtr) {
            return refPtr->As<RE::Actor>();
        }

        return nullptr;
    }

    bool ProcessItem(RE::TESBoundObject* a_this, bool originalResult)
    {
        if (!Settings::bEnableMod) return originalResult;
        if (!originalResult) return false;

        if (a_this->GetGoldValue() >= Settings::fValueThresholdForLoot) return true;
        if (a_this->HasKeywordInArray(Settings::uniqueKeywords, false)) return true;

        bool isWeapon = a_this->IsWeapon();
        bool isArmor = false, isClothing = false;
        bool isHead = false, isChest = false, isArms = false, isLegs = false, isShield = false;

        if (a_this->IsArmor()) {
			auto armor = static_cast<RE::TESObjectARMO*>(a_this);
            auto CheckSlot = [&](RE::BIPED_MODEL::BipedObjectSlot a_slot) -> bool {
                return armor->GetSlotMask().any(a_slot);
            };

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

        bool shouldHide = false;
        bool requireWorn = true;

        if (isWeapon) {
            shouldHide = Settings::bUnlootableWeapons;
            requireWorn = Settings::bWeaponsWornOnly;
        }
        else if (isClothing) {
            shouldHide = Settings::bUnlootableClothing;
            requireWorn = Settings::bClothingWornOnly;
        }
        else if (isArmor) {
            if (isShield) {
                shouldHide = Settings::bUnlootableArmorShield;
            }
            else {
                if (Settings::bUnlootableArmor) {
                    shouldHide = true;
                }
                else {
                    if (isHead && Settings::bUnlootableArmorHead) shouldHide = true;
                    if (isChest && Settings::bUnlootableArmorChest) shouldHide = true;
                    if (isArms && Settings::bUnlootableArmorArms) shouldHide = true;
                    if (isLegs && Settings::bUnlootableArmorLegs) shouldHide = true;
                }
            }
            requireWorn = Settings::bArmorWornOnly;
        }
        if (!shouldHide) return true;

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

        auto actor = GetTargetActor();
        if (!actor) return true;

        auto baseObj = actor->GetBaseObject();
        if (baseObj) {
            auto formID = baseObj->GetFormID();
            if (std::find(Settings::excludedNPCBaseIDs.begin(), Settings::excludedNPCBaseIDs.end(), formID) != Settings::excludedNPCBaseIDs.end()) {
                return true;
            }
        }

        bool isPickpocketing = false;
        if (ui && ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
            if (!actor->IsDead()) {
                isPickpocketing = true;
            }
        }

        bool isValidTarget = actor->IsDead() || (Settings::bIncludePickpocket && isPickpocketing);

        if (isValidTarget) {
            bool isQuestObject = false;
            bool isWorn = false;
            bool foundInNPCInventory = false;

            auto invMap = actor->GetInventory();
            for (auto& [item, data] : invMap) {
                if (item && item->GetFormID() == a_this->GetFormID()) {
                    foundInNPCInventory = true;

                    auto* entryData = data.second.get();
                    if (entryData) {
                        if (entryData->IsQuestObject()) isQuestObject = true;
                        if (entryData->IsWorn()) isWorn = true;
                    }
                    break;
                }
            }

            if (!foundInNPCInventory || isQuestObject)  return true;

            if (requireWorn) {
                if (isWorn) return false;
            }
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

    void Install()
    {
        Settings::Load();

        REL::Relocation<std::uintptr_t> armoVTable(RE::VTABLE_TESObjectARMO[0]);
        original_ARMO_GetPlayable = armoVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_ARMO_GetPlayable));

        REL::Relocation<std::uintptr_t> weapVTable(RE::VTABLE_TESObjectWEAP[0]);
        original_WEAP_GetPlayable = weapVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_WEAP_GetPlayable));

        logs::info("Successfully installed hooks!");
    }
}