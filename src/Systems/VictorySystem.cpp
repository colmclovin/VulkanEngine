// VictorySystem.cpp
#include "VictorySystem.h"
#include "../Components/Components.h"

GameResult VictorySystem::Update(entt::registry &registry, bool &hadCatalyst) {
    int mine = 0;
    auto cv = registry.view<CatalystComponent, TeamComponent>();
    for (auto c : cv)
        if (cv.get<TeamComponent>(c).team == 0) mine++;
    if (mine > 0) hadCatalyst = true;
    if (hadCatalyst && mine == 0) return GameResult::Lost;
    return GameResult::Playing;
}