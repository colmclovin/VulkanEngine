#pragma once
#include "DamageType.h"

struct CombatStatsComponent {
    float damage = 10.0f;
    DamageType damageType = DamageType::Physical;
    float range = 8.0f;
    float attackInterval = 1.0f;
    float cooldown = 0.0f;
    float moveSpeed = 10.0f;
};