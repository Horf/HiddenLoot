#pragma once

// ===== Default Library =====
#include <atomic>
#include <chrono>
#include <array>
#include <mutex>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <cstdarg>
#include <string_view>
#include <vector>

// ===== SKSE =====
#include <SKSE/Logger.h>

// ===== RE (Game Types) =====
#include <RE/Offsets_VTABLE.h>
#include <REL/Relocation.h>
#include <REL/Module.h>

#include <RE/A/Actor.h>
#include <RE/A/AlchemyItem.h>

#include <RE/B/BSTEvent.h>
#include <RE/B/BSContainer.h>
#include <RE/B/BSFixedString.h>
#include <RE/B/BGSKeywordForm.h>
#include <RE/B/BGSBipedObjectForm.h>
#include <RE/B/BSPointerHandle.h>
#include <RE/B/BSCoreTypes.h>
#include <RE/B/BarterMenu.h>

#include <RE/C/ContainerMenu.h>
#include <RE/C/CraftingMenu.h>
#include <RE/C/CrosshairPickData.h>

#include <RE/E/ExtraAshPileRef.h>
#include <RE/E/ExtraDataTypes.h>

#include <RE/F/FavoritesMenu.h>
#include <RE/F/FormTypes.h>

#include <RE/G/GiftMenu.h>

#include <RE/M/MenuOpenCloseEvent.h>
#include <RE/M/MagicMenu.h>

#include <RE/N/NiSmartPointer.h>

#include <RE/I/InventoryMenu.h>

#include <RE/P/PlayerCharacter.h>
#include <RE/P/ProcessLists.h>

#include <RE/S/ScrollItem.h>

#include <RE/T/TESBoundObject.h>
#include <RE/T/TESObjectREFR.h>
#include <RE/T/TESContainer.h>
#include <RE/T/TESEnchantableForm.h>
#include <RE/T/TESObjectMISC.h>
#include <RE/T/TESObjectARMO.h>
#include <RE/T/TESObjectWEAP.h>
#include <RE/T/TESObjectBOOK.h>

// ===== Project =====
#include "Settings.h"
#include "DeathTracker.h"

namespace LootHook
{
    using GetPlayable_t = bool(*)(RE::TESBoundObject*);
    inline REL::Relocation<GetPlayable_t> original_ARMO_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_WEAP_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_MISC_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_ALCH_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_BOOK_GetPlayable;
    inline REL::Relocation<GetPlayable_t> original_SCRL_GetPlayable;

	// Tracks the open/close state of relevant menus to determine if an attempt to identify a target reference for the item should be queried
    class MenuTracker : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static MenuTracker* GetSingleton() {
            static MenuTracker singleton;
            return &singleton;
        }
        std::atomic<bool> bLootMenuOpen{ false };
        std::atomic<long long> lastLootMenuCloseTime{ 0 };
        std::atomic<bool> bContainerMenuOpen{ false };
        std::atomic<bool> bOtherMenuOpen{ false };

        virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            static const RE::BSFixedString lootMenuName("LootMenu");

            if (a_event->menuName == lootMenuName) {
                bLootMenuOpen = a_event->opening;
                if (!a_event->opening) {
                    auto now = std::chrono::steady_clock::now().time_since_epoch();
                    lastLootMenuCloseTime = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
                }
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

		// Due to the asynchronous nature of menu updates, this is a way to consider the loot menu "effectively open"
        // for a brief window after it closes to prevent flickering of items
        bool IsLootMenuEffectivelyOpen() const {
            if (bLootMenuOpen) return true;
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
            // 250ms grace period bridges the gap during UI fade-out animations or 
            // rapid crosshair jitter to prevent items from "blinking" into view
            if (nowMs - lastLootMenuCloseTime < 250) return true;
            return false;
        }
    };

