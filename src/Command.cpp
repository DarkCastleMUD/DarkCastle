#include "DC/Command.h"
#include "DC/character.h"
#include "DC/common.h"

QMap<QString, Command> Commands::qstring_command_map_ = {};
QMap<cmd_t, Command> Commands::cmd_t_command_map_ = {};

// The last integer will affect a char being removed from hide when they perform the command.
// 0  - char will always become visibile.
// 1  - char will not become visible when using this command.
// 2+ - char has a greater chance of breaking hide as this increases.
const QList<Command> Commands::commands_ =
    {
        // Movement commands
        Command(QStringLiteral("north"), do_move, position_t::STANDING, 0, cmd_t::NORTH, true, 1, CommandType::all),
        Command(QStringLiteral("east"), do_move, position_t::STANDING, 0, cmd_t::EAST, true, 1, CommandType::all),
        Command(QStringLiteral("south"), do_move, position_t::STANDING, 0, cmd_t::SOUTH, true, 1, CommandType::all),
        Command(QStringLiteral("west"), do_move, position_t::STANDING, 0, cmd_t::WEST, true, 1, CommandType::all),
        Command(QStringLiteral("up"), do_move, position_t::STANDING, 0, cmd_t::UP, true, 1, CommandType::all),
        Command(QStringLiteral("down"), do_move, position_t::STANDING, 0, cmd_t::DOWN, true, 1, CommandType::all),

        // Common commands
        Command(QStringLiteral("newbie"), do_newbie, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("cast"), do_cast, position_t::SITTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("filter"), do_cast, position_t::SITTING, 0, cmd_t::FILTER, 0, 0, CommandType::all),
        Command(QStringLiteral("sing"), do_sing, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("exits"), do_exits, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("f"), do_fire, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(QStringLiteral("get"), do_get, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("inventory"), do_inventory, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("k"), do_kill, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("ki"), do_ki, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("kill"), do_kill, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("look"), do_look, position_t::RESTING, 0, cmd_t::LOOK, true, 1, CommandType::all),
        Command(QStringLiteral("loot"), do_get, position_t::RESTING, 0, cmd_t::LOOT, 0, 0, CommandType::all),
        Command(QStringLiteral("glance"), do_look, position_t::RESTING, 0, cmd_t::GLANCE, true, 1, CommandType::all),
        Command(QStringLiteral("order"), do_order, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("rest"), do_rest, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("recite"), do_recite, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("recall"), &Character::do_recall, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("score"), do_score, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("scan"), do_scan, position_t::RESTING, 1, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(QStringLiteral("stand"), do_stand, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("switch"), do_switch, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(QStringLiteral("tell"), &Character::do_tell, position_t::RESTING, 0, cmd_t::TELL, 0, 1, CommandType::all),
        Command(QStringLiteral("tellhistory"), do_tellhistory, position_t::RESTING, 0, cmd_t::TELLH, 0, 1, CommandType::all),
        Command(QStringLiteral("wield"), do_wield, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("innate"), do_innate, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("orchestrate"), do_sing, position_t::RESTING, 0, cmd_t::ORCHESTRATE, 0, 0, CommandType::all),

        // Informational commands
        Command(QStringLiteral("alias"), &Character::do_alias, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(QStringLiteral("toggle"), &Character::do_toggle, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("consider"), do_consider, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("configure"), &Character::do_config, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(QStringLiteral("credits"), do_credits, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("equipment"), do_equipment, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("ohelp"), do_help, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("help"), do_new_help, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("idea"), do_idea, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("info"), do_info, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("leaderboard"), do_leaderboard, position_t::DEAD, 3, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("news"), do_news, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("thenews"), do_news, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("story"), do_story, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("tick"), do_tick, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("time"), do_time, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("title"), do_title, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("notitle"), &Character::do_notitle, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("typo"), do_typo, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("weather"), do_weather, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("who"), &Character::do_who, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("wizlist"), do_wizlist, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("socials"), do_social, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("index"), do_index, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("areas"), do_areas, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("commands"), do_new_help, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("experience"), &Character::do_experience, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(QStringLiteral("version"), do_version, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("identify"), &Character::do_identify, position_t::DEAD, 0, cmd_t::DEFAULT, true, 0, CommandType::all),

        // Communication commands
        Command(QStringLiteral("ask"), do_ask, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("auction"), &Character::do_auction, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("awaymsgs"), do_awaymsgs, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("channel"), do_channel, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("dream"), do_dream, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("emote"), do_emote, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral(":"), do_emote, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("gossip"), do_gossip, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("trivia"), do_trivia, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("gtell"), do_grouptell, position_t::DEAD, 0, cmd_t::GTELL, 0, 1, CommandType::all),
        Command(QStringLiteral("."), do_grouptell, position_t::DEAD, 0, cmd_t::GTELL, 0, 1, CommandType::all),
        Command(QStringLiteral("ignore"), do_ignore, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("insult"), do_insult, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("reply"), do_reply, position_t::RESTING, 0, cmd_t::REPLY, 0, 1, CommandType::all),
        Command(QStringLiteral("report"), do_report, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("say"), do_say, position_t::RESTING, 0, cmd_t::SAY, true, 0, CommandType::all),
        Command(QStringLiteral("psay"), do_psay, position_t::RESTING, 0, cmd_t::SAY, true, 0, CommandType::all),
        Command(QStringLiteral("'"), do_say, position_t::RESTING, 0, cmd_t::SAY, true, 0, CommandType::all),
        Command(QStringLiteral("shout"), do_shout, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("whisper"), do_whisper, position_t::RESTING, 0, cmd_t::WHISPER, 0, 0, CommandType::all),

        // Object manipulation
        Command(QStringLiteral("slip"), do_slip, position_t::STANDING, 0, cmd_t::SLIP, 0, 1, CommandType::all),
        Command(QStringLiteral("batter"), do_batter, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("brace"), do_brace, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("close"), do_close, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("donate"), do_donate, position_t::RESTING, 0, cmd_t::DONATE, true, 1, CommandType::all),
        Command(QStringLiteral("drink"), do_drink, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("drop"), do_drop, position_t::RESTING, 0, cmd_t::DROP, true, 25, CommandType::all),
        Command(QStringLiteral("eat"), do_eat, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("fill"), do_fill, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("give"), &Character::do_give, position_t::RESTING, 0, cmd_t::GIVE, true, 25, CommandType::all),
        Command(QStringLiteral("grab"), do_grab, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("hold"), do_grab, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("lock"), do_lock, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("open"), do_open, position_t::RESTING, 0, cmd_t::OPEN, true, 25, CommandType::all),
        Command(QStringLiteral("pour"), do_pour, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("put"), do_put, position_t::RESTING, 0, cmd_t::PUT, true, 0, CommandType::all),
        Command(QStringLiteral("quaff"), do_quaff, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("read"), do_read, position_t::RESTING, 0, cmd_t::READ, 0, 1, CommandType::all),
        Command(QStringLiteral("remove"), do_remove, position_t::RESTING, 0, cmd_t::REMOVE, true, 25, CommandType::all),
        Command(QStringLiteral("erase"), &Character::generic_command, position_t::RESTING, 0, cmd_t::ERASE, 0, 0, CommandType::all),
        Command(QStringLiteral("sip"), do_sip, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("track"), &Character::do_track, position_t::STANDING, 0, cmd_t::TRACK, 0, 10, CommandType::all),
        Command(QStringLiteral("take"), do_get, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("palm"), do_get, position_t::RESTING, 3, cmd_t::PALM, 0, 1, CommandType::all),
        Command(QStringLiteral("sacrifice"), do_sacrifice, position_t::RESTING, 0, cmd_t::SACRIFICE, true, 25, CommandType::all),
        Command(QStringLiteral("taste"), do_taste, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("unlock"), do_unlock, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("use"), do_use, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("wear"), do_wear, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(QStringLiteral("scribe"), do_scribe, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("brew"), do_brew, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        //  { "poisonmaking"), do_poisonmaking, nullptr,nullptr, position_t::RESTING, 0, 9,   0, 0  ,CommandType::all),

        // Combat commands
        Command(QStringLiteral("bash"), do_bash, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("retreat"), do_retreat, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("disarm"), do_disarm, position_t::FIGHTING, 2, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("flee"), do_flee, position_t::FIGHTING, 0, cmd_t::FLEE, true, 0, CommandType::all),
        Command(QStringLiteral("escape"), do_flee, position_t::FIGHTING, 0, cmd_t::ESCAPE, 0, 0, CommandType::all),
        Command(QStringLiteral("hit"), &Character::do_hit, position_t::FIGHTING, 0, cmd_t::HIT, true, 0, CommandType::all),
        Command(QStringLiteral("join"), &Character::do_join, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("battlesense"), do_battlesense, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("stance"), do_defenders_stance, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("perseverance"), do_perseverance, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("smite"), do_smite, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // Junk movedso join precedes it
        Command(QStringLiteral("junk"), do_sacrifice, position_t::RESTING, 0, cmd_t::SACRIFICE, true, 25, CommandType::all),

        Command(QStringLiteral("murder"), do_murder, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("rescue"), &Character::do_rescue, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("trip"), do_trip, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("deathstroke"), do_deathstroke, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("circle"), do_circle, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("kick"), &Character::do_kick, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("battlecry"), do_battlecry, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("behead"), do_behead, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("rage"), &Character::do_rage, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("berserk"), do_berserk, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("golemscore"), do_golem_score, position_t::DEAD, 1, cmd_t::GOLEMSCORE, 0, 1, CommandType::players_only),
        Command(QStringLiteral("fscore"), do_golem_score, position_t::DEAD, 1, cmd_t::FSCORE, 0, 1, CommandType::players_only),
        Command(QStringLiteral("stun"), do_stun, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("redirect"), do_redirect, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("hitall"), do_hitall, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("quiveringpalm"), do_quivering_palm, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("eagleclaw"), do_eagle_claw, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("headbutt"), do_headbutt, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("cripple"), do_cripple, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("fire"), do_fire, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(QStringLiteral("layhands"), do_layhands, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("harmtouch"), do_harmtouch, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("bloodfury"), do_bloodfury, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("primalfury"), do_primalfury, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("bladeshield"), do_bladeshield, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("repelance"), do_focused_repelance, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("vitalstrike"), do_vitalstrike, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("crazedassault"), do_crazedassault, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("bullrush"), do_bullrush, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("ferocity"), do_ferocity, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("tactics"), do_tactics, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("deceit"), do_deceit, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("knockback"), do_knockback, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("appraise"), do_appraise, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("make camp"), do_make_camp, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("leadership"), do_leadership, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("triage"), do_triage, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("onslaught"), do_onslaught, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("pursue"), do_pursue, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // Position commands
        Command(QStringLiteral("sit"), do_sit, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(QStringLiteral("sleep"), do_sleep, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("wake"), &Character::do_wake, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // Miscellaneous commands
        Command(QStringLiteral("editor"), do_editor, position_t::SLEEPING, 100, cmd_t::EDITOR, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("autojoin"), do_autojoin, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("visible"), do_visible, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("ctell"), do_ctell, position_t::SLEEPING, 0, cmd_t::CTELL, 0, 1, CommandType::all),
        Command(QStringLiteral("outcast"), &Character::do_outcast, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("accept"), do_accept, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("whoclan"), do_whoclan, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("cpromote"), do_cpromote, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("clans"), do_clans, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("clanarea"), &Character::do_clanarea, position_t::RESTING, 11, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("cinfo"), do_cinfo, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("ambush"), &Character::do_ambush, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("whoarena"), do_whoarena, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("joinarena"), do_joinarena, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("backstab"), &Character::do_backstab, position_t::STANDING, 0, cmd_t::BACKSTAB, 0, 0, CommandType::all),
        Command(QStringLiteral("bs"), &Character::do_backstab, position_t::STANDING, 0, cmd_t::BACKSTAB, 0, 0, CommandType::all),
        Command(QStringLiteral("sbs"), &Character::do_backstab, position_t::STANDING, 0, cmd_t::SBS, 0, 0, CommandType::all), // single backstab
        Command(QStringLiteral("boss"), do_boss, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("jab"), do_jab, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("enter"), do_enter, position_t::STANDING, 0, cmd_t::ENTER, true, 20, CommandType::all),
        Command(QStringLiteral("climb"), do_climb, position_t::STANDING, 0, cmd_t::CLIMB, true, 20, CommandType::all),
        Command(QStringLiteral("examine"), do_examine, position_t::RESTING, 0, cmd_t::EXAMINE, 0, 1, CommandType::all),
        Command(QStringLiteral("follow"), do_follow, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("stalk"), do_stalk, position_t::STANDING, 6, cmd_t::DEFAULT, 0, 10, CommandType::all),
        Command(QStringLiteral("group"), do_group, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("found"), do_found, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("disband"), do_disband, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("abandon"), do_abandon, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("consent"), do_consent, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("whogroup"), do_whogroup, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("forage"), do_forage, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("whosolo"), do_whosolo, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("count"), do_count, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("hide"), do_hide, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("leave"), do_leave, position_t::STANDING, 0, cmd_t::LEAVE, true, 20, CommandType::all),
        Command(QStringLiteral("name"), do_name, position_t::DEAD, 1, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("pick"), do_pick, position_t::STANDING, 0, cmd_t::PICK, 0, 20, CommandType::all),
        Command(QStringLiteral("quest"), do_quest, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(QStringLiteral("qui"), do_qui, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("levels"), do_levels, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("quit"), do_quit, position_t::DEAD, 0, cmd_t::QUIT, 0, 1, CommandType::all),
        Command(QStringLiteral("return"), do_return, position_t::DEAD, 0, cmd_t::DEFAULT, true, 1, CommandType::all),
        Command(QStringLiteral("tame"), do_tame, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("free animal"), do_free_animal, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("prompt"), do_prompt, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("lastprompt"), do_lastprompt, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("save"), &Character::do_save, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("sneak"), do_sneak, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("home"), do_home, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("split"), &Character::do_split, position_t::RESTING, 0, cmd_t::SPLIT, 0, 0, CommandType::all),
        Command(QStringLiteral("spells"), do_spells, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("skills"), do_skills, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("songs"), do_songs, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("steal"), do_steal, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 10, CommandType::all),
        Command(QStringLiteral("pocket"), do_pocket, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 10, CommandType::all),
        Command(QStringLiteral("motd"), do_motd, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("cmotd"), do_cmotd, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("cbalance"), do_cbalance, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("cdeposit"), &Character::do_cdeposit, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("cwithdraw"), do_cwithdraw, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("ctax"), do_ctax, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("where"), do_where, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("write"), do_write, position_t::STANDING, 0, cmd_t::WRITE, 0, 0, CommandType::all),
        Command(QStringLiteral("beacon"), do_beacon, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("beep"), do_beep, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("guard"), do_guard, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("release"), do_release, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(QStringLiteral("eyegouge"), do_eyegouge, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("vault"), do_vault, position_t::DEAD, 10, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("suicide"), do_suicide, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("vote"), do_vote, position_t::RESTING, 0, cmd_t::VOTE, 0, 0, CommandType::all),
        Command(QStringLiteral("huntitems"), do_showhunt, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("random"), do_random, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(QStringLiteral("arena"), &Character::do_arena, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::players_only),
        Command(QStringLiteral("search"), &Character::do_search, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(QStringLiteral("vend"), do_vend, position_t::STANDING, 2, cmd_t::VEND, 0, 0, CommandType::players_only),
        Command(QStringLiteral("practice"), do_practice, position_t::SLEEPING, 1, cmd_t::PRACTICE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("practise"), do_practice, position_t::SLEEPING, 1, cmd_t::PRACTICE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("pray"), do_pray, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(QStringLiteral("profession"), do_profession, position_t::SLEEPING, 1, cmd_t::PROFESSION, 0, 0, CommandType::players_only),
        Command(QStringLiteral("promote"), do_promote, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::players_only),
        Command(QStringLiteral("assemble"), do_assemble, position_t::RESTING, 0, cmd_t::ASSEMBLE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("select"), do_natural_selection, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::players_only),
        Command(QStringLiteral("sector"), do_sector, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),

        // Special procedure commands
        Command(QStringLiteral("gag"), &Character::generic_command, position_t::STANDING, 0, cmd_t::GAG, 0, 0, CommandType::players_only),
        Command(QStringLiteral("design"), &Character::generic_command, position_t::STANDING, 0, cmd_t::DESIGN, 0, 0, CommandType::players_only),
        Command(QStringLiteral("stock"), &Character::generic_command, position_t::STANDING, 0, cmd_t::STOCK, 0, 0, CommandType::players_only),
        Command(QStringLiteral("buy"), &Character::generic_command, position_t::STANDING, 0, cmd_t::BUY, 0, 0, CommandType::players_only),
        Command(QStringLiteral("sell"), &Character::generic_command, position_t::STANDING, 0, cmd_t::SELL, 0, 0, CommandType::players_only),
        Command(QStringLiteral("value"), &Character::generic_command, position_t::STANDING, 0, cmd_t::VALUE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("watch"), &Character::generic_command, position_t::STANDING, 0, cmd_t::WATCH, 0, 0, CommandType::players_only),
        Command(QStringLiteral("list"), &Character::generic_command, position_t::STANDING, 0, cmd_t::LIST, 0, 0, CommandType::players_only),
        Command(QStringLiteral("estimate"), &Character::generic_command, position_t::STANDING, 0, cmd_t::ESTIMATE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("repair"), &Character::generic_command, position_t::STANDING, 0, cmd_t::REPAIR, 0, 0, CommandType::players_only),
        Command(QStringLiteral("price"), &Character::generic_command, position_t::RESTING, 1, cmd_t::PRICE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("train"), &Character::generic_command, position_t::RESTING, 1, cmd_t::TRAIN, 0, 0, CommandType::players_only),
        Command(QStringLiteral("gain"), &Character::generic_command, position_t::STANDING, 1, cmd_t::GAIN, 0, 0, CommandType::players_only),
        Command(QStringLiteral("balance"), &Character::generic_command, position_t::STANDING, 0, cmd_t::BALANCE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("deposit"), &Character::generic_command, position_t::STANDING, 0, cmd_t::DEPOSIT, 0, 0, CommandType::players_only),
        Command(QStringLiteral("withdraw"), &Character::generic_command, position_t::STANDING, 0, cmd_t::WITHDRAW, 0, 0, CommandType::players_only),
        Command(QStringLiteral("clean"), &Character::generic_command, position_t::RESTING, 0, cmd_t::CLEAN, 0, 0, CommandType::players_only),
        Command(QStringLiteral("play"), &Character::generic_command, position_t::RESTING, 0, cmd_t::PLAY, 0, 0, CommandType::players_only),
        Command(QStringLiteral("finish"), &Character::generic_command, position_t::RESTING, 0, cmd_t::FINISH, 0, 0, CommandType::players_only),
        Command(QStringLiteral("veternarian"), &Character::generic_command, position_t::RESTING, 0, cmd_t::VETERNARIAN, 0, 0, CommandType::players_only),
        Command(QStringLiteral("feed"), &Character::generic_command, position_t::RESTING, 0, cmd_t::FEED, 0, 0, CommandType::players_only),
        Command(QStringLiteral("pay"), &Character::generic_command, position_t::STANDING, 0, cmd_t::PAY, 0, 0, CommandType::players_only),
        Command(QStringLiteral("restring"), &Character::generic_command, position_t::STANDING, 0, cmd_t::RESTRING, 0, 0, CommandType::players_only),
        Command(QStringLiteral("push"), &Character::generic_command, position_t::STANDING, 0, cmd_t::PUSH, 0, 0, CommandType::players_only),
        Command(QStringLiteral("pull"), &Character::generic_command, position_t::STANDING, 0, cmd_t::PULL, 0, 0, CommandType::players_only),
        Command(QStringLiteral("gaze"), &Character::generic_command, position_t::FIGHTING, 0, cmd_t::GAZE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("tremor"), &Character::generic_command, position_t::FIGHTING, 0, cmd_t::TREMOR, 0, 0, CommandType::players_only),
        Command(QStringLiteral("bet"), &Character::generic_command, position_t::STANDING, 0, cmd_t::BET, 0, 0, CommandType::players_only),
        Command(QStringLiteral("insurance"), &Character::generic_command, position_t::STANDING, 0, cmd_t::INSURANCE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("double"), &Character::generic_command, position_t::STANDING, 0, cmd_t::DOUBLE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("stay"), &Character::generic_command, position_t::STANDING, 0, cmd_t::STAY, 0, 0, CommandType::players_only),
        Command(QStringLiteral("remort"), &Character::generic_command, position_t::RESTING, GIFTED_COMMAND, cmd_t::REMORT, 0, 1, CommandType::players_only),
        Command(QStringLiteral("reroll"), &Character::generic_command, position_t::RESTING, 0, cmd_t::REROLL, 0, 0, CommandType::players_only),
        Command(QStringLiteral("choose"), &Character::generic_command, position_t::RESTING, 0, cmd_t::CHOOSE, 0, 0, CommandType::players_only),
        Command(QStringLiteral("confirm"), &Character::generic_command, position_t::RESTING, 0, cmd_t::CONFIRM, 0, 0, CommandType::players_only),
        Command(QStringLiteral("cancel"), &Character::generic_command, position_t::RESTING, 0, cmd_t::CANCEL, 0, 0, CommandType::players_only),
        Command(QStringLiteral("redeem"), &Character::generic_command, position_t::RESTING, 0, cmd_t::REDEEM, 0, 0, CommandType::players_only),

        // Immortal commands
        Command(QStringLiteral("voteset"), do_setvote, position_t::DEAD, 108, cmd_t::SETVOTE, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("thunder"), do_thunder, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("wizlock"), do_wizlock, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("processes"), do_processes, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("bestow"), &Character::do_bestow, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("oclone"), do_oclone, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("mclone"), do_mclone, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("huntclear"), do_huntclear, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("areastats"), do_areastats, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("huntstart"), do_huntstart, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("revoke"), do_revoke, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("chpwd"), do_chpwd, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("advance"), do_advance, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("skillmax"), do_maxes, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("damage"), do_dmg_eq, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("affclear"), do_clearaff, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("guide"), &Character::do_guide, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("addnews"), do_addnews, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("linkload"), &Character::do_linkload, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("listproc"), do_listproc, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("zap"), &Character::do_zap, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("slay"), do_slay, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("rename"), &Character::do_rename_char, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("archive"), do_archive, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("unarchive"), do_unarchive, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("stealth"), do_stealth, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("disconnect"), do_disconnect, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("force"), do_force, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("pardon"), do_pardon, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("goto"), &Character::do_goto, position_t::DEAD, 102, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("restore"), do_restore, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("purloin"), do_purloin, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("set"), do_set, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("unban"), do_unban, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("ban"), do_ban, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("echo"), do_echo, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("eqmax"), do_eqmax, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("send"), do_send, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("at"), do_at, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("fakelog"), do_fakelog, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("global"), do_global, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("log"), do_log, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("snoop"), &Character::do_snoop, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("pview"), do_pview, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("load"), do_load, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("prize"), do_load, position_t::DEAD, GIFTED_COMMAND, cmd_t::PRIZE, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("test"), &Character::do_test, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("testport"), do_testport, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("testuser"), do_testuser, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("shutdow"), &Character::do_shutdow, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("shutdown"), &Character::do_shutdown, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("opedit"), do_opedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("opstat"), do_opstat, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("procedit"), do_procedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("procstat"), do_mpstat, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("range"), do_range, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        // { "pshopedit",	do_pshopedit,	position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1  ,CommandType::immortals_only),
        Command(QStringLiteral("sedit"), do_sedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("sockets"), &Character::do_sockets, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("punish"), do_punish, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("sqedit"), do_sqedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("qedit"), do_qedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("install"), do_install, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        //  { "motdload",       do_motdload,    position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1  ,CommandType::immortals_only),
        Command(QStringLiteral("hedit"), do_hedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("hindex"), do_hindex, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("reload"), do_reload, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("plats"), do_plats, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("bellow"), do_thunder, position_t::DEAD, DEITY, cmd_t::BELLOW, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("string"), do_string, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("transfer"), do_transfer, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("gtrans"), do_gtrans, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("boot"), do_boot, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("linkdead"), do_linkdead, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("teleport"), do_teleport, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("purge"), do_purge, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("show"), do_show, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("fighting"), do_fighting, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("peace"), do_peace, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("check"), do_check, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("zoneexits"), do_zoneexits, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("find"), do_find, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("stat"), do_stat, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("redit"), do_redit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("guild"), do_guild, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("oedit"), &Character::do_oedit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("clear"), do_clear, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("repop"), do_repop, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("medit"), do_medit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("rdelete"), do_rdelete, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("oneway"), do_oneway, position_t::DEAD, ANGEL, cmd_t::ONEWAY, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("twoway"), do_oneway, position_t::DEAD, ANGEL, cmd_t::TWOWAY, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("zsave"), &Character::do_zsave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("rsave"), do_rsave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("msave"), do_msave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("osave"), do_osave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("rstat"), do_rstat, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("possess"), do_possess, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("fsave"), do_fsave, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("zedit"), do_zedit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("colors"), do_colors, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("colours"), do_colors, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("incognito"), do_incognito, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("high5"), do_highfive, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("holylite"), do_holylite, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("immort"), do_wiz, position_t::DEAD, IMMORTAL, cmd_t::IMMORT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral(";"), do_wiz, position_t::DEAD, IMMORTAL, cmd_t::IMMORT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("impchan"), do_wiz, position_t::DEAD, GIFTED_COMMAND, cmd_t::IMPCHAN, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("/"), do_wiz, position_t::DEAD, GIFTED_COMMAND, cmd_t::IMPCHAN, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("nohassle"), do_nohassle, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("wizinvis"), do_wizinvis, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("poof"), do_poof, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("wizhelp"), &Character::do_wizhelp, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("imotd"), do_imotd, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("mhelp"), do_mortal_help, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("testhand"), do_testhand, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("varstat"), do_varstat, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("matrixinfo"), do_matrixinfo, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("maxcheck"), do_findfix, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("export"), do_export, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("mscore"), do_mscore, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("world"), do_world, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),

        // Special procedure commands placed to not disrupt god commands
        Command(QStringLiteral("setup"), do_mortal_set, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("metastat"), do_metastat, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("testhit"), do_testhit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("acfinder"), do_acfinder, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("findpath"), do_findPath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("findpath2"), do_findpath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("addroom"), do_addRoom, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("newpath"), do_newPath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("listpathsbyzone"), do_listPathsByZone, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("listallpaths"), do_listAllPaths, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("dopathpath"), do_pathpath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("botcheck"), &Character::do_botcheck, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("showbits"), do_showbits, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(QStringLiteral("debug"), do_debug, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),

        // Bug way down here after 'buy'
        Command(QStringLiteral("bug"), do_bug, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        // imbue after 'im' for us lazy immortal types :)
        Command(QStringLiteral("imbue"), do_imbue, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // MOBprogram commands
        Command(QStringLiteral("mpasound"), do_mpasound, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpbestow"), do_mpbestow, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpjunk"), do_mpjunk, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpecho"), do_mpecho, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpechoat"), do_mpechoat, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpechoaround"), do_mpechoaround, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpechoaroundnotbad"), do_mpechoaroundnotbad, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpkill"), do_mpkill, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mphit"), do_mphit, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpaddlag"), do_mpaddlag, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpmload"), do_mpmload, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpoload"), do_mpoload, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mppurge"), do_mppurge, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpgoto"), do_mpgoto, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpat"), do_mpat, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mptransfer"), do_mptransfer, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpthrow"), do_mpthrow, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpforce"), do_mpforce, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mppeace"), do_mppeace, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpsetalign"), do_mpsetalign, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpsettemp"), &Character::do_mpsettemp, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpxpreward"), do_mpxpreward, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpteachskill"), do_mpteachskill, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),

        Command(QStringLiteral("mpdamage"), do_mpdamage, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpothrow"), do_mpothrow, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mppause"), do_mppause, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpretval"), do_mpretval, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpsetmath"), do_mpsetmath, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(QStringLiteral("mpteleport"), do_mpteleport, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),

        // End of the line
        Command(QStringLiteral(""), &Character::generic_command, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only)};

auto Commands::find(QString arg) -> std::expected<Command, search_error>
{
        if (qstring_command_map_.contains(arg))
        {
                return qstring_command_map_.value(arg);
        }

        return std::unexpected(search_error::not_found);
}

auto Commands::find(cmd_t cmd) -> std::expected<Command, search_error>
{
        if (cmd_t_command_map_.contains(cmd))
        {
                return cmd_t_command_map_.value(cmd);
        }

        return std::unexpected(search_error::not_found);
}

Commands::Commands(void)
{
        for (const auto &command : commands_)
        {
                qstring_command_map_[command.getName()] = command;
                for (qsizetype position = 1; position < command.getName().length(); position++)
                {
                        auto keyword = command.getName();
                        keyword.truncate(position);
                        if (!qstring_command_map_.contains(keyword))
                        {
                                qstring_command_map_[keyword] = command;
                        }
                }

                if (command.getNumber() != cmd_t::DEFAULT &&
                    command.getNumber() != cmd_t::UNDEFINED)
                {
                        if (!cmd_t_command_map_.contains(command.getNumber()))
                        {
                                cmd_t_command_map_[command.getNumber()] = command;
                        }
                }
        }
}

bool isCommandTypeDirection(cmd_t cmd)
{
        switch (cmd)
        {
        case cmd_t::NORTH:
        case cmd_t::EAST:
        case cmd_t::SOUTH:
        case cmd_t::WEST:
        case cmd_t::UP:
        case cmd_t::DOWN:
                return true;
        default:
                break;
        }
        return false;
}

bool isCommandTypeCasino(cmd_t cmd)
{

        switch (cmd)
        {
        case cmd_t::BET:
        case cmd_t::INSURANCE:
        case cmd_t::DOUBLE:
        case cmd_t::STAY:
        case cmd_t::SPLIT:
        case cmd_t::HIT:
                return true;
                break;
        default:
                break;
        }
        return false;
}

auto getCommandFromDirection(int dir) -> std::expected<cmd_t, bool>
{
        switch (dir)
        {
        case NORTH:
                return cmd_t::NORTH;
                break;
        case EAST:
                return cmd_t::EAST;
                break;
        case SOUTH:
                return cmd_t::SOUTH;
                break;
        case WEST:
                return cmd_t::WEST;
                break;
        case UP:
                return cmd_t::UP;
                break;
        case DOWN:
                return cmd_t::DOWN;
                break;
        default:
                break;
        }
        return std::unexpected(false);
}

auto getDirectionFromCommand(cmd_t cmd) -> std::expected<int, bool>
{
        switch (cmd)
        {
        case cmd_t::NORTH:
                return NORTH;
                break;
        case cmd_t::EAST:
                return EAST;
                break;
        case cmd_t::SOUTH:
                return SOUTH;
                break;
        case cmd_t::WEST:
                return WEST;
                break;
        case cmd_t::UP:
                return UP;
                break;
        case cmd_t::DOWN:
                return DOWN;
                break;
        default:
                break;
        }
        return std::unexpected(false);
}