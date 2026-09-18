#pragma once
#include <JSON/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glm {
inline void to_json(nlohmann::json &j, const glm::vec3 &v) {
    j = { v.x, v.y, v.z };
}
inline void from_json(const nlohmann::json &j, glm::vec3 &v) {
    v.x = j.at(0);
    v.y = j.at(1);
    v.z = j.at(2);
}

inline void to_json(nlohmann::json &j, const glm::quat &q) {
    j = { q.x, q.y, q.z, q.w };
}
inline void from_json(const nlohmann::json &j, glm::quat &q) {
    q.x = j.at(0);
    q.y = j.at(1);
    q.z = j.at(2);
    q.w = j.at(3);
}
} // namespace glm