#include "Command.h"
#include "character.h"
#include "levels.h"
#include "common.h"

// **DEFINE LIST FOUND IN interp.h**

// Temp removal to perfect system. 1/25/06 Eas
// WARNING WARNING WARNING WARNING WARNING
// The command list was modified to account for toggle_hide.
// The last integer will affect a char being removed from hide when they perform the command.
// 0  - char will always become visibile.
// 1  - char will not become visible when using this command.
// 2+ - char has a greater chance of breaking hide as this increases.
// These numbers are overruled by the act() STAYHIDE flag.
// Eas 1/21/06
//
// The command number should be CMD_DEFAULT for any user command that is not used
// in a spec_proc.  If it is, then it should be a number that is not
// already in use.
const QList<command_info> Command::cmd_info =
    {
        // Movement commands
        {"north", do_move, nullptr, nullptr, position_t::STANDING, 0, CMD_NORTH, true, 1, CommandType::all},
        {"east", do_move, nullptr, nullptr, position_t::STANDING, 0, CMD_EAST, true, 1, CommandType::all},
        {"south", do_move, nullptr, nullptr, position_t::STANDING, 0, CMD_SOUTH, true, 1, CommandType::all},
        {"west", do_move, nullptr, nullptr, position_t::STANDING, 0, CMD_WEST, true, 1, CommandType::all},
        {"up", do_move, nullptr, nullptr, position_t::STANDING, 0, CMD_UP, true, 1, CommandType::all},
        {"down", do_move, nullptr, nullptr, position_t::STANDING, 0, CMD_DOWN, true, 1, CommandType::all},

        // Common commands
        {"newbie", do_newbie, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cast", do_cast, nullptr, nullptr, position_t::SITTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"filter", do_cast, nullptr, nullptr, position_t::SITTING, 0, CMD_FILTER, 0, 0, CommandType::all},
        {"sing", do_sing, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"exits", do_exits, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"f", do_fire, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 25, CommandType::all},
        {"get", do_get, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"inventory", do_inventory, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"k", do_kill, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"ki", do_ki, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"kill", do_kill, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"look", do_look, nullptr, nullptr, position_t::RESTING, 0, CMD_LOOK, true, 1, CommandType::all},
        {"loot", do_get, nullptr, nullptr, position_t::RESTING, 0, CMD_LOOT, 0, 0, CommandType::all},
        {"glance", do_look, nullptr, nullptr, position_t::RESTING, 0, CMD_GLANCE, true, 1, CommandType::all},
        {"order", do_order, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"rest", do_rest, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"recite", do_recite, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"recall", nullptr, nullptr, &Character::do_recall, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"score", do_score, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"scan", do_scan, nullptr, nullptr, position_t::RESTING, 1, CMD_DEFAULT, 0, 25, CommandType::all},
        {"stand", do_stand, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"switch", do_switch, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 25, CommandType::all},
        {"tell", nullptr, nullptr, &Character::do_tell, position_t::RESTING, 0, CMD_TELL, 0, 1, CommandType::all},
        {"tellhistory", nullptr, do_tellhistory, nullptr, position_t::RESTING, 0, CMD_TELLH, 0, 1, CommandType::all},
        {"wield", do_wield, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"innate", do_innate, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"orchestrate", do_sing, nullptr, nullptr, position_t::RESTING, 0, CMD_ORCHESTRATE, 0, 0, CommandType::all},

        // Informational commands
        {"alias", do_alias, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"toggle", nullptr, nullptr, &Character::do_toggle, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"consider", do_consider, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"configure", nullptr, nullptr, &Character::do_config, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::players_only},
        {"credits", do_credits, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"equipment", do_equipment, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ohelp", do_help, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"help", do_new_help, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"idea", do_idea, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"info", do_info, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"leaderboard", do_leaderboard, nullptr, nullptr, position_t::DEAD, 3, CMD_DEFAULT, 0, 1, CommandType::all},
        {"news", do_news, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"thenews", do_news, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"story", do_story, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"tick", do_tick, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"time", do_time, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"title", do_title, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"typo", do_typo, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"weather", do_weather, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"who", nullptr, nullptr, &Character::do_who, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizlist", do_wizlist, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"socials", do_social, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"index", do_index, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"areas", do_areas, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"commands", do_new_help, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"experience", nullptr, nullptr, &Character::do_experience, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::players_only},
        {"version", do_version, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"identify", nullptr, nullptr, &Character::do_identify, position_t::DEAD, 0, CMD_DEFAULT, true, 0, CommandType::all},

        // Communication commands
        {"ask", do_ask, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"auction", nullptr, nullptr, &Character::do_auction, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"awaymsgs", do_awaymsgs, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"channel", do_channel, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"dream", do_dream, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"emote", do_emote, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {":", do_emote, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"gossip", do_gossip, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"trivia", do_trivia, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"gtell", do_grouptell, nullptr, nullptr, position_t::DEAD, 0, CMD_GTELL, 0, 1, CommandType::all},
        {".", do_grouptell, nullptr, nullptr, position_t::DEAD, 0, CMD_GTELL, 0, 1, CommandType::all},
        {"ignore", nullptr, do_ignore, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"insult", do_insult, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"reply", nullptr, do_reply, nullptr, position_t::RESTING, 0, CMD_REPLY, 0, 1, CommandType::all},
        {"report", do_report, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"say", nullptr, do_say, nullptr, position_t::RESTING, 0, CMD_SAY, true, 0, CommandType::all},
        {"psay", nullptr, do_psay, nullptr, position_t::RESTING, 0, CMD_SAY, true, 0, CommandType::all},
        {"'", nullptr, do_say, nullptr, position_t::RESTING, 0, CMD_SAY, true, 0, CommandType::all},
        {"shout", do_shout, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whisper", do_whisper, nullptr, nullptr, position_t::RESTING, 0, CMD_WHISPER, 0, 0, CommandType::all},

        // Object manipulation
        {"slip", do_slip, nullptr, nullptr, position_t::STANDING, 0, CMD_SLIP, 0, 1, CommandType::all},
        {"batter", do_batter, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"brace", do_brace, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"close", do_close, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"donate", do_donate, nullptr, nullptr, position_t::RESTING, 0, CMD_DONATE, true, 1, CommandType::all},
        {"drink", do_drink, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"drop", do_drop, nullptr, nullptr, position_t::RESTING, 0, CMD_DROP, true, 25, CommandType::all},
        {"eat", do_eat, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"fill", do_fill, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"give", do_give, nullptr, nullptr, position_t::RESTING, 0, CMD_GIVE, true, 25, CommandType::all},
        {"grab", do_grab, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"hold", do_grab, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"lock", do_lock, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"open", do_open, nullptr, nullptr, position_t::RESTING, 0, CMD_OPEN, true, 25, CommandType::all},
        {"pour", do_pour, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"put", do_put, nullptr, nullptr, position_t::RESTING, 0, CMD_PUT, true, 0, CommandType::all},
        {"quaff", do_quaff, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"read", do_read, nullptr, nullptr, position_t::RESTING, 0, CMD_READ, 0, 1, CommandType::all},
        {"remove", do_remove, nullptr, nullptr, position_t::RESTING, 0, CMD_REMOVE, true, 25, CommandType::all},
        {"erase", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_ERASE, 0, 0, CommandType::all},
        {"sip", do_sip, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"track", nullptr, nullptr, &Character::do_track, position_t::STANDING, 0, CMD_TRACK, 0, 10, CommandType::all},
        {"take", do_get, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"palm", do_get, nullptr, nullptr, position_t::RESTING, 3, CMD_PALM, 0, 1, CommandType::all},
        {"sacrifice", do_sacrifice, nullptr, nullptr, position_t::RESTING, 0, CMD_SACRIFICE, true, 25, CommandType::all},
        {"taste", do_taste, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"unlock", do_unlock, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"use", do_use, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"wear", do_wear, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 25, CommandType::all},
        {"scribe", do_scribe, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"brew", do_brew, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        //  { "poisonmaking", do_poisonmaking, nullptr,nullptr, position_t::RESTING, 0, 9,   0, 0  ,CommandType::all},

        // Combat commands
        {"bash", do_bash, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"retreat", do_retreat, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"disarm", do_disarm, nullptr, nullptr, position_t::FIGHTING, 2, CMD_DEFAULT, 0, 0, CommandType::all},
        {"flee", do_flee, nullptr, nullptr, position_t::FIGHTING, 0, CMD_FLEE, true, 0, CommandType::all},
        {"escape", do_flee, nullptr, nullptr, position_t::FIGHTING, 0, CMD_ESCAPE, 0, 0, CommandType::all},
        {"hit", nullptr, nullptr, &Character::do_hit, position_t::FIGHTING, 0, CMD_HIT, true, 0, CommandType::all},
        {"join", nullptr, nullptr, &Character::do_join, position_t::FIGHTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"battlesense", do_battlesense, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"stance", do_defenders_stance, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"perseverance", do_perseverance, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"smite", do_smite, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // Junk movedso join precedes it
        {"junk", do_sacrifice, nullptr, nullptr, position_t::RESTING, 0, CMD_SACRIFICE, true, 25, CommandType::all},

        {"murder", do_murder, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"rescue", nullptr, nullptr, &Character::do_rescue, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"trip", do_trip, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"deathstroke", do_deathstroke, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"circle", do_circle, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"kick", do_kick, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"battlecry", do_battlecry, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"behead", do_behead, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"rage", nullptr, nullptr, &Character::do_rage, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"berserk", do_berserk, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"golemscore", do_golem_score, nullptr, nullptr, position_t::DEAD, 1, CMD_DEFAULT, 0, 1, CommandType::all},
        {"stun", do_stun, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"redirect", do_redirect, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"hitall", do_hitall, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"quiveringpalm", do_quivering_palm, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"eagleclaw", do_eagle_claw, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"headbutt", do_headbutt, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"cripple", do_cripple, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"fire", do_fire, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 25, CommandType::all},
        {"layhands", do_layhands, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"harmtouch", do_harmtouch, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"bloodfury", do_bloodfury, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"primalfury", do_primalfury, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"bladeshield", do_bladeshield, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"repelance", do_focused_repelance, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"vitalstrike", do_vitalstrike, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"crazedassault", do_crazedassault, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"bullrush", do_bullrush, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"ferocity", do_ferocity, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"tactics", do_tactics, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"deceit", do_deceit, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"knockback", do_knockback, nullptr, nullptr, position_t::FIGHTING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"appraise", do_appraise, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 1, CommandType::all},
        {"make camp", do_make_camp, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"leadership", do_leadership, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"triage", do_triage, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"onslaught", do_onslaught, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"pursue", do_pursue, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // Position commands
        {"sit", do_sit, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, true, 0, CommandType::all},
        {"sleep", do_sleep, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"wake", nullptr, nullptr, &Character::do_wake, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // Miscellaneous commands
        {"editor", do_editor, nullptr, nullptr, position_t::SLEEPING, CMD_EDITOR, CMD_DEFAULT, 0, 1, CommandType::all},
        {"autojoin", nullptr, do_autojoin, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"visible", do_visible, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ctell", do_ctell, nullptr, nullptr, position_t::SLEEPING, 0, CMD_CTELL, 0, 1, CommandType::all},
        {"outcast", nullptr, nullptr, &Character::do_outcast, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"accept", do_accept, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whoclan", do_whoclan, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cpromote", do_cpromote, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"clans", do_clans, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"clanarea", nullptr, nullptr, &Character::do_clanarea, position_t::RESTING, 11, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cinfo", do_cinfo, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ambush", nullptr, nullptr, &Character::do_ambush, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whoarena", do_whoarena, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"joinarena", do_joinarena, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"backstab", nullptr, nullptr, &Character::do_backstab, position_t::STANDING, 0, CMD_BACKSTAB, 0, 0, CommandType::all},
        {"bs", nullptr, nullptr, &Character::do_backstab, position_t::STANDING, 0, CMD_BACKSTAB, 0, 0, CommandType::all},
        {"sbs", nullptr, nullptr, &Character::do_backstab, position_t::STANDING, 0, CMD_SBS, 0, 0, CommandType::all}, // single backstab
        {"boss", do_boss, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"jab", do_jab, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"enter", do_enter, nullptr, nullptr, position_t::STANDING, 0, CMD_ENTER, true, 20, CommandType::all},
        {"climb", do_climb, nullptr, nullptr, position_t::STANDING, 0, CMD_CLIMB, true, 20, CommandType::all},
        {"examine", do_examine, nullptr, nullptr, position_t::RESTING, 0, CMD_EXAMINE, 0, 1, CommandType::all},
        {"follow", do_follow, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"stalk", do_stalk, nullptr, nullptr, position_t::STANDING, 6, CMD_DEFAULT, 0, 10, CommandType::all},
        {"group", do_group, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"found", do_found, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"disband", do_disband, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"abandon", do_abandon, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"consent", do_consent, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"whogroup", do_whogroup, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"forage", do_forage, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"whosolo", do_whosolo, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"count", do_count, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"hide", do_hide, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"leave", do_leave, nullptr, nullptr, position_t::STANDING, 0, CMD_LEAVE, true, 20, CommandType::all},
        {"name", do_name, nullptr, nullptr, position_t::DEAD, 1, CMD_DEFAULT, 0, 1, CommandType::all},
        {"pick", do_pick, nullptr, nullptr, position_t::STANDING, 0, CMD_PICK, 0, 20, CommandType::all},
        {"quest", do_quest, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"qui", do_qui, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"levels", do_levels, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"quit", do_quit, nullptr, nullptr, position_t::DEAD, 0, CMD_QUIT, 0, 1, CommandType::all},
        {"return", do_return, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, true, 1, CommandType::all},
        {"tame", do_tame, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"free animal", do_free_animal, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"prompt", do_prompt, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"lastprompt", do_lastprompt, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"save", nullptr, nullptr, &Character::do_save, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"sneak", do_sneak, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"home", do_home, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"split", nullptr, nullptr, &Character::do_split, position_t::RESTING, 0, CMD_SPLIT, 0, 0, CommandType::all},
        {"spells", do_spells, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"skills", do_skills, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"songs", do_songs, nullptr, nullptr, position_t::SLEEPING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"steal", do_steal, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 10, CommandType::all},
        {"pocket", do_pocket, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 10, CommandType::all},
        {"motd", do_motd, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cmotd", do_cmotd, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"cbalance", do_cbalance, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"cdeposit", nullptr, nullptr, &Character::do_cdeposit, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"cwithdraw", do_cwithdraw, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"ctax", do_ctax, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"where", do_where, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"write", do_write, nullptr, nullptr, position_t::STANDING, 0, CMD_WRITE, 0, 0, CommandType::all},
        {"beacon", do_beacon, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"beep", do_beep, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"guard", do_guard, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"release", do_release, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"eyegouge", do_eyegouge, nullptr, nullptr, position_t::FIGHTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"vault", do_vault, nullptr, nullptr, position_t::DEAD, 10, CMD_DEFAULT, 0, 0, CommandType::all},
        {"suicide", do_suicide, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"vote", do_vote, nullptr, nullptr, position_t::RESTING, 0, CMD_VOTE, 0, 0, CommandType::all},
        {"huntitems", do_showhunt, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"random", do_random, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        // Special procedure commands

        {"vend", do_vend, nullptr, nullptr, position_t::STANDING, 2, CMD_VEND, 0, 0, CommandType::all},
        {"gag", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_GAG, 0, 0, CommandType::all},
        {"design", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_DESIGN, 0, 0, CommandType::all},
        {"stock", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_STOCK, 0, 0, CommandType::all},
        {"buy", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_BUY, 0, 0, CommandType::all},
        {"sell", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_SELL, 0, 0, CommandType::all},
        {"value", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_VALUE, 0, 0, CommandType::all},
        {"watch", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_WATCH, 0, 0, CommandType::all},
        {"list", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_LIST, 0, 0, CommandType::all},
        {"estimate", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_ESTIMATE, 0, 0, CommandType::all},
        {"repair", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_REPAIR, 0, 0, CommandType::all},
        {"practice", do_practice, nullptr, nullptr, position_t::SLEEPING, 1, CMD_PRACTICE, 0, 0, CommandType::all},
        {"practise", do_practice, nullptr, nullptr, position_t::SLEEPING, 1, CMD_PRACTICE, 0, 0, CommandType::all},
        {"pray", do_pray, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"profession", do_profession, nullptr, nullptr, position_t::SLEEPING, 1, CMD_PROFESSION, 0, 0, CommandType::all},
        {"promote", do_promote, nullptr, nullptr, position_t::STANDING, 1, CMD_DEFAULT, 0, 0, CommandType::all},
        {"price", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 1, CMD_PRICE, 0, 0, CommandType::all},
        {"train", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 1, CMD_TRAIN, 0, 0, CommandType::all},
        {"gain", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 1, CMD_GAIN, 0, 0, CommandType::all},
        {"balance", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_BALANCE, 0, 0, CommandType::all},
        {"deposit", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_DEPOSIT, 0, 0, CommandType::all},
        {"withdraw", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_WITHDRAW, 0, 0, CommandType::all},
        {"clean", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_CLEAN, 0, 0, CommandType::all},
        {"play", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_PLAY, 0, 0, CommandType::all},
        {"finish", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_FINISH, 0, 0, CommandType::all},
        {"veternarian", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_VETERNARIAN, 0, 0, CommandType::all},
        {"feed", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_FEED, 0, 0, CommandType::all},
        {"assemble", do_assemble, nullptr, nullptr, position_t::RESTING, 0, CMD_ASSEMBLE, 0, 0, CommandType::all},
        {"pay", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_PAY, 0, 0, CommandType::all},
        {"restring", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_RESTRING, 0, 0, CommandType::all},
        {"push", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_PUSH, 0, 0, CommandType::all},
        {"pull", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_PULL, 0, 0, CommandType::all},
        {"gaze", nullptr, nullptr, &Character::generic_command, position_t::FIGHTING, 0, CMD_GAZE, 0, 0, CommandType::all},
        {"tremor", nullptr, nullptr, &Character::generic_command, position_t::FIGHTING, 0, CMD_TREMOR, 0, 0, CommandType::all},
        {"bet", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_BET, 0, 0, CommandType::all},
        {"insurance", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_INSURANCE, 0, 0, CommandType::all},
        {"double", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_DOUBLE, 0, 0, CommandType::all},
        {"stay", nullptr, nullptr, &Character::generic_command, position_t::STANDING, 0, CMD_STAY, 0, 0, CommandType::all},
        {"select", do_natural_selection, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 0, CommandType::all},
        {"sector", do_sector, nullptr, nullptr, position_t::RESTING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"remort", nullptr, nullptr, &Character::generic_command, position_t::RESTING, GIFTED_COMMAND, CMD_REMORT, 0, 1, CommandType::all},
        {"reroll", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_REROLL, 0, 0, CommandType::all},
        {"choose", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_CHOOSE, 0, 0, CommandType::all},
        {"confirm", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_CONFIRM, 0, 0, CommandType::all},
        {"cancel", nullptr, nullptr, &Character::generic_command, position_t::RESTING, 0, CMD_CANCEL, 0, 0, CommandType::all},

        // Immortal commands
        {"voteset", do_setvote, nullptr, nullptr, position_t::DEAD, 108, CMD_SETVOTE, 0, 1, CommandType::all},
        {"thunder", do_thunder, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizlock", do_wizlock, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"processes", do_processes, nullptr, nullptr, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"bestow", nullptr, nullptr, &Character::do_bestow, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"oclone", do_oclone, nullptr, nullptr, position_t::DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mclone", do_mclone, nullptr, nullptr, position_t::DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"huntclear", do_huntclear, nullptr, nullptr, position_t::DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"areastats", do_areastats, nullptr, nullptr, position_t::DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"huntstart", do_huntstart, nullptr, nullptr, position_t::DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"revoke", do_revoke, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"chpwd", do_chpwd, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"advance", do_advance, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"skillmax", do_maxes, nullptr, nullptr, position_t::DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"damage", do_dmg_eq, nullptr, nullptr, position_t::DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"affclear", do_clearaff, nullptr, nullptr, position_t::DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"guide", nullptr, nullptr, &Character::do_guide, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"addnews", do_addnews, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"linkload", nullptr, nullptr, &Character::do_linkload, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"listproc", do_listproc, nullptr, nullptr, position_t::DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"zap", nullptr, nullptr, &Character::do_zap, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"slay", do_slay, nullptr, nullptr, position_t::DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rename", nullptr, nullptr, &Character::do_rename_char, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"archive", do_archive, nullptr, nullptr, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"unarchive", do_unarchive, nullptr, nullptr, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"stealth", do_stealth, nullptr, nullptr, position_t::DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"disconnect", do_disconnect, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"force", nullptr, do_force, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"pardon", do_pardon, nullptr, nullptr, position_t::DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"goto", nullptr, nullptr, &Character::do_goto, position_t::DEAD, 102, CMD_DEFAULT, 0, 1, CommandType::immortals_only},
        {"restore", do_restore, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"purloin", do_purloin, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"set", do_set, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"unban", do_unban, nullptr, nullptr, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"ban", do_ban, nullptr, nullptr, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"echo", do_echo, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"eqmax", do_eqmax, nullptr, nullptr, position_t::DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"send", do_send, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"at", do_at, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"fakelog", do_fakelog, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"global", do_global, nullptr, nullptr, position_t::DEAD, 108, CMD_DEFAULT, 0, 1, CommandType::all},
        {"log", do_log, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"snoop", nullptr, nullptr, &Character::do_snoop, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"pview", do_pview, nullptr, nullptr, position_t::DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"arena", do_arena, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"load", do_load, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"prize", do_load, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_PRIZE, 0, 1, CommandType::all},
        {"test", nullptr, nullptr, &Character::do_test, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"testport", do_testport, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"testuser", do_testuser, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"shutdow", nullptr, nullptr, &Character::do_shutdow, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"shutdown", nullptr, nullptr, &Character::do_shutdown, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"opedit", do_opedit, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"opstat", do_opstat, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"procedit", do_procedit, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"procstat", do_mpstat, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"range", do_range, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        // { "pshopedit",	do_pshopedit,	position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1  ,CommandType::all},
        {"sedit", do_sedit, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"sockets", nullptr, nullptr, &Character::do_sockets, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::immortals_only},
        {"punish", do_punish, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"sqedit", do_sqedit, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"qedit", do_qedit, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"install", do_install, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        //  { "motdload",       do_motdload,    position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1  ,CommandType::all},
        {"hedit", do_hedit, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"hindex", do_hindex, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"reload", do_reload, nullptr, nullptr, position_t::DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"plats", do_plats, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"bellow", do_thunder, nullptr, nullptr, position_t::DEAD, DEITY, CMD_BELLOW, 0, 1, CommandType::all},
        {"std::string", do_string, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"transfer", nullptr, do_transfer, nullptr, position_t::DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"gtrans", do_gtrans, nullptr, nullptr, position_t::DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"boot", do_boot, nullptr, nullptr, position_t::DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"linkdead", do_linkdead, nullptr, nullptr, position_t::DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"teleport", do_teleport, nullptr, nullptr, position_t::DEAD, DEITY, CMD_DEFAULT, 0, 1, CommandType::all},
        {"purge", do_purge, nullptr, nullptr, position_t::DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"show", do_show, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"search", nullptr, nullptr, &Character::do_search, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"fighting", do_fighting, nullptr, nullptr, position_t::DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"peace", do_peace, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"check", do_check, nullptr, nullptr, position_t::DEAD, 105, CMD_DEFAULT, 0, 1, CommandType::all},
        {"zoneexits", do_zoneexits, nullptr, nullptr, position_t::DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"find", do_find, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"stat", do_stat, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"redit", do_redit, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"guild", do_guild, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"oedit", do_oedit, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"clear", do_clear, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"repop", nullptr, do_repop, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"medit", do_medit, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rdelete", do_rdelete, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"oneway", do_oneway, nullptr, nullptr, position_t::DEAD, ANGEL, 1, 0, 1, CommandType::all},
        {"twoway", do_oneway, nullptr, nullptr, position_t::DEAD, ANGEL, 2, 0, 1, CommandType::all},
        {"zsave", nullptr, nullptr, &Character::do_zsave, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rsave", do_rsave, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"msave", do_msave, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"osave", do_osave, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"rstat", do_rstat, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"possess", do_possess, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"fsave", nullptr, do_fsave, nullptr, position_t::DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"zedit", do_zedit, nullptr, nullptr, position_t::DEAD, ANGEL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"colors", do_colors, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"colours", do_colors, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"incognito", do_incognito, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"high5", do_highfive, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"holylite", do_holylite, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"immort", nullptr, do_wiz, nullptr, position_t::DEAD, IMMORTAL, CMD_IMMORT, 0, 1, CommandType::all},
        {";", nullptr, do_wiz, nullptr, position_t::DEAD, IMMORTAL, CMD_IMMORT, 0, 1, CommandType::all},
        {"impchan", nullptr, do_wiz, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_IMPCHAN, 0, 1, CommandType::all},
        {"/", nullptr, do_wiz, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_IMPCHAN, 0, 1, CommandType::all},
        {"nohassle", do_nohassle, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizinvis", do_wizinvis, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"poof", do_poof, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"wizhelp", do_wizhelp, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"imotd", do_imotd, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mhelp", do_mortal_help, nullptr, nullptr, position_t::DEAD, IMMORTAL, CMD_DEFAULT, 0, 1, CommandType::all},
        {"testhand", do_testhand, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"varstat", do_varstat, nullptr, nullptr, position_t::DEAD, 104, CMD_DEFAULT, 0, 1, CommandType::all},
        {"matrixinfo", do_matrixinfo, nullptr, nullptr, position_t::DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"maxcheck", do_findfix, nullptr, nullptr, position_t::DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"export", do_export, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mscore", do_mscore, nullptr, nullptr, position_t::DEAD, 103, CMD_DEFAULT, 0, 1, CommandType::all},
        {"world", nullptr, do_world, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},

        // Special procedure commands placed to not disrupt god commands
        {"setup", do_mortal_set, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"metastat", do_metastat, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"testhit", do_testhit, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"acfinder", do_acfinder, nullptr, nullptr, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"findpath", do_findPath, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"findpath2", do_findpath, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"addroom", do_addRoom, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"newpath", do_newPath, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"listpathsbyzone", do_listPathsByZone, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"listallpaths", do_listAllPaths, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"dopathpath", do_pathpath, nullptr, nullptr, position_t::DEAD, GIFTED_COMMAND, CMD_DEFAULT, 0, 1, CommandType::all},
        {"botcheck", nullptr, nullptr, &Character::do_botcheck, position_t::DEAD, 106, CMD_DEFAULT, 0, 1, CommandType::all},
        {"showbits", do_showbits, nullptr, nullptr, position_t::DEAD, OVERSEER, CMD_DEFAULT, 0, 1, CommandType::all},
        {"debug", do_debug, nullptr, nullptr, position_t::DEAD, IMPLEMENTER, CMD_DEFAULT, 0, 1, CommandType::all},

        // Bug way down here after 'buy'
        {"bug", do_bug, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        // imbue after 'im' for us lazy immortal types :)
        {"imbue", do_imbue, nullptr, nullptr, position_t::STANDING, 0, CMD_DEFAULT, 0, 0, CommandType::all},

        // MOBprogram commands
        {"mpasound", do_mpasound, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpbestow", do_mpbestow, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpjunk", do_mpjunk, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpecho", do_mpecho, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpechoat", do_mpechoat, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpechoaround", do_mpechoaround, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpechoaroundnotbad", do_mpechoaroundnotbad, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpkill", do_mpkill, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mphit", do_mphit, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpaddlag", do_mpaddlag, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpmload", do_mpmload, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpoload", do_mpoload, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mppurge", do_mppurge, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpgoto", do_mpgoto, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpat", do_mpat, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mptransfer", do_mptransfer, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpthrow", do_mpthrow, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpforce", do_mpforce, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mppeace", do_mppeace, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpsetalign", do_mpsetalign, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpsettemp", nullptr, nullptr, &Character::do_mpsettemp, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpxpreward", do_mpxpreward, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpteachskill", do_mpteachskill, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpdamage", do_mpdamage, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpothrow", do_mpothrow, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mppause", do_mppause, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpretval", do_mpretval, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpsetmath", do_mpsetmath, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},
        {"mpteleport", do_mpteleport, nullptr, nullptr, position_t::DEAD, 0, CMD_DEFAULT, 0, 1, CommandType::all},

        // End of the line
        {"", nullptr, nullptr, &Character::generic_command, position_t::DEAD, 0, CMD_DEFAULT, true, 0}};

cmd_hash_info *Command::cmd_radix_ = nullptr;

void Command::add_commands_to_radix(void)
{
    if (cmd_info.isEmpty())
    {
        return;
    }

    cmd_radix_ = new cmd_hash_info;
    cmd_radix_->command = cmd_info.value(0);

    for (qsizetype x = 1; x < cmd_info.size(); x++)
    {
        add_command_to_radix(cmd_info.value(x));
    }
}

void Command::free_command_radix_nodes(cmd_hash_info *curr)
{
    if (curr->left)
        free_command_radix_nodes(curr->left);
    if (curr->right)
        free_command_radix_nodes(curr->right);
    dc_free(curr);
}

void Command::add_command_to_radix(command_info cmd)
{
    cmd_hash_info *curr = nullptr;
    cmd_hash_info *temp = nullptr;
    cmd_hash_info *next = nullptr;
    int whichway = 0;

    // At the end of this loop, temp will contain the parent of
    // the new node.  Whether it is the left or right node depends
    // on whether whichway is positive or negative.
    for (curr = cmd_radix_; curr; curr = next)
    {
        if ((whichway = cmd.command_name.compare(curr->command.command_name)) < 0)
            next = curr->left;
        else
            next = curr->right;
        temp = curr;
    }

    curr = new cmd_hash_info;
    curr->command = cmd;

    if (whichway < 0)
        temp->left = curr;
    else
        temp->right = curr;
}

auto Command::find_cmd_in_radix(QString arg) -> std::expected<command_info, search_error>
{
    cmd_hash_info *curr{};
    cmd_hash_info *next{};
    int whichway{};

    for (curr = cmd_radix_; curr; curr = next)
    {
        qDebug() << "Comparing" << arg << "vs" << curr->command.command_name << len_cmp(arg, curr->command.command_name);
        if ((whichway = len_cmp(arg, curr->command.command_name)) == 0)
            return curr->command;
        if (whichway < 0)
            next = curr->left;
        else
            next = curr->right;
    }

    return std::unexpected(search_error::not_found);
}
