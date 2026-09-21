#include "combat.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace tehi {

// ---- NPC -------------------------------------------------

void NPCDb::register_defaults() {
    NPCDef cortana;
    cortana.id = "cortana"; cortana.name = "Cortana"; cortana.role = "AI";
    cortana.faction = "UNSC"; cortana.current_map = "any";
    {
        NPCSchedule sched;
        sched.location = "pilot_seat";
        sched.lines = {
            {2, "Master Chief, I've detected Covenant signatures ahead."},
            {6, "The ring... it's not what we thought. It's a weapon."},
            {10, "I'll guide you through the crash site."},
            {14, "Chief, I've been analyzing the Forerunner structures."},
            {18, "We must find the Index before the Covenant does."},
            {22, "I am your sword and your shield, Chief."},
        };
        cortana.schedules.push_back(sched);
    }
    cortana.general_lines = {"I'm still here, Chief."};
    m_npcs.push_back(cortana);

    NPCDef chief;
    chief.id = "chief"; chief.name = "Master Chief"; chief.role = "ally";
    chief.faction = "UNSC"; chief.current_map = "crash_site";
    chief.general_lines = {"Spartans never die. They're just missing in action."};
    m_npcs.push_back(chief);

    NPCDef johnson;
    johnson.id = "johnson"; johnson.name = "Sergeant Johnson"; johnson.role = "ally";
    johnson.faction = "UNSC"; johnson.current_map = "crash_site";
    johnson.general_lines = {
        "I ain't no Elf, I'm a Sergeant!",
        "Don't make a girl spell it out for you.",
        "Kickin' your ass should be fun!"
    };
    m_npcs.push_back(johnson);

    NPCDef keyes;
    keyes.id = "keyes"; keyes.name = "Captain Keyes"; keyes.role = "ally";
    keyes.faction = "UNSC"; keyes.current_map = "crash_site";
    keyes.general_lines = {"Chief, get me off this ship. I'm not going to die here."};
    m_npcs.push_back(keyes);

    NPCDef arbiter;
    arbiter.id = "arbiter"; arbiter.name = "The Arbiter"; arbiter.role = "enemy";
    arbiter.faction = "Covenant"; arbiter.current_map = "ring";
    arbiter.general_lines = {"I will not lead the Covenant to ruin."};
    m_npcs.push_back(arbiter);

    NPCDef grunt;
    grunt.id = "grunt"; grunt.name = "Unggoy (Grunt)"; grunt.role = "enemy";
    grunt.faction = "Covenant"; grunt.current_map = "any";
    grunt.general_lines = {"The Demon! Run!"};
    m_npcs.push_back(grunt);

    printf("[NPC] %zu HALO roster registered\n", m_npcs.size());
}

const NPCDef* NPCDb::find(const std::string& id) const {
    for (const auto& n : m_npcs) if (n.id == id) return &n;
    return nullptr;
}

std::vector<const NPCDef*> NPCDb::in_map(const std::string& map) const {
    std::vector<const NPCDef*> out;
    for (const auto& n : m_npcs)
        if (n.current_map == map || n.current_map == "any") out.push_back(&n);
    return out;
}

std::vector<const NPCDef*> NPCDb::by_faction(const std::string& faction) const {
    std::vector<const NPCDef*> out;
    for (const auto& n : m_npcs)
        if (n.faction == faction) out.push_back(&n);
    return out;
}

std::string NPCDb::get_line(const NPCDef& npc, Hour hour) const {
    for (const auto& sched : npc.schedules) {
        auto it = sched.lines.find(hour);
        if (it != sched.lines.end()) return it->second;
    }
    if (!npc.general_lines.empty())
        return npc.general_lines[hour % npc.general_lines.size()];
    return "";
}

// ---- Combat ----------------------------------------------

CombatEngine::CombatEngine(uint32_t seed) {
    std::srand(seed);
}

void CombatEngine::engage(const EnemyState& enemy, float player_hp, float player_max_hp,
                          float player_shield, float player_max_shield) {
    m_enemy = enemy;
    m_enemy_hp = enemy.hp;
    m_enemy_max_hp = enemy.max_hp;
    m_enemy_shield = enemy.shield;
    m_enemy_max_shield = enemy.max_shield;
    m_player_hp = player_hp;
    m_player_max_hp = player_max_hp;
    m_player_shield = player_shield;
    m_player_max_shield = player_max_shield;
    m_ammo = m_mag_size;
    m_grenades = 2;
    m_state = CombatState::player_turn;
    printf("[Combat] Engaged %s (hp=%u, shield=%u)\n", enemy.name.c_str(), m_enemy_hp, m_enemy_shield);
}

