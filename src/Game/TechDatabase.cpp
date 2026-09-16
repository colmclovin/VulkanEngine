#include "TechDatabase.h"
#include <stdexcept>

std::unordered_map<TechId, TechDef> TechDatabase::s_Techs;
std::vector<TechId> TechDatabase::s_Order;


void TechDatabase::Init()
{
    s_Techs[TechId::Electrification] = { "Electrification", "Unlocks power poles and coal generators.", 1, {} };
    s_Order.push_back(TechId::Electrification);


}


const TechDef &TechDatabase::Get(TechId id) {
    auto it = s_Techs.find(id);
    if (it == s_Techs.end()) throw std::runtime_error("TechDatabase::Get: unregistered tech");
    return it->second;
}

const std::vector<TechId> &TechDatabase::GetAllIds() {
    return s_Order;
}