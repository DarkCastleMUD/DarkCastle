#pragma once
#include <qcontainerfwd.h> // for QStringList
#include <qtypes.h>        // for quint64
#include <QString>         // for QString
#include <cstdint>         // for uint_fast8_t
#include <string>          // for string
#include "DC/types.h"
template <typename Key, typename T>
class QMap;

enum class attribute_t : uint_fast8_t
{
  UNDEFINED = 0,
  STRENGTH = 1,
  DEXTERITY = 2,
  INTELLIGENCE = 3,
  WISDOM = 4,
  CONSTITUTION = 5
};

enum class position_t : uint_fast8_t
{
  DEAD = 0,
  STUNNED = 3,
  SLEEPING = 4,
  RESTING = 5,
  SITTING = 6,
  FIGHTING = 7,
  STANDING = 8
};

enum class inet_protocol_family_t
{
  UNKNOWN,
  TCP4,
  TCP6,
  UNRECOGNIZED
};

enum class vault_search_type
{
  UNDEFINED,
  KEYWORD,
  LEVEL,
  MIN_LEVEL,
  MAX_LEVEL
};

enum class cmd_t
{
  UNDEFINED,   // 0
  NORTH,       // 1
  EAST,        // 2
  SOUTH,       // 3
  WEST,        // 4
  UP,          // 5
  DOWN,        // 6
  BELLOW,      // 8
  DEFAULT,     // 9
  TRACK,       // 10
  PALM,        // 10
  SAY,         // 11
  LOOK,        // 12
  BACKSTAB,    // 13
  SBS,         // 14
  ORCHESTRATE, // 15
  REPLY,       // 16
  WHISPER,     // 17
  GLANCE,      // 20
  FLEE,        // 28
  ESCAPE,      // 29
  PICK,        // 35
  STOCK,       // 56
  BUY,         // 56
  SELL,        // 57
  VALUE,       // 58
  LIST,        // 59
  ENTER,       // 60
  CLIMB,       // 60
  DESIGN,      // 62
  PRICE,       // 65
  REPAIR,      // 66
  READ,        // 67
  REMOVE,      // 69
  ERASE,       // 70
  ESTIMATE,    // 71
  REMORT,      // 80
  REROLL,      // 81
  CHOOSE,      // 82
  CONFIRM,     // 83
  CANCEL,      // 84
  SLIP,        // 87
  GIVE,        // 88
  DROP,        // 89
  DONATE,      // 90
  QUIT,        // 91
  SACRIFICE,   // 92
  PUT,         // 93
  OPEN,        // 98
  EDITOR,      // 100
  FORCE,       // 123
  WRITE,       // 128
  WATCH,       // 155
  PRACTICE,    // 164
  TRAIN,       // 165
  PROFESSION,  // 166
  GAIN,        // 171
  BALANCE,     // 172
  DEPOSIT,     // 173
  WITHDRAW,    // 174
  CLEAN,       // 177
  PLAY,        // 178
  FINISH,      // 179
  VETERNARIAN, // 180
  FEED,        // 181
  ASSEMBLE,    // 182
  PAY,         // 183
  RESTRING,    // 184
  PUSH,        // 185
  PULL,        // 186
  LEAVE,       // 187
  TREMOR,      // 188
  BET,         // 189
  INSURANCE,   // 190
  DOUBLE,      // 191
  STAY,        // 192
  SPLIT,       // 193
  HIT,         // 194
  LOOT,        // 195
  GTELL,       // 200
  CTELL,       // 201
  SETVOTE,     // 202
  VOTE,        // 203
  VEND,        // 204
  FILTER,      // 205
  EXAMINE,     // 206
  GAG,         // 207
  IMMORT,      // 208
  IMPCHAN,     // 209
  TELL,        // 210
  TELLH,       // 211
  PRIZE,       // 999
  OTHER,       // 999
  TELL_REPLY,  // 9999
  GAZE,        // 1820
  SAVE_SILENTLY,
  ONEWAY,
  TWOWAY,
  MLOCATE_CHARACTER,
  FEAR,
  PAGING_HELP,
  QUEST_CANCEL,
  QUEST_START,
  QUEST_FINISH,
  QUEST_LIST,
  GOLEMSCORE,
  FSCORE,
  REDEEM
};

enum class search_error
{
  invalid_input,
  not_found
};

enum class load_status_t
{
  unknown,  // default unknown value
  success,  // successfully loaded something
  missing,  // not found
  error,    // error loading
  bad_input // bad input
};

bool operator!(load_status_t ls);

typedef int command_return_t;
typedef int (*command_gen1_t)(CharacterPtr ch, char *argument, cmd_t cmd);
typedef int (*command_gen1b_t)(CharacterPtr ch, const char *argument, cmd_t cmd);
typedef command_return_t (*command_gen2_t)(CharacterPtr ch, std::string argument, cmd_t cmd);
typedef command_return_t (Character::*command_gen3_t)(QStringList arguments, cmd_t cmd);
typedef command_return_t (Character::*command_special_t)(QString arguments, cmd_t cmd);
typedef quint64 level_t;
typedef QMap<QString, QString> aliases_t;

[[nodiscard]] inline constexpr bool isSet(auto flag, auto bit)
{
  return flag & bit;
};