std::vector<std::string> CombatEngine::attack(uint32_t weapon_damage, uint32_t weapon_ap) {
    std::vector<std::string> log;
    if (m_state != CombatState::player_turn || m_enemy_hp == 0) return log;
    if (m_ammo == 0) { log.push_back("Click! Reload."); return log; }

    uint32_t dmg = weapon_damage + std::rand() % 10;
    bool crit = (std::rand() % 100) < 15;
    bool shield_break = false;

    if (crit) dmg = dmg * 2;

    // Apply to shield first
    if (m_enemy_shield > 0) {
        uint32_t absorbed = std::min(m_enemy_shield, dmg);
        m_enemy_shield -= absorbed;
        dmg -= absorbed;
        shield_break = (m_enemy_shield == 0);
    }
    if (dmg > 0) m_enemy_hp = std::max(0u, m_enemy_hp - dmg);

    m_ammo--;

    log.push_back("You fire at " + m_enemy.name + " (" + std::to_string(m_ammo) + "/" + std::to_string(m_mag_size) + ")");
    if (shield_break) log.push_back("\x1b[38;5;33mShield broken!\x1b[0m");
    if (crit) log.push_back("\x1b[1mCritical hit!\x1b[0m");

    check_victory();
    if (m_state != CombatState::victory) {
        m_state = CombatState::enemy_turn;
        enemy_attack();
    }
    return log;
}

std::vector<std::string> CombatEngine::use_grenade(uint32_t damage) {
    std::vector<std::string> log;
    if (m_grenades == 0) { log.push_back("No grenades!"); return log; }
    m_grenades--;
    uint32_t dmg = damage + std::rand() % 30;
    if (m_enemy_shield > 0) {
        uint32_t absorbed = std::min(m_enemy_shield, dmg);
        m_enemy_shield -= absorbed;
        dmg -= absorbed;
    }
    m_enemy_hp = std::max(0u, m_enemy_hp - dmg);
    log.push_back("Grenade! " + std::to_string(dmg) + " damage");
    if (m_enemy_shield == 0) log.push_back("Shield broken!");
    check_victory();
    return log;
}

std::vector<std::string> CombatEngine::melee() {
    std::vector<std::string> log;
    if (m_state != CombatState::player_turn || m_enemy_hp == 0) return log;
    uint32_t dmg = 50 + std::rand() % 20;
    if (m_enemy_shield > 0) {
        uint32_t absorbed = std::min(m_enemy_shield, dmg);
        m_enemy_shield -= absorbed;
        dmg -= absorbed;
    }
    m_enemy_hp = std::max(0u, m_enemy_hp - dmg);
    log.push_back("Assault! " + std::to_string(dmg) + " damage to " + m_enemy.name);
    if (m_enemy_shield == 0) log.push_back("Shield broken!");
    check_victory();
    if (m_state != CombatState::victory) {
        m_state = CombatState::enemy_turn;
        enemy_attack();
    }
    return log;
}

std::vector<std::string> CombatEngine::reload() {
    uint32_t need = m_mag_size - m_ammo;
    uint32_t take = std::min(need, m_ammo_reserve);
    m_ammo += take;
    m_ammo_reserve -= take;
    return {"Reloaded. " + std::to_string(m_ammo) + "/" + std::to_string(m_mag_size)};
}

std::vector<std::string> CombatEngine::recharge_shield() {
    m_player_shield = std::min(m_player_max_shield, m_player_shield + 25.0f);
    return {"Shield recharged to " + std::to_string((int)m_player_shield)};
}

std::vector<std::string> CombatEngine::flee() {
    if ((std::rand() % 100) < 40) {
        m_state = CombatState::idle;
        return {"You break contact."};
    }
    enemy_attack();
    return {"Can't escape!"};
}

void CombatEngine::enemy_attack() {
    if (m_enemy_hp == 0) return;
    uint32_t dmg = m_enemy.damage + std::rand() % 5;
    if ((std::rand() % 100) > m_enemy.accuracy) {
        m_state = CombatState::player_turn;
        return;
    }
    // Shield absorbs first
    if (m_player_shield > 0) {
        uint32_t absorbed = std::min((uint32_t)m_player_shield, dmg);
        m_player_shield -= absorbed;
        dmg -= absorbed;
    }
    if (dmg > 0) m_player_hp = std::max(0.0f, m_player_hp - dmg);
    printf("[Combat] %s hits you for %u\n", m_enemy.name.c_str(), dmg);

    if (m_player_hp <= 0) {
        m_state = CombatState::defeat;
    } else {
        m_state = CombatState::player_turn;
    }
}

void CombatEngine::check_victory() {
    if (m_enemy_hp == 0) {
        m_state = CombatState::victory;
        printf("[Combat] %s down.\n", m_enemy.name.c_str());
    }
}

} // namespace tehi
