#pragma once

#include <SKSE/API.h>
#include <SKSE/Interfaces.h>

#include <RE/E/ExtraDataList.h>
#include <RE/I/InventoryEntryData.h>
#include <RE/T/TESBoundObject.h>
#include <RE/T/TESForm.h>

#include <cstdint>

namespace JunkIt {
    constexpr const char* PLUGIN_NAME = "JunkIt";
    constexpr std::uint32_t API_VERSION = 1;
    constexpr std::uint32_t kMessage_GetAPI = 'JAPI';

    class IAPI {
    public:
        virtual std::uint32_t GetVersion() const = 0;
        virtual bool IsJunk(RE::InventoryEntryData* a_entry) const = 0;
        virtual bool IsJunk(RE::TESBoundObject* a_object, const RE::ExtraDataList* a_extraList, const char* a_displayName) const = 0;
        virtual bool IsAnyJunkForForm(RE::TESForm* a_form) const = 0;
    };

    inline void ListenForAPI(SKSE::MessagingInterface::EventCallback* cb) {
        SKSE::GetMessagingInterface()->RegisterListener(PLUGIN_NAME, cb);
    }

    inline IAPI* API = nullptr;
}