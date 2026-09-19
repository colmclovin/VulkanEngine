#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct Bone {
    std::string name;
    int parentIndex = -1; // -1 = root
    glm::mat4 inverseBindMatrix{ 1.0f }; // transforms from mesh space into this bone's local space
};

struct Skeleton {
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToIndex;
};