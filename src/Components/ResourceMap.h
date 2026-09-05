#pragma once
#include "../Game/ItemDatabase.h"
#include <vector>
#include <cmath>

struct ResourceCell {
	ItemId resource = ItemId::None;
	float amount = 0.0f;
};
enum class RegionType {
    Plains,
    IronDeposit,
    CopperDeposit,
    Forest,
};

class ResourceMap
{
public:

	void Generate(int gridWidth, int gridDepth, float cellSize, int seed);
	ResourceCell* GetCellAtWorldPos(float worldX, float worldY);
	void ExtractFromCell(ResourceCell* cell, float amount);

	int getGridWidth() const { return m_GridWidth; }
	int getGridDepth() const { return m_GridDepth; }
	float getCellSize() const { return m_CellSize; }
	ResourceCell& GetCell(int x, int z) { return m_Cells[z * m_GridWidth + x]; }
    static RegionType DetermineRegion(float regionNoiseValue);
	RegionType GetRegionAtWorldPos(float worldX, float worldZ, int seed) const;


private:

	std::vector<ResourceCell> m_Cells;


	int m_GridWidth = 0;
	int m_GridDepth = 0;
	float m_CellSize = 1.0f;



};

