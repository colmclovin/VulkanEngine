#pragma once
#include "PlacementGrid.h" // reuse GridCoord/GridCoordHash
#include <unordered_map>

class DepletionMap {
public:
	float GetRemainingFraction(int cellX, int cellZ) const;
	void Deplete(int cellX, int cellZ, float amount, float originalAmount);

	const std::unordered_map<GridCoord, float, GridCoordHash> &GetAll() const { return m_Depleted; }
	void LoadFrom(const std::unordered_map<GridCoord, float, GridCoordHash> &data) { m_Depleted = data; }

private:
	std::unordered_map<GridCoord, float, GridCoordHash> m_Depleted; // stores remaining FRACTION (1.0 = full, 0.0 = empty)
};
