// NestManager.cpp
#include "NestManager.h"
#include "FogOfWar.h"
#include "NestPlacement.h"
#include <algorithm>
#include <cmath>

void NestManager::Activate(glm::vec2 c, int seed, const FogOfWar &fog, int count, double now) {
    m_Nests.clear();
    std::vector<NestState> cands;
    const float MIN_D = 350.0f, MAX_D = 1800.0f; // tune: closest and farthest starting nests
    int r = (int)(MAX_D / NestPlacement::CELL) + 1;
    int ccx = (int)std::floor(c.x / NestPlacement::CELL), ccz = (int)std::floor(c.y / NestPlacement::CELL);

    for (int cz = ccz - r; cz <= ccz + r; cz++)
        for (int cx = ccx - r; cx <= ccx + r; cx++) {
            auto s = NestPlacement::GetNestInCell(cx, cz, seed, c);
            if (!s) continue;
            float d = glm::length(s->pos - c);
            if (d < MIN_D || d > MAX_D) continue;
            if (fog.GetState(s->pos.x, s->pos.y) != FogOfWar::Unexplored) continue; // never pop into view
            NestState n;
            n.cellX = s->cellX;
            n.cellZ = s->cellZ;
            n.pos = s->pos;
            n.tier = s->distanceTier;
            n.nextDecisionTime = now + 120.0;
            cands.push_back(n);
        }

    // Spread picks across distance: sort by distance, take evenly spaced entries
    std::sort(cands.begin(), cands.end(), [&](auto &a, auto &b) { return glm::length(a.pos - c) < glm::length(b.pos - c); });
    if (cands.empty()) return;
    for (int i = 0; i < count && i < (int)cands.size(); i++)
        m_Nests.push_back(cands[(size_t)i * cands.size() / std::max(count, 1)]);
}

void NestManager::Tick(float dt, double now) {
    m_Accum += dt;
    if (m_Accum < 1.0f) return; // economy ticks once a second
    float step = m_Accum;
    m_Accum = 0.0f;

    for (auto &n : m_Nests) {
        float income = (1.0f + n.tier * 0.5f) * step; // tier-based income; difficulty multiplier comes later
        n.stockpile += income;
        // Convert stockpile into strength in chunks (a "unit" costs 20 resource, gives 10 strength)
        while (n.stockpile >= 20.0f) {
            n.stockpile -= 20.0f;
            n.strength += 10.0f;
        }
    }
}