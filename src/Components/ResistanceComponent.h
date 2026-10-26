#pragma once
#include "DamageType.h"

struct ResistanceComponent {
    // Multiplier per damage type. 1 = normal, 0.5 = half damage, 0 = immune, 1.5 = vulnerable.
    float mult[DAMAGE_TYPE_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    float Get(DamageType t) const { return mult[static_cast<int>(t)]; }
    void Set(DamageType t, float m) { mult[static_cast<int>(t)] = m; }
};