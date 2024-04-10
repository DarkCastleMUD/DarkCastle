/***************************************************************************
 *  file: obj.h , Structures                               Part of DIKUMUD *
 *  Usage: Declarations of object data structures                          *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Performance optimization and bug fixes by MERC Industries.             *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: obj.h,v 1.36 2015/06/16 04:10:54 pirahna Exp $ */
#ifndef OBJ_H_
#define OBJ_H_

#include <vector>
#include <QStringList>

#include "common.h"
#include "structs.h" // uint8_t
#include "character.h"
#include "DC.h"
#include "casino.h"
#include "room.h"

/* The following defs are for Object  */

/* For 'type_flag' */

#define ITEM_LIGHT 1
#define ITEM_SCROLL 2
#define ITEM_WAND 3
#define ITEM_STAFF 4
#define ITEM_WEAPON 5
#define ITEM_FIREWEAPON 6
#define ITEM_MISSILE 7
#define ITEM_TREASURE 8
#define ITEM_ARMOR 9
#define ITEM_POTION 10
#define ITEM_WORN 11 // not used, can change
#define ITEM_OTHER 12
#define ITEM_TRASH 13
#define ITEM_TRAP 14
#define ITEM_CONTAINER 15
#define ITEM_NOTE 16
#define ITEM_DRINKCON 17
#define ITEM_KEY 18
#define ITEM_FOOD 19
#define ITEM_MONEY 20
#define ITEM_PEN 21
#define ITEM_BOAT 22
#define ITEM_BOARD 23
#define ITEM_PORTAL 24
#define ITEM_FOUNTAIN 25
#define ITEM_INSTRUMENT 26
#define ITEM_UTILITY 27
#define ITEM_BEACON 28
#define ITEM_LOCKPICK 29
#define ITEM_CLIMBABLE 30
#define ITEM_MEGAPHONE 31
#define ITEM_ALTAR 32
#define ITEM_TOTEM 33
#define ITEM_KEYRING 34
#define ITEM_TYPE_MAX 34

/* Bitvector For 'wear_flags' */

#define ITEM_TAKE 1
#define ITEM_WEAR_FINGER 2
#define ITEM_WEAR_NECK 4
#define ITEM_WEAR_BODY 8
#define ITEM_WEAR_HEAD 16
#define ITEM_WEAR_LEGS 32
#define ITEM_WEAR_FEET 64
#define ITEM_WEAR_HANDS 128
#define ITEM_WEAR_ARMS 256
#define ITEM_WEAR_SHIELD 512
#define ITEM_WEAR_ABOUT 1024
#define ITEM_WEAR_WAISTE 2048
#define ITEM_WEAR_WRIST 4096
#define ITEM_WEAR_WIELD 8192
#define ITEM_WEAR_HOLD 16384
#define ITEM_THROW 32768
#define ITEM_LIGHT_SOURCE 65536
#define ITEM_WEAR_FACE 131072
#define ITEM_WEAR_EAR 262144

/* Bitvector for 'extra_flags' */

#define ITEM_GLOW 1U
#define ITEM_HUM 1U << 1
#define ITEM_DARK 1U << 2
#define ITEM_LOCK 1U << 3
#define ITEM_ANY_CLASS 1U << 4 // Any class can use
#define ITEM_INVISIBLE 1U << 5
#define ITEM_MAGIC 1U << 6
#define ITEM_NODROP 1U << 7
#define ITEM_BLESS 1U << 8
#define ITEM_ANTI_GOOD 1U << 9
#define ITEM_ANTI_EVIL 1U << 10
#define ITEM_ANTI_NEUTRAL 1U << 11
#define ITEM_WARRIOR 1U << 12
#define ITEM_MAGE 1U << 13
#define ITEM_THIEF 1U << 14
#define ITEM_CLERIC 1U << 15
#define ITEM_PAL 1U << 16
#define ITEM_ANTI 1U << 17
#define ITEM_BARB 1U << 18
#define ITEM_MONK 1U << 19
#define ITEM_RANGER 1U << 20
#define ITEM_DRUID 1U << 21
#define ITEM_BARD 1U << 22
#define ITEM_TWO_HANDED 1U << 23
#define ITEM_ENCHANTED 1U << 24
#define ITEM_SPECIAL 1U << 25
#define ITEM_NOSAVE 1U << 26
#define ITEM_NOSEE 1U << 27
#define ITEM_NOREPAIR 1U << 28
#define ITEM_NEWBIE 1U << 29 // Newbie flagged.
#define ITEM_PC_CORPSE 1U << 30
#define ITEM_QUEST 1U << 31

#define ALL_CLASSES ITEM_WARRIOR | ITEM_MAGE | ITEM_THIEF | ITEM_CLERIC | ITEM_PAL | ITEM_ANTI | ITEM_BARB | ITEM_MONK | ITEM_RANGER | ITEM_DRUID | ITEM_BARD