    // Helper to check if a specific item exists in a reference's inventory (Base or Dynamic)
    bool ContainerHasItem(RE::TESObjectREFR* a_ref, RE::TESBoundObject* a_item, bool a_allowShortcut)
    {
        if (!a_ref || !a_item) return false;

        auto base = a_ref->GetBaseObject();
        if (!base) return false;

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
        auto container = base->As<RE::TESContainer>();
        if (container) {
            bool found = false;
            container->ForEachContainerObject([&](RE::ContainerObject& a_obj) {
                if (a_obj.obj && a_obj.obj->GetFormID() == a_item->GetFormID()) {
                    found = true;
                    return RE::BSContainer::ForEachResult::kStop;
                }
                return RE::BSContainer::ForEachResult::kContinue;
                });
            if (found) return true;
        }

        // Skip the item scan for activators/special container (e.g. Ash Piles)
        // These don't hold traditional inventories in the same way actors do
        if (a_allowShortcut) {
            // Activators (Ash Piles) and dynamic containers (0xFF) often haven't initialized 
            // their inventory changes yet. The shortcut assumes they "own" the item to 
            // maintain UI synchronization until the engine catches up
            bool isActivator = base->Is(RE::FormType::Activator);
            bool isDynamicContainer = base->Is(RE::FormType::Container) && ((a_ref->GetFormID() >> 24) == 0xFF);
            if (isActivator || isDynamicContainer) return true;
        }

        return false;
    }

