#pragma once

#include <SKSE/Translation.h>

#include <string>

namespace Translation
{
    inline std::string Get(const std::string& a_key, const std::string& a_fallback)
    {
        std::string result;
        if (SKSE::Translation::Translate(a_key, result)) {
            return result;
        }
        return a_fallback;
    }

    inline std::string GetImGui(const std::string& a_key, const std::string& a_fallback)
    {
        return Get(a_key, a_fallback) + "##" + a_key;
    }
}