/* Bitvector for 'more_flags' */

#define ITEM_NO_RESTRING 1U
#define ITEM_LIMIT_SACRIFICE 1U << 1
#define ITEM_UNIQUE 1U << 2
#define ITEM_NO_TRADE 1U << 3
#define ITEM_NONOTICE 1U << 4 // Item doesn't show up on 'look' but
                              // can still be accessed with 'get' etc FUTURE
#define ITEM_NOLOCATE 1U << 5
#define ITEM_UNIQUE_SAVE 1U << 6 // for corpse saving, didn't want to affect other unique flag so made a new one

#define ITEM_NPC_CORPSE 1U << 7
#define ITEM_PC_CORPSE_LOOTED 1U << 8
#define ITEM_NO_SCRAP 1U << 9
#define ITEM_CUSTOM 1U << 10
#define ITEM_24H_SAVE 1U << 11
#define ITEM_NO_DISARM 1U << 12
#define ITEM_TOGGLE 1U << 13 // Toggles for certain items.
#define ITEM_NO_CUSTOM 1U << 14
#define ITEM_24H_NO_SELL 1U << 15 // Item can't be sold for 24 RL hours

/* Bitvector for 'size' */
#define SIZE_ANY 1U
#define SIZE_SMALL 1U << 1
#define SIZE_MEDIUM 1U << 2
#define SIZE_LARGE 1U << 3

/* Different types of 'utility' items */

#define UTILITY_CATSTINK 1
#define UTILITY_EXIT_TRAP 2
#define UTILITY_MOVEMENT_TRAP 3
#define UTILITY_MORTAR 4
#define UTILITY_ITEM_MAX 4

/* Some different kind of liquids */
#define LIQ_WATER 0
#define LIQ_BEER 1
#define LIQ_WINE 2
#define LIQ_ALE 3
#define LIQ_DARKALE 4
#define LIQ_WHISKY 5
#define LIQ_LEMONADE 6
#define LIQ_FIREBRT 7
#define LIQ_LOCALSPC 8
#define LIQ_SLIME 9
#define LIQ_MILK 10
#define LIQ_TEA 11
#define LIQ_COFFEE 12
#define LIQ_BLOOD 13
#define LIQ_SALTWATER 14
#define LIQ_COKE 15
#define LIQ_GATORADE 16
#define LIQ_HOLYWATER 17
#define LIQ_INK 18

/* for containers  - value[1] */

#define CONT_CLOSEABLE 1
#define CONT_PICKPROOF 2
#define CONT_CLOSED 4
#define CONT_LOCKED 8

typedef uint64_t vnum_t;
typedef uint64_t room_t;

class active_object
{
    class Object *obj = {};
    active_object *next = {};
};

#define OBJ_NOTIMER -7000000

typedef uint16_t object_type_t;
typedef int32_t object_value_t;

struct obj_flag_data
{
    object_value_t value[4] = {}; /* Values of the item (see list)    */
    object_type_t type_flag = {}; /* Type of item                     */
    uint32_t wear_flags = {};     /* Where you can wear it            */
    uint16_t size = {};           /* Race restrictions                */
    uint32_t extra_flags = {};    /* If it hums, glows etc            */
    int16_t weight = {};          /* Weight what else                 */
    int32_t cost = {};            /* Value when sold (gp.)            */
    uint32_t more_flags = {};     /* A second bitvector (extra_flags2)*/
    level_t eq_level = {};        /* Min level to use it for eq       */
    int16_t timer = {};           /* Timer for object                 */
    Character *origin = {};       /* Creator of object, previously was stored at value[3] */
};

typedef int32_t location_t;
typedef int32_t modifier_t;
struct obj_affected_type
{
    int32_t location = {}; /* Which ability to change (APPLY_XXX) */
    int32_t modifier = {}; /* How much it changes by              */
};

/* ======================== Structure for object ========================= */
class Object : public Entity
{
public:
    enum class portal_types_t
    {
        Player = 0,
        Permanent = 1,
        Temp = 2,
        LookOnly = 3,
        PermanentNoLook = 4
    };

    enum portal_flags_t
    {
        No_Leave = 1 << 0,
        No_Enter = 1 << 1
    };

    static const QStringList wear_bits;
    static const QStringList size_bits;
    static const QStringList more_obj_bits;
    static const QStringList extra_bits;
    static const QStringList apply_types;

    int32_t item_number = {};     /* Where in data-base               */
    int vroom = {};               /* for corpse saving */
    obj_flag_data obj_flags = {}; /* Object information               */
    int16_t num_affects = {};
    obj_affected_type *affected = {}; /* Which abilities in PC to change  */

    char *name = {}; /* Title of object :get etc.        */
    inline const char *getNameC(void) { return name; }
    QString getName(void) { return name; }