    // Improved target retrieval with a history buffer to sync fast crosshair movement with slow UI threads
    RE::TESObjectREFR* GetTargetRef(RE::TESBoundObject* a_item, bool a_isLootMenuOpen, bool a_isContainerOpen)
    {
		// History buffer size
        constexpr size_t kHistorySize = 20;

        // Store the last unique references to bridge the gap between the real-time crosshair 
        // and asynchronous UI updates (like QuickLoot IE)
        static std::array<RE::ObjectRefHandle, kHistorySize> s_targetHistory{};
        static size_t s_historySize = 0;
        static std::mutex s_historyMutex;

        size_t localMaxDepth = 0;
        std::array<RE::ObjectRefHandle, kHistorySize> localHistory{};
        
        auto crosshair = RE::CrosshairPickData::GetSingleton();
        if (!crosshair) return nullptr;

        RE::NiPointer<RE::TESObjectREFR> refPtr;

        // Handle Skyrim VR specific crosshair/hand targeting
        if (REL::Module::IsVR()) {
            const auto player = RE::PlayerCharacter::GetSingleton();
            if (player) {
                const auto& vrData = player->GetVRPlayerRuntimeData();
                const uint32_t mainHand = vrData.isRightHandMainHand ? 1 : 0;
                const uint32_t offHand = vrData.isRightHandMainHand ? 0 : 1;

                if (crosshair->grabPickRef[mainHand]) refPtr = crosshair->grabPickRef[mainHand].get();
                else if (crosshair->grabPickRef[offHand]) refPtr = crosshair->grabPickRef[offHand].get();

                else if (crosshair->targetActor[mainHand]) refPtr = crosshair->targetActor[mainHand].get();
                else if (crosshair->targetActor[offHand]) refPtr = crosshair->targetActor[offHand].get();
                
                else if (crosshair->target[mainHand]) refPtr = crosshair->target[mainHand].get();
                else if (crosshair->target[offHand]) refPtr = crosshair->target[offHand].get();
            }
        }
        else {
            // Skyrim SE/AE targeting
            if (crosshair->target[0]) refPtr = crosshair->target[0].get();
        }

        {
            // Thread safety: Lock the history buffer while updating and reading
            std::lock_guard<std::mutex> lock(s_historyMutex);

            // Update history: Push new target to front and keep only the most recent entries
            if (refPtr) {
                auto currentHandle = refPtr->CreateRefHandle();
                // Only add if it's not already the newest entry
                if (s_historySize == 0 || s_targetHistory[0] != currentHandle) {
                    for (size_t i = kHistorySize - 1; i > 0; --i) {
                        s_targetHistory[i] = s_targetHistory[i - 1];
                    }
                    s_targetHistory[0] = currentHandle;
                    if (s_historySize < kHistorySize) s_historySize++;
                }
            }

            // Security check: Only use history if a looting UI is actually active
            // This prevents the mod from "detecting" a corpse owner while the player is just browsing their own inventory
            if (!refPtr && !a_isLootMenuOpen && !a_isContainerOpen) return nullptr;

            // Search depth: QuickLoot needs the full history due to async lag
            // The ContainerMenu (paused) only needs the most recent target to prevent "filter bleeding" between corpses
            localMaxDepth = a_isLootMenuOpen ? s_historySize : (s_historySize == 0 ? 0 : 1);

            // Fast copy of only the relevant handles so the lock can be released immediately
            for (size_t i = 0; i < localMaxDepth; ++i) {
                localHistory[i] = s_targetHistory[i];
            }
        } // <--- Lock is explicitly released here. The slow inventory scanning below now happens lock-free

        // Strict search in history. Prioritizing finding the item in a known 
        // container's real inventory before falling back to the crosshair shortcut
        for (size_t i = 0; i < localMaxDepth; ++i) {
            auto ref = localHistory[i].get().get();
            if (!ref) continue;
            auto base = ref->GetBaseObject();
            bool isSpecial = base && (base->Is(RE::FormType::Activator) || (base->Is(RE::FormType::Container) && (ref->GetFormID() >> 24) == 0xFF));
            if (ContainerHasItem(ref, a_item, isSpecial)) return ref;
        }

		// Final check: If the current crosshair target has the item, it's the most likely candidate
        if (refPtr && ContainerHasItem(refPtr.get(), a_item, true)) {
            return refPtr.get();
        }

        // Item found nowhere (it was just looted or crosshair moved to empty space)
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
        bool isLootMenuOpen = menuTracker->IsLootMenuEffectivelyOpen();
        bool isContainerOpen = menuTracker->bContainerMenuOpen;
        bool isAnyOtherMenuOpen = menuTracker->bOtherMenuOpen;

        if (isAnyOtherMenuOpen && !isContainerOpen) return true;

		// Check if player is loaded (if a valid player reference with a parent cell exists)
        // This prevents the mod from interfering with item interactions during loading screens or when the player is not fully initialized
        auto player = RE::PlayerCharacter::GetSingleton();
        bool isPlayerLoaded = player && player->Is3DLoaded();

        // Ownership validation: Find out which recent target owns the item
        auto targetRef = GetTargetRef(a_this, isLootMenuOpen, isContainerOpen);

        // Phantom-Item protection: Hide items that lag in QuickLoot to prevent flickering
        if (!targetRef) {
            // Never aggressively hide items if ContainerMenu is open
            if (isContainerOpen) return true;

            // Never hide the player's own items
            if (isPlayerLoaded && ContainerHasItem(player, a_this, false)) return true;

            // If the QuickLoot menu is open, but we can't find a valid target reference that owns the item, it's likely a phantom item due to async lag
            if (isLootMenuOpen) return false;

            // No crosshair target, no UI open so true is the default to prevent hiding items for other scripted interactions (like Odin's Gonar's Greed spell)
            return true;
        }

        // Helper to check if the item has any keyword from a given list
        auto HasKeywordFromList = [&](const std::vector<RE::BSFixedString>& keywordList) -> bool {
            if (keywordList.empty()) return false;
            auto kwForm = a_this->As<RE::BGSKeywordForm>();
            if (kwForm) {
                for (const auto& kw : keywordList) {
                    if (kwForm->HasKeywordString(kw)) return true;
                }
            }
            return false;
        };

        bool isClutter = !a_this->IsWeapon() && !a_this->IsArmor();
        float currentHideChance = Settings::fHideChance;
        bool shouldHide = false;
        bool requireWorn = true;

        if (isClutter) {
            // Absolute safety nets for Misc Items - never hide Gold or Lockpicks
            auto formID = a_this->GetFormID();
            if (formID == 0x0000000F || formID == 0x0000000A) return true;

            // Never hide Gems
            auto kwForm = a_this->As<RE::BGSKeywordForm>();
            if (kwForm && kwForm->HasKeywordString("VendorItemGem")) return true;

            // If the misc item isn't specifically blacklisted, show it
            if (!HasKeywordFromList(Settings::miscHideKeywordsList)) return true;

            shouldHide = true;
            // Clutter items are never worn
            requireWorn = false;
            currentHideChance = Settings::fMiscHideChance;
        }
        else {
            // Keyword blacklist: If the item has any of the user-defined blacklist keywords, it should be hidden
            if (HasKeywordFromList(Settings::hideKeywordsList)) return false;

            // Special handling for backpacks - armor/clothing with ModBack slot (47)
            bool isBackpack = false;
            if (a_this->IsArmor()) {
                auto armor = static_cast<RE::TESObjectARMO*>(a_this);
                if (armor->GetSlotMask().any(RE::BIPED_MODEL::BipedObjectSlot::kModBack)) {
                    isBackpack = true;
                }
            }

            // Static whitelists: Items above value threshold or with specific keywords (uniques, artifacts, etc.) are always lootable (skip if it's a backpack)
            if (!isBackpack) {
                if (a_this->GetGoldValue() >= Settings::fValueThresholdForLoot) return true;
                if (a_this->HasKeywordInArray(Settings::uniqueKeywords, false)) return true;
            }

            // Option: Whitelist all naturally enchanted items (skip if it's a backpack)
            if (Settings::bAlwaysShowEnchanted && !isBackpack) {
                auto enchantable = a_this->As<RE::TESEnchantableForm>();
                if (enchantable && enchantable->formEnchanting) return true;
            }

            // Category detection (Armor, Weapon, Clothing, Jewelry)
            bool isWeapon = a_this->IsWeapon();
            bool isArmor = false, isClothing = false, isJewelry = false;
            bool isHead = false, isChest = false, isArms = false, isLegs = false, isShield = false;

            // Categorize armor types and body slots (skip if it's a backpack)
            if (a_this->IsArmor() && !isBackpack) {
                auto armor = static_cast<RE::TESObjectARMO*>(a_this);
                auto CheckSlot = [&](RE::BIPED_MODEL::BipedObjectSlot a_slot) -> bool {
                    return armor->GetSlotMask().any(a_slot);
                };

                // Differentiate between generic clothing/armor and Jewelry (Rings/Amulets)
                if (CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kAmulet) ||
                    CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kRing)) {
                    isJewelry = true;
                }
                else {
                    // Categorize by body slots for granular control
                    isHead = CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kHead) ||
                             CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kHair) ||
                             CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kCirclet);

                    isChest = CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kBody) ||
                              CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModChestPrimary) ||
                              CheckSlot(RE::BIPED_MODEL::BipedObjectSlot::kModChestSecondary);

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

            // Match item type to user settings
            if (isBackpack) {
                shouldHide = Settings::bUnlootableBackpacks;
                requireWorn = Settings::bBackpacksWornOnly;
            }
            else if (isWeapon) {
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
        }

        // If the item type isn't configured to be hidden, allow it
        if (!shouldHide) return true;

        // From here on, 'targetRef' is guaranteed to be a valid owner from history
        auto actor = targetRef->As<RE::Actor>();

        auto baseObj = targetRef->GetBaseObject();
        if (!baseObj) return true;

        RE::Actor* sourceActor = actor;
        bool isAshGhostCorpseContainer = false;

        // Never filter items on living followers, regardless of UI state
        // This should prevent NFF/AFT/EFF framework scripts from being blocked
        if (actor && !actor->IsDead() && !isAshGhostCorpseContainer && actor->IsPlayerTeammate()) return true;

        if (actor) {
            // Check Base-ID whitelist (e.g. Gunjar) to prevent progression blockers
            auto formID = baseObj->GetFormID();
            if (std::find(Settings::excludedNPCBaseIDs.begin(), Settings::excludedNPCBaseIDs.end(), formID) != Settings::excludedNPCBaseIDs.end()) {
                return true;
            }

            // Check if the actor is a Nemesis from Shadow of Sykrim. Items on NPCs with these keywords will always be visible so the player is able to get them back
            if (actor->HasKeywordString("_Nemesis") || actor->HasKeywordString("_ValidateNemesis")) {
                return true;
            }
        }
        else {
            // Detect if the target are Ash Piles, Ghost Remains or a custom corpse container
            auto formType = baseObj->GetFormType();
            bool isActivator = (formType == RE::FormType::Activator);
            bool isDynamicContainer = (formType == RE::FormType::Container && (targetRef->GetFormID() >> 24) == 0xFF);
            // Specialized handling for non-actor corpse containers/activator (e.g. Ash Piles, FEC Frozen Containers)
            if (isActivator || isDynamicContainer) {
                // Default true for activators (Ash Piles)
                // Default false for 0xFF containers to protect Campfire backpacks and Hearthfire furniture
                isAshGhostCorpseContainer = isActivator;

                // Hardcode fallback for FEC, MaximumCarnage, MaximumDestruction: These mods spawn standalone 0xFF corpse containers without linking them
                if (isDynamicContainer) {
                    auto file = baseObj->GetFile(0);
                    if (file) {
                        std::string_view fileName = file->GetFilename();

                        // If Shadow of Skyrim is detected, all of its various backpack containers are excluded from hiding
                        if (fileName == "Shadow of Skyrim.esp") {
                            return true;
                        }

						// If FEC or Maximum Carnage/Destruction are detected, their standalone corpse containers are included to allow hiding their contents
                        if (fileName == "FEC.esp" || fileName == "MaximumCarnage.esp" || fileName == "MaximumDestruction.esp") {
                            isAshGhostCorpseContainer = true;
                        }
                    }
                }

                // Caching the last detected Ash Pile to optimize performance
                static std::unordered_map<RE::FormID, RE::ActorHandle> s_ashPileMap;
                static std::mutex ashPileMutex;

                // Find the original actor that turned into this ash pile
                std::lock_guard<std::mutex> lock(ashPileMutex);
                auto it = s_ashPileMap.find(targetRef->GetFormID());
                if (it != s_ashPileMap.end()) {
                    // Check if the handle is valid before getting the actor
                    if (it->second) sourceActor = it->second.get().get();
                }
                else {
                    if (auto processLists = RE::ProcessLists::GetSingleton()) {
                        // Ash Piles don't store their owner directly; scan loaded actors to find who points
                        // to this specific activator as their "ExtraAshPileRef"
                        for (auto& handle : processLists->highActorHandles) {
                            if (auto loadedActor = handle.get().get()) {
                                if (auto xAsh = loadedActor->extraList.GetByType<RE::ExtraAshPileRef>()) {
                                    if (xAsh->ashPileRef.get().get() == targetRef) {
                                        s_ashPileMap[targetRef->GetFormID()] = handle;
                                        sourceActor = loadedActor;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (!sourceActor) s_ashPileMap[targetRef->GetFormID()] = RE::ActorHandle();
                    if (s_ashPileMap.size() > 20) s_ashPileMap.clear();
                }

                // If a dead actor explicitly points to this container, it is 100% a corpse!
                if (sourceActor) isAshGhostCorpseContainer = true;
            }
        }
        
        // Check if the actor is in a "knocked out" state (bleeding out or unconscious)
        // This increases potential compatibility with death alternative or knock-out mods 
        // that change the actor's state to incapacitated instead of killing them
        bool isKnockedOut = false;
        if (actor && !actor->IsDead()) {
            auto actorState = actor->AsActorState();
            if (actorState && (actorState->IsBleedingOut() || actorState->IsUnconscious())) isKnockedOut = true;
        }

		// Check if the player is currently sneaking
        bool isPlayerSneaking = false;
        if (auto player = RE::PlayerCharacter::GetSingleton()) {
            isPlayerSneaking = player->IsSneaking();
        }

        // Determine if the player is attempting to pickpocket the target 
        // (actor is alive, not knocked out, container is open, and player is sneaking)
        bool isPickpocketing = actor && !actor->IsDead() && !isKnockedOut && (isContainerOpen || isLootMenuOpen) && isPlayerSneaking;

        // Valid target check: The item is eligible for hiding if it's owned by a dead actor, 
        // a knocked-out actor, a specialized corpse container, or via pickpocketing (if enabled)
        bool isValidTarget = (actor && actor->IsDead()) || isKnockedOut || isAshGhostCorpseContainer || (Settings::bIncludePickpocket && isPickpocketing);

        if (isValidTarget) {

			// Early exit if no pickpocketing and all death categories are disabled
            if (!isPickpocketing && !Settings::bApplyToPreDead && !Settings::bApplyToNPCKills && !Settings::bApplyToPlayerKills) {
                return true;
            }

            // Death category check: Determine the death category and apply category-specific settings
            if (!isPickpocketing) {
				// Fallback logic for orphans
                CorpseCategory category = CorpseCategory::kPlayerKill;

                if (sourceActor && sourceActor->IsDead()) {
                    category = DeathTracker::GetSingleton()->GetCategory(sourceActor);
                }
                else if (isAshGhostCorpseContainer) {
                    if ((targetRef->GetFormID() >> 24) == 0xFF) {
                        category = CorpseCategory::kPlayerKill;
                    }
                    else {
                        category = CorpseCategory::kPrePlacedDead;
                    }
                }

                if (category == CorpseCategory::kPrePlacedDead && !Settings::bApplyToPreDead) return true;
                if (category == CorpseCategory::kNPCKill && !Settings::bApplyToNPCKills) return true;
                if (category == CorpseCategory::kPlayerKill && !Settings::bApplyToPlayerKills) return true;
            }

            bool isQuestObject = false;
            bool isWorn = false;
            bool isExtraEnchanted = false;
            bool isPlayerModified = false;
            bool foundInNPCInventory = false;

            RE::TESObjectREFR* inventoryOwner = sourceActor ? sourceActor : targetRef;

            // Fetch live inventory data from the confirmed owner
            auto changes = inventoryOwner->GetInventoryChanges();
            if (changes && changes->entryList) {
                for (auto* entry : *changes->entryList) {
                    if (entry && entry->object && entry->object->GetFormID() == a_this->GetFormID()) {
                        foundInNPCInventory = true;
                        if (entry->IsQuestObject()) isQuestObject = true;
                        if (entry->IsWorn()) isWorn = true;
                        // Check for individual enchanted items in the inventory if the setting is enabled
                        if (entry->IsEnchanted() && Settings::bAlwaysShowEnchanted) isExtraEnchanted = true;
                        
						// If the item has been modified by the player it should be considered as "player-owned" and not hidden
                        if (entry->extraLists) {
                            for (auto* xList : *entry->extraLists) {
                                if (xList) {
                                    if (xList->HasType(RE::ExtraDataType::kTextDisplayData) ||
                                        xList->HasType(RE::ExtraDataType::kEnchantment) ||
                                        (!Settings::bIgnoreHealthExtraData && xList->HasType(RE::ExtraDataType::kHealth))) {
                                        isPlayerModified = true;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }

			// If the item wasn't found in the dynamic inventory changes, it might still be in the base container data (e.g. pre-looted corpse or static NPC inventory)
            if (!foundInNPCInventory && ContainerHasItem(inventoryOwner, a_this, false)) foundInNPCInventory = true;

            // If an item is still rendered by QuickLoot but missing from inventory, hide it
            if (!foundInNPCInventory) {        
                // Never aggressively hide items if the big ContainerMenu is open
                if (isContainerOpen) return true;

                // If the player owns it, it's not a UI-Lag artifact, it's the player's inventory being queried
                if (isPlayerLoaded && ContainerHasItem(player, a_this, false)) return true;

                // Hide any ghost item during QuickLoot
                if (isLootMenuOpen) return false;

                // Default to true for background scripts and spells
                return true;
            }

            // Safety: Never hide Quest Items, specifically whitelisted enchanted gear or player-modified items (e.g. via tempering or enchanting)
            if (isQuestObject || isExtraEnchanted || isPlayerModified)  return true;

            // Apply deterministic 'random' hiding based on the actor-item seed
            if (currentHideChance < 100.0f) {
                uint32_t seed = targetRef->GetFormID() ^ a_this->GetFormID();

                seed = (seed ^ 61) ^ (seed >> 16);
                seed = seed + (seed << 3);
                seed = seed ^ (seed >> 4);
                seed = seed * 0x27d4eb2d;
                seed = seed ^ (seed >> 15);

                float randomVal = static_cast<float>(seed % 10000) / 100.0f;
                if (randomVal >= currentHideChance) return true;
            }

            // Final decision based on 'WornOnly' setting
            if (actor && requireWorn && !isAshGhostCorpseContainer) {
                // Hide only if worn
                if (isWorn) return false;
            }
            else if (isAshGhostCorpseContainer) {
                // Specialized containers lose the 'isWorn' flag, so hide them forcefully if hiding is enabled
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

    bool Hook_MISC_GetPlayable(RE::TESObjectMISC* a_this) {
        return ProcessItem(a_this, original_MISC_GetPlayable(a_this));
    }

    bool Hook_ALCH_GetPlayable(RE::AlchemyItem* a_this) {
        return ProcessItem(a_this, original_ALCH_GetPlayable(a_this));
    }

    bool Hook_BOOK_GetPlayable(RE::TESObjectBOOK* a_this) {
        return ProcessItem(a_this, original_BOOK_GetPlayable(a_this));
    }

    bool Hook_SCRL_GetPlayable(RE::ScrollItem* a_this) {
        return ProcessItem(a_this, original_SCRL_GetPlayable(a_this));
    }

    void InstallHooks()
    {
        // Hook GetPlayable for Armors
        REL::Relocation<std::uintptr_t> armoVTable(RE::VTABLE_TESObjectARMO[0]);
        original_ARMO_GetPlayable = armoVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_ARMO_GetPlayable));

        // Hook GetPlayable for Weapons
        REL::Relocation<std::uintptr_t> weapVTable(RE::VTABLE_TESObjectWEAP[0]);
        original_WEAP_GetPlayable = weapVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_WEAP_GetPlayable));

        // Hook GetPlayable for MISC
        REL::Relocation<std::uintptr_t> miscVTable(RE::VTABLE_TESObjectMISC[0]);
        original_MISC_GetPlayable = miscVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_MISC_GetPlayable));

        // Hook GetPlayable for ALCH (Food, Poison and Potions)
        REL::Relocation<std::uintptr_t> alchVTable(RE::VTABLE_AlchemyItem[0]);
        original_ALCH_GetPlayable = alchVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_ALCH_GetPlayable));

        // Hook GetPlayable for BOOK (Books, Notes and Journals)
        REL::Relocation<std::uintptr_t> bookVTable(RE::VTABLE_TESObjectBOOK[0]);
        original_BOOK_GetPlayable = bookVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_BOOK_GetPlayable));

        // Hook GetPlayable for SCRL (Scrolls)
        REL::Relocation<std::uintptr_t> scrlVTable(RE::VTABLE_ScrollItem[0]);
        original_SCRL_GetPlayable = scrlVTable.write_vfunc(0x19, reinterpret_cast<std::uintptr_t>(Hook_SCRL_GetPlayable));

        logs::info("VTable hooks applied successfully.");
    }
}