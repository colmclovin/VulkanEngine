#pragma once
#include "TechDatabase.h"
#include <unordered_set>

class TechState {
public:
    int techPoints = 0;

    bool IsUnlocked(TechId id) const { return m_Unlocked.count(id) > 0; }

    bool CanUnlock(TechId id) const {
        const TechDef &def = TechDatabase::Get(id);
        if (IsUnlocked(id)) return false;
        if (techPoints < def.cost) return false;
        for (TechId prereq : def.prerequisites) {
            if (!IsUnlocked(prereq)) return false;
        }
        return true;
    }

    bool TryUnlock(TechId id) {
        if (!CanUnlock(id)) return false;
        techPoints -= TechDatabase::Get(id).cost;
        m_Unlocked.insert(id);
        return true;
    }

private:
    std::unordered_set<TechId> m_Unlocked;
};