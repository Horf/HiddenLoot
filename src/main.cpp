#include "LootHook.h"
#include "MenuIntegration.h"

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    logs::info("Hidden Loot Plugin loading...");
    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
        // Wait until all data forms (esp/esm) are loaded before caching forms or installing hooks
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
            LootHook::Install();
			MenuIntegration::Install();
        }
        });
    return true;
}