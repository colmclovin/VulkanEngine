#pragma once
#include <vector>
#include <glm/glm.hpp>

enum class VisibilityState : uint8_t {
    Unexplored,
    Explored,
    Visible,
};

class FogOfWarMap {
public:
    void Generate(int gridWidth, int gridDepth);
    void UpdateVisibility(glm::vec3 viewerPos, float radius, float cellSize);
    VisibilityState GetStateAt(float worldX, float worldZ, float cellSize) const;

private:
    std::vector<VisibilityState> m_Cells;
    int m_GridWidth = 0, m_GridDepth = 0;
};