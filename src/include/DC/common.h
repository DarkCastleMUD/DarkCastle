#ifndef COMMON_H
#define COMMON_H
#include <cstdint>
#include <string>
#include <QString>
#include <QStringList>

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
    FSCORE
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
typedef int (*command_gen1_t)(class Character *ch, char *argument, cmd_t cmd);
typedef int (*command_gen1b_t)(class Character *ch, const char *argument, cmd_t cmd);
typedef command_return_t (*command_gen2_t)(class Character *ch, std::string argument, cmd_t cmd);
typedef command_return_t (Character::*command_gen3_t)(QStringList arguments, cmd_t cmd);
typedef command_return_t (Character::*command_special_t)(QString arguments, cmd_t cmd);
typedef quint64 level_t;
typedef QMap<QString, QString> aliases_t;

[[nodiscard]] inline constexpr bool isSet(auto flag, auto bit)
{
    return flag & bit;
};

command_return_t do_mscore(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_huntstart(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_huntclear(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_metastat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_findfix(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_reload(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_abandon(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_accept(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_acfinder(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_action(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_addnews(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_addRoom(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_advance(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_areastats(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_awaymsgs(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_archive(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_autojoin(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_unban(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ambush(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_appraise(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_assemble(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_release(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_jab(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_areas(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ask(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_at(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_auction(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ban(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bandwidth(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bash(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_batter(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_battlecry(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_battlesense(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_beacon(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_beep(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_behead(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_berserk(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bladeshield(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bloodfury(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_boot(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_boss(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_brace(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_brew(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bug(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_bullrush(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cast(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_channel(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_check(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_chpwd(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cinfo(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_circle(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_clans(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_clear(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_clearaff(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_climb(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_close(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cmotd(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ctax(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cwithdraw(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cbalance(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_colors(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_config(Character *ch, QStringList arguments, cmd_t cmd);
command_return_t do_consent(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_consider(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_count(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cpromote(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_crazedassault(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_credits(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_cripple(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ctell(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_deathstroke(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_debug(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_deceit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_defenders_stance(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_disarm(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_disband(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_disconnect(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_dmg_eq(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_donate(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_dream(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_drink(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_drop(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_eat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_eagle_claw(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_echo(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_emote(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_setvote(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vote(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_enter(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_equipment(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_eyegouge(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_examine(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_exits(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_experience(Character *ch, QStringList arguments, cmd_t cmd);
command_return_t do_export(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ferocity(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fighting(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fill(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_find(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_findpath(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_findPath(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fire(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_flee(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
/* DO_FUN  do_fly; */
command_return_t do_focused_repelance(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_follow(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_forage(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_force(Character *ch, std::string argument, cmd_t cmd = cmd_t::FORCE);
command_return_t do_found(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_free_animal(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_freeze(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fsave(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_get(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_give(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_global(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_gossip(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_golem_score(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_guild(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_install(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_reload_help(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hindex(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_grab(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_group(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_grouptell(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_gtrans(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_guard(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_harmtouch(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_help(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mortal_help(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_new_help(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hide(Character *ch, const char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_highfive(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_hitall(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_holylite(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_home(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_idea(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_identify(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_ignore(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_imotd(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_imbue(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_incognito(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_index(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_info(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_initiate(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_innate(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_instazone(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_insult(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_inventory(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
auto do_joinarena(Character *ch, char *argument, cmd_t cmd) -> command_return_t;
command_return_t do_ki(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_kick(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_kill(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_knockback(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
// command_return_t do_land (Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_layhands(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_leaderboard(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_leadership(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_leave(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_levels(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_linkdead(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_listAllPaths(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_listPathsByZone(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_listproc(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_load(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_medit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mortal_set(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
// command_return_t do_motdload (Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_msave(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_procedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpbestow(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpstat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_opedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_eqmax(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_opstat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_lock(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_log(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_look(Character *ch, const char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_make_camp(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_matrixinfo(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_maxes(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mlocate(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_move(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_motd(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpretval(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpasound(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpdamage(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpecho(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpechoaround(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpechoaroundnotbad(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpechoat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpforce(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpgoto(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpjunk(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpkill(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mphit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpsetmath(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpaddlag(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpmload(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpoload(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mppause(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mppeace(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mppurge(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpteachskill(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpsetalign(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpthrow(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpothrow(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mptransfer(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpxpreward(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mpteleport(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_murder(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_name(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_natural_selection(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_newbie(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_newPath(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_news(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_noemote(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_nohassle(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_noname(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t generic_command(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_oclone(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_mclone(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_oedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_offer(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_olocate(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_oneway(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_onslaught(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_open(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_order(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_orchestrate(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_osave(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pardon(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pathpath(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_peace(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_perseverance(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pick(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_plats(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pocket(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_poisonmaking(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pour(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_poof(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_possess(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_practice(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pray(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_profession(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_primalfury(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_promote(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_prompt(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_lastprompt(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_processes(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_psay(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
// command_return_t do_pshopedit (Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pview(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_punish(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_purge(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_purloin(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_put(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_qedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quaff(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quest(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_qui(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quivering_palm(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_quit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rage(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_random(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_range(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rdelete(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_read(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_recite(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_redirect(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_redit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_remove(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rent(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_reply(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_repop(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_report(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rest(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_restore(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_retreat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_return(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_revoke(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rsave(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_rstat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sacrifice(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_say(Character *ch, std::string argument, cmd_t cmd = cmd_t::SAY);
command_return_t do_scan(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_score(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_scribe(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sector(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_send(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_set(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_shout(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_showhunt(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_skills(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_social(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_songs(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stromboli(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

command_return_t do_headbutt(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_show(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_showbits(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_silence(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stupid(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sing(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sip(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_slay(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sleep(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_slip(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_smite(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sneak(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_spells(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_sqedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stalk(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stand(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_steal(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stealth(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_story(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_string(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_stun(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_suicide(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_switch(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tactics(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tame(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_taste(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_teleport(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tellhistory(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testhand(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testhit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testport(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_testuser(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_thunder(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_tick(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_time(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_title(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_transfer(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_triage(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_trip(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_trivia(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_typo(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_unarchive(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_unlock(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_use(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_varstat(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vault(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vend(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_version(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_visible(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_vitalstrike(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wear(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_weather(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_where(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whisper(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whoarena(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whoclan(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whogroup(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_whosolo(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wield(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_fakelog(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wiz(Character *ch, std::string argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wizinvis(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wizlist(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_wizlock(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_world(Character *ch, std::string args, cmd_t cmd);
command_return_t do_write_skillquest(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_write(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_zedit(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_zoneexits(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_editor(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);
command_return_t do_pursue(Character *ch, char *argument, cmd_t cmd = cmd_t::DEFAULT);

#endif
