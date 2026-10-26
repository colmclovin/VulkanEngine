#pragma once
#include "../Components/ResistanceComponent.h"

namespace ResistancePresets {
inline ResistanceComponent Player() {
    ResistanceComponent r;
    r.Set(DamageType::Physical, 0.9f);
    r.Set(DamageType::Fire, 0.8f);
    return r;
}
inline ResistanceComponent Infantry() {
    return ResistanceComponent{};
}
inline ResistanceComponent Building() {
    ResistanceComponent r;
    r.Set(DamageType::Physical, 0.6f);
    r.Set(DamageType::Fire, 0.5f);
    r.Set(DamageType::Explosive, 1.3f); // buildings are vulnerable to explosives
    r.Set(DamageType::ArmorPiercing, 0.8f);
    return r;
}
inline ResistanceComponent Catalyst() {
    ResistanceComponent r = Building();
    r.Set(DamageType::Physical, 0.4f);
    r.Set(DamageType::Explosive, 1.0f);
    return r;
}
} // namespace ResistancePresets