#include "LootHook.h"

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    SKSE::AllocTrampoline(128);
    logs::info("Hidden Loot Plugin loading...");
    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener([](SKSE::MessagingInterface::Message* a_msg) {
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
            LootHook::Install();
        }
        });
    return true;
}