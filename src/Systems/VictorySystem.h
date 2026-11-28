// VictorySystem.h
#include "../Components/Components.h"

enum class GameResult { Playing,
                        Lost,
                        Won };
class VictorySystem {
public:
    // A team "has lost" only if it once had a Catalyst and now has none
    static GameResult Update(entt::registry &registry, bool &hadCatalyst);
};

