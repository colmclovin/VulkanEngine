// DepletionMap.cpp
#include "DepletionMap.h"
#include <algorithm>

float DepletionMap::GetRemainingFraction(int cellX, int cellZ) const {
	auto it = m_Depleted.find({ cellX, cellZ });
	return it != m_Depleted.end() ? it->second : 1.0f;
}

void DepletionMap::Deplete(int cellX, int cellZ, float amount, float originalAmount) {
	GridCoord coord{ cellX, cellZ };
	float currentFraction = GetRemainingFraction(cellX, cellZ);
	float currentAmount = currentFraction * originalAmount;
	float newAmount = std::max(0.0f, currentAmount - amount);
	m_Depleted[coord] = (originalAmount > 0.0f) ? (newAmount / originalAmount) : 0.0f;
}
