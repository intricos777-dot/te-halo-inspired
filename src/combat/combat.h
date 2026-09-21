#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace tehi {

using Hour = uint32_t;

struct NPCSchedule {
    std::string location;
    std::map<Hour, std::string> lines;
};

struct NPCDef {
    std::string id;
    std::string name;
    std::string role;       // "ally", "enemy", "scientist", "pilot", "cortana"
    std::string faction;    // "UNSC", "Covenant", "Forerunner", "Flood"
    std::string current_map;
    float x = 0, y = 0;
    std::vector<NPCSchedule> schedules;
    std::vector<std::string> general_lines;
};

class NPCDb {
public:
    void register_defaults();
    const std::vector<NPCDef>& npcs() const { return m_npcs; }
    const NPCDef* find(const std::string& id) const;
    std::vector<const NPCDef*> in_map(const std::string& map) const;
    std::vector<const NPCDef*> by_faction(const std::string& faction) const;
    std::string get_line(const NPCDef& npc, Hour hour) const;
private:
    std::vector<NPCDef> m_npcs;
};

// ---- Combat ---------------------------------------------

struct CombatRound {
    std::string attacker;
    std::string target;
    uint32_t damage = 0;
    std::string action;
    bool crit = false;
    bool shield_break = false;
};

enum class CombatState { idle, player_turn, enemy_turn, victory, defeat };

struct EnemyState {
    std::string name;
    std::string type;  // "grunt", "elite", "jackal", "hunter", "brute", "flood"
    uint32_t hp;
    uint32_t max_hp;
    uint32_t shield;
    uint32_t max_shield;
    uint32_t damage;
    uint32_t accuracy;
    uint32_t armor_pierce;
};

class CombatEngine {
public:
    CombatEngine(uint32_t seed);

    void engage(const EnemyState& enemy, float player_hp, float player_max_hp,
                float player_shield, float player_max_shield);

    std::vector<std::string> attack(uint32_t weapon_damage, uint32_t weapon_ap);
    std::vector<std::string> use_grenade(uint32_t damage);
    std::vector<std::string> melee();
    std::vector<std::string> reload();
    std::vector<std::string> recharge_shield();
    std::vector<std::string> flee();

    bool is_over() const { return m_state == CombatState::victory || m_state == CombatState::defeat; }
    bool player_won() const { return m_state == CombatState::victory; }
    uint32_t enemy_hp() const { return m_enemy_hp; }
    uint32_t enemy_max_hp() const { return m_enemy_max_hp; }
    float player_hp() const { return m_player_hp; }
    float player_shield() const { return m_player_shield; }
    CombatState state() const { return m_state; }

private:
    CombatState m_state = CombatState::idle;
    EnemyState m_enemy;
    uint32_t m_enemy_hp = 0;
    uint32_t m_enemy_max_hp = 0;
    uint32_t m_enemy_shield = 0;
    uint32_t m_enemy_max_shield = 0;
    float m_player_hp = 0;
    float m_player_max_hp = 0;
    float m_player_shield = 0;
    float m_player_max_shield = 0;
    uint32_t m_grenades = 2;
    uint32_t m_ammo = 24;
    uint32_t m_mag_size = 32;
    uint32_t m_ammo_reserve = 96;

    void enemy_attack();
    void check_victory();
};

} // namespace tehi