command_return_t do_mscore(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_huntstart(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_huntclear(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_metastat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_findfix(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_reload(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_abandon(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_accept(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_acfinder(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_action(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_addnews(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_addRoom(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_advance(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_areastats(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_awaymsgs(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_archive(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_autojoin(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_unban(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ambush(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_appraise(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_assemble(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_release(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_jab(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_areas(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ask(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_at(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_auction(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ban(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bandwidth(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bash(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_batter(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_battlecry(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_battlesense(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_beacon(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_beep(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_behead(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_berserk(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bladeshield(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bloodfury(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_boot(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_boss(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_brace(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_brew(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bug(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bullrush(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cast(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_channel(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_check(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_chpwd(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cinfo(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_circle(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_clans(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_clear(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_clearaff(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_climb(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_close(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cmotd(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ctax(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cwithdraw(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cbalance(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_colors(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_config(CharacterPtr ch, QStringList arguments, cmd_t cmd);
command_return_t do_consent(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_consider(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_count(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cpromote(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_crazedassault(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_credits(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cripple(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ctell(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_deathstroke(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_debug(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_deceit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_defenders_stance(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_disarm(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_disband(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_disconnect(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_dmg_eq(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_donate(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_dream(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_drop(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_eagle_claw(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_echo(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_emote(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_setvote(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vote(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_enter(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_equipment(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_eyegouge(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_examine(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_exits(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_export(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ferocity(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fighting(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fill(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_find(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_findpath(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_findPath(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fire(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_flee(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
/* DO_FUN  do_fly; */
command_return_t do_focused_repelance(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_follow(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_forage(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_found(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_free_animal(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_freeze(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fsave(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_get(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_global(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_gossip(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_golem_score(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_guild(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_install(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_reload_help(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hindex(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hedit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_grab(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_group(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_grouptell(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_gtrans(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_guard(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_harmtouch(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_help(CharacterPtr ch, const char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mortal_help(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hide(CharacterPtr ch, const char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_highfive(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hitall(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_holylite(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_home(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_idea(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_identify(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_imotd(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_imbue(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_incognito(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_index(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_info(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_initiate(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_innate(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_instazone(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_insult(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_inventory(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
auto do_joinarena(CharacterPtr ch, char *argument, cmd_t cmd) -> command_return_t;
command_return_t do_ki(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_kill(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_knockback(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
// command_return_t do_land (CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_layhands(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_leaderboard(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_leadership(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_leave(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_levels(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_linkdead(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_listAllPaths(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_listPathsByZone(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_listproc(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_load(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_medit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mortal_set(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
// command_return_t do_motdload (CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_msave(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_procedit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpbestow(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpstat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_opedit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_eqmax(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_opstat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_lock(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_log(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_look(CharacterPtr ch, const char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_make_camp(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_matrixinfo(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_maxes(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mlocate(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_move(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_motd(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpretval(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpasound(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpdamage(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpecho(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpechoaround(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpechoaroundnotbad(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpechoat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpforce(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpgoto(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpjunk(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpkill(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mphit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpsetmath(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpaddlag(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpmload(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpoload(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mppause(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mppeace(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mppurge(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpteachskill(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpsetalign(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpthrow(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpothrow(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mptransfer(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpxpreward(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpteleport(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_murder(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_name(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_natural_selection(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_newbie(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_newPath(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_news(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_noemote(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_nohassle(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_noname(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t generic_command(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_oclone(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mclone(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_offer(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_olocate(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_oneway(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_onslaught(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_open(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_order(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_orchestrate(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_osave(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pardon(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pathpath(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_peace(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_perseverance(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pick(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_plats(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pocket(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_poisonmaking(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pour(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_poof(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_possess(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_practice(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pray(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_profession(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_primalfury(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_promote(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_prompt(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_lastprompt(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_processes(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_psay(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
// command_return_t do_pshopedit (CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pview(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_punish(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_purge(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_purloin(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_put(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_qedit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quaff(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quest(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_qui(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quivering_palm(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rage(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_random(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_range(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rdelete(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_read(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_recite(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_redirect(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_redit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_remove(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rent(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_reply(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_repop(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_report(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rest(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_restore(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_retreat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_return(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_revoke(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rsave(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rstat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sacrifice(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_scan(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_score(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_scribe(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sector(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sedit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_send(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_set(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_shout(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_showhunt(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_skills(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_social(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_songs(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stromboli(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_headbutt(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_show(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_showbits(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_silence(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stupid(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sing(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sip(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_slay(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sleep(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_slip(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_smite(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sneak(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_spells(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sqedit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stalk(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stand(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_steal(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stealth(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_story(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_string(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stun(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_suicide(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_switch(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tactics(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tame(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_taste(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_teleport(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tellhistory(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testhand(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testhit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testport(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testuser(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_thunder(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tick(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_time(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_title(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_transfer(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_triage(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_trip(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_trivia(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_typo(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_unarchive(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_unlock(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_use(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_varstat(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vault(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vend(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_version(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_visible(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vitalstrike(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wear(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_weather(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_where(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whisper(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whoarena(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whoclan(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whogroup(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whosolo(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wield(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fakelog(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wiz(CharacterPtr ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wizinvis(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wizlist(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wizlock(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_world(CharacterPtr ch, std::string args, cmd_t cmd);
command_return_t do_write_skillquest(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_write(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_zedit(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_zoneexits(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_editor(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pursue(CharacterPtr ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
