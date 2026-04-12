#include "DC/Command.h"
#include "DC/DC.h"
#include "DC/levels.h"
#include <QMap>
#include <QString>
#include <DC/Direction.h>
QMap<QString, Command> Commands::qstring_command_map_ = {};
QMap<cmd_t, Command> Commands::cmd_t_command_map_ = {};

// The last integer will affect a character being removed from hide when they perform the command.
// 0  - character will always become visibile.
// 1  - character will not become visible when using this command.
// 2+ - character has a greater chance of breaking hide as this increases.
QList<Command> Commands::commands_ =
    {
        // Movement commands
        Command(u"north"_s, do_move, position_t::STANDING, 0, cmd_t::NORTH, true, 1, CommandType::all),
        Command(u"east"_s, do_move, position_t::STANDING, 0, cmd_t::EAST, true, 1, CommandType::all),
        Command(u"south"_s, do_move, position_t::STANDING, 0, cmd_t::SOUTH, true, 1, CommandType::all),
        Command(u"west"_s, do_move, position_t::STANDING, 0, cmd_t::WEST, true, 1, CommandType::all),
        Command(u"up"_s, do_move, position_t::STANDING, 0, cmd_t::UP, true, 1, CommandType::all),
        Command(u"down"_s, do_move, position_t::STANDING, 0, cmd_t::DOWN, true, 1, CommandType::all),

        // Common commands
        Command(u"newbie"_s, do_newbie, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"cast"_s, do_cast, position_t::SITTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"filter"_s, do_cast, position_t::SITTING, 0, cmd_t::FILTER, 0, 0, CommandType::all),
        Command(u"sing"_s, do_sing, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"exits"_s, do_exits, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"f"_s, do_fire, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(u"get"_s, do_get, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"inventory"_s, do_inventory, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"k"_s, do_kill, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"ki"_s, do_ki, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"kill"_s, do_kill, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"look"_s, do_look, position_t::RESTING, 0, cmd_t::LOOK, true, 1, CommandType::all),
        Command(u"loot"_s, do_get, position_t::RESTING, 0, cmd_t::LOOT, 0, 0, CommandType::all),
        Command(u"glance"_s, do_look, position_t::RESTING, 0, cmd_t::GLANCE, true, 1, CommandType::all),
        Command(u"order"_s, do_order, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"rest"_s, do_rest, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"recite"_s, do_recite, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"recall"_s, &Character::do_recall, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"score"_s, do_score, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"scan"_s, do_scan, position_t::RESTING, 1, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(u"stand"_s, do_stand, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"switch"_s, do_switch, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(u"tell"_s, &Character::do_tell, position_t::RESTING, 0, cmd_t::TELL, 0, 1, CommandType::all),
        Command(u"tellhistory"_s, do_tellhistory, position_t::RESTING, 0, cmd_t::TELLH, 0, 1, CommandType::all),
        Command(u"wield"_s, do_wield, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"innate"_s, do_innate, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"orchestrate"_s, do_sing, position_t::RESTING, 0, cmd_t::ORCHESTRATE, 0, 0, CommandType::all),

        // Informational commands
        Command(u"alias"_s, &Character::do_alias, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(u"toggle"_s, &Character::do_toggle, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"consider"_s, do_consider, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"configure"_s, &Character::do_config, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(u"credits"_s, do_credits, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"equipment"_s, do_equipment, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"ohelp"_s, do_help, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"help"_s, do_new_help, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"idea"_s, do_idea, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"info"_s, do_info, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"leaderboard"_s, do_leaderboard, position_t::DEAD, 3, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"news"_s, do_news, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"thenews"_s, do_news, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"story"_s, do_story, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"tick"_s, do_tick, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"time"_s, do_time, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"title"_s, do_title, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"notitle"_s, &Character::do_notitle, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"typo"_s, do_typo, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"weather"_s, do_weather, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"who"_s, &Character::do_who, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"wizlist"_s, do_wizlist, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"socials"_s, do_social, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"index"_s, do_index, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"areas"_s, do_areas, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"commands"_s, do_new_help, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"experience"_s, &Character::do_experience, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(u"version"_s, do_version, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"identify"_s, &Character::do_identify, position_t::DEAD, 0, cmd_t::DEFAULT, true, 0, CommandType::all),

        // Communication commands
        Command(u"ask"_s, do_ask, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"auction"_s, &Character::do_auction, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"awaymsgs"_s, do_awaymsgs, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"channel"_s, do_channel, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"dream"_s, do_dream, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"emote"_s, do_emote, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u":"_s, do_emote, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"gossip"_s, do_gossip, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"trivia"_s, do_trivia, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"gtell"_s, do_grouptell, position_t::DEAD, 0, cmd_t::GTELL, 0, 1, CommandType::all),
        Command(u"."_s, do_grouptell, position_t::DEAD, 0, cmd_t::GTELL, 0, 1, CommandType::all),
        Command(u"ignore"_s, do_ignore, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"insult"_s, do_insult, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"reply"_s, do_reply, position_t::RESTING, 0, cmd_t::REPLY, 0, 1, CommandType::all),
        Command(u"report"_s, do_report, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"say"_s, do_say, position_t::RESTING, 0, cmd_t::SAY, true, 0, CommandType::all),
        Command(u"psay"_s, do_psay, position_t::RESTING, 0, cmd_t::SAY, true, 0, CommandType::all),
        Command(u"'"_s, do_say, position_t::RESTING, 0, cmd_t::SAY, true, 0, CommandType::all),
        Command(u"shout"_s, do_shout, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"whisper"_s, do_whisper, position_t::RESTING, 0, cmd_t::WHISPER, 0, 0, CommandType::all),

        // Object manipulation
        Command(u"slip"_s, do_slip, position_t::STANDING, 0, cmd_t::SLIP, 0, 1, CommandType::all),
        Command(u"batter"_s, do_batter, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"brace"_s, do_brace, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"close"_s, do_close, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"donate"_s, do_donate, position_t::RESTING, 0, cmd_t::DONATE, true, 1, CommandType::all),
        Command(u"drink"_s, &Character::do_drink, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"drop"_s, do_drop, position_t::RESTING, 0, cmd_t::DROP, true, 25, CommandType::all),
        Command(u"eat"_s, &Character::do_eat, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"fill"_s, do_fill, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"give"_s, &Character::do_give, position_t::RESTING, 0, cmd_t::GIVE, true, 25, CommandType::all),
        Command(u"grab"_s, do_grab, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"hold"_s, do_grab, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"lock"_s, do_lock, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"open"_s, &Character::do_open, position_t::RESTING, 0, cmd_t::OPEN, true, 25, CommandType::all),
        Command(u"pour"_s, do_pour, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"put"_s, do_put, position_t::RESTING, 0, cmd_t::PUT, true, 0, CommandType::all),
        Command(u"quaff"_s, do_quaff, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"read"_s, do_read, position_t::RESTING, 0, cmd_t::READ, 0, 1, CommandType::all),
        Command(u"remove"_s, do_remove, position_t::RESTING, 0, cmd_t::REMOVE, true, 25, CommandType::all),
        Command(u"erase"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::ERASE, 0, 0, CommandType::all),
        Command(u"sip"_s, do_sip, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"track"_s, &Character::do_track, position_t::STANDING, 0, cmd_t::TRACK, 0, 10, CommandType::all),
        Command(u"take"_s, do_get, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"palm"_s, do_get, position_t::RESTING, 3, cmd_t::PALM, 0, 1, CommandType::all),
        Command(u"sacrifice"_s, do_sacrifice, position_t::RESTING, 0, cmd_t::SACRIFICE, true, 25, CommandType::all),
        Command(u"taste"_s, do_taste, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"unlock"_s, do_unlock, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"use"_s, do_use, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"wear"_s, do_wear, position_t::RESTING, 0, cmd_t::DEFAULT, true, 25, CommandType::all),
        Command(u"scribe"_s, do_scribe, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"brew"_s, do_brew, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        //  { "poisonmaking"), do_poisonmaking, nullptr,nullptr, position_t::RESTING, 0, 9,   0, 0  ,CommandType::all),

        // Combat commands
        Command(u"bash"_s, do_bash, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"retreat"_s, do_retreat, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"disarm"_s, do_disarm, position_t::FIGHTING, 2, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"flee"_s, do_flee, position_t::FIGHTING, 0, cmd_t::FLEE, true, 0, CommandType::all),
        Command(u"escape"_s, do_flee, position_t::FIGHTING, 0, cmd_t::ESCAPE, 0, 0, CommandType::all),
        Command(u"hit"_s, &Character::do_hit, position_t::FIGHTING, 0, cmd_t::HIT, true, 0, CommandType::all),
        Command(u"join"_s, &Character::do_join, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"battlesense"_s, do_battlesense, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"stance"_s, do_defenders_stance, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"perseverance"_s, do_perseverance, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"smite"_s, do_smite, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // Junk movedso join precedes it
        Command(u"junk"_s, do_sacrifice, position_t::RESTING, 0, cmd_t::SACRIFICE, true, 25, CommandType::all),

        Command(u"murder"_s, do_murder, position_t::FIGHTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"rescue"_s, &Character::do_rescue, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"trip"_s, do_trip, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"deathstroke"_s, do_deathstroke, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"circle"_s, do_circle, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"kick"_s, &Character::do_kick, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"battlecry"_s, do_battlecry, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"behead"_s, do_behead, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"rage"_s, &Character::do_rage, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"berserk"_s, do_berserk, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"golemscore"_s, do_golem_score, position_t::DEAD, 1, cmd_t::GOLEMSCORE, 0, 1, CommandType::players_only),
        Command(u"fscore"_s, do_golem_score, position_t::DEAD, 1, cmd_t::FSCORE, 0, 1, CommandType::players_only),
        Command(u"stun"_s, do_stun, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"redirect"_s, do_redirect, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"hitall"_s, do_hitall, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"quiveringpalm"_s, do_quivering_palm, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"eagleclaw"_s, do_eagle_claw, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"headbutt"_s, do_headbutt, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"cripple"_s, do_cripple, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"fire"_s, do_fire, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 25, CommandType::all),
        Command(u"layhands"_s, do_layhands, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"harmtouch"_s, do_harmtouch, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"bloodfury"_s, do_bloodfury, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"primalfury"_s, do_primalfury, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"bladeshield"_s, do_bladeshield, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"repelance"_s, do_focused_repelance, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"vitalstrike"_s, do_vitalstrike, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"crazedassault"_s, do_crazedassault, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"bullrush"_s, do_bullrush, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"ferocity"_s, do_ferocity, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"tactics"_s, do_tactics, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"deceit"_s, do_deceit, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"knockback"_s, do_knockback, position_t::FIGHTING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"appraise"_s, do_appraise, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"make camp"_s, do_make_camp, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"leadership"_s, do_leadership, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"triage"_s, do_triage, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"onslaught"_s, do_onslaught, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"pursue"_s, do_pursue, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // Position commands
        Command(u"sit"_s, do_sit, position_t::RESTING, 0, cmd_t::DEFAULT, true, 0, CommandType::all),
        Command(u"sleep"_s, do_sleep, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"wake"_s, &Character::do_wake, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // Miscellaneous commands
        Command(u"editor"_s, do_editor, position_t::SLEEPING, 100, cmd_t::EDITOR, 0, 1, CommandType::immortals_only),
        Command(u"autojoin"_s, do_autojoin, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"visible"_s, do_visible, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"ctell"_s, do_ctell, position_t::SLEEPING, 0, cmd_t::CTELL, 0, 1, CommandType::all),
        Command(u"outcast"_s, &Character::do_outcast, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"accept"_s, do_accept, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"whoclan"_s, do_whoclan, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"cpromote"_s, do_cpromote, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"clans"_s, do_clans, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"clanarea"_s, &Character::do_clanarea, position_t::RESTING, 11, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"cinfo"_s, do_cinfo, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"ambush"_s, &Character::do_ambush, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"whoarena"_s, do_whoarena, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"joinarena"_s, do_joinarena, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"backstab"_s, &Character::do_backstab, position_t::STANDING, 0, cmd_t::BACKSTAB, 0, 0, CommandType::all),
        Command(u"bs"_s, &Character::do_backstab, position_t::STANDING, 0, cmd_t::BACKSTAB, 0, 0, CommandType::all),
        Command(u"sbs"_s, &Character::do_backstab, position_t::STANDING, 0, cmd_t::SBS, 0, 0, CommandType::all), // single backstab
        Command(u"boss"_s, do_boss, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"jab"_s, do_jab, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"enter"_s, do_enter, position_t::STANDING, 0, cmd_t::ENTER, true, 20, CommandType::all),
        Command(u"climb"_s, do_climb, position_t::STANDING, 0, cmd_t::CLIMB, true, 20, CommandType::all),
        Command(u"examine"_s, do_examine, position_t::RESTING, 0, cmd_t::EXAMINE, 0, 1, CommandType::all),
        Command(u"follow"_s, do_follow, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"stalk"_s, do_stalk, position_t::STANDING, 6, cmd_t::DEFAULT, 0, 10, CommandType::all),
        Command(u"group"_s, do_group, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"found"_s, do_found, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"disband"_s, do_disband, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"abandon"_s, do_abandon, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"consent"_s, do_consent, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"whogroup"_s, do_whogroup, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"forage"_s, do_forage, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"whosolo"_s, do_whosolo, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"count"_s, do_count, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"hide"_s, do_hide, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"leave"_s, do_leave, position_t::STANDING, 0, cmd_t::LEAVE, true, 20, CommandType::all),
        Command(u"name"_s, do_name, position_t::DEAD, 1, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"pick"_s, do_pick, position_t::STANDING, 0, cmd_t::PICK, 0, 20, CommandType::all),
        Command(u"quest"_s, do_quest, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(u"qui"_s, do_qui, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"levels"_s, do_levels, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"quit"_s, do_quit, position_t::DEAD, 0, cmd_t::QUIT, 0, 1, CommandType::all),
        Command(u"return"_s, do_return, position_t::DEAD, 0, cmd_t::DEFAULT, true, 1, CommandType::all),
        Command(u"tame"_s, do_tame, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"free animal"_s, do_free_animal, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"prompt"_s, do_prompt, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"lastprompt"_s, do_lastprompt, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"save"_s, &Character::do_save, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"sneak"_s, do_sneak, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"home"_s, do_home, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"split"_s, &Character::do_split, position_t::RESTING, 0, cmd_t::SPLIT, 0, 0, CommandType::all),
        Command(u"spells"_s, do_spells, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"skills"_s, do_skills, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"songs"_s, do_songs, position_t::SLEEPING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"steal"_s, do_steal, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 10, CommandType::all),
        Command(u"pocket"_s, do_pocket, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 10, CommandType::all),
        Command(u"motd"_s, do_motd, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"cmotd"_s, do_cmotd, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"cbalance"_s, do_cbalance, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"cdeposit"_s, &Character::do_cdeposit, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"cwithdraw"_s, do_cwithdraw, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"ctax"_s, do_ctax, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"where"_s, do_where, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"write"_s, do_write, position_t::STANDING, 0, cmd_t::WRITE, 0, 0, CommandType::all),
        Command(u"beacon"_s, do_beacon, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"beep"_s, do_beep, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"guard"_s, do_guard, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"release"_s, do_release, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        Command(u"eyegouge"_s, do_eyegouge, position_t::FIGHTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"vault"_s, do_vault, position_t::DEAD, 10, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"suicide"_s, do_suicide, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"vote"_s, do_vote, position_t::RESTING, 0, cmd_t::VOTE, 0, 0, CommandType::all),
        Command(u"huntitems"_s, do_showhunt, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"random"_s, do_random, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),
        Command(u"arena"_s, &Character::do_arena, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 0, CommandType::players_only),
        Command(u"search"_s, &Character::do_search, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(u"vend"_s, do_vend, position_t::STANDING, 2, cmd_t::VEND, 0, 0, CommandType::players_only),
        Command(u"practice"_s, do_practice, position_t::SLEEPING, 1, cmd_t::PRACTICE, 0, 0, CommandType::players_only),
        Command(u"practise"_s, do_practice, position_t::SLEEPING, 1, cmd_t::PRACTICE, 0, 0, CommandType::players_only),
        Command(u"pray"_s, do_pray, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(u"profession"_s, do_profession, position_t::SLEEPING, 1, cmd_t::PROFESSION, 0, 0, CommandType::players_only),
        Command(u"promote"_s, do_promote, position_t::STANDING, 1, cmd_t::DEFAULT, 0, 0, CommandType::players_only),
        Command(u"assemble"_s, do_assemble, position_t::RESTING, 0, cmd_t::ASSEMBLE, 0, 0, CommandType::players_only),
        Command(u"select"_s, do_natural_selection, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 0, CommandType::players_only),
        Command(u"sector"_s, do_sector, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),
        Command(u"pets"_s, &Character::do_pets, position_t::RESTING, 0, cmd_t::DEFAULT, 0, 1, CommandType::players_only),

        // Special procedure commands
        Command(u"gag"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::GAG, 0, 0, CommandType::players_only),
        Command(u"design"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::DESIGN, 0, 0, CommandType::players_only),
        Command(u"stock"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::STOCK, 0, 0, CommandType::players_only),
        Command(u"buy"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::BUY, 0, 0, CommandType::players_only),
        Command(u"sell"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::SELL, 0, 0, CommandType::players_only),
        Command(u"value"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::VALUE, 0, 0, CommandType::players_only),
        Command(u"watch"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::WATCH, 0, 0, CommandType::players_only),
        Command(u"list"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::LIST, 0, 0, CommandType::players_only),
        Command(u"estimate"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::ESTIMATE, 0, 0, CommandType::players_only),
        Command(u"repair"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::REPAIR, 0, 0, CommandType::players_only),
        Command(u"price"_s, &Character::generic_command, position_t::RESTING, 1, cmd_t::PRICE, 0, 0, CommandType::players_only),
        Command(u"train"_s, &Character::generic_command, position_t::RESTING, 1, cmd_t::TRAIN, 0, 0, CommandType::players_only),
        Command(u"gain"_s, &Character::generic_command, position_t::STANDING, 1, cmd_t::GAIN, 0, 0, CommandType::players_only),
        Command(u"balance"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::BALANCE, 0, 0, CommandType::players_only),
        Command(u"deposit"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::DEPOSIT, 0, 0, CommandType::players_only),
        Command(u"withdraw"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::WITHDRAW, 0, 0, CommandType::players_only),
        Command(u"clean"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::CLEAN, 0, 0, CommandType::players_only),
        Command(u"play"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::PLAY, 0, 0, CommandType::players_only),
        Command(u"finish"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::FINISH, 0, 0, CommandType::players_only),
        Command(u"veternarian"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::VETERNARIAN, 0, 0, CommandType::players_only),
        Command(u"feed"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::FEED, 0, 0, CommandType::players_only),
        Command(u"pay"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::PAY, 0, 0, CommandType::players_only),
        Command(u"restring"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::RESTRING, 0, 0, CommandType::players_only),
        Command(u"push"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::PUSH, 0, 0, CommandType::players_only),
        Command(u"pull"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::PULL, 0, 0, CommandType::players_only),
        Command(u"gaze"_s, &Character::generic_command, position_t::FIGHTING, 0, cmd_t::GAZE, 0, 0, CommandType::players_only),
        Command(u"tremor"_s, &Character::generic_command, position_t::FIGHTING, 0, cmd_t::TREMOR, 0, 0, CommandType::players_only),
        Command(u"bet"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::BET, 0, 0, CommandType::players_only),
        Command(u"insurance"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::INSURANCE, 0, 0, CommandType::players_only),
        Command(u"double"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::DOUBLE, 0, 0, CommandType::players_only),
        Command(u"stay"_s, &Character::generic_command, position_t::STANDING, 0, cmd_t::STAY, 0, 0, CommandType::players_only),
        Command(u"remort"_s, &Character::generic_command, position_t::RESTING, GIFTED_COMMAND, cmd_t::REMORT, 0, 1, CommandType::players_only),
        Command(u"reroll"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::REROLL, 0, 0, CommandType::players_only),
        Command(u"choose"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::CHOOSE, 0, 0, CommandType::players_only),
        Command(u"confirm"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::CONFIRM, 0, 0, CommandType::players_only),
        Command(u"cancel"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::CANCEL, 0, 0, CommandType::players_only),
        Command(u"redeem"_s, &Character::generic_command, position_t::RESTING, 0, cmd_t::REDEEM, 0, 0, CommandType::players_only),

        // Immortal commands
        Command(u"voteset"_s, do_setvote, position_t::DEAD, 108, cmd_t::SETVOTE, 0, 1, CommandType::immortals_only),
        Command(u"thunder"_s, do_thunder, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"wizlock"_s, do_wizlock, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"processes"_s, do_processes, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"bestow"_s, &Character::do_bestow, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"oclone"_s, do_oclone, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"mclone"_s, do_mclone, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"huntclear"_s, do_huntclear, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"areastats"_s, do_areastats, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"huntstart"_s, do_huntstart, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"revoke"_s, do_revoke, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"chpwd"_s, do_chpwd, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"advance"_s, do_advance, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"skillmax"_s, do_maxes, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"damage"_s, do_dmg_eq, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"affclear"_s, do_clearaff, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"guide"_s, &Character::do_guide, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"addnews"_s, do_addnews, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"linkload"_s, &Character::do_linkload, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"listproc"_s, do_listproc, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"zap"_s, &Character::do_zap, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"slay"_s, do_slay, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"rename"_s, &Character::do_rename_char, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"archive"_s, do_archive, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"unarchive"_s, do_unarchive, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"stealth"_s, do_stealth, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"disconnect"_s, do_disconnect, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"force"_s, &Character::do_force, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"pardon"_s, do_pardon, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"goto"_s, &Character::do_goto, position_t::DEAD, 102, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"restore"_s, do_restore, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"purloin"_s, do_purloin, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"set"_s, do_set, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"unban"_s, &Character::do_unban, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"ban"_s, &Character::do_ban, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"echo"_s, do_echo, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"eqmax"_s, do_eqmax, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"send"_s, do_send, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"at"_s, do_at, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"fakelog"_s, do_fakelog, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"global"_s, do_global, position_t::DEAD, 108, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"log"_s, do_log, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"snoop"_s, &Character::do_snoop, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"pview"_s, &Character::do_pview, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"load"_s, do_load, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"prize"_s, do_load, position_t::DEAD, GIFTED_COMMAND, cmd_t::PRIZE, 0, 1, CommandType::immortals_only),
        Command(u"test"_s, &Character::do_test, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"testport"_s, do_testport, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"testuser"_s, do_testuser, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"shutdow"_s, &Character::do_shutdow, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"shutdown"_s, &Character::do_shutdown, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"opedit"_s, do_opedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"opstat"_s, do_opstat, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"procedit"_s, do_procedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"procstat"_s, do_mpstat, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"range"_s, do_range, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        // { "pshopedit",	do_pshopedit,	position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1  ,CommandType::immortals_only),
        Command(u"sedit"_s, do_sedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"sockets"_s, &Character::do_sockets, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"punish"_s, do_punish, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"sqedit"_s, do_sqedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"qedit"_s, do_qedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"install"_s, do_install, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        //  { "motdload",       do_motdload,    position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1  ,CommandType::immortals_only),
        Command(u"hedit"_s, do_hedit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"hindex"_s, do_hindex, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"reload"_s, do_reload, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"plats"_s, do_plats, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"bellow"_s, do_thunder, position_t::DEAD, DEITY, cmd_t::BELLOW, 0, 1, CommandType::immortals_only),
        Command(u"string"_s, do_string, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"transfer"_s, do_transfer, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"gtrans"_s, do_gtrans, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"boot"_s, do_boot, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"linkdead"_s, do_linkdead, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"teleport"_s, do_teleport, position_t::DEAD, DEITY, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"purge"_s, do_purge, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"show"_s, do_show, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"fighting"_s, do_fighting, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"peace"_s, do_peace, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"check"_s, do_check, position_t::DEAD, 105, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"zoneexits"_s, do_zoneexits, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"find"_s, do_find, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"stat"_s, do_stat, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"redit"_s, do_redit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"guild"_s, do_guild, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"oedit"_s, &Character::do_oedit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"clear"_s, do_clear, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"repop"_s, do_repop, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"medit"_s, do_medit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"rdelete"_s, do_rdelete, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"oneway"_s, do_oneway, position_t::DEAD, ANGEL, cmd_t::ONEWAY, 0, 1, CommandType::immortals_only),
        Command(u"twoway"_s, do_oneway, position_t::DEAD, ANGEL, cmd_t::TWOWAY, 0, 1, CommandType::immortals_only),
        Command(u"zsave"_s, &Character::do_zsave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"rsave"_s, do_rsave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"msave"_s, do_msave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"osave"_s, do_osave, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"rstat"_s, do_rstat, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"possess"_s, do_possess, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"fsave"_s, do_fsave, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"zedit"_s, do_zedit, position_t::DEAD, ANGEL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"colors"_s, do_colors, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"colours"_s, do_colors, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"incognito"_s, do_incognito, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"high5"_s, do_highfive, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"holylite"_s, do_holylite, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"immort"_s, do_wiz, position_t::DEAD, IMMORTAL, cmd_t::IMMORT, 0, 1, CommandType::immortals_only),
        Command(u";"_s, do_wiz, position_t::DEAD, IMMORTAL, cmd_t::IMMORT, 0, 1, CommandType::immortals_only),
        Command(u"impchan"_s, do_wiz, position_t::DEAD, GIFTED_COMMAND, cmd_t::IMPCHAN, 0, 1, CommandType::immortals_only),
        Command(u"/"_s, do_wiz, position_t::DEAD, GIFTED_COMMAND, cmd_t::IMPCHAN, 0, 1, CommandType::immortals_only),
        Command(u"nohassle"_s, do_nohassle, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"wizinvis"_s, do_wizinvis, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"poof"_s, do_poof, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"wizhelp"_s, &Character::do_wizhelp, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"imotd"_s, do_imotd, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"mhelp"_s, do_mortal_help, position_t::DEAD, IMMORTAL, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"testhand"_s, do_testhand, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"varstat"_s, do_varstat, position_t::DEAD, 104, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"matrixinfo"_s, do_matrixinfo, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"maxcheck"_s, do_findfix, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"export"_s, do_export, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"mscore"_s, do_mscore, position_t::DEAD, 103, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"world"_s, do_world, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),

        // Special procedure commands placed to not disrupt god commands
        Command(u"setup"_s, do_mortal_set, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"metastat"_s, do_metastat, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"testhit"_s, do_testhit, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"acfinder"_s, do_acfinder, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"findpath"_s, do_findPath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"findpath2"_s, do_findpath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"addroom"_s, do_addRoom, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"newpath"_s, do_newPath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"listpathsbyzone"_s, do_listPathsByZone, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"listallpaths"_s, do_listAllPaths, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"dopathpath"_s, do_pathpath, position_t::DEAD, GIFTED_COMMAND, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"botcheck"_s, &Character::do_botcheck, position_t::DEAD, 106, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"showbits"_s, do_showbits, position_t::DEAD, OVERSEER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),
        Command(u"debug"_s, do_debug, position_t::DEAD, IMPLEMENTER, cmd_t::DEFAULT, 0, 1, CommandType::immortals_only),

        // Bug way down here after 'buy'
        Command(u"bug"_s, do_bug, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::all),
        // imbue after 'im' for us lazy immortal types :)
        Command(u"imbue"_s, do_imbue, position_t::STANDING, 0, cmd_t::DEFAULT, 0, 0, CommandType::all),

        // MOBprogram commands
        Command(u"mpasound"_s, do_mpasound, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpbestow"_s, do_mpbestow, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpjunk"_s, do_mpjunk, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpecho"_s, do_mpecho, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpechoat"_s, do_mpechoat, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpechoaround"_s, do_mpechoaround, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpechoaroundnotbad"_s, do_mpechoaroundnotbad, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpkill"_s, do_mpkill, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mphit"_s, do_mphit, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpaddlag"_s, do_mpaddlag, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpmload"_s, do_mpmload, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpoload"_s, do_mpoload, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mppurge"_s, do_mppurge, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpgoto"_s, do_mpgoto, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpat"_s, do_mpat, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mptransfer"_s, do_mptransfer, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpthrow"_s, do_mpthrow, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpforce"_s, do_mpforce, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mppeace"_s, do_mppeace, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpsetalign"_s, do_mpsetalign, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpsettemp"_s, &Character::do_mpsettemp, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpxpreward"_s, do_mpxpreward, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpteachskill"_s, do_mpteachskill, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),

        Command(u"mpdamage"_s, do_mpdamage, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpothrow"_s, do_mpothrow, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mppause"_s, do_mppause, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpretval"_s, do_mpretval, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpsetmath"_s, do_mpsetmath, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),
        Command(u"mpteleport"_s, do_mpteleport, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only),

        // End of the line
        Command(u""_s, &Character::generic_command, position_t::DEAD, 0, cmd_t::DEFAULT, 0, 1, CommandType::non_players_only)};

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
  for (auto &command : commands_)
  {
    qstring_command_map_[command.name()] = command;
    for (qsizetype position = 1; position < command.name().length(); position++)
    {
      auto keyword = command.name();
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

auto getCommandFromDirection(qint32 dir) -> std::expected<cmd_t, bool>
{
  switch (dir)
  {
  case Direction::NORTH:
    return cmd_t::NORTH;
    break;
  case Direction::EAST:
    return cmd_t::EAST;
    break;
  case Direction::SOUTH:
    return cmd_t::SOUTH;
    break;
  case Direction::WEST:
    return cmd_t::WEST;
    break;
  case Direction::UP:
    return cmd_t::UP;
    break;
  case Direction::DOWN:
    return cmd_t::DOWN;
    break;
  default:
    break;
  }
  return std::unexpected(false);
}

auto getDirectionFromCommand(cmd_t cmd) -> std::expected<qint32, bool>
{
  switch (cmd)
  {
  case cmd_t::NORTH:
    return Direction::NORTH;
    break;
  case cmd_t::EAST:
    return Direction::EAST;
    break;
  case cmd_t::SOUTH:
    return Direction::SOUTH;
    break;
  case cmd_t::WEST:
    return Direction::WEST;
    break;
  case cmd_t::UP:
    return Direction::UP;
    break;
  case cmd_t::DOWN:
    return Direction::DOWN;
    break;
  default:
    break;
  }
  return std::unexpected(false);
}
