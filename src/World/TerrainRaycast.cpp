// TerrainRaycast.cpp
#include "TerrainRaycast.h"
#include "TerrainGenerator.h"
#include "../Utils/GpuNoiseMatch.h"
//glm::vec3 TerrainRaycast::RaycastToTerrain(glm::vec3 rayOrigin, glm::vec3 rayDir, const TerrainSettings &terrainSettings) {
//	const float stepSize = 0.25f;
//	const float maxDistance = 600.0f;
//
//	glm::vec3 pos = rayOrigin;
//	for (float t = 0.0f; t < maxDistance; t += stepSize) {
//		pos = rayOrigin + rayDir * t;
//		float terrainY = TerrainGenerator::SampleHeight(pos.x, pos.z, terrainSettings);
//		if (pos.y <= terrainY) {
//			return glm::vec3(pos.x, terrainY, pos.z);
//		}
//	}
//	return rayOrigin + rayDir * maxDistance;
//}
glm::vec3 TerrainRaycast::RaycastToTerrain(glm::vec3 origin, glm::vec3 dir, const TerrainSettings &s) {
    const float seed = (float)s.seed;
    const float maxDist = 600.0f; // match whatever your current limit is
    const float step = 2.0f;

    float prevT = 0.0f;
    bool prevAbove = true;
    for (float t = 0.0f; t < maxDist; t += step) {
        glm::vec3 p = origin + dir * t;
        bool above = p.y > GpuNoiseMatch::SampleHeightFast(p.x, p.z, seed) + 3.0f; // margin covers fast/exact mismatch
        if (!above) {
            // hit region between prevT and t: refine with the exact height
            float lo = prevT, hi = t;
            for (int i = 0; i < 8; i++) {
                float mid = 0.5f * (lo + hi);
                glm::vec3 m = origin + dir * mid;
                if (m.y > GpuNoiseMatch::SampleHeight(m.x, m.z, seed))
                    lo = mid;
                else
                    hi = mid;
            }
            return origin + dir * (0.5f * (lo + hi));
        }
        prevT = t;
    }
    return origin + dir * maxDist; // no hit; use your existing fallback
}