#pragma once

#include "Settings.h"
#include <unordered_map>
#include <mutex>

namespace LootHook
{
    // Categorizes how an actor died to determine if loot rules should apply
    enum class CorpseCategory : std::uint32_t {
        kPlayerKill = 0,
        kNPCKill = 1,
        kPrePlacedDead = 2
    };

    class DeathTracker : public RE::BSTEventSink<RE::TESDeathEvent>
    {
    public:
        // SKSE Serialization constants
        static constexpr std::uint32_t kSerializationVersion = 1;
        static constexpr std::uint32_t kSerializationID = 'DTHK';

        static DeathTracker* GetSingleton() {
            static DeathTracker singleton;
            return &singleton;
        }

        // Listens for actor deaths to record the killer's faction
        virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override {
            if (!Settings::bEnableMod) return RE::BSEventNotifyControl::kContinue;

            if (a_event && a_event->actorDying) {
                auto dyingID = a_event->actorDying->GetFormID();
                bool isPlayerKill = false;

                if (a_event->actorKiller) {
                    auto killer = a_event->actorKiller->As<RE::Actor>();
                    if (killer) {
                        // Check if killer is player or part of the player's team
                        if (killer->IsPlayerRef() || killer->IsPlayerTeammate()) {
                            isPlayerKill = true;
                        }
                        else {
                            // Check for summons/commanded actors owned by player
                            auto commander = killer->GetCommandingActor();
                            if (commander && (commander->IsPlayerRef() || commander->IsPlayerTeammate())) {
                                isPlayerKill = true;
                            }
                        }
                    }
                }

                std::lock_guard<std::mutex> lock(_mutex);
                _deadActors[dyingID] = isPlayerKill ? CorpseCategory::kPlayerKill : CorpseCategory::kNPCKill;
            }
            return RE::BSEventNotifyControl::kContinue;
        }

        // Resolves the death category for a given actor
        CorpseCategory GetCategory(RE::Actor* a_actor) {
            if (!a_actor || !a_actor->IsDead()) return CorpseCategory::kPlayerKill;

            // Check tracked deaths from current or previous sessions
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto it = _deadActors.find(a_actor->GetFormID());
                if (it != _deadActors.end()) return it->second;
            }

            // Check if the actor was flagged as 'StartsDead'
            if ((a_actor->GetFormFlags() & 0x80000) != 0) {
                return CorpseCategory::kPrePlacedDead;
            }

            // Fallback for deaths occurred before mod installation
            return CorpseCategory::kPlayerKill;
        }

        // Persists death data to the SKSE co-save
        void Save(SKSE::SerializationInterface* a_intfc) {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!a_intfc->OpenRecord(kSerializationID, kSerializationVersion)) return;

            std::uint32_t size = static_cast<std::uint32_t>(_deadActors.size());
            a_intfc->WriteRecordData(size);

            for (const auto& [formID, category] : _deadActors) {
                a_intfc->WriteRecordData(formID);
                a_intfc->WriteRecordData(category);
            }
        }

        // Loads death data and resolves FormIDs for the current load order
        void Load(SKSE::SerializationInterface* a_intfc) {
            std::lock_guard<std::mutex> lock(_mutex);
            _deadActors.clear();

            std::uint32_t type, version, length;
            while (a_intfc->GetNextRecordInfo(type, version, length)) {
                if (type != kSerializationID || version != kSerializationVersion) continue;

                std::uint32_t size;
                a_intfc->ReadRecordData(size);

                for (std::uint32_t i = 0; i < size; ++i) {
                    RE::FormID oldFormID;
                    CorpseCategory category;

                    a_intfc->ReadRecordData(oldFormID);
                    a_intfc->ReadRecordData(category);

                    // Resolve FormID to handle potential load order changes since last save
                    RE::FormID newFormID;
                    if (a_intfc->ResolveFormID(oldFormID, newFormID)) {
                        _deadActors[newFormID] = category;
                    }
                }
            }
        }

        // Clears data when reverting to a clean state (e.g., main menu)
        void Revert(SKSE::SerializationInterface*) {
            std::lock_guard<std::mutex> lock(_mutex);
            _deadActors.clear();
        }

    private:
        DeathTracker() = default;
        std::unordered_map<RE::FormID, CorpseCategory> _deadActors;
        std::mutex _mutex;
    };
}