    char *description = {};                       /* When in room                     */
    char *short_description = {};                 /* when worn/carry/in cont.         */
    struct extra_descr_data *ex_description = {}; /* extra descriptions     */
    Character *carried_by = {};                   /* Carried by :NULL in room/conta   */
    Character *equipped_by = {};                  /* so I can access the player :)    */

    Object *in_obj = {};   /* In what object NULL when none    */
    Object *contains = {}; /* Contains objects                 */

    Object *next_content = {}; /* For 'contains' lists             */
    Object *next = {};         /* For the object list              */
    Object *next_skill = {};
    table_data *table = {};
    class machine_data *slot = {};
    class wheel_data *wheel = {};
    time_t save_expiration = {};
    time_t no_sell_expiration = {};

    bool isDark(void);
    bool isPortal(void)
    {
        return obj_flags.type_flag == ITEM_PORTAL;
    }
    room_t getPortalDestinationRoom(void)
    {
        if (!isPortal())
        {
            return 0;
        }
        return obj_flags.value[0];
    }
    void setPortalDestinationRoom(room_t room)
    {
        if (!isPortal())
        {
            return;
        }
        obj_flags.value[0] = room;
    }

    portal_types_t getPortalType(void)
    {
        if (!isPortal())
        {
            return portal_types_t::Player;
        }
        return static_cast<portal_types_t>(obj_flags.value[1]);
    }
    bool isPortalTypePlayer(void)
    {
        return getPortalType() == portal_types_t::Player;
    }
    bool isPortalTypePermanent(void)
    {
        return getPortalType() == portal_types_t::Permanent;
    }
    bool isPortalTypeTemp(void)
    {
        return getPortalType() == portal_types_t::Temp;
    }
    bool isPortalTypeLookOnly(void)
    {
        return getPortalType() == portal_types_t::LookOnly;
    }
    bool isPortalTypePermanentNoLook(void)
    {
        return getPortalType() == portal_types_t::PermanentNoLook;
    }

    int32_t getPortalLeaveZone(void)
    {
        if (!isPortal())
        {
            return -1;
        }
        return obj_flags.value[2];
    }
    int32_t getPortalFlags(void)
    {
        if (!isPortal())
        {
            return 0;
        }
        return obj_flags.value[3];
    }
    bool hasPortalFlagNoLeave(void);
    bool hasPortalFlagNoEnter(void);

    uint64_t getLevel(void);

    int keywordfind(void);
    void setOwner(QString owner) { owner_ = owner; }
    QString getOwner(void) { return owner_; }

    [[nodiscard]] inline bool isCorpse(void) const
    {
        return isSet(obj_flags.extra_flags, ITEM_PC_CORPSE) || isSet(obj_flags.extra_flags, ITEM_PC_CORPSE_LOOTED);
    }
    [[nodiscard]] inline bool isTradable(void) const
    {
        return !isSet(obj_flags.more_flags, ITEM_NO_TRADE);
    }
    void ActionDescription(QString action_description)
    {
        action_description_ = action_description;
    }
    QString ActionDescription(void) { return action_description_; }

private:
    QString owner_;
    QString action_description_ = {}; /* What to write when used          */
};

/* For 'equipment' */

#define WEAR_LIGHT 0
#define WEAR_FINGER_R 1
#define WEAR_FINGER_L 2
#define WEAR_NECK_1 3
#define WEAR_NECK_2 4
#define WEAR_BODY 5
#define WEAR_HEAD 6
#define WEAR_LEGS 7
#define WEAR_FEET 8
#define WEAR_HANDS 9
#define WEAR_ARMS 10
#define WEAR_SHIELD 11
#define WEAR_ABOUT 12
#define WEAR_WAISTE 13
#define WEAR_WRIST_R 14
#define WEAR_WRIST_L 15
#define WIELD 16
#define SECOND_WIELD 17
#define HOLD 18
#define HOLD2 19
#define WEAR_FACE 20
#define WEAR_EAR_L 21
#define WEAR_EAR_R 22
// #define WEAR_MAX        22

/* ***********************************************************************
 *  file element for object file. BEWARE: Changing it will ruin the file  *
 *********************************************************************** */

#define CURRENT_OBJ_VERSION 1

struct obj_file_elem
{
    int16_t version = {};
    int32_t item_number = {};
    int16_t timer = {};
    int16_t wear_pos = {};
    int16_t container_depth = {};
    int32_t other[5] = {}; // unused
};

// functions from objects.cpp
int eq_max_damage(Object *obj);
int damage_eq_once(Object *obj);
int eq_current_damage(Object *obj);
void eq_remove_damage(Object *obj);
void add_obj_affect(Object *obj, int loc, int mod);
void remove_obj_affect_by_index(Object *obj, int index);
void remove_obj_affect_by_type(Object *obj, int loc);
bool fullSave(Object *obj);
void heightweight(Character *ch, bool add);
void wear(Character *ch, class Object *obj_object, int keyword);
int obj_from(Object *obj);

typedef QStringList item_types_t;

#endif
