#pragma once

#include <string>
#include <vector>

#include <unordered_map>
#include "ItemDatabase.h"

enum class TechId {
    Electrification
};

struct TechDef {
    std::string name;
    std::string description;
    int cost = 1;
    std::vector<TechId> prerequisites; // Items or machines unlocked by this tech
};

class TechDatabase {
public:
    void Init();
    static const TechDef &Get(TechId id);
    static const std::vector<TechId> &GetAllIds();

private:

    static std::unordered_map<TechId, TechDef> s_Techs;
    static std::vector<TechId> s_Order; // display order

};