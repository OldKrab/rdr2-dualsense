#include "catalog.h"

#include "log.h"

#include <fstream>

namespace dsr2 {

using nlohmann::json;

json deep_merge(json base, const json& over) {
    if (!over.is_object()) return base;
    for (auto& [k, v] : over.items()) {
        if (v.is_object() && base.contains(k) && base[k].is_object()) base[k] = deep_merge(base[k], v);
        else base[k] = v;
    }
    return base;
}

bool Catalog::load(const fs::path& path) {
    by_name_.clear();
    by_hash_.clear();
    std::ifstream in(path);
    if (!in) {
        LOGE("weapons.json: could not open");
        return false;
    }
    json raw;
    try {
        raw = json::parse(in);
    } catch (const std::exception& e) {
        LOGE("weapons.json: %s", e.what());
        return false;
    }
    const json classes = raw.contains("classes") && raw["classes"].is_object() ? raw["classes"] : json::object();
    const json weapons = raw.contains("weapons") && raw["weapons"].is_array() ? raw["weapons"] : json::array();
    for (const auto& w : weapons) {
        if (!w.is_object()) continue;
        std::string name = w.value("name", "");
        if (name.empty()) continue;
        std::string cls = w.value("class", "");
        json prof = classes.contains(cls) && classes[cls].is_object() ? classes[cls] : json::object();
        json meta = w;
        meta.erase("tune");
        prof = deep_merge(std::move(prof), meta);
        if (w.contains("tune") && w["tune"].is_object()) prof = deep_merge(std::move(prof), w["tune"]);
        prof["id"] = name;
        by_hash_[joaat(name)] = name;
        by_name_[name] = std::move(prof);
    }
    LOGI("catalog: %zu weapons, %zu classes", by_hash_.size(), classes.size());
    return true;
}

json Catalog::resolve_name(const std::string& name) const {
    auto it = by_name_.find(name);
    return it == by_name_.end() ? json() : it->second;
}

json Catalog::resolve(uint32_t hash, uint32_t group, std::string& name_out) const {
    auto it = by_hash_.find(hash);
    if (it != by_hash_.end()) {
        name_out = it->second;
        return resolve_name(it->second);
    }
    // unknown weapon: behave like a typical member of its group (pre-hashed once)
    struct GroupFallback { uint32_t hash; const char* weapon; };
    static const GroupFallback groups[] = {
        {joaat("GROUP_REVOLVER"), "WEAPON_REVOLVER_CATTLEMAN"},
        {joaat("GROUP_PISTOL"), "WEAPON_PISTOL_SEMIAUTO"},
        {joaat("GROUP_REPEATER"), "WEAPON_REPEATER_CARBINE"},
        {joaat("GROUP_RIFLE"), "WEAPON_RIFLE_BOLTACTION"},
        {joaat("GROUP_SNIPER"), "WEAPON_SNIPERRIFLE_ROLLINGBLOCK"},
        {joaat("GROUP_SHOTGUN"), "WEAPON_SHOTGUN_PUMP"},
        {joaat("GROUP_BOW"), "WEAPON_BOW"},
        {joaat("GROUP_THROWN"), "WEAPON_THROWN_TOMAHAWK"},
        {joaat("GROUP_MELEE"), "WEAPON_MELEE_KNIFE"},
        {joaat("GROUP_UNARMED"), "WEAPON_UNARMED"},
        {joaat("GROUP_LASSO"), "WEAPON_LASSO"},
        {joaat("GROUP_FISHINGROD"), "WEAPON_FISHINGROD"},
        {joaat("GROUP_HEAVY"), "WEAPON_TURRET_GATLING"},
        {joaat("GROUP_MG"), "WEAPON_TURRET_GATLING"},
    };
    for (const auto& g : groups) {
        if (g.hash == group) {
            name_out = std::string(g.weapon) + " (group fallback)";
            json p = resolve_name(g.weapon);
            if (!p.is_null()) p["id"] = name_out;
            return p;
        }
    }
    name_out = "";
    return json();
}

}  // namespace dsr2
