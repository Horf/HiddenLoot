// ===== Default Library =====
#include <Windows.h>

// ===== SKSE =====
#include <SKSE/API.h>
#include <SKSE/Logger.h>
#include <SKSE/Interfaces.h>
#include <SKSE/Translation.h>

// ===== RE (Game Types) =====
#include <RE/B/BSInputDeviceManager.h>
#include <RE/M/MenuOpenCloseEvent.h>
#include <RE/S/ScriptEventSourceHolder.h>
#include <RE/T/TESDeathEvent.h>
#include <RE/T/TESContainerChangedEvent.h>
#include <RE/U/UI.h>

// ===== Project =====
#include "LootHook.h"
#include "MenuIntegration.h"
#include "DeathTracker.h"
#include "Settings.h"

// ===== APIs =====
#include "JunkIt.h"
#include "ToolRequirements.h"

// Serialization Callbacks
static void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    LootHook::DeathTracker::GetSingleton()->Save(a_intfc);
}
static void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    LootHook::DeathTracker::GetSingleton()->Load(a_intfc);
}
static void RevertCallback(SKSE::SerializationInterface* a_intfc) {
    LootHook::DeathTracker::GetSingleton()->Revert(a_intfc);
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    logs::info("Hidden Loot Plugin loading...");

	// Register serialization callbacks to save/load the death tracking data
    // This allows the mod to remember who killed whom after reloading a save
    auto serialization = SKSE::GetSerializationInterface();
    if (serialization) {
        serialization->SetUniqueID(LootHook::DeathTracker::kSerializationID);
        serialization->SetSaveCallback(SaveCallback);
        serialization->SetLoadCallback(LoadCallback);
        serialization->SetRevertCallback(RevertCallback);
        logs::info("Serialization callbacks registered.");
    }

    Settings::LoadINI();
	LootHook::InstallHooks();

    logs::info("Hooks installed and INI loaded. Waiting for Data Loaded event...");
    
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
        
        // Register Junk It API listener
        if (a_msg->type == SKSE::MessagingInterface::kPostLoad) {
            // Check if the DLL is actually loaded before registering
            if (GetModuleHandleA("JunkIt.dll")) {
                JunkIt::ListenForAPI([](SKSE::MessagingInterface::Message* api_msg) {
                    if (api_msg->type == JunkIt::kMessage_GetAPI) {
                        if (!JunkIt::API) {
                            JunkIt::API = static_cast<JunkIt::IAPI*>(api_msg->data);
                            logs::info("Successfully loaded Junk It API version: {}", JunkIt::API->GetVersion());
                        }
                    }
                });
            }
            else {
                logs::info("Junk It plugin not detected. API integration disabled.");
            }
        }
        
        // Wait until all data forms (esp/esm) are loaded before caching forms
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
            SKSE::Translation::ParseTranslation("HiddenLoot");

			Settings::LoadGameData();
			MenuIntegration::Install();

            // Load Tool Requirement Rules from JSON
            ToolRequirements::Manager::GetSingleton()->LoadRules();

			// Register for menu open/close events to track when the player is interacting with loot/container UIs
            auto ui = RE::UI::GetSingleton();
            if (ui) {
                ui->AddEventSink<RE::MenuOpenCloseEvent>(LootHook::MenuTracker::GetSingleton());
                logs::info("Menu event sink registered successfully.");
            }

            // Register hotkey listener
            auto inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
            if (inputDeviceManager) {
                inputDeviceManager->AddEventSink(LootHook::InputListener::GetSingleton());
                logs::info("Input event sink registered successfully.");
            }

			// Register for death events after game data is loaded to ensure necessary forms are cached
            auto sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            if (sourceHolder) {
                sourceHolder->AddEventSink<RE::TESDeathEvent>(LootHook::DeathTracker::GetSingleton());
                sourceHolder->AddEventSink<RE::TESContainerChangedEvent>(ToolRequirements::Manager::GetSingleton());
                logs::info("Game event sinks registered successfully.");
            }
            logs::info("Game data loaded and Menu integrated.");
        }
        // Scan the player's inventory to prime the tool cache
        else if (a_msg->type == SKSE::MessagingInterface::kPostLoadGame || a_msg->type == SKSE::MessagingInterface::kNewGame) {
            ToolRequirements::Manager::GetSingleton()->ScanPlayerInventory();
            logs::info("Player inventory scanned successfully.");
        }
    });
    return true;
}