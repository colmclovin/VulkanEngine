#pragma once
#include <cstdint>

enum class DamageType : uint8_t {
    Physical,
    Fire,
    Explosive,
    ArmorPiercing,
    Energy,
    Count
};

constexpr int DAMAGE_TYPE_COUNT = static_cast<int>(DamageType::Count);