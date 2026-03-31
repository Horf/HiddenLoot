#include "LootHook.h"
#include "MenuIntegration.h"
#include "DeathTracker.h"

// Serialization Callbacks
void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    LootHook::DeathTracker::GetSingleton()->Save(a_intfc);
}
void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    LootHook::DeathTracker::GetSingleton()->Load(a_intfc);
}
void RevertCallback(SKSE::SerializationInterface* a_intfc) {
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
    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
        // Wait until all data forms (esp/esm) are loaded before caching forms
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
			Settings::LoadGameData();
			MenuIntegration::Install();

			// Register for menu open/close events to track when the player is interacting with loot/container UIs
            auto ui = RE::UI::GetSingleton();
            if (ui) {
                ui->AddEventSink<RE::MenuOpenCloseEvent>(LootHook::MenuTracker::GetSingleton());
                logs::info("Menu event sink registered successfully.");
            }

			// Register for death events after game data is loaded to ensure necessary forms are cached
            auto sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            if (sourceHolder) {
                sourceHolder->AddEventSink<RE::TESDeathEvent>(LootHook::DeathTracker::GetSingleton());
                logs::info("Death event sink registered successfully.");
            }
            logs::info("Game data loaded and Menu integrated.");
        }
    });
    return true;
}