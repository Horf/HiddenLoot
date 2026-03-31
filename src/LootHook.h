#pragma once

#include "Settings.h"
#include "DeathTracker.h"

namespace LootHook
{
    using GetPlayable_t = bool(*)(RE::TESBoundObject*);
    inline REL::Relocation<GetPlayable_t> original_ARMO_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_WEAP_GetPlayable;

	// Tracks the open/close state of relevant menus to determine if an attempt to identify a target reference for the item should be queried
    class MenuTracker : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static MenuTracker* GetSingleton() {
            static MenuTracker singleton;
            return &singleton;
        }
        std::atomic<bool> bLootMenuOpen{ false };
        std::atomic<bool> bContainerMenuOpen{ false };
        std::atomic<bool> bOtherMenuOpen{ false };

        virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            if (a_event->menuName == "LootMenu") {
                bLootMenuOpen = a_event->opening;
            }
            else if (a_event->menuName == RE::ContainerMenu::MENU_NAME) {
                bContainerMenuOpen = a_event->opening;
            }
            else if (
                a_event->menuName == RE::InventoryMenu::MENU_NAME ||
                a_event->menuName == RE::MagicMenu::MENU_NAME ||
                a_event->menuName == RE::FavoritesMenu::MENU_NAME ||
                a_event->menuName == RE::BarterMenu::MENU_NAME ||
                a_event->menuName == RE::CraftingMenu::MENU_NAME ||
                a_event->menuName == RE::GiftMenu::MENU_NAME
                ) {
                bOtherMenuOpen = a_event->opening;
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

	// Helper function to find the original actor that turned into an Ash Pile
    inline RE::Actor* GetAshPileOwner(RE::TESObjectREFR* a_ashPile) {
        if (!a_ashPile) return nullptr;

        static RE::FormID lastAshPileID = 0;
        static RE::ActorHandle lastAshPileOwnerHandle;
        static std::mutex ashPileMutex;

        std::lock_guard<std::mutex> lock(ashPileMutex);
        if (a_ashPile->GetFormID() == lastAshPileID) {
            return lastAshPileOwnerHandle.get().get();
        }

        lastAshPileID = a_ashPile->GetFormID();
        lastAshPileOwnerHandle = RE::ActorHandle();

        if (auto processLists = RE::ProcessLists::GetSingleton()) {
            for (auto& handle : processLists->highActorHandles) {
                if (auto loadedActor = handle.get().get()) {
                    if (auto xAsh = loadedActor->extraList.GetByType<RE::ExtraAshPileRef>()) {
                        if (xAsh->ashPileRef.get().get() == a_ashPile) {
                            lastAshPileOwnerHandle = handle;
                            break;
                        }
                    }
                }
            }
        }
        return lastAshPileOwnerHandle.get().get();
    }

    // Helper to check if a specific item exists in a reference's inventory (Base or Dynamic)
    bool ContainerHasItem(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_item)
    {
        if (!a_ref || !a_item) return false;

        auto base = a_ref->GetBaseObject();
        if (!base) return false;

        RE::TESObjectREFR* inventoryOwner = a_ref;

        // Skip the item scan for activators/special container (e.g. Ash Piles)
        // These don't hold traditional inventories in the same way actors do
        if (base->Is(RE::FormType::Activator)) {
            RE::Actor* owner = GetAshPileOwner(a_ref);
            if (owner)
                inventoryOwner = owner;
            else
                return false;
        }
        else if (base->Is(RE::FormType::Container) && (a_ref->GetFormID() >> 24) == 0xFF) {
            return true;
        }

        // Check dynamic inventory changes (worn, added via scripts, or moved items)
        auto changes = a_ref->GetInventoryChanges();
        if (changes && changes->entryList) {
            for (auto* entry : *changes->entryList) {
                if (entry && entry->object && entry->object->GetFormID() == a_item->GetFormID()) {
                    return true;
                }
            }
        }

        // Check base container data (default loot defined in ESP/ESM that hasn't been instantiated yet)
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
    RE::TESObjectREFR* GetTargetRef(RE::TESBoundObject* a_item, bool a_isLootMenuOpen, bool a_isContainerOpen)
    {
		// History buffer size
        constexpr size_t kHistorySize = 10;

        // Store the last unique references to bridge the gap between the real-time crosshair 
        // and asynchronous UI updates (like QuickLoot IE).
        static std::array<RE::ObjectRefHandle, kHistorySize> s_targetHistory{};
        static size_t s_historySize = 0;
        static std::mutex s_historyMutex;

        auto crosshair = RE::CrosshairPickData::GetSingleton();
        if (!crosshair) return nullptr;

        RE::NiPointer<RE::TESObjectREFR> refPtr;

        // Handle Skyrim VR specific crosshair/hand targeting
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

        // Security check: Only use history if a looting UI is actually active.
        // This prevents the mod from "detecting" a corpse owner while the player is just browsing their own inventory.
        if (!refPtr && !a_isLootMenuOpen && !a_isContainerOpen) return nullptr;

		// Thread safety: Lock the history buffer while updating and reading
        std::lock_guard<std::mutex> lock(s_historyMutex);

        // Update history: Push new target to front and keep only the most recent entries.
        if (refPtr) {
            auto currentHandle = refPtr->CreateRefHandle();
            // Only add if it's not already the newest entry
            if (s_historySize == 0 || s_targetHistory[0] != currentHandle) {
                for (size_t i = kHistorySize - 1; i > 0; --i) {
                    s_targetHistory[i] = s_targetHistory[i - 1];
                }
                s_targetHistory[0] = currentHandle;
                if (s_historySize < 5) s_historySize++;
            }
        }

        // Search depth: QuickLoot needs the full history due to async lag. 
        // The ContainerMenu (paused) only needs the most recent target to prevent "filter bleeding" between corpses.
        size_t maxDepth = a_isLootMenuOpen ? s_historySize : (s_historySize == 0 ? 0 : 1);

        // Which of the recent targets actually owns the item the UI is asking for?
        // Iterating from newest to oldest to find the most likely match.
        for (size_t i = 0; i < maxDepth; ++i) {
            auto ref = s_targetHistory[i].get().get();
            if (ContainerHasItem(ref, a_item)) return ref;
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

        // UI Context check
        auto menuTracker = MenuTracker::GetSingleton();
        bool isLootMenuOpen = menuTracker->bLootMenuOpen;
        bool isContainerOpen = menuTracker->bContainerMenuOpen;
        bool isAnyOtherMenuOpen = menuTracker->bOtherMenuOpen;

        if (isAnyOtherMenuOpen && !isContainerOpen) return true;

        // Ownership validation: Find out which recent target owns the item
        auto targetRef = GetTargetRef(a_this, isLootMenuOpen, isContainerOpen);

        // Phantom-Item protection: If the item exists in the UI but no owner is found in history
        // it's likely a lagging UI artifact or was just looted. Hide it to prevent flickering
        if (!targetRef) {
            if (isLootMenuOpen) return false;
            return true;
        }

        // Keyword blacklist: If the item has any of the user-defined blacklist keywords, it should be hidden
		if (a_this->HasKeywordInArray(Settings::cachedHideKeywords, false)) return false;

        // Static whitelists: Items above value threshold or with specific keywords (uniques, artifacts, etc.) are always lootable
        if (a_this->GetGoldValue() >= Settings::fValueThresholdForLoot) return true; 
        if (a_this->HasKeywordInArray(Settings::uniqueKeywords, false)) return true;

        // Option: Whitelist all naturally enchanted items.
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

            // Differentiate between generic clothing/armor and Jewelry (Rings/Amulets).
            if (CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kAmulet) ||
                CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kRing)) {
                isJewelry = true;
            }
            else {
                // Categorize by body slots for granular control.
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

        // Match item type to user settings.
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

        // From here on, 'targetRef' is guaranteed to be a valid owner from history
        auto actor = targetRef->As<RE::Actor>();

        auto baseObj = targetRef->GetBaseObject();
        if (!baseObj) return true;

        RE::Actor* sourceActor = actor;
        bool isAshGhostCorpseContainer = false;

        if (actor) {
            // Check Base-ID whitelist (e.g. Gunjar) to prevent progression blockers.
            auto formID = baseObj->GetFormID();
            if (std::find(Settings::excludedNPCBaseIDs.begin(), Settings::excludedNPCBaseIDs.end(), formID) != Settings::excludedNPCBaseIDs.end()) {
                return true;
            }
        }
        else {
            // Detect if the target are Ash Piles, Ghost Remains or a custom corpse container
            auto formType = baseObj->GetFormType();
            // Specialized handling for non-actor corpse containers/activator (e.g. Ash Piles)
            if (formType == RE::FormType::Activator) {
                isAshGhostCorpseContainer = true;
                sourceActor = GetAshPileOwner(targetRef);
            }
            // Specialized handling for temporary dynamic containers (formID starts with 0xFF)
            else if (formType == RE::FormType::Container && (targetRef->GetFormID() >> 24) == 0xFF) {
                isAshGhostCorpseContainer = true;
            }
        }

        bool isPickpocketing = actor && !actor->IsDead() && isContainerOpen;
        bool isValidTarget = (actor && actor->IsDead()) || isAshGhostCorpseContainer || (Settings::bIncludePickpocket && isPickpocketing);

        if (isValidTarget) {

			// Death category check: If the source actor is dead, determine the death category and apply category-specific settings
            if (sourceActor && sourceActor->IsDead()) {
                auto category = DeathTracker::GetSingleton()->GetCategory(sourceActor);
                if (category == CorpseCategory::kPrePlacedDead && !Settings::bApplyToPreDead) return true;
                if (category == CorpseCategory::kNPCKill && !Settings::bApplyToNPCKills) return true;
                if (category == CorpseCategory::kPlayerKill && !Settings::bApplyToPlayerKills) return true;
            }

            bool isQuestObject = false;
            bool isWorn = false;
            bool isExtraEnchanted = false;
            bool foundInNPCInventory = false;

            RE::TESObjectREFR* inventoryOwner = sourceActor ? sourceActor : targetRef;

            // Fetch live inventory data from the confirmed owner.
            auto changes = inventoryOwner->GetInventoryChanges();
            if (changes && changes->entryList) {
                for (auto* entry : *changes->entryList) {
                    if (entry && entry->object && entry->object->GetFormID() == a_this->GetFormID()) {
                        foundInNPCInventory = true;
                        if (entry->IsQuestObject()) isQuestObject = true;
                        if (entry->IsWorn()) isWorn = true;
                        // Check for individual enchanted items in the inventory if the setting is enabled
                        if (entry->IsEnchanted() && Settings::bAlwaysShowEnchanted) isExtraEnchanted = true;

                        break;
                    }
                }
            }

			// If the item wasn't found in the dynamic inventory changes, it might still be in the base container data (e.g. pre-looted corpse or static NPC inventory)
            if (!foundInNPCInventory) {
                if (ContainerHasItem(inventoryOwner, a_this)) {
                    foundInNPCInventory = true;
                }
            }

            // Fallback for UI-Lag: If an item is still rendered by QuickLoot but missing from inventory, hide it
            if (!foundInNPCInventory) {
                if (isLootMenuOpen) return false;
                return true;
            }

            // Safety: Never hide Quest Items or specifically whitelisted enchanted gear
            if (isQuestObject || isExtraEnchanted)  return true;

            // Apply deterministic 'random' hiding based on the actor-item seed
            if (Settings::fHideChance < 100.0f) {
                uint32_t seed = targetRef->GetFormID() ^ a_this->GetFormID();

                seed = (seed ^ 61) ^ (seed >> 16);
                seed = seed + (seed << 3);
                seed = seed ^ (seed >> 4);
                seed = seed * 0x27d4eb2d;
                seed = seed ^ (seed >> 15);

                float randomVal = static_cast<float>(seed % 10000) / 100.0f;
                if (randomVal >= Settings::fHideChance) return true;
            }

            // Final decision based on 'WornOnly' setting
            if (actor && requireWorn && !isAshGhostCorpseContainer) {
                // Hide only if worn
                if (isWorn) return false;
            }
            else if (isAshGhostCorpseContainer) {
                // Specialized containers lose the 'isWorn' flag, so hide them forcefully if hiding is enabled.
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