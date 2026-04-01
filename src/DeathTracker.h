#pragma once

// ===== Default Library =====
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <deque>

// ===== SKSE =====
#include <SKSE/Interfaces.h>

// ===== RE (Game Types) =====
#include <RE/A/Actor.h>
#include <RE/B/BSTEvent.h>
#include <RE/B/BSCoreTypes.h>
#include "RE/T/TESDeathEvent.h"

// ===== Project =====
#include "Settings.h"

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
                if (_deadActors.find(dyingID) == _deadActors.end()) {
                    _deathHistory.push_back(dyingID);
                }
                _deadActors[dyingID] = isPlayerKill ? CorpseCategory::kPlayerKill : CorpseCategory::kNPCKill;
                while (_deathHistory.size() > 200) {
                    RE::FormID oldestID = _deathHistory.front();
                    _deathHistory.pop_front();
                    _deadActors.erase(oldestID);
                }
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

            if ((a_actor->GetFormID() >> 24) == 0xFF) {
                // Dynamic actors (0xFF) are often temporary spawns; defaulting to kPlayerKill 
                // ensures loot rules stay consistent after save/load even if the original event is lost
                return CorpseCategory::kPlayerKill;
            }

			// If not tracked, determine if pre-placed dead based on record flags
            return CorpseCategory::kPrePlacedDead;
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
            _deathHistory.clear();

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
                        _deathHistory.push_back(newFormID);
                    }
                }
            }
        }

        // Clears data when reverting to a clean state (e.g., main menu)
        void Revert(SKSE::SerializationInterface*) {
            std::lock_guard<std::mutex> lock(_mutex);
            _deadActors.clear();
            _deathHistory.clear();
        }

    private:
        DeathTracker() = default;
        std::unordered_map<RE::FormID, CorpseCategory> _deadActors;
        std::deque<RE::FormID> _deathHistory;
        std::mutex _mutex;
    };
}