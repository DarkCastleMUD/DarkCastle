/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

#include "DC/DC.h"

// Extern variables

CharacterPtr rndm2;

qint32 activeProgs = {}; // loop protection

CharacterPtr activeActor = {};
CharacterPtr activeRndm = {};
CharacterPtr activeTarget = {};
ObjectPtr activeObj = {};
void *activeVo = {};

QString activeProg;
QString activePos;
QString activeProgTmpBuf;
// Global defined here

bool MOBtrigger;
mprog_throw_type *g_mprog_throw_list = {}; // holds all pending mprog throws

SelfPurge::SelfPurge()
{
}

SelfPurge::SelfPurge(bool s)
{
  state = s;
}

SelfPurge::operator bool() const
{
  return state;
}

void SelfPurge::setOwner(CharacterPtr c, QString m)
{
  owner = c;
  function = m;
}

QString SelfPurge::getFunction(void) const
{
  return function;
}

bool SelfPurge::getState(void) const
{
  return state;
}

selfpurge_t selfpurge = {};

qint32 cIfs; // for MPPAUSE
qint32 ifpos;

// This 2 variables keep track of what command and line number a mprog script
// is on for error logging purposes.
ReturnValues mprog_command_num = {};
ReturnValues mprog_line_num = {};

/*
 * Local function prototypes
 */

ReturnValues mprog_veval(qint64 lhs, QString opr, qint64 rhs);
ReturnValues mprog_do_ifchck(QString ifchck, CharacterPtr mob,
                             CharacterPtr actor, ObjectPtr obj,
                             void *vo, CharacterPtr rndm);
QString mprog_process_if(QString ifchck, QString com_list,
                         CharacterPtr mob, CharacterPtr actor,
                         ObjectPtr obj, void *vo,
                         CharacterPtr rndm, mprog_throw_type *thrw = {});
void mprog_translate(CharacterPtr ch, QString t, CharacterPtr mob,
                     CharacterPtr actor, ObjectPtr obj,
                     void *vo, CharacterPtr rndm);
ReturnValues mprog_process_cmnd(QString cmnd, CharacterPtr mob,
                                CharacterPtr actor, ObjectPtr obj,
                                void *vo, CharacterPtr rndm);

/***************************************************************************
 * Local function code and brief comments.
 */

/* Used to get sequential lines of a multi line QString (separated by "\r\n")
 * Thus its like one_argument(), but a trifle different. It is deive
 * to the multi line QString argument, and thus clist must not be shared.
 */
QString mprog_next_command(QString clist)
{
  bool open = false;
  QString pointer = clist;

  for (; *pointer != '\0'; pointer++)
  {
    if (!open && (*pointer == '\n' || *pointer == '\r'))
      break;
    if (*pointer == '{')
      open = true;
    if (open && *pointer == '}')
      open = false;
  }
  //  while ( *pointer != '\n' && *pointer != '\0' )
  //    pointer++;
  while (*pointer == '\n' || *pointer == '\r')
    *pointer++ = '\0';

  /*  if ( *pointer == '\n' )
          *pointer++ = '\0';
    if ( *pointer == '\r' )
          *pointer++ = '\0';*/

  mprog_line_num++;
  return (pointer);
}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the QString operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */

bool Character::mprog_seval(QString lhs, QString opr, QString rhs)
{
  if (lhs.isEmpty() || rhs.isEmpty())
    return false;

  if (opr == "==")
    return lhs == rhs;
  if (opr == "!=")
    return lhs != rhs;
  if (opr == "/")
    return !str_infix(rhs, lhs);
  if (opr == "!/")
    return str_infix(rhs, lhs);

  prog_error(u"Improper MOBprog operator"_s);
  logworld(u"Improper MOBprog operator"_s);
  return false;
}

/*
ReturnValues mprog_veval( qint32 lhs, QString opr, qint32 rhs )
{

  if ( !str_cmp( opr, "==" ) )
        return ( lhs == rhs );
  if ( !str_cmp( opr, "!=" ) )
        return ( lhs != rhs );
  if ( !str_cmp( opr, ">" ) )
        return ( lhs > rhs );
  if ( !str_cmp( opr, "<" ) )
        return ( lhs < rhs );
  if ( !str_cmp( opr, "<=" ) )
        return ( lhs <= rhs );
  if ( !str_cmp( opr, ">=" ) )
        return ( lhs >= rhs );
  if ( !str_cmp( opr, "&" ) )
        return ( lhs & rhs );
  if ( !str_cmp( opr, "|" ) )
        return ( lhs | rhs );

  dc_->logf( IMMORTAL, DC::LogChannel::LOG_WORLD,  "Improper MOBprog operator\r\n", 0 );
  return 0;

}
*/
ReturnValues mprog_veval(qint64 lhs, QString opr, qint64 rhs)
{

  if (!str_cmp(opr, "=="))
    return (lhs == rhs);
  if (!str_cmp(opr, "!="))
    return (lhs != rhs);
  if (!str_cmp(opr, ">"))
    return (lhs > rhs);
  if (!str_cmp(opr, "<"))
    return (lhs < rhs);
  if (!str_cmp(opr, "<="))
    return (lhs <= rhs);
  if (!str_cmp(opr, ">="))
    return (lhs >= rhs);
  if (!str_cmp(opr, "&"))
    return (lhs & rhs);
  if (!str_cmp(opr, "|"))
    return (lhs | rhs);

  dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Improper MOBprog operator\r\n", 0);
  return 0;
}
/*
ReturnValues mprog_veval( quint64 lhs, QString opr, quint64 rhs )
{
  if ( !str_cmp( opr, "==" ) )
        return ( lhs == rhs );
  if ( !str_cmp( opr, "!=" ) )
        return ( lhs != rhs );
  if ( !str_cmp( opr, ">" ) )
        return ( lhs > rhs );
  if ( !str_cmp( opr, "<" ) )
        return ( lhs < rhs );
  if ( !str_cmp( opr, "<=" ) )
        return ( lhs <= rhs );
  if ( !str_cmp( opr, ">=" ) )
        return ( lhs >= rhs );
  if ( !str_cmp( opr, "&" ) )
        return ( lhs & rhs );
  if ( !str_cmp( opr, "|" ) )
        return ( lhs | rhs );

  dc_->logf( IMMORTAL, DC::LogChannel::LOG_WORLD,  "Improper MOBprog operator\r\n", 0 );
  return 0;

}
*/

bool Character::isTank(void)
{
  if (!in_room)
    return false;
  for (auto victim = dc_->world[in_room]->people_; victim; victim = victim->next_in_room)
    if (victim->fighting == this && victim != this)
      return true;
  return false;
}

void translate_value(QString leftptr, QString rightptr, qint16 **vali,
                     quint32 **valui, QString **valstr, qint64 **vali64, quint64 **valui64, qint8 **valb,
                     CharacterPtr mob, CharacterPtr actor, ObjectPtr obj, void *vo,
                     CharacterPtr rndm, QString &valqstr)
{
  /*
   $n.age
   '$n' = left
   'age' = right

   $n,7.hasskill
   '$n' = left
   '7' = half
   'hasskill' = right
   */

  CharacterPtr target = {};
  ObjectPtr otarget = {};
  qint32 rtarget = -1, ztarget = -1;
  bool valset = false; // done like that to determine if value is set, since it can be 0
  tempvariable *mobTempVar = {};
  if (mob)
  {
    mobTempVar = mob->tempVariable;
  }

  activeTarget = {};
  QString tmp, half;
  half[0] = '\0';
  if ((tmp = strchr(leftptr, ',')) != nullptr)
  {
    *tmp = '\0';
    tmp++;
    one_argument(tmp, half); // strips whatever spaces
  }

  // Less nitpicky about the mobprogs with this stuff below in.
  QString larr;
  one_argument(leftptr, larr);
  QString left = &larr[0];

  QString rarr;
  one_argument(rightptr, rarr);
  QString right = &rarr[0];
  bool silent = false;

  if (!str_prefix("world_", left))
  {
    left += 6;

    const auto &character_list = dc_->character_list;
    auto result = std::find_if(character_list.begin(), character_list.end(),
                               [&target, &left](CharacterPtr const &tmp)
                               {
                                 if (isexact(left, qPrintable(tmp->name())))
                                 {
                                   target = tmp;
                                   return true;
                                 }
                                 else
                                 {
                                   return false;
                                 }
                               });
  }
  else if (!str_prefix("zone_", left))
  {
    left += 5;

    const auto &character_list = dc_->character_list;
    auto result = std::find_if(character_list.begin(), character_list.end(),
                               [&target, &left, &mob](CharacterPtr const &tmp)
                               {
                                 if (tmp->in_room != INVALID_ROOM && dc_->world[mob->in_room]->zone == dc_->world[tmp->in_room]->zone && isexact(left, qPrintable(tmp->name())))
                                 {
                                   target = tmp;
                                   return true;
                                 }
                                 else
                                 {
                                   return false;
                                 }
                               });
  }
  else if (!str_prefix("mroom_", left))
  {
    left += 6;
    CharacterPtr tmp;
    for (tmp = dc_->world[mob->in_room]->people_; tmp; tmp = tmp->next_in_room)
    {
      if (isexact(left, qPrintable(tmp->name())))
      {
        target = tmp;
        break;
      }
    }
  }
  else if (!str_prefix("room_", left))
  {
    left += 5;
    if (is_number(left))
      rtarget = dc_atoi(left);
    else
      rtarget = mob->in_room;
  }
  else if (!str_prefix("oworld_", left))
  {
    left += 7;
    otarget = get_obj(left);
  }
  else if (!str_prefix("ozone_", left))
  {
    left += 6;
    ObjectPtr otmp;
    qint32 z = dc_->world[mob->in_room]->zone;
    for (otmp = dc_->object_list; otmp; otmp = otmp->next)
    {
      ObjectPtr cmp = otmp->in_obj ? otmp->in_obj : otmp;
      if ((cmp->in_room != INVALID_ROOM && dc_->world[cmp->in_room]->zone == z) || (cmp->carried_by && dc_->world[cmp->carried_by->in_room]->zone == z) || (cmp->equipped_by && dc_->world[cmp->equipped_by->in_room]->zone == z))
        if (isexact(left, otmp->name()))
        {
          otarget = otmp;
          break;
        }
    }
    otarget = get_obj(left);
  }
  else if (!str_prefix("oroom_", left))
  {
    left += 6;
    otarget = get_obj_in_list(left, dc_->world[mob->in_room]->contents_);
  }
  else if (!str_prefix("zone_", left))
  {
    ztarget = dc_->world[mob->in_room]->zone;
    left += 5;
  }
  else if (*left == '$')
  {
    quint8 number_of_dollar_signs = 1;
    while (*(left + number_of_dollar_signs) == '$')
    {
      number_of_dollar_signs++;
    }

    switch (*(left + number_of_dollar_signs))
    {
    case '$':
      break;
    case 'n':
      target = actor;
      break;
    case 'i':
      target = mob;
      break;
    case 'r':
      target = rndm;
      silent = true;
      break;

    case 't':
      target = (CharacterPtr)vo;
      break;
    case 'o':
      otarget = obj;
      break;
    case 'p':
      otarget = vo;
      break;
    case 'f':
      if (actor)
        target = actor->fighting;
      break;
    case 'g':
      if (mob)
        target = mob->fighting;
      break;
    case 'v':
      QString buf;
      buf[0] = '\0';
      qint32 i;
      for (i = 2; *(left + i); i++)
      {
        if (*(left + i) == '[')
          continue;
        if (*(left + i) == ']')
        {
          buf[i - 3] = '\0';
          break;
        }
        buf[i - 3] = *(left + i);
      }
      for (; mobTempVar; mobTempVar = mobTempVar->next)
        if (!str_cmp(buf, qPrintable(mobTempVar->name)))
          break;
      if (mobTempVar)
        dc_strcpy(buf, qPrintable(mobTempVar->data));

      if (buf[0] != '\0')
      {
        if (!is_number(buf))
          target = get_char_room(buf, mob->in_room);
        else
        {
          valset = true;
        }
      }
      break;
    default:
      break;
    }
  }
  else
  {
    if (!is_number(left))
      target = get_char_room(left, mob->in_room);
    else
    {
      valset = true;
    }
  }

  if (!target && !otarget && ztarget == -1 && rtarget == -1 && !valset && str_cmp(right, "numpcs") && str_cmp(right, "hitpoints") && str_cmp(right, "move") && str_cmp(right, "mana") && str_cmp(right, "isdaytime") && str_cmp(right, "israining"))
  {
    if (!silent)
    {
      if (mob == nullptr)
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob == nullptr", left, right);
      }
      else if (mob->mobdata == nullptr)
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata == nullptr", left, right);
      }
      else if (mob->mobdata->nr < 0)
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata->nr = %d < 0 ", left, right, mob->mobdata->nr);
      }
      else
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: Mob: %d invalid target in mobprog", dc_->mob_index_[mob->mobdata->nr]->vnum());
      }
    }
    return;
  }
  activeTarget = target;
  // target acquired. fucking boring code.
  // more boring code. FUCK.
  qint16 *intval = {};
  quint32 *uintval = {};
  QString *stringval = {};
  QString *qstringval = {};
  qint64 *llval = {};
  quint64 *ullval = {};
  qint8 *sbval = {};
  bool tError = false;

  /*
   When a variable is created and assigned the value of the target-data, it is because it is
   not meant to be modify-able through mob-progs, such as character class.
   */

  switch (LOWER(*right))
  {
  case 'a':
    if (!str_cmp(right, "armor"))
    {
      if (!target)
        tError = true;
      else
      {
        intval = &target->armor;
        break;
      }
    }
    else if (!str_cmp(right, "actflags1"))
    {
      if (!target || target->isPlayer())
        tError = true;
      else
      {
        uintval = &target->mobdata->actflags[0];
        break;
      }
    }
    else if (!str_cmp(right, "actflags2"))
    {
      if (!target || target->isPlayer())
        tError = true;
      else
      {
        uintval = &target->mobdata->actflags[1];
        break;
      }
    }
    else if (!str_cmp(right, "affected1"))
    {
      if (!target)
        tError = true;
      else
      {
        uintval = &target->affected_by[0];
        break;
      }
    }
    else if (!str_cmp(right, "affected2"))
    {
      if (!target)
        tError = true;
      else
      {
        uintval = &target->affected_by[1];
        break;
      }
    }
    else if (!str_cmp(right, "alignment"))
    {
      if (!target)
        tError = true;
      else
      {
        intval = &target->alignment;
        break;
      }
    }
    else if (!str_cmp(right, "acidsave"))
    {
      if (!target)
        tError = true;
      else
      {
        intval = &target->saves[SAVE_TYPE_ACID];
        break;
      }
    }
    else if (!str_cmp(right, "age"))
    {
      if (!target)
        tError = true;
      else
      {
        qint16 ageint = target->age().year;
        intval = &ageint;
      }
    }
    break;
  case 'b':
    if (!str_cmp(right, "bank"))
    {
      if (!target || !target->player)
        tError = true;
      else
      {
        uintval = &target->player->bank;
      }
    }
    break;
  case 'c':
    if (!str_cmp(right, "carriedby"))
    {
      if (!otarget)
        tError = true;
      else if (otarget->carried_by)
      {
        valqstr = otarget->carried_by->name();
      }
      else if (otarget->equipped_by)
      {
        valqstr = otarget->equipped_by->name();
      }
      else if (otarget->in_obj && otarget->in_obj->carried_by)
      {
        valqstr = otarget->in_obj->carried_by->name();
      }
      else if (otarget->in_obj && otarget->in_obj->equipped_by)
      {
        valqstr = otarget->in_obj->equipped_by->name();
      }
      else
        stringval = {};
    }
    else if (!str_cmp(right, "carryingitems"))
    {
      if (!target)
        tError = true;
      else
      {
        qint16 car = target->carry_items;
        intval = &car;
      }
    }
    else if (!str_cmp(right, "carryingweight"))
    {
      if (!target)
        tError = true;
      else
      {
        qint16 car = target->carry_weight;
        intval = &car;
      }
    }
    else if (!str_cmp(right, "class"))
    {
      if (!target)
        tError = true;
      else
      {
        sbval = &target->c_class;
      }
    }
    else if (!str_cmp(right, "coldsave"))
    {
      if (!target)
        tError = true;
      else
      {
        intval = &target->saves[SAVE_TYPE_COLD];
      }
    }
    else if (!str_cmp(right, "constitution"))
    {
      if (!target)
        tError = true;
      else
      {
        sbval = &target->con;
      }
    }
    else if (!str_cmp(right, "cost"))
    {
      if (!otarget)
        tError = true;
      else
      {
        uintval = (quint32 *)&otarget->flags_.cost;
      }
    }
    break;
  case 'd':
    if (!str_cmp(right, "damroll"))
    {
      if (!target)
        tError = true;
      else
      {
        intval = &target->damroll;
      }
    }
    else if (!str_cmp(right, "description"))
    {
      if (!target && !otarget && !rtarget)
        tError = true;
      else if (otarget)
      {
        stringval = &otarget->long_description;
      }
      else if (rtarget >= 0)
      {
        if (dc_->rooms.contains(rtarget))
          stringval = &dc_->world[rtarget].description;
        else
          tError = true;
      }
      else
      {
        stringval = &target->description;
      }
    }
    else if (!str_cmp(right, "dexterity"))
    {
      if (!target)
        tError = true;
      else
      {
        sbval = &target->dex;
      }
    }
    else if (!str_cmp(right, "drunk"))
    {
      if (!target)
        tError = true;
      else
      {
        sbval = &target->conditions[DRUNK];
      }
    }
    break;
  case 'e':
    if (!str_cmp(right, "energysaves"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->saves[SAVE_TYPE_ACID];
    }
    else if (!str_cmp(right, "experience"))
    {
      if (!target)
        tError = true;
      else
      {
        llval = &target->exp;
      }
    }
    else if (!str_cmp(right, "extra"))
    {
      if (!otarget)
        tError = true;
      else
      {
        uintval = &otarget->flags_.extra_flags;
      }
    }
    break;
  case 'f':
    if (!str_cmp(right, "firesaves"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->saves[SAVE_TYPE_FIRE];
    }
    else if (!str_cmp(right, "flags"))
    {
      if (!rtarget)
        tError = true;
      else
        uintval = &dc_->world[rtarget]->room_flags_;
    }
    break;
  case 'g':
    if (!str_cmp(right, "gold"))
    {
      if (!target)
        tError = true;
      else
        ullval = &target->getGoldReference();
    }
    else if (!str_cmp(right, "glowfactor"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->glow_factor;
    }
    break;
  case 'h':
    if (!str_cmp(right, "hasskill"))
    {
      if (!target)
        tError = true;
      else
      {
        qint32 skl = {};
        if (*half == '\0' || (skl = dc_atoi(half)) < 0)
        {
          dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD,
                    "translate_value: Mob: %d invalid skillnumber in hasskill",
                    dc_->mob_index_[mob->mobdata->nr]->vnum());
          tError = true;
        }
        qint16 sklint = target->has_skill(skl);
        intval = &sklint;
      }
    }
    else if (!str_cmp(right, "height"))
    {
      if (!target)
        tError = true;
      else
        sbval = (qint8 *)(&target->height);
    }
    else if (!str_cmp(right, "hitpoints"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->hit;
    }
    else if (!str_cmp(right, "hitroll"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->hitroll;
    }
    else if (!str_cmp(right, "homeroom"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->hometown;
    }
    else if (!str_cmp(right, "hunger"))
    {
      if (!target)
        tError = true;
      else
      {
        sbval = &target->conditions[FULL];
      }
    }
    break;
  case 'i':
    if (!str_cmp(right, "immune"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->immune;
    }
    else if (!str_cmp(right, "inroom"))
    {
      if (!target && !otarget)
        tError = true;
      else if (target)
      {
        static quint32 tmp;
        tmp = (quint32)target->in_room;
        uintval = &tmp;
      }
      else
      {
        static quint32 tmp;
        tmp = (quint32)otarget->in_room;
        uintval = &tmp;
      }
    }
    else if (!str_cmp(right, "intelligence"))
    {
      if (!target)
        tError = true;
      else
        sbval = (qint8 *)&target->intel;
    }
    break;
  case 'l':
    if (!str_cmp(right, "level"))
    {
      if (!target && !otarget)
        tError = true;
      else if (otarget)
      {
        intval = reinterpret_cast<decltype(intval)>(&otarget->flags_.eq_level);
      }
      else
      {
        if (target->isNonPlayer())
          sbval = reinterpret_cast<decltype(sbval)>(target->getLevelPtr());
        else
          sbval = reinterpret_cast<decltype(sbval)>(target->getLevelPtr());
      }
    }
    else if (!str_cmp(right, "long"))
    {
      if (!target)
        tError = true;
      else
      {
        stringval = &target->long_desc;
      }
    }
    break;
  case 'k':
    if (!str_cmp(right, "ki"))
    {
      if (!target)
        tError = true;
      else
      {
        uintval = (quint32 *)&target->ki;
      }
    }
    break;
  case 'm':
    if (!str_cmp(right, "magicsaves"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->saves[SAVE_TYPE_MAGIC];
    }
    else if (!str_cmp(right, "mana"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->mana;
    }
    else if (!str_cmp(right, "maxhitpoints"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->max_hit;
    }
    else if (!str_cmp(right, "maxmana"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->max_mana;
    }
    else if (!str_cmp(right, "maxmove"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->max_move;
    }
    else if (!str_cmp(right, "maxki"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->max_ki;
    }
    else if (!str_cmp(right, "meleemit"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->melee_mitigation;
    }
    else if (!str_cmp(right, "misc"))
    {
      if (!target)
        tError = true;
      else
        uintval = &target->misc;
    }
    else if (!str_cmp(right, "more"))
    {
      if (!otarget)
        tError = true;
      else
      {
        uintval = &otarget->flags_.more_flags;
      }
    }
    else if (!str_cmp(right, "move"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)target->getMovePtr();
    }
    break;
  case 'n':
    if (!str_cmp(right, "name"))
    {
      if (!target && !rtarget)
        tError = true;
      else if (rtarget >= 0)
      {
        if (dc_->rooms.contains(rtarget))
          stringval = &dc_->world[rtarget].name;
        else
          tError = true;
      }
      else
      {
        valqstr = target->name();
      }
    }
    break;
  case 'o':
    break;
  case 'p':
    if (!str_cmp(right, "platinum"))
    {
      if (!target)
        tError = true;
      else
        uintval = &target->plat;
    }
    else if (!str_cmp(right, "poisonsaves"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->saves[SAVE_TYPE_POISON];
    }
    else if (!str_cmp(right, "position"))
    {
      if (!target)
        tError = true;
      else
        intval = (qint16 *)target->getPositionPtr();
    }
    else if (!str_cmp(right, "practices"))
    {
      if (!target || !target->player)
        tError = true;
      else
        intval = (qint16 *)&target->player->practices;
    }
    break;
  case 'q':
    break;
  case 'r':
    if (!str_cmp(right, "race"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->race;
    }
    else if (!str_cmp(right, "rawstr"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->raw_str;
    }
    else if (!str_cmp(right, "rawcon"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->raw_con;
    }
    else if (!str_cmp(right, "rawwis"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->raw_wis;
    }
    else if (!str_cmp(right, "rawdex"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->raw_dex;
    }
    else if (!str_cmp(right, "rawint"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->raw_intel;
    }
    else if (!str_cmp(right, "rawhit"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->raw_hit;
    }
    else if (!str_cmp(right, "rawmana"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->raw_mana;
    }
    else if (!str_cmp(right, "rawmove"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->raw_move;
    }
    else if (!str_cmp(right, "rawki"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->raw_ki;
    }
    else if (!str_cmp(right, "resist"))
    {
      if (!target)
        tError = true;
      else
        uintval = &target->resist;
    }
    break;
  case 's':
    if (!str_cmp(right, "sex"))
    {
      if (!target)
        tError = true;
      else
        sbval = reinterpret_cast<decltype(sbval)>(&target->sex);
    }
    else if (!str_cmp(right, "size"))
    {
      if (!otarget)
        tError = true;
      else
      {
        intval = (qint16 *)&otarget->flags_.size;
      }
    }
    else if (!str_cmp(right, "short"))
    {
      if (!target && !otarget)
        tError = true;
      else if (otarget)
      {
        stringval = &otarget->short_description;
      }
      else
        stringval = &target->short_desc;
    }
    else if (!str_cmp(right, "songmit"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->song_mitigation;
    }
    else if (!str_cmp(right, "spelleffect"))
    {
      if (!target)
        tError = true;
      else
        uintval = (quint32 *)&target->spelldamage;
    }
    else if (!str_cmp(right, "spellmit"))
    {
      if (!target)
        tError = true;
      else
        intval = &target->spell_mitigation;
    }
    else if (!str_cmp(right, "strength"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->str;
    }
    else if (!str_cmp(right, "suscept"))
    {
      if (!target)
        tError = true;
      else
        uintval = &target->suscept;
    }
    break;
  case 't':
    if (!str_cmp(right, "temp"))
    {
      if (!half[0] || !target)
      {
        tError = true;
      }
      else
      {
        valqstr = target->getTemp(half);
      }
    }
    else if (!str_cmp(right, "title"))
    {
      if (!target)
        tError = true;
      else
        qstringval = &target->title;
    }
    else if (!str_cmp(right, "type"))
    {
      if (!otarget)
        tError = true;
      else
        sbval = (qint8 *)&otarget->flags_.type_flag;
    }
    else if (!str_cmp(right, "thirst"))
    {
      if (!target)
        tError = true;
      else
      {
        sbval = &target->conditions[THIRST];
      }
    }
    break;
  case 'v':
    if (!str_cmp(right, "value0"))
    {
      if (!otarget)
        tError = true;
      else
        uintval = (quint32 *)&otarget->flags_.value[0];
    }
    else if (!str_cmp(right, "value1"))
    {
      if (!otarget)
        tError = true;
      else
        uintval = (quint32 *)&otarget->flags_.value[1];
    }
    else if (!str_cmp(right, "value2"))
    {
      if (!otarget)
        tError = true;
      else
        uintval = (quint32 *)&otarget->flags_.value[2];
    }
    else if (!str_cmp(right, "value3"))
    {
      if (!otarget)
        tError = true;
      else
        uintval = (quint32 *)&otarget->flags_.value[3];
    }
    break;
  case 'w':
    if (!str_cmp(right, "wearable"))
    {
      if (!otarget)
        tError = true;
      else
      {
        auto tmp_quint32 = otarget->flags_.wear_flags.toInt();
        uintval = &tmp_quint32;
      }
    }
    else if (!str_cmp(right, "weight"))
    {
      if (!target && !otarget)
        tError = true;
      else if (otarget)
      {
        intval = &otarget->flags_.weight;
      }
      else
        sbval = (qint8 *)&target->weight;
    }
    else if (!str_cmp(right, "wisdom"))
    {
      if (!target)
        tError = true;
      else
        sbval = &target->wis;
    }
    break;
  default:
    break;
  }
  if (tError)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s target=%p actor=%p mob=%p", left, right, target, actor, mob);

    if (mob)
    {
      if (mob->mobdata == nullptr)
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata == nullptr", left, right);
      }
      else if (mob->mobdata->nr < 0)
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: %s.%s mob->mobdata->nr = %d < 0 ", left, right, mob->mobdata->nr);
      }
      else
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "translate_value: Mob: %d tried to access non-existent field of target", dc_->mob_index_[mob->mobdata->nr]->vnum());
      }
    }
    return;
  }
  if (intval)
    *vali = intval;
  if (uintval)
    *valui = uintval;
  if (stringval)
    *valstr = stringval;
  if (llval)
    *vali64 = llval;
  if (ullval)
    *valui64 = ullval;
  if (sbval)
    *valb = sbval;
}

/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return -1 otherwise return boolean 1,0
 */

QMap<QString, mprog_ifs> load_ifchecks()
{
  QMap<QString, mprog_ifs> ifcheck_tmp;

  ifcheck_tmp["rand"] = eRAND;
  ifcheck_tmp["rand1k"] = eRAND1K;
  ifcheck_tmp["amtitems"] = eAMTITEMS;
  ifcheck_tmp["numpcs"] = eNUMPCS;
  ifcheck_tmp["numofmobsinworld"] = eNUMOFMOBSINWORLD;
  ifcheck_tmp["numofobjsinworld"] = eNUMOFOBJSINWORLD;
  ifcheck_tmp["numofmobsinroom"] = eNUMOFMOBSINROOM;
  ifcheck_tmp["ispc"] = eISPC;
  ifcheck_tmp["iswielding"] = eISWIELDING;
  ifcheck_tmp["isweappri"] = eISWEAPPRI;
  ifcheck_tmp["isweapsec"] = eISWEAPSEC;

  ifcheck_tmp["isnpc"] = eISNPC;
  ifcheck_tmp["isgood"] = eISGOOD;
  ifcheck_tmp["isneutral"] = eISNEUTRAL;
  ifcheck_tmp["isevil"] = eISEVIL;
  ifcheck_tmp["isfight"] = eISFIGHT;

  ifcheck_tmp["istank"] = eISTANK;
  ifcheck_tmp["isimmort"] = eISIMMORT;
  ifcheck_tmp["ischarmed"] = eISCHARMED;
  ifcheck_tmp["isfollow"] = eISFOLLOW;
  ifcheck_tmp["isspelled"] = eISSPELLED;
  ifcheck_tmp["isworn"] = eISWORN;

  ifcheck_tmp["isaffected"] = eISAFFECTED;
  ifcheck_tmp["hitprcnt"] = eHITPRCNT;
  ifcheck_tmp["wears"] = eWEARS;
  ifcheck_tmp["carries"] = eCARRIES;
  ifcheck_tmp["number"] = eNUMBER;

  ifcheck_tmp["tempvar"] = eTEMPVAR;
  ifcheck_tmp["ismobvnuminroom"] = eISMOBVNUMINROOM;
  ifcheck_tmp["isobjvnuminroom"] = eISOBJVNUMINROOM;
  ifcheck_tmp["cansee"] = eCANSEE;

  ifcheck_tmp["insamezone"] = eINSAMEZONE;
  ifcheck_tmp["clan"] = eCLAN;

  ifcheck_tmp["isdaytime"] = eISDAYTIME;
  ifcheck_tmp["israining"] = eISRAINING;

  return ifcheck_tmp;
}

QMap<QString, mprog_ifs> ifcheck = load_ifchecks();

ReturnValues mprog_do_ifchck(QString ifchck, CharacterPtr mob, CharacterPtr actor,
                             ObjectPtr obj, void *vo, CharacterPtr rndm)
{

  QString buf;
  QString arg;
  QString opr;
  QString val;
  QString val2; // used for non-traditional
  CharacterPtr vict = (CharacterPtr)vo;
  ObjectPtr v_obj = vo;
  QString bufpt = buf;
  QString argpt = arg;
  QString oprpt = opr;
  QString valpt = val;
  QString point = ifchck;
  val2[0] = '\0';

  if (*point == '\0')
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: null ifchck: '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
    return -1;
  }
  /* skip leading spaces */
  while (*point == ' ')
    point++;
  bool traditional = false;

  /* get whatever comes before the left paren.. ignore spaces */
  while (*point)
    if (*point == '(')
    {
      traditional = true;
      break;
    }
    else if (*point == ' ')
    {
      mob->prog_error(u"ifchck syntax error: '%1'"_s.arg(ifchck));
      return -1;
    }
    else if (*point == '.')
    {
      break;
    }
    else if (*point == '\0')
    {
      mob->prog_error(u"ifchck syntax error"_s);
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: ifchck syntax error: '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    else if (*point == ' ')
      point++;
    else
      *bufpt++ = *point++;

  *bufpt = '\0';
  point++;

  /* get whatever is in between the parens.. ignore spaces */
  while (*point)
    if (traditional && *point == ')')
      break;
    else if (!traditional && !isalpha(*point))
    {
      point--;
      break;
    }
    else if (*point == '\0')
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: ifchck syntax error: '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    else if (*point == ' ')
      point++;
    else
      *argpt++ = *point++;

  *argpt = '\0';
  point++;

  /* check to see if there is an operator */

  // same for both traditional and non-traditional
  while (*point == ' ' || *point == '\r')
    point++;
  if (*point == '\0')
  {
    *opr = '\0';
    *val = '\0';
  }
  else /* there should be an operator and value, so get them */
  {
    while ((*point != ' ') && (!isalnum(*point)))
      if (*point == '\0')
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: ifchck operator without value: '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
        return -1;
      }
      else
        *oprpt++ = *point++;

    *oprpt = '\0';

    /* finished with operator, skip spaces and then get the value */
    while (*point == ' ')
      point++;
    for (;;)
    {
      if (*point == '.')
      {
        *valpt = '\0';
        valpt = val2;
        point++;
      }
      else if (*point == '\0' || *point == '\r' || *point == '\n')
        break;
      else
        *valpt++ = *point++;
    }

    *valpt = '\0';
  }
  bufpt = buf;
  argpt = arg;
  oprpt = opr;
  valpt = val;

  /* Ok... now buf contains the ifchck, arg contains the inside of the
   *  parentheses, opr contains an operator if one is present, and val
   *  has the value if an operator was present.
   *  So.. basically use if statements and run over all known ifchecks
   *  Once inside, use the argument and expand the lhs. Then if need be
   *  send the lhs,opr,rhs off to be evaluated.
   */

  CharacterPtr fvict = {};
  bool ye = false;
  if (arg[0] == '$' && arg[1] == 'v')
  {
    tempvariable *eh = mob->tempVariable;
    QString buf1;
    buf1[0] = '\0';
    qint32 i;
    for (i = 2; arg[i]; i++)
    {
      if (arg[i] == '[')
        continue;
      if (arg[i] == ']')
      {
        buf1[i - 3] = '\0';
        break;
      }
      buf1[i - 3] = arg[i];
    }
    for (; eh; eh = eh->next)
      if (eh->name == buf1)
        break;
    if (eh)
      dc_strcpy(arg, qPrintable(eh->data));
  }

  if (!is_number(arg) && !(arg[0] == '$') && traditional)
  {
    fvict = get_char_room(arg, mob->in_room, true);
    ye = true;
  }
  if (!(arg[0] == '$') && is_number(arg) && traditional)
  {
    CharacterPtr te;
    qint32 vnum = dc_atoi(arg);
    for (te = dc_->world[mob->in_room]->people_; te; te = te->next)
    {
      if (te->isPlayer())
        continue;
      if (dc_->mob_index_[te->mobdata->nr]->vnum() == vnum)
      {
        fvict = te;
        break;
      }
    }
    ye = true;
  }

  qint16 *lvali = {};
  quint32 *lvalui = {};
  QString *lvalstr = {};
  QString lvalqstr;
  qint64 *lvali64 = {};
  quint64 *lvalui64 = {};
  qint8 *lvalb = {};
  //  qint32 type = {};

  if (!traditional)
    translate_value(buf, arg, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, mob, actor, obj, vo, rndm, lvalqstr);
  else
    // switch order of traditional so it'd be $n(ispc), to conform with
    // new ifchecks
    translate_value(arg, buf, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, mob, actor, obj, vo, rndm, lvalqstr);

  if (val2[0] == '\0')
  {
    if (lvali)
      return mprog_veval(*lvali, opr, dc_atoi(val));
    if (lvalui)
      return mprog_veval(*lvalui, opr, (uint)dc_atoi(val));
    if (lvali64)
      return mprog_veval((qint32)*lvali64, opr, dc_atoi(val));
    if (lvalui64)
      return mprog_veval((qint32)*lvalui64, opr, dc_atoi(val));
    if (lvalb)
      return mprog_veval((qint32)*lvalb, opr, dc_atoi(val));
    if (lvalstr)
      return mob->mprog_seval(*lvalstr, opr, val);
    if (!lvalqstr.isEmpty())
      return mob->mprog_seval(lvalqstr, opr, val);
  }
  else
  {
    qint16 *rvali = {};
    quint32 *rvalui = {};
    QString *rvalstr = {};
    QString rvalqstr;
    qint64 *rvali64 = {};
    quint64 *rvalui64 = {};
    qint8 *rvalb = {};
    translate_value(val, val2, &rvali, &rvalui, &rvalstr, &rvali64, &rvalui64, &rvalb, mob, actor, obj, vo, rndm, rvalqstr);
    qint64 rval = {};
    if (rvalstr || rvali || rvalui || rvali64 || rvalui64 || rvalb)
    {
      if (rvalstr && lvalstr)
        return mob->mprog_seval(*lvalstr, opr, *rvalstr);
      // The rest fit in an qint64, so let's just use that.
      if (rvalstr)
        rval = dc_atoi(*rvalstr);
      if (rvali)
        rval = *rvali;
      if (rvalui)
        rval = *rvalui;
      if (rvalb)
        rval = *rvalb;
      if (rvali64)
        rval = *rvali64;
      if (rvalui64)
        rval = *rvalui64;

      if (lvali)
        return mprog_veval(*lvali, opr, rval);
      if (lvalui)
        return mprog_veval(*lvalui, opr, rval);
      if (lvali64)
        return mprog_veval(*lvali64, opr, rval);
      if (lvalb)
        return mprog_veval((qint32)*lvalb, opr, rval);
    }
  }

  switch (ifcheck[buf])
  {
  case eRAND:
    return (ch->dc_->number(1, 100) <= dc_atoi(arg));
    break;

  case eRAND1K:
    return (ch->dc_->number(1, 1000) <= dc_atoi(arg));
    break;

  case eAMTITEMS:
    return mprog_veval(dc_->obj_index_[real_object(dc_atoi(arg))]->qty, opr, dc_atoi(val));
    break;

  case eNUMPCS:
  {
    CharacterPtr p;
    qint32 count = {};
    for (p = dc_->world[mob->in_room]->people_; p; p = p->next_in_room)
      if (p->isPlayer())
        count++;
    return mprog_veval(count, opr, dc_atoi(val));
  }
  break;

  case eNUMOFMOBSINWORLD:
  {
    qint32 target = dc_atoi(arg);
    qint32 count = {};

    const auto &character_list = dc_->character_list;
    count = std::count_if(character_list.begin(), character_list.end(),
                          [&target](CharacterPtr vch)
                          {
                            if (vch->isNonPlayer() && vch->in_room != INVALID_ROOM && dc_->mob_index_[vch->mobdata->nr]->vnum() == target)
                            {
                              return true;
                            }
                            else
                            {
                              return false;
                            }
                          });

    return mprog_veval(count, opr, dc_atoi(val));
  }
  break;
  case eNUMOFOBJSINWORLD:
  {
    qint32 target = dc_atoi(arg);
    qint32 count = {};

    ObjectPtr p;
    for (p = dc_->object_list; p; p = p->next)
    {
      if (dc_->obj_index_[p->item_number]->vnum() == target)
      {
        count++;
      }
    }

    return mprog_veval(count, opr, dc_atoi(val));
  }
  break;

  case eNUMOFMOBSINROOM:
  {
    qint32 target = dc_atoi(arg);
    CharacterPtr p;
    qint32 count = {};
    for (p = dc_->world[mob->in_room]->people_; p; p = p->next_in_room)
      if (p->isNonPlayer() && dc_->mob_index_[p->mobdata->nr]->vnum() == target)
        count++;

    return mprog_veval(count, opr, dc_atoi(val));
  }
  break;

  case eISPC:
    if (fvict)
      return fvict->isPlayer();
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return 0;
    case 'z':
      if (mob->beacon)
        return reinterpret_cast<CharacterPtr>(mob->beacon)->isNonPlayer();
      else
        return -1;
    case 'n':
      if (actor)
        return (actor->isPlayer());
      else
        return 0;
    case 't':
      if (vict)
        return (vict->isPlayer());
      else
        return 0;
    case 'r':
      if (rndm)
        return (rndm->isPlayer());
      else
        return 0;
    case 'f':
      if (actor && actor->fighting)
        return (actor->fighting->isPlayer());
      else
        return 0;
    case 'g':
      if (mob && mob->fighting)
        return (mob->fighting->isPlayer());
      else
        return 0;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'ispc'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eISWIELDING:
    if (fvict)
      return fvict->equipment[WEAR_WIELD] ? 1 : 0;
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return (mob->equipment[WEAR_WIELD]) ? 1 : 0;
    case 'z':
      if (mob->beacon)
        return ((CharacterPtr)mob->beacon)->equipment[WEAR_WIELD] ? 1 : 0;
      else
        return -1;

    case 'n':
      if (actor)
        return (actor->equipment[WEAR_WIELD]) ? 1 : 0;
      else
        return 0;
    case 't':
      if (vict)
        return (vict->equipment[WEAR_WIELD]) ? 1 : 0;
      else
        return 0;
    case 'r':
      if (rndm)
        return (rndm->equipment[WEAR_WIELD]) ? 1 : 0;
      else
        return 0;
    case 'f':
      if (actor && actor->fighting)
        return (actor->fighting->equipment[WEAR_WIELD]) ? 1 : 0;
      else
        return 0;
    case 'g':
      if (mob && mob->fighting)
        return (mob->fighting->equipment[WEAR_WIELD]) ? 1 : 0;
      else
        return 0;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'iswielding'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eISWEAPPRI:
    if (fvict && fvict->equipment[WEAR_WIELD])
      return mprog_veval(fvict->equipment[WEAR_WIELD]->flags_.value[3], opr, dc_atoi(val));
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      if (mob->equipment[WEAR_WIELD])
        return mprog_veval(mob->equipment[WEAR_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    case 'z':
      if (mob->beacon && ((CharacterPtr)mob->beacon)->equipment[WEAR_WIELD])
        return mprog_veval(((CharacterPtr)mob->beacon)->equipment[WEAR_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return -1;
    case 'n':
      if (actor && actor->equipment[WEAR_WIELD])
        return mprog_veval(actor->equipment[WEAR_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    case 't':
      if (vict && vict->equipment[WEAR_WIELD])
        return mprog_veval(vict->equipment[WEAR_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    case 'r':
      if (rndm && rndm->equipment[WEAR_WIELD])
        return mprog_veval(rndm->equipment[WEAR_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isweappri'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eISWEAPSEC:
    if (fvict && fvict->equipment[WEAR_SECOND_WIELD])
      return mprog_veval(fvict->equipment[WEAR_SECOND_WIELD]->flags_.value[3], opr, dc_atoi(val));
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      if (mob->equipment[WEAR_SECOND_WIELD])
        return mprog_veval(mob->equipment[WEAR_SECOND_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    case 'z':
      if (mob->beacon && ((CharacterPtr)mob->beacon)->equipment[WEAR_SECOND_WIELD])
        return mprog_veval(((CharacterPtr)mob->beacon)->equipment[WEAR_SECOND_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return -1;
    case 'n':
      if (actor && actor->equipment[WEAR_SECOND_WIELD])
        return mprog_veval(actor->equipment[WEAR_SECOND_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    case 't':
      if (vict && vict->equipment[WEAR_SECOND_WIELD])
        return mprog_veval(vict->equipment[WEAR_SECOND_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    case 'r':
      if (rndm && rndm->equipment[WEAR_SECOND_WIELD])
        return mprog_veval(rndm->equipment[WEAR_SECOND_WIELD]->flags_.value[3], opr, dc_atoi(val));
      else
        return 0;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isweapsec'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eISNPC:
    if (fvict)
      return fvict->isNonPlayer();
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return true;
    case 'z':
      if (mob->beacon)
        return ((CharacterPtr)mob->beacon)->isNonPlayer();
      else
        return false;

    case 'n':
      if (actor)
        return actor->isNonPlayer();
      else
        return false;
    case 't':
      if (vict)
        return vict->isNonPlayer();
      else
        return false;
    case 'r':
      if (rndm)
        return rndm->isNonPlayer();
      else
        return false;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isnpc(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return false;
    }
    break;

  case eISGOOD:
    if (fvict)
      return IS_GOOD(fvict);
    if (ye)
      return false;

    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return IS_GOOD(mob);
    case 'z':
      if (mob->beacon)
        return IS_GOOD(((CharacterPtr)mob->beacon));
      else
        return -1;
    case 'n':
      if (actor)
        return IS_GOOD(actor);
      else
        return -1;
    case 't':
      if (vict)
        return IS_GOOD(vict);
      else
        return -1;
    case 'r':
      if (rndm)
        return IS_GOOD(rndm);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isgood(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    break;

  case eISNEUTRAL:
    if (fvict)
      return IS_NEUTRAL(fvict);
    if (ye)
      return false;

    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return IS_NEUTRAL(mob);
    case 'z':
      if (mob->beacon)
        return IS_NEUTRAL(((CharacterPtr)mob->beacon));
      else
        return -1;
    case 'n':
      if (actor)
        return IS_NEUTRAL(actor);
      else
        return -1;
    case 't':
      if (vict)
        return IS_NEUTRAL(vict);
      else
        return -1;
    case 'r':
      if (rndm)
        return IS_NEUTRAL(rndm);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isgood(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    break;

  case eISEVIL:
    if (fvict)
      return IS_EVIL(fvict);
    if (ye)
      return false;

    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return IS_EVIL(mob);
    case 'z':
      if (mob->beacon)
        return IS_EVIL(((CharacterPtr)mob->beacon));
      else
        return -1;
    case 'n':
      if (actor)
        return IS_EVIL(actor);
      else
        return -1;
    case 't':
      if (vict)
        return IS_EVIL(vict);
      else
        return -1;
    case 'r':
      if (rndm)
        return IS_EVIL(rndm);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isgood(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    break;

  case eISWORN:
  {
    ObjectPtr o = {};
    if (mob->mobdata->isObject())
    {
      o = mob->mobdata->getObject();
    }
    if (fvict)
      return is_wearing(fvict, o);
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'z':
      if (mob->beacon)
        return is_wearing(((CharacterPtr)mob->beacon), o);
      else
        return -1;
    case 'i':
      return -1;
    case 'n':
      if (actor)
        return is_wearing(actor, o);
      else
        return -1;
    case 't':
      if (vict)
        return is_wearing(actor, o);
      else
        return -1;
    case 'r':
      if (rndm)
        return is_wearing(rndm, o);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isword(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
  }
  break;

  case eISFIGHT:
    if (fvict)
      return fvict->fighting ? 1 : 0;
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'z':
      if (mob->beacon)
        return ((CharacterPtr)mob->beacon)->fighting ? 1 : 0;
      else
        return -1;

    case 'i':
      return (mob->fighting) ? 1 : 0;
    case 'n':
      if (actor)
        return (actor->fighting) ? 1 : 0;
      else
        return -1;
    case 't':
      if (vict)
        return (vict->fighting) ? 1 : 0;
      else
        return -1;
    case 'r':
      if (rndm)
        return (rndm->fighting) ? 1 : 0;
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isfight(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    break;

  case eISTANK:
    if (fvict)
      return fvict->isTank();
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'z':
      if (mob->beacon)
        return reinterpret_cast<CharacterPtr>(mob->beacon)->isTank();
      else
        return -1;

    case 'i':
      return mob->isTank();
    case 'n':
      if (actor)
        return actor->isTank();
      else
        return -1;
    case 't':
      if (vict)
        return vict->isTank() ? 1 : 0;
      else
        return -1;
    case 'r':
      if (rndm)
        return rndm->isTank() ? 1 : 0;
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to istank(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    break;

  case eISIMMORT:
    if (fvict)
      return fvict->getLevel() > IMMORTAL;
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return (mob->getLevel() > IMMORTAL);
    case 'z':
      if (mob->beacon)
        return ((CharacterPtr)mob->beacon)->getLevel() > IMMORTAL;
      else
        return -1;

    case 'n':
      if (actor)
        return (actor->getLevel() > IMMORTAL);
      else
        return -1;
    case 't':
      if (vict)
        return (vict->getLevel() > IMMORTAL);
      else
        return -1;
    case 'r':
      if (rndm)
        return (rndm->getLevel() > IMMORTAL);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: v%d r%d: bad argument to isimmort(): '%s'", dc_->mob_index_[mob->mobdata->nr]->vnum(), mob->mobdata->nr, ifchck);
      return -1;
    }
    break;

  case eISCHARMED:
    if (fvict)
      return IS_AFFECTED(fvict, AFF_CHARM);
    if (ye)
      return false;

    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return IS_AFFECTED(mob, AFF_CHARM);
    case 'z':
      if (mob->beacon)
        return IS_AFFECTED(((CharacterPtr)mob->beacon), AFF_CHARM);
      else
        return -1;

    case 'n':
      if (actor)
        return IS_AFFECTED(actor, AFF_CHARM);
      else
        return -1;
    case 't':
      if (vict)
        return IS_AFFECTED(vict, AFF_CHARM);
      else
        return -1;
    case 'r':
      if (rndm)
        return IS_AFFECTED(rndm, AFF_CHARM);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'ischarmed'",
                dc_->mob_index_[mob->mobdata->nr]->vnum());

      return -1;
    }
    break;

  case eISFOLLOW:
    if (fvict)
      return (fvict->master != nullptr && fvict->master->in_room == fvict->in_room);
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return (mob->master != nullptr && mob->master->in_room == mob->in_room);
    case 'z':
      if (mob->beacon)
        return ((CharacterPtr)mob->beacon)->master && ((CharacterPtr)mob->beacon)->master->in_room == ((CharacterPtr)mob->beacon)->in_room;
      else
        return -1;
    case 'n':
      if (actor)
        return (actor->master != nullptr && actor->master->in_room == actor->in_room);
      else
        return -1;
    case 't':
      if (vict)
        return (vict->master != nullptr && vict->master->in_room == vict->in_room);
      else
        return -1;
    case 'r':
      if (rndm)
        return (rndm->master != nullptr && rndm->master->in_room == rndm->in_room);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isfollow'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eISSPELLED:
  {
    qint32 find_skill_num(QString name);

    if (!str_cmp(val, "fly")) // needs special check.. sigh..
    {
      if (fvict && IS_AFFECTED(fvict, AFF_FLYING))
        return true;
      if (ye)
        return false;
      switch (arg[1])
      {
      case 'i': // mob
        if (IS_AFFECTED(mob, AFF_FLYING))
          return true;
        break;
      case 'z':
        if (mob->beacon)
          if (IS_AFFECTED(((CharacterPtr)mob->beacon), AFF_FLYING))
            return true;
        break;
      case 'n': // actor
        if (actor)
          if (IS_AFFECTED(actor, AFF_FLYING))
            return true;
        break;
      case 't': // vict
        if (vict)
          if (IS_AFFECTED(vict, AFF_FLYING))
            return true;
        break;
      case 'r': // rand
        if (rndm)
          if (IS_AFFECTED(rndm, AFF_FLYING))
            return true;
        break;
      case 'f':
        if (actor && actor->fighting)
          if (IS_AFFECTED(actor->fighting, AFF_FLYING))
            return true;
        break;
      case 'g':
        if (mob && mob->fighting)
          if (IS_AFFECTED(mob->fighting, AFF_FLYING))
            return true;
        break;
      default:
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isspelled'",
                  dc_->mob_index_[mob->mobdata->nr]->vnum());
        return -1;
      }
    }

    if (fvict)
      return (qint64)(fvict->affected_by_spell(find_skill_num(val)));
    if (ye)
      return false;
    switch (arg[1])
    {
    case 'i': // mob
      return (qint64)(mob->affected_by_spell(find_skill_num(val)));
    case 'z':
      if (mob->beacon)
        return (qint64)((CharacterPtr)mob->beacon)->affected_by_spell((find_skill_num(val)));
      else
        return -1;

    case 'n': // actor
      if (actor)
        return (qint64)(actor->affected_by_spell(find_skill_num(val)));
      else
        return -1;
    case 't': // vict
      if (vict)
        return (qint64)(vict->affected_by_spell(find_skill_num(val)));
      else
        return -1;
    case 'r': // rand
      if (rndm)
        return (qint64)(rndm->affected_by_spell(find_skill_num(val)));
      return -1;
    case 'f':
      if (actor && actor->fighting)
        return (qint64)(actor->fighting->affected_by_spell(find_skill_num(val)));
      else
        return -1;
    case 'g':
      if (mob && mob->fighting)
        return (qint64)(mob->fighting->affected_by_spell(find_skill_num(val)));
      else
        return 0;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isspelled'",
                dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
  }
  break;

  case eISAFFECTED:
    if (fvict)
      return (ISSET(fvict->affected_by, dc_atoi(val)));
    if (ye)
      return false;

    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      return (ISSET(mob->affected_by, dc_atoi(val)));
    case 'z':
      if (mob->beacon)
        return (ISSET(((CharacterPtr)mob->beacon)->affected_by, dc_atoi(val)));
      else
        return -1;

    case 'n':
      if (actor)
        return (ISSET(actor->affected_by, dc_atoi(val)));
      else
        return -1;
    case 't':
      if (vict)
        return (ISSET(vict->affected_by, dc_atoi(val)));
      else
        return -1;
    case 'r':
      if (rndm)
        return (ISSET(rndm->affected_by, dc_atoi(val)));
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isaffected'",
                dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eHITPRCNT:
  {
    qint32 lhsvl, rhsvl;
    if (fvict)
    {
      lhsvl = (fvict->hit * 100) / fvict->max_hit;
      rhsvl = dc_atoi(val);
      return mprog_veval(lhsvl, opr, rhsvl);
    }
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      lhsvl = (mob->hit * 100) / mob->max_hit;
      rhsvl = dc_atoi(val);
      return mprog_veval(lhsvl, opr, rhsvl);
    case 'z':
      if (mob->beacon)
      {
        lhsvl = (((CharacterPtr)mob->beacon)->hit * 100) / ((CharacterPtr)mob->beacon)->max_hit;
        rhsvl = dc_atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      }
      else
        return -1;

    case 'n':
      if (actor)
      {
        lhsvl = (actor->hit * 100) / actor->max_hit;
        rhsvl = dc_atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      }
      else
        return -1;
    case 't':
      if (vict)
      {
        lhsvl = (vict->hit * 100) / vict->max_hit;
        rhsvl = dc_atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      }
      else
        return -1;
    case 'r':
      if (rndm)
      {
        lhsvl = (rndm->hit * 100) / rndm->max_hit;
        rhsvl = dc_atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      }
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'hitprcnt'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
  }
  break;

  case eWEARS:
  {
    ObjectPtr obj = {};
    CharacterPtr take;
    QString bufeh;
    QString valu = one_argument(val, bufeh);

    if (fvict)
    {
      obj = search_char_for_item(fvict, real_object(dc_atoi(valu)), true);
      take = fvict;
    }
    else
    {
      if (ye)
        return false;
      switch (arg[1])
      {
      case 'z':
        if (!mob->beacon)
          return -1;
        obj = search_char_for_item(((CharacterPtr)mob->beacon), real_object(dc_atoi(valu)), true);
        take = ((CharacterPtr)mob->beacon);
      case 'i': // mob
        obj = search_char_for_item(mob, real_object(dc_atoi(valu)), true);
        take = mob;
        break;
      case 'n': // actor
        if (!actor)
          return -1;
        obj = search_char_for_item(actor, real_object(dc_atoi(valu)), true);
        take = actor;
        break;
      case 't': // vict
        if (!vict)
          return -1;
        obj = search_char_for_item(vict, real_object(dc_atoi(valu)), true);
        take = vict;
        break;
      case 'r': // rndm
        if (!rndm)
          return -1;
        obj = search_char_for_item(rndm, real_object(dc_atoi(valu)), true);
        take = rndm;
        break;
      default:
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'carries'", dc_->mob_index_[mob->mobdata->nr]->vnum());
        return -1;
      }
    }
    if (!obj)
      return 0;
    if (!str_cmp(bufeh, "keep"))
      return 1;
    else if (!str_cmp(bufeh, "take"))
    {
      qint32 i;
      if (obj->carried_by)
        obj_from_char(obj);
      for (i = {}; i < MAX_WEAR; i++)
        if (obj == take->equipment[i])
        {
          obj_from_char(take->unequip_char(i));
        }
      extract_obj(obj);
      return 1;
    }
    return -1;
  }
  break;

  case eCARRIES:
  {
    ObjectPtr obj = {};
    CharacterPtr take;
    QString bufeh;
    QString valu = one_argument(val, bufeh);
    if (fvict)
    {
      obj = search_char_for_item(fvict, real_object(dc_atoi(valu)), false);
      take = fvict;
    }
    else
    {
      if (ye)
        return false;
      switch (arg[1])
      {
      case 'z':
        if (!mob->beacon)
          return -1;
        obj = search_char_for_item(((CharacterPtr)mob->beacon), real_object(dc_atoi(valu)), false);
        take = ((CharacterPtr)mob->beacon);
      case 'i': // mob
        obj = search_char_for_item(mob, real_object(dc_atoi(valu)), false);
        take = mob;
        break;
      case 'n': // actor
        if (!actor)
          return -1;
        obj = search_char_for_item(actor, real_object(dc_atoi(valu)), false);
        take = actor;
        break;
      case 't': // vict
        if (!vict)
          return -1;
        obj = search_char_for_item(vict, real_object(dc_atoi(valu)), false);
        take = vict;
        break;
      case 'r': // rndm
        if (!rndm)
          return -1;
        obj = search_char_for_item(rndm, real_object(dc_atoi(valu)), false);
        take = rndm;
        break;
      default:
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'carries'", dc_->mob_index_[mob->mobdata->nr]->vnum());
        return -1;
      }
    }
    if (!obj)
      return 0;
    if (!str_cmp(bufeh, "keep"))
      return 1;
    else if (!str_cmp(bufeh, "take"))
    {
      qint32 i;
      if (obj->carried_by)
        obj_from_char(obj);
      for (i = {}; i < MAX_WEAR; i++)
        if (obj == take->equipment[i])
        {
          obj_from_char(take->unequip_char(i));
        }
      extract_obj(obj);
      return 1;
    }
    return -1;
  }
  break;

  case eNUMBER:
  {
    qint32 lhsvl, rhsvl;
    if (fvict)
    {
      if (fvict->isPlayer())
        return 0;
      lhsvl = dc_->mob_index_[fvict->mobdata->nr]->vnum();
      rhsvl = dc_atoi(val);
      return mprog_veval(lhsvl, opr, rhsvl);
    }
    if (ye)
      return false;

    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'i':
      lhsvl = dc_->mob_index_[mob->mobdata->nr]->vnum();
      rhsvl = dc_atoi(val);
      return mprog_veval(lhsvl, opr, rhsvl);
    case 'z':
      if (mob->beacon)
      {
        if (((CharacterPtr)mob->beacon)->isNonPlayer())
        {
          lhsvl = dc_->mob_index_[((CharacterPtr)mob->beacon)->mobdata->nr]->vnum();
          rhsvl = dc_atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        }
        else
          return 0;
      }
      else
        return 0;

    case 'n':
      if (actor)
      {
        if (actor->isNonPlayer())
        {
          lhsvl = dc_->mob_index_[actor->mobdata->nr]->vnum();
          rhsvl = dc_atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        }
      }
      else
        return 0;
    case 't':
      if (vict)
      {
        if (actor->isNonPlayer())
        {
          lhsvl = dc_->mob_index_[vict->mobdata->nr]->vnum();
          rhsvl = dc_atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        }
      }
      else
        return 0;
    case 'r':
      if (rndm)
      {
        if (actor->isNonPlayer())
        {
          lhsvl = dc_->mob_index_[rndm->mobdata->nr]->vnum();
          rhsvl = dc_atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        }
      }
      else
        return 0;
    case 'o':
      if (obj)
      {
        lhsvl = dc_->obj_index_[obj->item_number]->vnum();
        rhsvl = dc_atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      }
      else
        return -1;
    case 'p':
      if (v_obj)
      {
        lhsvl = dc_->obj_index_[v_obj->item_number]->vnum();
        rhsvl = dc_atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      }
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'number'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
  }
  break;

  case eTEMPVAR:
  {
    QString buf4, *buf4pt;
    if (arg[2] != '[')
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d badtarget  to 'tempvar'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    buf4pt = &arg[3];
    dc_strcpy(buf4, buf4pt);
    buf4pt = &buf4[0];
    while (*buf4pt != ']' && *buf4pt != '\0')
      buf4pt++;
    if (*buf4pt == '\0')
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad target to 'tempvar'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    *buf4pt = '\0';
    if (val[0] == '$' && val[1])
      mprog_translate(val[1], val, mob, actor, obj, vo, rndm);

    if (fvict)
      return mob->mprog_seval(fvict->getTemp(buf4), opr, val);
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'z':
      if (mob->beacon)
      {
        return mob->mprog_seval(((CharacterPtr)mob->beacon)->getTemp(buf4), opr, val);
      }
      else
        return -1;
    case 'i':
      return mob->mprog_seval(mob->getTemp(buf4), opr, val);
    case 'n':
      if (actor)
        return mob->mprog_seval(actor->getTemp(buf4), opr, val);
      else
        return -1;
    case 't':
      if (vict)
        return mob->mprog_seval(vict->getTemp(buf4), opr, val);
      else
        return -1;
    case 'r':
      if (rndm)
        return mob->mprog_seval(rndm->getTemp(buf4), opr, val);
      else
        return -1;
    case 'o':
      return -1;
    case 'p':
      return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'tempvar'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
  }
  break;

  // search a room for a mob with vnum arg
  case eISMOBVNUMINROOM:
  {
    qint32 target = dc_atoi(arg);

    for (CharacterPtr vch = dc_->world[mob->in_room]->people_;
         vch;
         vch = vch->next_in_room)
    {
      if (vch->isNonPlayer() &&
          vch != mob &&
          dc_->mob_index_[vch->mobdata->nr]->vnum() == target)
        return 1;
    }
    return 0;
  }
  break;

  // search a room for a obj with vnum arg
  case eISOBJVNUMINROOM:
  {
    qint32 target = dc_atoi(arg);

    for (ObjectPtr obj = dc_->world[mob->in_room]->contents_;
         obj;
         obj = obj->next_content)
    {
      if (dc_->obj_index_[obj->item_number]->vnum() == target)
        return 1;
    }
    return 0;
  }
  break;

  case eCANSEE:
    if (fvict)
      return CAN_SEE(mob, fvict, true);
    if (ye)
      return false;
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
    case 'z':
      return 1; // can always see holder
    case 'i':
      return 1;
    case 'n':
      if (actor)
        return CAN_SEE(mob, actor, true);
      else
        return -1;
    case 't':
      if (vict)
        return CAN_SEE(mob, vict, true);
      else
        return -1;
    case 'r':
      if (rndm)
        return CAN_SEE(mob, rndm, true);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'isnpc'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eINSAMEZONE:
    if (fvict)
      return (dc_->world[(fvict)->in_room]->zone == dc_->world[(mob)->in_room]->zone);
    else if ((fvict = get_pc(arg)) != nullptr)
      return (dc_->world[(fvict)->in_room]->zone == dc_->world[(mob)->in_room]->zone);

    switch (arg[1])
    {
    case 'i':
      return 1; // always in the same zone as itself
    case 'n':
      if (actor)
        return (dc_->world[(actor)->in_room]->zone == dc_->world[(mob)->in_room]->zone);
      else
        return -1;
    case 'r':
    case 't':
      if (vict)
        return (dc_->world[(vict)->in_room]->zone == dc_->world[(mob)->in_room]->zone);
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'insamezone'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eCLAN:
    if (fvict)
      return fvict->clan_id_;
    switch (arg[1])
    {
    case 'i':
      return mob->clan_id_; // always in the same zone as itself
    case 'n':
      if (actor)
        return actor->clan_id_;
      else
        return -1;
    case 'r':
    case 't':
      if (vict)
        return vict->clan_id_;
      else
        return -1;
    default:
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad argument to 'clan'", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return -1;
    }
    break;

  case eISDAYTIME:
    if (weather_info.sunlight == SUN_DARK)
      return false;
    return true;
    break;

  case eISRAINING:
    if (weather_info.sky == SKY_LIGHTNING || weather_info.sky == SKY_RAINING || weather_info.sky == SKY_HEAVY_RAIN)
      return true;
    return false;
    break;

  default:
    /* Ok... all the ifchcks are done, so if we didnt find ours then something
     * odd happened.  So report the bug and abort the MOBprogram (return error)
     */
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d unknown ifchck  \"%s\" value %d",
              dc_->mob_index_[mob->mobdata->nr]->vnum(), buf, ifcheck[buf]);
    return -1;
  }
}
/* Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */
// Null is kept globally so it doesn't go out of scope when we return in
// -pir 11/18/01
QString null;
// Mprog_cur_result holds the current status of the mob that is acting.
// It will hold ReturnValue::eCH_DIED if the mob died doing it's proc.  We need to
// make sure this is not true when returning from an if check or the
// mob's pointer is no longer valid
ReturnValues mprog_cur_result;
#define DIFF(a, b) ((a - b) > 0 ? (a - b) : (b - a))

QString mprog_process_if(QString ifchck, QString com_list, CharacterPtr mob,
                         CharacterPtr actor, ObjectPtr obj, void *vo,
                         CharacterPtr rndm, mprog_throw_type *thrw)
{

  QString buf;
  QString morebuf = {};
  QString cmnd = {};
  bool loopdone = false;
  bool flag = false;
  qint32 legal;

  *null = '\0';

  CharacterPtr ur = {};
  if (ur)
    ur->sendln(u"\r\nProg initiated."_s);

  if (!thrw || DIFF(ifchck, activeProgTmpBuf) >= thrw->startPos)
  {
    /* check for trueness of the ifcheck */
    if ((cIfs[ifpos++] = legal = mprog_do_ifchck(ifchck, mob, actor, obj, vo, rndm)))
    {
      if (legal >= 1)
        flag = true;
      else
        return null;
    }
    if (ur)
      ur->send(u"%d>%d\r\n"_s.arg(ifpos - 1).arg(legal));
  }
  else
  {
    legal = thrw->ifchecks[thrw->cPos++];
    cIfs[ifpos++] = legal;
    if (legal >= 1)
      flag = true;
    else if (legal < 0)
      return {};
    if (ur)
      ur->send(u"%d>-%d\r\n"_s.arg(thrw->cPos - 1).arg(legal));
  }

  while (loopdone == false) /*scan over any existing or statements */
  {
    cmnd = com_list;
    activePos = com_list = mprog_next_command(com_list);
    while (*cmnd == ' ')
      cmnd++;
    if (*cmnd == '\0')
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d no commands after IF/OR", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return null;
    }
    morebuf = one_argument(cmnd, buf);
    if (!str_cmp(buf, "or"))
    {

      if (!thrw || DIFF(morebuf, activeProgTmpBuf) >= thrw->startPos)
      {
        if ((cIfs[ifpos++] = legal = mprog_do_ifchck(morebuf, mob, actor, obj, vo, rndm)))
        {
          if (legal == 1)
            flag = true;
          else
            return null;
        }
        if (ur)
          ur->send(u"%d<%d\r\n"_s.arg(ifpos - 1).arg(legal));
      }
      else
      {
        legal = thrw->ifchecks[thrw->cPos++];
        cIfs[ifpos++] = legal;
        if (legal == 1)
          flag = true;
        else if (legal < 0)
          return {};
        if (ur)
          ur->send(u"%d<-%d\r\n"_s.arg(thrw->cPos - 1).arg(legal));
      }
    }
    else
      loopdone = true;
  }

  if (flag)
    for (;;) /*ifcheck was true, do commands but ignore else to endif*/
    {
      if (!str_cmp(buf, "if"))
      {
        com_list = mprog_process_if(morebuf, com_list, mob, actor, obj, vo, rndm, thrw);
        if (isSet(mprog_cur_result, ReturnValue::eCH_DIED))
          return null;
        while (*cmnd == ' ')
          cmnd++;
        if (*com_list == '\0')
          return null;
        cmnd = com_list;
        activePos = com_list = mprog_next_command(com_list);
        morebuf = one_argument(cmnd, buf);
        continue;
      }
      if (!str_cmp(buf, "break"))
        return null;
      if (!str_cmp(buf, "endif"))
        return com_list;
      if (!str_cmp(buf, "else"))
      {
        qint32 nest = {};
        while (str_cmp(buf, "endif") || nest > 0)
        {
          if (!str_cmp(buf, "if"))
            nest++;
          if (!str_cmp(buf, "endif"))
            nest--;
          cmnd = com_list;
          activePos = com_list = mprog_next_command(com_list);
          while (*cmnd == ' ')
            cmnd++;
          if (*cmnd == '\0')
          {
            dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing endif after else",
                      dc_->mob_index_[mob->mobdata->nr]->vnum());
            return null;
          }
          morebuf = one_argument(cmnd, buf);
        }
        return com_list;
      }

      if (!thrw || DIFF(cmnd, activeProgTmpBuf) >= thrw->startPos)
      {
        SET_BIT(mprog_cur_result, mprog_process_cmnd(cmnd, mob, actor, obj, vo, rndm));
        if (isSet(mprog_cur_result, ReturnValue::eCH_DIED) || isSet(mprog_cur_result, ReturnValue::eDELAYED_EXEC) || isSet(mprog_cur_result, ReturnValue::eVICT_DIED))
          return {};
      }
      cmnd = com_list;
      activePos = com_list = mprog_next_command(com_list);
      while (*cmnd == ' ')
        cmnd++;
      if (*cmnd == '\0')
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing else or endif", dc_->mob_index_[mob->mobdata->nr]->vnum());
        return null;
      }
      morebuf = one_argument(cmnd, buf);
    }
  else /*false ifcheck, find else and do existing commands or quit at endif*/
  {
    qint32 nest = {};
    while (true)
    { // Fix here 13/4 2004. Nested ifs are now taken into account.
      if (!str_cmp(buf, "if"))
        nest++;
      if (!str_cmp(buf, "endif"))
      {
        if (nest == 0)
          break;
        else
          nest--;
      }
      if (!nest && !str_cmp(buf, "else"))
        break;

      cmnd = com_list;
      activePos = com_list = mprog_next_command(com_list);
      while (*cmnd == ' ')
        cmnd++;
      if (*cmnd == '\0')
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing an else or endif",
                  dc_->mob_index_[mob->mobdata->nr]->vnum());
        return null;
      }
      morebuf = one_argument(cmnd, buf);
    }

    /* found either an else or an endif.. act accordingly */
    if (!str_cmp(buf, "endif"))
      return com_list;
    cmnd = com_list;
    activePos = com_list = mprog_next_command(com_list);
    while (*cmnd == ' ')
      cmnd++;
    if (*cmnd == '\0')
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing endif", dc_->mob_index_[mob->mobdata->nr]->vnum());
      return null;
    }
    morebuf = one_argument(cmnd, buf);

    for (;;) /*process the post-else commands until an endif is found.*/
    {
      if (!str_cmp(buf, "if"))
      {
        com_list = mprog_process_if(morebuf, com_list, mob, actor,
                                    obj, vo, rndm, thrw);
        if (isSet(mprog_cur_result, ReturnValue::eCH_DIED))
          return null;
        while (*cmnd == ' ')
          cmnd++;
        if (*com_list == '\0')
          return null;
        cmnd = com_list;
        activePos = com_list = mprog_next_command(com_list);
        morebuf = one_argument(cmnd, buf);
        continue;
      }
      if (!str_cmp(buf, "else"))
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d found else in an else section",
                  dc_->mob_index_[mob->mobdata->nr]->vnum());
        return null;
      }
      if (!str_cmp(buf, "break"))
        return null;
      if (!str_cmp(buf, "endif"))
        return com_list;

      if (!thrw || DIFF(cmnd, activeProgTmpBuf) >= thrw->startPos)
      {
        SET_BIT(mprog_cur_result, mprog_process_cmnd(cmnd, mob, actor, obj, vo, rndm));
        if (isSet(mprog_cur_result, ReturnValue::eCH_DIED) || isSet(mprog_cur_result, ReturnValue::eDELAYED_EXEC))
          return null;
      }
      cmnd = com_list;
      activePos = com_list = mprog_next_command(com_list);
      while (*cmnd == ' ')
        cmnd++;
      if (*cmnd == '\0')
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d missing endif in else section",
                  dc_->mob_index_[mob->mobdata->nr]->vnum());
        return null;
      }
      morebuf = one_argument(cmnd, buf);
    }
  }

  return {};
}

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object
 * conn->e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
void mprog_translate(CharacterPtr ch, QString t, CharacterPtr mob, CharacterPtr actor,
                     ObjectPtr obj, void *vo, CharacterPtr rndm)
{
  static const QStringList he_she = {"it", "he", "she"};
  static const QStringList him_her = {"it", "him", "her"};
  static const QStringList his_her = {"its", "his", "her"};
  CharacterPtr vict = (CharacterPtr)vo;
  ObjectPtr v_obj = vo;

  *t = '\0';

  auto q = mob->name();
  auto s = q.toStdString();
  auto mob_nameC = qPrintable(s);

  switch (ch)
  {
  case 'i':
    one_argument(mob_nameC, t);
    break;
  case 'z':
    if (mob->beacon)
    {
      one_argument(qPrintable(((CharacterPtr)mob->beacon)->name()), t);
      break;
    }
    dc_strcpy(t, "error");
    break;
  case 'Z':
    if (mob->beacon)
      dc_strcpy(t, qPrintable(((CharacterPtr)mob->beacon)->short_description()));
    else
      dc_strcpy(t, "error");
    break;
  case 'I':
    dc_strcpy(t, qPrintable(mob->short_description()));
    break;
  case 'x':
    if (mob->beacon && ((CharacterPtr)mob->beacon)->fighting)
      one_argument(qPrintable(((CharacterPtr)mob->beacon)->fighting->name()), t);
    else
      dc_strcpy(t, "error");
    *t = UPPER(*t);
    break;
  case 'n':
    if (actor)
    {
      // Mobs can see them no matter what.  Use "cansee()" if you don't want that
      //	   if ( CAN_SEE( mob,actor ) )
      one_argument(qPrintable(actor->name()), t);
      if (actor->isPlayer())
        *t = UPPER(*t);
    }
    else
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $ n in MOBProg.", dc_->mob_index_[mob->mobdata->nr]->vnum());
    break;

  case 'N':
    if (actor)
      if (CAN_SEE(mob, actor))
        if (actor->isNonPlayer())
          dc_strcpy(t, actor->short_desc);
        else
        {
          dc_strcpy(t, qPrintable(actor->name()));
          dc_strcat(t, " ");
          dc_strcat(t, actor->title);
        }
      else
        dc_strcpy(t, "someone");
    else
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $ N in MOBProg.", dc_->mob_index_[mob->mobdata->nr]->vnum());
    break;

  case 't':
    if (vict)
    {
      if (CAN_SEE(mob, vict))
        one_argument(qPrintable(vict->name()), t);
      if (vict->isPlayer())
        *t = UPPER(*t);
    }
    else
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $ t in MOBProg.", dc_->mob_index_[mob->mobdata->nr]->vnum());
    break;

  case 'T':
    if (vict)
      if (CAN_SEE(mob, vict))
        if (vict->isNonPlayer())
          dc_strcpy(t, vict->short_desc);
        else
        {
          dc_strcpy(t, qPrintable(vict->name()));
          dc_strcat(t, " ");
          dc_strcat(t, vict->title);
        }
      else
        dc_strcpy(t, "someone");
    else
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob %d trying illegal $T in MOBProg.", dc_->mob_index_[mob->mobdata->nr]->vnum());
    break;

  case 'f':
    if (actor && actor->fighting)
    {
      if (CAN_SEE(mob, actor->fighting))
        one_argument(actor->qPrintable(fighting->name()), t);
      if (actor->fighting->isPlayer())
        *t = UPPER(*t);
    }
    break;
  case 'F':
    if (actor && actor->fighting)
    {
      if (CAN_SEE(mob, actor->fighting))
      {
        if (actor->fighting->isNonPlayer())
          dc_strcpy(t, actor->fighting->short_desc);
        else
        {
          dc_strcpy(t, actor->qPrintable(fighting->name()));
          dc_strcat(t, " ");
          dc_strcat(t, actor->fighting->title);
        }
      }
    }
    break;
  case 'g':
    if (mob && mob->fighting)
    {
      if (CAN_SEE(mob, mob->fighting))
        one_argument(mob->qPrintable(fighting->name()), t);
      if (mob->fighting->isPlayer())
        *t = UPPER(*t);
    }
    break;
  case 'G':
    if (mob && mob->fighting)
    {
      if (CAN_SEE(mob, mob->fighting))
      {
        if (mob->fighting->isNonPlayer())
          dc_strcpy(t, mob->fighting->short_desc);
        else
        {
          dc_strcpy(t, mob->qPrintable(fighting->name()));
          dc_strcat(t, " ");
          dc_strcat(t, mob->fighting->title);
        }
      }
    }
    break;
  case 'r':
    if (rndm)
    {
      if (CAN_SEE(mob, rndm))
        one_argument(qPrintable(rndm->name()), t);
      if (rndm->isPlayer())
        *t = UPPER(*t);
    }
    break;

  case 'R':
    if (rndm)
    {
      if (CAN_SEE(mob, rndm))
      {
        if (rndm->isNonPlayer())
        {
          dc_strcpy(t, rndm->short_desc);
        }
        else
        {
          dc_strcpy(t, qPrintable(rndm->name()));
          dc_strcat(t, " ");
          dc_strcat(t, rndm->title);
        }
      }
      else
      {
        dc_strcpy(t, "someone");
      }
    }
    break;

  case 'e':
    if (actor)
      CAN_SEE(mob, actor) ? dc_strcpy(t, he_she[actor->sex])
                          : dc_strcpy(t, "someone");
    break;

  case 'm':
    if (actor)
      CAN_SEE(mob, actor) ? dc_strcpy(t, him_her[actor->sex])
                          : dc_strcpy(t, "someone");
    break;

  case 's':
    if (actor)
      CAN_SEE(mob, actor) ? dc_strcpy(t, his_her[actor->sex])
                          : dc_strcpy(t, "someone's");
    break;

  case 'E':
    if (vict)
      CAN_SEE(mob, vict) ? dc_strcpy(t, he_she[vict->sex])
                         : dc_strcpy(t, "someone");
    break;

  case 'M':
    if (vict)
      CAN_SEE(mob, vict) ? dc_strcpy(t, him_her[vict->sex])
                         : dc_strcpy(t, "someone");
    break;

  case 'S':
    if (vict)
      CAN_SEE(mob, vict) ? dc_strcpy(t, his_her[vict->sex])
                         : dc_strcpy(t, "someone's");
    break;
  case 'q':
    QString buf;
    dc_sprintf(buf, "%d", dc_->mob_index_[mob->mobdata->nr]->vnum());
    dc_strcpy(t, buf);
    break;
  case 'j':
    dc_strcpy(t, he_she[mob->sex]);
    break;

  case 'k':
    dc_strcpy(t, him_her[mob->sex]);
    break;

  case 'l':
    dc_strcpy(t, his_her[mob->sex]);
    break;

  case 'J':
    if (rndm)
      CAN_SEE(mob, rndm) ? dc_strcpy(t, he_she[rndm->sex])
                         : dc_strcpy(t, "someone");
    break;

  case 'K':
    if (rndm)
      CAN_SEE(mob, rndm) ? dc_strcpy(t, him_her[rndm->sex])
                         : dc_strcpy(t, "someone");
    break;

  case 'L':
    if (rndm)
      CAN_SEE(mob, rndm) ? dc_strcpy(t, his_her[rndm->sex])
                         : dc_strcpy(t, "someone's");
    break;

  case 'o':
    if (obj)
      CAN_SEE_OBJ(mob, obj) ? dc_strcpy(t, qPrintable(obj->name().split(' ').value(0)))
                            : dc_strcpy(t, "something");
    break;

  case 'O':
    if (obj)
      CAN_SEE_OBJ(mob, obj) ? dc_strcpy(t, qPrintable(obj->short_description()))
                            : dc_strcpy(t, "something");
    break;

  case 'p':
    if (v_obj)
      CAN_SEE_OBJ(mob, v_obj) ? dc_strcpy(t, qPrintable(v_obj->name().split(' ').value(0)))
                              : dc_strcpy(t, "something");
    break;

  case 'P':
    if (v_obj)
      CAN_SEE_OBJ(mob, v_obj) ? dc_strcpy(t, v_obj->short_description)
                              : dc_strcpy(t, "something");
    break;

  case 'a':
    if (obj)
      switch (*qPrintable(obj->name()))
      {
      case 'a':
      case 'e':
      case 'i':
      case 'o':
      case 'u':
        dc_strcpy(t, "an");
        break;
      default:
        dc_strcpy(t, "a");
      }
    break;

  case 'A':
    if (v_obj)
      switch (*(qPrintable(v_obj->name())))
      {
      case 'a':
      case 'e':
      case 'i':
      case 'o':
      case 'u':
        dc_strcpy(t, "an");
        break;
      default:
        dc_strcpy(t, "a");
      }
    break;

  case '$':
    dc_strcpy(t, "$");
    break;

  default:
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d bad $$var : %c", dc_->mob_index_[mob->mobdata->nr]->vnum(), ch);
    break;
  }
}

bool do_bufs(QString bufpt, QString argpt, QString point)
{
  bool traditional = false;

  /* get whatever comes before the left paren.. ignore spaces */
  while (*point)
    if (*point == '(')
    {
      traditional = true;
      break;
    }
    else if (*point == ' ')
    {
      return false;
    }
    else if (*point == '.')
    {
      break;
    }
    else if (*point == '\0')
    {
      return false;
    }
    else if (*point == ' ')
      point++;
    else
      *bufpt++ = *point++;
  *bufpt = '\0';
  point++;

  /* get whatever is in between the parens.. ignore spaces */
  while (*point)
    if (traditional && *point == ')')
      break;
    else if (!traditional && !isalpha(*point))
    {
      point--;
      break;
    }
    else if (*point == '\0')
    {
      return false;
    }
    else if (*point == ' ')
      point++;
    else
      *argpt++ = *point++;

  *argpt = '\0';
  //  point++;
  return true;
}

void debugpoint() {};
/* This procedure simply copies the cmnd to a buffer while expanding
 * any variables by calling the translate procedure.  The observant
 * code scrutinizer will notice that this is taken from act()
 */
ReturnValues mprog_process_cmnd(QString cmnd, CharacterPtr mob, CharacterPtr actor,
                                ObjectPtr obj, void *vo, CharacterPtr rndm)
{
  QString buf;
  QString tmp;
  QString str;
  QString i;
  QString point;

  point = buf;
  str = cmnd;
  while (*str == ' ')
    str++;
  /*
    while (*str != '\0')
    {
           if ((*str == '=' || *str == '+' || *str == '-' || *str == '&' || *str == '|' ||
                  *str == '*' || *str == '/') && *(str+1) == '=' && *(str+2) != '\0')
           {
            qint16 *lvali = {};
            quint32 *lvalui = {};
            QString *lvalstr = {};
            qint64 *lvali64 = {};
            qint8 *lvalb = {};
            *str = '\0';
            if (do_bufs(&buf[0], &tmp[0], cmnd))
                  translate_value(buf, tmp, &lvali,&lvalui, &lvalstr,&lvali64, &lvalb,mob,actor, obj, vo, rndm);
            else dc_strcpy(left, cmnd);

            str += 2;

            while ( *str == ' ' )
                  str++;
            if (do_bufs(&buf[0], &tmp[0], str))
                  translate_value(buf,tmp,&lvali,&lvalui, &lvalstr,&lvali64, &lvalb,mob,actor, obj, vo, rndm);
            else dc_strcpy(right, str);
          QString buf;
          buf[0] = '\0';
          if (lvali)
            dc_sprintf(buf, "%sLvali: %d\n", buf,*lvali);
          if (lvalui)
            dc_sprintf(buf, "%sLvalui: %d\n", buf,*lvalui);
          if (lvali64)
            dc_sprintf(buf, "%sLvali64: %ld\n", buf,*lvali64);
          if (lvalb)
            dc_sprintf(buf, "%sLvalb: %d\n", buf,*lvalb);
          if (lvalstr)
            dc_sprintf(buf, "%sLvalstr: %s\n", buf,lvalstr);
          dc_sprintf(buf,"%sLeft: %s\n",buf,left);
          dc_sprintf(buf,"%sRight: %s\n",buf,right);
  //        if (actor)
  //	  actor->send(buf);
          return ReturnValue::eSUCCESS;
           }
           str++;
    }*/
  str = cmnd;
  while (*str != '\0')
  {
    if (*str != '$')
    {
      *point++ = *str++;
      continue;
    }
    str++;
    if (*str == '\0')
      break; // panic!
    if (*(str + 1) == '.')
    {
      qint16 *lvali = {};
      quint32 *lvalui = {};
      QString *lvalstr = {};
      QString lvalqstr;
      qint64 *lvali64 = {};
      quint64 *lvalui64 = {};
      qint8 *lvalb = {};
      QString left, right;
      left[0] = '$';
      left[1] = *str;
      left[2] = '\0';
      str = one_argument(str + 2, right);
      translate_value(left, right, &lvali, &lvalui, &lvalstr, &lvali64, &lvalui64, &lvalb, mob, actor, obj, vo, rndm, lvalqstr);
      QString buf;
      buf[0] = '\0';
      if (lvali)
        dc_sprintf(buf, "%d", *lvali);
      if (lvalui)
        dc_sprintf(buf, "%u", *lvalui);
      if (lvalstr)
        dc_sprintf(buf, "%s", *lvalstr);
      if (lvali64)
        dc_sprintf(buf, "%ld", *lvali64);
      if (lvalui64)
        dc_sprintf(buf, "%lu", *lvalui64);
      if (lvalb)
        dc_sprintf(buf, "%d", *lvalb);
      for (qint32 i = {}; buf[i]; i++)
        *point++ = buf[i];
      continue;
    }

    if (LOWER(*str) == 'v' || LOWER(*str) == 'w')
    {
      QChar a = *str;
      str++;
      if (*str == '[')
      {
        QString tmp1 = &tmp[0];
        str++;
        while (*str != ']' && *str != '\0')
          *tmp1++ = *str++;
        *tmp1 = '\0';
        CharacterPtr who = {};
        if (a == 'v')
          who = mob;
        else if (a == 'V')
          who = actor;
        else if (a == 'w')
          who = (CharacterPtr)vo;
        else if (a == 'W')
          who = rndm;
        if (who)
        {
          tempvariable *eh = who->tempVariable;
          for (; eh; eh = eh->next)
            if (eh->name == tmp)
              break;
          if (eh)
          {
            dc_strcpy(tmp, qPrintable(eh->data));
            if (!eh->data.isEmpty())
              eh->data[0] = eh->data[0].toUpper();
          }
          else
            continue; // Doesn't have the variable.
        }
      }
    }
    else
      mprog_translate(*str, tmp, mob, actor, obj, vo, rndm);
    i = tmp;
    ++str;
    while ((*point = *i) != '\0')
      ++point, ++i;
  }
  *point = '\0';

  //  if(dc_strlen(buf) > MAX_INPUT_LENGTH-1)
  //    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Warning!  Mob '%s' has MobProg command longer than max input.", qPrintable(mob->name()));

  return mob->command_interpreter(buf, true);
}

bool objExists(ObjectPtr obj)
{
  ObjectPtr tobj;

  for (tobj = dc_->object_list; tobj; tobj = tobj->next)
    if (tobj == obj)
      break;
  if (tobj)
    return true;
  return false;
}

/* The main focus of the MOBprograms.  This routine is called
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 */
void mprog_driver(QString com_list, CharacterPtr mob, CharacterPtr actor, ObjectPtr obj, void *vo, mprog_throw_type *thrw, CharacterPtr rndm)
{
  QString tmpcmndlst;
  QString buf;
  QString morebuf;
  QString command_list;
  QString cmnd;
  // CharacterPtr rndm  = {};
  CharacterPtr vch = {};
  qint32 count = {};
  if (IS_AFFECTED(mob, AFF_CHARM))
    return;
  selfpurge = false;
  mprog_cur_result = ReturnValue::eSUCCESS;
  mprog_line_num = {};

  activeProgs++;
  if (activeProgs > 20)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Mob: %d : Too many active mobprograms : LOOP", dc_->mob_index_[mob->mobdata->nr]->vnum());
    activeProgs--;
    return;
  }

  // qint32 cIfs; // for MPPAUSE
  // qint32 ifpos;
  ifpos = {};
  memset(&cIfs[0], 0, sizeof(qint32) * 256);

  if (!charExists(actor))
    actor = {};
  if (!objExists(obj))
    obj = {};

  activeActor = actor;
  activeObj = obj;
  activeVo = vo;

  activeProg = com_list;

  if (!com_list) // this can happen when someone is editing
  {
    activeActor = activeRndm = {};
    activeObj = {};
    activeVo = {};
    activeProgs--;
    return;
  }

  // count valid random victs in room
  if (mob->in_room > 0)
  {
    for (vch = dc_->world[mob->in_room]->people_; vch; vch = vch->next_in_room)
      if (CAN_SEE(mob, vch, true) && vch->isPlayer())
        count++;

    if (count)
      count = dc_->number(1, count); // if we have valid victs, choose one

    if (!rndm && count)
    {
      for (vch = dc_->world[mob->in_room]->people_; vch && count;)
      {
        if (CAN_SEE(mob, vch, true) && vch->isPlayer())
          count--;
        if (count)
          vch = vch->next_in_room;
      }
      rndm = vch;
    }

    activeRndm = rndm;
  }

  dc_strcpy(tmpcmndlst, com_list);

  command_list = tmpcmndlst;
  cmnd = command_list;
  activeProgTmpBuf = command_list;
  activePos = command_list = mprog_next_command(command_list);

  // if (thrw) thrw->orig = &tmpcmndlst[0];
  while (*cmnd != '\0')
  {
    morebuf = one_argument(cmnd, buf);

    if (!str_cmp(buf, "if"))
    {
      activePos = command_list = mprog_process_if(morebuf, command_list, mob,
                                                  actor, obj, vo, rndm, thrw);
      if (isSet(mprog_cur_result, ReturnValue::eCH_DIED) || isSet(mprog_cur_result, ReturnValue::eDELAYED_EXEC))
      {
        activeActor = activeRndm = {};
        activeObj = {};
        activeVo = {};
        activeProgs--;
        return;
      }
    }
    else
    {

      if (!thrw || DIFF(cmnd, activeProgTmpBuf) >= thrw->startPos)
      {
        SET_BIT(mprog_cur_result, mprog_process_cmnd(cmnd, mob, actor, obj, vo, rndm));
        if (isSet(mprog_cur_result, ReturnValue::eCH_DIED) || selfpurge || isSet(mprog_cur_result, ReturnValue::eDELAYED_EXEC))
        {
          activeActor = activeRndm = {};
          activeObj = {};
          activeVo = {};
          activeProgs--;
          return;
        }
      }
    }
    cmnd = command_list;
    activePos = command_list = mprog_next_command(command_list);
  }
  activeActor = activeRndm = {};
  activeObj = {};
  activeVo = {};
  activeProgs--;
}

/***************************************************************************
 * Global function code and brief comments.
 */

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
// Returns true if match
// false if no match
ReturnValues mprog_wordlist_check(QString arg, CharacterPtr mob, CharacterPtr actor,
                                  ObjectPtr obj, void *vo, qint32 type, bool reverse)
// reverse ALSO IMPLIES IT ALSO ONLY CHECKS THE WEAR_WIELD WORD
{

  QString temp1 = {};
  QString temp2 = {};
  QString word = {};
  MobileProgramPtr mprg = {};
  MobileProgramPtr next = {};
  QString list = {};
  QString start = {};
  QString dupl = {};
  QString end = {};
  qint32 i = {};
  ReturnValues retval = {};
  bool done = {};
  //  for ( mprg = dc_->mob_index_[mob->mobdata->nr]->programs_; mprg != nullptr; mprg
  //= next )
  mprg = dc_->mob_index_[mob->mobdata->nr]->programs_;
  if (!mprg)
  {
    done = true;
    mprg = dc_->mob_index_[mob->mobdata->nr].mobspec;
  }

  mprog_command_num = {};
  for (; mprg != nullptr; mprg = next)
  {
    mprog_command_num++;
    next = mprg->next;
    if (mprg->type & type)
    {
      if (!reverse)
        dc_strcpy(temp1, mprg->arglist);
      else
        dc_strcpy(temp1, qPrintable(arg));

      list = temp1;
      for (i = {}; i < (qint32)dc_strlen(list); i++)
        list[i] = LOWER(list[i]);

      if (!reverse)
        dc_strcpy(temp2, qPrintable(arg));
      else
        dc_strcpy(temp2, mprg->arglist);

      dupl = temp2;
      for (i = {}; i < (qint32)dc_strlen(dupl); i++)
        dupl[i] = LOWER(dupl[i]);
      if ((list[0] == 'p') && (list[1] == ' '))
      {
        list += 2;
        while ((start = strstr(dupl, list)))
          if ((start == dupl || *(start - 1) == ' ') && (*(end = start + dc_strlen(list)) == ' ' || *end == '\n' || *end == '\r' || *end == '\0'
                                                         // allow punctuation at the end
                                                         || *end == '.' || *end == '?' || *end == '!'))
          {
            retval = 1;
            mprog_driver(mprg->comlist, mob, actor, obj, vo, nullptr, nullptr);
            if (selfpurge || isSet(mprog_cur_result, ReturnValue::eCH_DIED))
              return retval;
            break;
          }
          else
            dupl = start + 1;
      }
      else
      {
        list = one_argument(list, word);
        for (; word[0] != '\0'; list = one_argument(list, word))
        {
          while ((start = strstr(dupl, word)))
            if ((start == dupl || *(start - 1) == ' ') && (*(end = start + dc_strlen(word)) == ' ' || *end == '\n' || *end == '\r' || *end == '\0'
                                                           // allow punctuation at the end
                                                           || *end == '.' || *end == '?' || *end == '!'))
            {
              retval = 1;
              mprog_driver(mprg->comlist, mob, actor, obj, vo,
                           nullptr, nullptr);
              if (selfpurge || isSet(mprog_cur_result, ReturnValue::eCH_DIED))
                return retval;
              break;
            }
            else
              dupl = start + 1;
          if (reverse)
            break;
        }
      }
    }
    if (next == nullptr && !done)
    {
      done = true;
      next = dc_->mob_index_[mob->mobdata->nr].mobspec;
    }
  }
  return retval;
}

void mprog_percent_check(CharacterPtr mob, CharacterPtr actor, ObjectPtr obj,
                         void *vo, qint32 type)
{
  MobileProgramPtr mprg = {};
  MobileProgramPtr next = {};
  bool done = false;
  mprg = dc_->mob_index_[mob->mobdata->nr]->programs_;
  if (!mprg)
  {
    done = true;
    mprg = dc_->mob_index_[mob->mobdata->nr].mobspec;
  }

  if (dc_->mob_index_[mob->mobdata->nr]->vnum() == 30013)
  {
    debugpoint();
  }

  mprog_command_num = {};
  for (; mprg != nullptr; mprg = next)
  {
    mprog_command_num++;
    next = mprg->next;
    if ((mprg->type & type) && (ch->dc_->number(0, 99) < dc_atoi(mprg->arglist)))
    {
      mprog_driver(mprg->comlist, mob, actor, obj, vo, nullptr, nullptr);
      if (selfpurge)
        return;
      if (type != GREET_PROG && type != ALL_GREET_PROG)
        break;
    }
    if (!next && !done)
    {
      done = true;
      next = dc_->mob_index_[mob->mobdata->nr].mobspec;
    }
  }
}

/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
ReturnValues mprog_act_trigger(QString buf, CharacterPtr mob, CharacterPtr ch,
                               ObjectPtr obj, void *vo)
{

  //  mob_prog_act_list * tmp_act;
  // mob_prog_act_list * curr;
  //  MobileProgramPtr  mprg;
  mprog_cur_result = ReturnValue::eSUCCESS;

  if (!MOBtrigger)
    return mprog_cur_result;

  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & ACT_PROG) && isPaused(mob) == false)
    mprog_wordlist_check(qPrintable(buf), mob, ch, obj, vo, ACT_PROG);

  return mprog_cur_result;
}

ReturnValues mprog_bribe_trigger(CharacterPtr mob, CharacterPtr ch, qint32 amount)
{

  MobileProgramPtr mprg = {};
  MobileProgramPtr next = {};
  ObjectPtr obj = {};
  bool done = false;

  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & BRIBE_PROG) && isPaused(mob) == false)
  {
    mob->removeGold(amount);

    mprg = dc_->mob_index_[mob->mobdata->nr]->programs_;
    if (!mprg)
    {
      done = true;
      mprg = dc_->mob_index_[mob->mobdata->nr].mobspec;
    }

    mprog_command_num = {};
    for (; mprg != nullptr; mprg = next)
    {
      mprog_command_num++;
      next = mprg->next;

      if ((mprg->type & BRIBE_PROG) && (amount >= dc_atoi(mprg->arglist)))
      {
        mprog_driver(mprg->comlist, mob, ch, obj, nullptr, nullptr, nullptr);
        if (selfpurge)
          return mprog_cur_result;
        break;
      }

      if (!next && !done)
      {
        next = dc_->mob_index_[mob->mobdata->nr].mobspec;
        done = true;
      }
    }
  }

  return mprog_cur_result;
}

ReturnValues mprog_damage_trigger(CharacterPtr mob, CharacterPtr ch, qint32 amount)
{

  MobileProgramPtr mprg = {};
  MobileProgramPtr next = {};
  ObjectPtr obj = {};
  bool done = false;
  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & DAMAGE_PROG) && isPaused(mob) == false)
  {
    mprg = dc_->mob_index_[mob->mobdata->nr]->programs_;

    if (!mprg)
    {
      done = true;
      mprg = dc_->mob_index_[mob->mobdata->nr].mobspec;
    }

    mprog_command_num = {};
    for (; mprg != nullptr; mprg = next)
    {
      mprog_command_num++;
      next = mprg->next;

      if ((mprg->type & DAMAGE_PROG) && (amount >= dc_atoi(mprg->arglist)))
      {
        mprog_driver(mprg->comlist, mob, ch, obj, nullptr, nullptr, nullptr);
        if (selfpurge)
          return mprog_cur_result;
        break;
      }
      if (!next && !done)
      {
        next = dc_->mob_index_[mob->mobdata->nr].mobspec;
        done = true;
      }
    }
  }

  return mprog_cur_result;
}

ReturnValues mprog_death_trigger(CharacterPtr mob, CharacterPtr killer)
{

  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & DEATH_PROG) && isPaused(mob) == false)
  {
    mprog_percent_check(mob, killer, nullptr, nullptr, DEATH_PROG);
  }
  if (!SOMEONE_DIED(mprog_cur_result))
    death_cry(mob);
  return mprog_cur_result;
}

ReturnValues mprog_entry_trigger(CharacterPtr mob)
{

  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & ENTRY_PROG) && isPaused(mob) == false)
    mprog_percent_check(mob, nullptr, nullptr, nullptr, ENTRY_PROG);

  return mprog_cur_result;
}

ReturnValues mprog_fight_trigger(CharacterPtr mob, CharacterPtr ch)
{

  if (mob->isNonPlayer() && MOB_WAIT_STATE(mob) <= 0 && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & FIGHT_PROG) && isPaused(mob) == false)
    mprog_percent_check(mob, ch, nullptr, nullptr, FIGHT_PROG);

  return mprog_cur_result;
}

ReturnValues mprog_attack_trigger(CharacterPtr mob, CharacterPtr ch)
{

  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & ATTACK_PROG) && isPaused(mob) == false)
    mprog_percent_check(mob, ch, nullptr, nullptr, ATTACK_PROG);

  return mprog_cur_result;
}

ReturnValues mprog_give_trigger(CharacterPtr mob, CharacterPtr ch, ObjectPtr obj)
{

  QString buf;
  MobileProgramPtr mprg = {};
  MobileProgramPtr next = {};
  bool done = false, okay = false;
  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & GIVE_PROG) && isPaused(mob) == false)
  {
    mprg = dc_->mob_index_[mob->mobdata->nr]->programs_;
    if (!mprg)
    {
      done = true;
      mprg = dc_->mob_index_[mob->mobdata->nr].mobspec;
    }

    mprog_command_num = {};
    for (; mprg != nullptr; mprg = next)
    {
      mprog_command_num++;

      next = mprg->next;
      one_argument(mprg->arglist, buf);
      if ((mprg->type & GIVE_PROG) && ((obj->name() == mprg->arglist)) || (!str_cmp("all", buf)))
      {
        okay = true;
        mprog_driver(mprg->comlist, mob, ch, obj, nullptr, nullptr, nullptr);
        if (selfpurge)
          return mprog_cur_result;
        break;
      }

      if (!next && !done)
      {
        done = true;
        next = dc_->mob_index_[mob->mobdata->nr].mobspec;
      }
    }
  }

  if (okay && !SOMEONE_DIED(mprog_cur_result))
  {
    ObjectPtr a;
    SET_BIT(mprog_cur_result, ReturnValue::eEXTRA_VALUE);
    for (a = mob->carrying; a; a = a->next_content)
      if (a == obj)
      {
        REMOVE_BIT(mprog_cur_result, ReturnValue::eEXTRA_VALUE);
      }
  }

  return mprog_cur_result;
}

qint32 Character::mprog_greet_trigger(void)
{
  mprog_cur_result = ReturnValue::eSUCCESS;
  for (auto vmob = dc_->world[in_room]->people_; vmob != nullptr; vmob = vmob->next_in_room)
    if (vmob->isNonPlayer() && (vmob->fighting == nullptr) && AWAKE(vmob))
    {
      if (this != vmob && CAN_SEE(vmob, this) && (dc_->mob_index_[vmob->mobdata->nr]->progtypes_ & GREET_PROG) && isPaused(vmob) == false)
        mprog_percent_check(vmob, this, nullptr, nullptr, GREET_PROG);
      else if ((dc_->mob_index_[vmob->mobdata->nr]->progtypes_ & ALL_GREET_PROG) && isPaused(vmob) == false)
        mprog_percent_check(vmob, this, nullptr, nullptr, ALL_GREET_PROG);

      if (SOMEONE_DIED(mprog_cur_result) || selfpurge)
        break;
    }
  return mprog_cur_result;
}

ReturnValues mprog_hitprcnt_trigger(CharacterPtr mob, CharacterPtr ch)
{
  MobileProgramPtr mprg = {};
  MobileProgramPtr next = {};
  bool done = false;

  if (mob->isNonPlayer() && MOB_WAIT_STATE(mob) <= 0 && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & HITPRCNT_PROG) && isPaused(mob) == false)
  {
    mprg = dc_->mob_index_[mob->mobdata->nr]->programs_;
    if (!mprg)
    {
      done = true;
      mprg = dc_->mob_index_[mob->mobdata->nr].mobspec;
    }

    mprog_command_num = {};
    for (; mprg != nullptr; mprg = next)
    {
      mprog_command_num++;
      next = mprg->next;

      if ((mprg->type & HITPRCNT_PROG) && ((100 * mob->hit / mob->max_hit) < dc_atoi(mprg->arglist)))
      {
        mprog_driver(mprg->comlist, mob, ch, nullptr, nullptr, nullptr, nullptr);
        if (selfpurge)
          return mprog_cur_result;
        break;
      }

      if (!next && !done)
      {
        done = true;
        next = dc_->mob_index_[mob->mobdata->nr].mobspec;
      }
    }
  }

  return mprog_cur_result;
}

ReturnValues mprog_random_trigger(CharacterPtr mob)
{
  mprog_cur_result = ReturnValue::eSUCCESS;

  if ((dc_->mob_index_[mob->mobdata->nr]->progtypes_ & RAND_PROG) && isPaused(mob) == false)
    mprog_percent_check(mob, nullptr, nullptr, nullptr, RAND_PROG);

  return mprog_cur_result;
}

ReturnValues mprog_load_trigger(CharacterPtr mob)
{
  if (!mob || mob->isDead() || mob->isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  mprog_cur_result = ReturnValue::eSUCCESS;
  if ((dc_->mob_index_[mob->mobdata->nr]->progtypes_ & LOAD_PROG) && isPaused(mob) == false)
    mprog_percent_check(mob, nullptr, nullptr, nullptr, LOAD_PROG);
  return mprog_cur_result;
}

ReturnValues mprog_arandom_trigger(CharacterPtr mob)
{
  if (!mob || mob->isDead() || mob->isNowhere())
  {
    return ReturnValue::eFAILURE;
  }
  mprog_cur_result = ReturnValue::eSUCCESS;
  if ((dc_->mob_index_[mob->mobdata->nr]->progtypes_ & ARAND_PROG) && isPaused(mob) == false)
    mprog_percent_check(mob, nullptr, nullptr, nullptr, ARAND_PROG);
  return mprog_cur_result;
}

qint32 Character::mprog_can_see_trigger(CharacterPtr mob)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  if (!mob || mob->isDead() || mob->isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  mprog_cur_result = ReturnValue::eSUCCESS;
  if ((dc_->mob_index_[mob->mobdata->nr]->progtypes_ & CAN_SEE_PROG) && isPaused(mob) == false)
    mprog_percent_check(mob, this, nullptr, nullptr, CAN_SEE_PROG);

  return mprog_cur_result;
}

qint32 Character::mprog_speech_trigger(const QString txt)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vmob;

  mprog_cur_result = ReturnValue::eSUCCESS;

  for (vmob = dc_->world[in_room]->people_; vmob != nullptr; vmob = vmob->next_in_room)
    if (vmob->isNonPlayer() && (dc_->mob_index_[vmob->mobdata->nr]->progtypes_ & SPEECH_PROG) && isPaused(vmob) == false)
    {
      if (mprog_wordlist_check(txt, vmob, this, nullptr, nullptr, SPEECH_PROG))
        break;
    }

  return mprog_cur_result;
}

ReturnValues mprog_catch_trigger(CharacterPtr mob, qint32 catch_num, QString var, qint32 opt, CharacterPtr actor, ObjectPtr obj, void *vo, CharacterPtr rndm)
{
  if (!mob || mob->isDead() || mob->isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  MobileProgramPtr mprg = {};
  MobileProgramPtr next = {};
  qint32 curr_catch;
  bool done = false;
  mprog_cur_result = ReturnValue::eFAILURE;

  if (mob->isNonPlayer() && (dc_->mob_index_[mob->mobdata->nr]->progtypes_ & CATCH_PROG) && isPaused(mob) == false)
  {
    mprg = dc_->mob_index_[mob->mobdata->nr]->programs_;
    if (!mprg || (opt & 1))
    {
      done = true;
      mprg = dc_->mob_index_[mob->mobdata->nr].mobspec;
    }

    mprog_command_num = {};
    for (; mprg != nullptr; mprg = next)
    {
      mprog_command_num++;
      next = mprg->next;
      if (mprg->type & CATCH_PROG)
      {
        if (!check_range_valid_and_convert(curr_catch, mprg->arglist, MPROG_CATCH_MIN, MPROG_CATCH_MAX))
        {
          dc_->logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "Invalid catch argument: vnum %d",
                    dc_->mob_index_[mob->mobdata->nr]->vnum());
          return ReturnValue::eFAILURE;
        }
        if (curr_catch == catch_num)
        {
          if (var)
          {
            tempvariable *eh;
            for (eh = mob->tempVariable; eh; eh = eh->next)
            {
              if (eh->name == "throw")
                break;
            }
            if (eh)
            {
              eh->data = var;
            }
            else
            {
              eh = new tempvariable;

              eh->data = var;
              eh->name = u"throw"_s;
              eh->next = mob->tempVariable;
              mob->tempVariable = eh;
            }
          }
          mprog_driver(mprg->comlist, mob, actor, obj, vo, nullptr, rndm);
          if (selfpurge)
            return mprog_cur_result;

          break;
        }
      }
      if (!next && !done)
      {
        done = true;
        next = dc_->mob_index_[mob->mobdata->nr].mobspec;
      }
    }
  }
  return mprog_cur_result;
}

void DC::update_mprog_throws(void)
{
  mprog_throw_type *curr;
  mprog_throw_type *action;
  mprog_throw_type *last = {};
  CharacterPtr vict;
  ObjectPtr vobj;
  for (curr = g_mprog_throw_list; curr;)
  {
    // update
    if (curr->delay > 0)
    {
      curr->delay--;
      last = curr;
      curr = curr->next;
      continue;
    }
    vobj = {};
    vict = {};

    //		if (curr->data_num == -999)
    //			debugpoint();

    if (curr->tMob && charExists(curr->tMob) && curr->tMob->in_room >= 0)
    {
      vict = curr->tMob;
    }
    else if (curr->mob)
    {
      // find target
      if (*curr->target_mob_name)
      { // find me by name
        vict = get_mob(curr->target_mob_name);
      }
      else
      { // find me by num
        vict = get_mob_vnum(curr->target_mob_num);
      }
    }
    else
    {
      if (*curr->target_mob_name)
        vobj = get_obj(curr->target_mob_name);
      else
        vobj = get_obj_vnum(curr->target_mob_num);
    }

    // remove from list
    if (last)
    {
      last->next = curr->next;
      action = curr;
      curr = last->next;
      // last doesn't move
    }
    else
    {
      g_mprog_throw_list = curr->next;
      action = curr;
      curr = g_mprog_throw_list;
    }

    // This is done this way in case the 'catch' does a 'throw' inside of it

    // if !vict, oh well....remove it anyway.  Someone killed him.
    if (action->data_num == -999 && vict)
    { // 'tis a pause
      // Only resume a MPPAUSE <duration> if we're not in the middle of a MPPAUSE all <duration>
      if (isPaused(action->tMob) == false)
      {
        mprog_driver(action->orig, vict, action->actor, action->obj, action->vo, action, action->rndm);
      }

      action->orig = {};
      action->orig = {};
    }
    else if (action->data_num == -1000 && vict)
    {
      if (isPaused(action->tMob))
      {
        action->tMob->mobdata->paused = false;
      }
      mprog_driver(action->orig, vict, action->actor, action->obj, action->vo, action, action->rndm);
      action->orig = {};
      action->orig = {};
    }
    else if (vict)
    { // activate
      if (vict->in_room >= 0)
      {
        mprog_catch_trigger(vict, action->data_num, action->var, action->opt, action->actor, action->obj, action->vo, action->rndm);
      }
    }
    else if (vobj)
    {
      oprog_catch_trigger(vobj, action->data_num, action->var, action->opt, action->actor, action->obj, action->vo, action->rndm);
    }

    action = {};
  }
}

CharacterPtr DC::initiate_oproc(CharacterPtr ch, ObjectPtr obj)
{ // Sneakiness.
  CharacterPtr temp;
  temp = clone_mobile(real_mobile(12));
  mob_index_[real_mobile(12)]->programs_ = obj_index_[obj->item_number]->programs_;
  mob_index_[real_mobile(12)]->progtypes_ = obj_index_[obj->item_number]->progtypes_;

  if (ch)
    char_to_room(temp, ch->in_room);
  else
    char_to_room(temp, obj->in_room);
  if (ch)
    temp->beacon = ch;
  temp->mobdata->setObject(obj);
  //  temp->master = ch;
  // temp->short_desc={};
  temp->short_desc = obj->short_description;
  QString buf;
  dc_sprintf(buf, "%s", qPrintable(obj->name()));
  for (qint32 i = dc_strlen(buf) - 1; i > 0; i--)
    if (buf[i] == ' ')
    {
      buf[i] = '\0';
      break;
    }
  temp->name(buf);

  temp->setType(Character::Type::Object);
  temp->objdata = obj;

  return temp;
}

void end_oproc(CharacterPtr ch)
{
  static qint32 core_counter = {};
  if (selfpurge)
  {
    dc_->logentry(u"Crash averted in end_oproc() %1 %2"_s.arg(selfpurge.getFunction().c_str()).arg(selfpurge.getState()), IMMORTAL, DC::LogChannel::LOG_BUG);

    if (core_counter++ < 10)
    {
      produce_coredump();
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Corefile produced.");
    }
  }
  else
  {
    trace.addTrack("end_oproc");
    extract_char(ch, true, trace);
    dc_->mob_index_[real_mobile(12)]->progtypes_ = {};
    dc_->mob_index_[real_mobile(12)]->programs_ = {};
  }
}

qint32 Character::oprog_can_see_trigger(ObjectPtr item)
{
  if (!isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vmob;
  mprog_cur_result = ReturnValue::eSUCCESS;

  if (dc_->obj_index_[item->item_number]->progtypes_ & CAN_SEE_PROG)
  {
    vmob = dc_->initiate_oproc(this, item);
    mprog_percent_check(vmob, this, item, nullptr, CAN_SEE_PROG);
    end_oproc(vmob);
    return mprog_cur_result;
  }
  return mprog_cur_result;
}

qint32 Character::oprog_speech_trigger(const QString txt)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vmob = {};
  ObjectPtr item;

  mprog_cur_result = ReturnValue::eSUCCESS;

  for (item = dc_->world[in_room]->contents_; item; item = item->next_content)
    if (dc_->obj_index_[item->item_number]->progtypes_ & SPEECH_PROG)
    {
      vmob = dc_->initiate_oproc(this, item);
      if (mprog_wordlist_check(txt, vmob, this, nullptr, nullptr, SPEECH_PROG))
      {
        end_oproc(vmob);
        return mprog_cur_result;
      }
      end_oproc(vmob);
    }
  for (item = carrying; item; item = item->next_content)
    if (dc_->obj_index_[item->item_number]->progtypes_ & SPEECH_PROG)
    {
      vmob = dc_->initiate_oproc(this, item);
      if (mprog_wordlist_check(txt, vmob, this, nullptr, nullptr, SPEECH_PROG))
      {
        end_oproc(vmob);
        return mprog_cur_result;
      }
      end_oproc(vmob);
    }

  for (qint32 i = {}; i < MAX_WEAR; i++)
    if (equipment[i])
      if (dc_->obj_index_[equipment[i]->item_number]->progtypes_ & SPEECH_PROG)
      {
        vmob = dc_->initiate_oproc(this, equipment[i]);
        if (mprog_wordlist_check(txt, vmob, this, nullptr, nullptr, SPEECH_PROG))
        {
          end_oproc(vmob);
        }
        end_oproc(vmob);
      }
  return mprog_cur_result;
}

qint32 DC::oprog_catch_trigger(ObjectPtr obj, qint32 catch_num, QString var, qint32 opt, CharacterPtr actor, ObjectPtr obj2, void *vo, CharacterPtr rndm)
{
  MobileProgramPtr mprg = {};
  qint32 curr_catch;
  mprog_cur_result = ReturnValue::eFAILURE;
  CharacterPtr vmob;

  if (obj_index_[obj->item_number]->progtypes_ & CATCH_PROG)
  {
    mprg = obj_index_[obj->item_number]->programs_;
    mprog_command_num = {};
    for (; mprg != nullptr; mprg = mprg->next)
    {
      mprog_command_num++;

      if (mprg->type & CATCH_PROG)
      {
        if (!check_range_valid_and_convert(curr_catch, mprg->arglist, MPROG_CATCH_MIN, MPROG_CATCH_MAX))
        {
          dc_->logf(IMMORTAL, LogChannel::LOG_WORLD, "Invalid catch argument: vnum %d", obj_index_[obj->item_number]->vnum());
          return ReturnValue::eFAILURE;
        }
        if (curr_catch == catch_num)
        {
          vmob = initiate_oproc(nullptr, obj);
          if (var)
          {
            tempvariable *eh;

            auto eh = new tempvariable;

            eh->data = var;
            eh->name = u"throw"_s;
            eh->next = vmob->tempVariable;
            vmob->tempVariable = eh;
          }

          mprog_driver(mprg->comlist, vmob, actor, obj2, vo, nullptr, rndm);
          if (selfpurge)
            return mprog_cur_result;
          end_oproc(vmob);
          break;
        }
      }
    }
  }
  return mprog_cur_result;
}

ReturnValues Character::oprog_act_trigger(QString txt)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vmob;
  ObjectPtr item;

  mprog_cur_result = ReturnValue::eSUCCESS;

  if (in_room == INVALID_ROOM)
    return mprog_cur_result;

  for (item = dc_->world[in_room]->contents_; item; item = item->next_content)
    if (dc_->obj_index_[item->item_number]->progtypes_ & ACT_PROG)
    {
      vmob = dc_->initiate_oproc(this, item);
      if (mprog_wordlist_check(txt, vmob, this, nullptr, nullptr, ACT_PROG))
      {
        end_oproc(vmob);
        return mprog_cur_result;
      }
      end_oproc(vmob);
    }
  for (item = carrying; item; item = item->next_content)
    if (dc_->obj_index_[item->item_number]->progtypes_ & ACT_PROG)
    {
      vmob = dc_->initiate_oproc(this, item);
      if (mprog_wordlist_check(txt, vmob, this, nullptr, nullptr, ACT_PROG))
      {
        end_oproc(vmob);
        return mprog_cur_result;
      }
      end_oproc(vmob);
    }

  for (qint32 i = {}; i < MAX_WEAR; i++)
    if (equipment[i])
      if (dc_->obj_index_[equipment[i]->item_number]->progtypes_ & ACT_PROG)
      {
        vmob = dc_->initiate_oproc(this, equipment[i]);
        if (mprog_wordlist_check(txt, vmob, this, nullptr, nullptr, ACT_PROG))
        {
          end_oproc(vmob);
          return mprog_cur_result;
        }
        end_oproc(vmob);
      }
  return mprog_cur_result;
}

qint32 Character::oprog_greet_trigger(void)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  mprog_cur_result = ReturnValue::eSUCCESS;

  for (auto item = dc_->world[in_room]->contents_; item; item = item->next_content)
    if (dc_->obj_index_[item->item_number]->progtypes_ & ALL_GREET_PROG)
    {
      auto vmob = dc_->initiate_oproc(this, item);
      mprog_percent_check(vmob, this, item, nullptr, ALL_GREET_PROG);
      end_oproc(vmob);
      return mprog_cur_result;
    }
  return mprog_cur_result;
}

qint32 DC::oprog_rand_trigger(ObjectPtr item)
{
  CharacterPtr vmob;
  //  ObjectPtr item;
  CharacterPtr ch;
  mprog_cur_result = ReturnValue::eSUCCESS;
  if (item->carried_by)
    ch = item->carried_by;
  else
    ch = {};
  if (obj_index_[item->item_number]->progtypes_ & RAND_PROG)
  {
    vmob = initiate_oproc(ch, item);
    mprog_percent_check(vmob, ch, item, nullptr, RAND_PROG);
    end_oproc(vmob);
    return mprog_cur_result;
  }
  return mprog_cur_result;
}

qint32 DC::oprog_arand_trigger(ObjectPtr item)
{
  CharacterPtr vmob;
  CharacterPtr ch;
  mprog_cur_result = ReturnValue::eSUCCESS;

  if (item->carried_by)
    ch = item->carried_by;
  else
    ch = {};
  if (obj_index_[item->item_number]->progtypes_ & ARAND_PROG)
  {
    vmob = initiate_oproc(ch, item);
    mprog_percent_check(vmob, ch, item, nullptr, ARAND_PROG);
    end_oproc(vmob);
    return mprog_cur_result;
  }
  return mprog_cur_result;
}

qint32 Character::oprog_load_trigger(void)
{
  CharacterPtr vmob;
  ObjectPtr item;

  mprog_cur_result = ReturnValue::eSUCCESS;

  for (item = dc_->world[in_room]->contents_; item; item = item->next_content)
    if (dc_->obj_index_[item->item_number]->progtypes_ & LOAD_PROG)
    {
      vmob = dc_->initiate_oproc(this, item);
      mprog_percent_check(vmob, this, item, nullptr, LOAD_PROG);
      end_oproc(vmob);
      return mprog_cur_result;
    }
  for (item = carrying; item; item = item->next_content)
    if (dc_->obj_index_[item->item_number]->progtypes_ & LOAD_PROG)
    {
      vmob = dc_->initiate_oproc(this, item);
      mprog_percent_check(vmob, this, item, nullptr, LOAD_PROG);
      end_oproc(vmob);
      return mprog_cur_result;
    }

  for (qint32 i = {}; i < MAX_WEAR; i++)
    if (equipment[i])
      if (dc_->obj_index_[equipment[i]->item_number]->progtypes_ & LOAD_PROG)
      {
        vmob = dc_->initiate_oproc(this, item);
        mprog_percent_check(vmob, this, item, nullptr, LOAD_PROG);
        end_oproc(vmob);
        return mprog_cur_result;
      }
  return mprog_cur_result;
}

qint32 Character::oprog_weapon_trigger(ObjectPtr item)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vmob;

  mprog_cur_result = ReturnValue::eSUCCESS;

  if (dc_->obj_index_[item->item_number]->progtypes_ & WEAPON_PROG)
  {
    vmob = dc_->initiate_oproc(this, item);
    mprog_percent_check(vmob, this, item, nullptr, WEAPON_PROG);
    end_oproc(vmob);
    return mprog_cur_result;
  }

  return mprog_cur_result;
}

qint32 Character::oprog_armour_trigger(ObjectPtr item)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vmob;

  mprog_cur_result = ReturnValue::eSUCCESS;

  if (dc_->obj_index_[item->item_number]->progtypes_ & ARMOUR_PROG)
  {
    vmob = dc_->initiate_oproc(this, item);
    mprog_percent_check(vmob, this, item, nullptr, ARMOUR_PROG);
    end_oproc(vmob);
    return mprog_cur_result;
  }

  return mprog_cur_result;
}

ReturnValue Character::oprog_command_trigger(QString command, QString arguments)
{
  if (isDead() || isNowhere())
  {
    return ReturnValue::eFAILURE;
  }

  CharacterPtr vmob = {};
  ObjectPtr item = {};
  mprog_cur_result = ReturnValue::eFAILURE;
  QString buf;
  if (in_room >= 0)
  {
    for (item = dc_->world[in_room]->contents_; item; item = item->next_content)
    {
      if (dc_->obj_index_[item->item_number]->progtypes_ & COMMAND_PROG)
      {
        if (!arguments.isEmpty())
        {
          do_mpsettemp(u"%1 lasttyped %2"_s.arg(name()).arg(arguments).split(' '));
        }

        vmob = dc_->initiate_oproc(this, item);
        if (mprog_wordlist_check(command, vmob, this, nullptr, nullptr, COMMAND_PROG, true))
        {
          end_oproc(vmob);
          return mprog_cur_result;
        }
        end_oproc(vmob);
      }
    }
  }

  for (item = carrying; item; item = item->next_content)
  {
    if (dc_->obj_index_[item->item_number]->progtypes_ & COMMAND_PROG)
    {
      if (!arguments.isEmpty())
      {
        do_mpsettemp(u"%1 lasttyped %2"_s.arg(name()).arg(arguments).split(' '));
      }
      vmob = dc_->initiate_oproc(this, item);
      if (mprog_wordlist_check(arguments, vmob, this, nullptr, nullptr, COMMAND_PROG, true))
      {
        end_oproc(vmob);
        return mprog_cur_result;
      }
      end_oproc(vmob);
    }
  }

  for (qint32 i = {}; i < MAX_WEAR; i++)
  {
    if (equipment[i])
    {
      if (dc_->obj_index_[equipment[i]->item_number]->progtypes_ & COMMAND_PROG)
      {
        if (!arguments.isEmpty())
        {
          do_mpsettemp(u"%1 lasttyped %2"_s.arg(name()).arg(arguments).split(' '));
        }

        vmob = dc_->initiate_oproc(this, equipment[i]);
        if (mprog_wordlist_check(arguments, vmob, this, nullptr, nullptr, COMMAND_PROG, true))
        {
          end_oproc(vmob);
          return mprog_cur_result;
        }
        end_oproc(vmob);
      }
    }
  }
  return mprog_cur_result;
}

bool isPaused(CharacterPtr mob)
{
  if (mob == nullptr || mob == (CharacterPtr)0x95959595)
  {
    return false;
  }

  if (!charExists(mob))
  {
    return false;
  }

  if (mob->mobdata == nullptr || mob->mobdata == (Mobile *)0x95959595)
  {
    return false;
  }

  if (mob->mobdata->paused == true)
  {
    return true;
  }

  return false;
}

[[nodiscard]] QString Program::typeString(void) const
{
  if (is_object_)
  {
    switch (type_)
    {
    case ALL_GREET_PROG:
      return u"all_greet_prog"_s;
    case WEAPON_PROG:
      return u"weapon_prog"_s;
    case ARMOUR_PROG:
      return u"armour_prog"_s;
    case LOAD_PROG:
      return u"load_prog"_s;
    case COMMAND_PROG:
      return u"command_prog"_s;
    case ACT_PROG:
      return u"act_prog"_s;
    case ARAND_PROG:
      return u"arand_prog"_s;
    case CATCH_PROG:
      return u"catch_prog"_s;
    case SPEECH_PROG:
      return u"speech_prog"_s;
    case RAND_PROG:
      return u"rand_prog"_s;
    case CAN_SEE_PROG:
      return u"can_see_prog"_s;
    default:
      return u"ERROR_PROG"_s;
    }
  }
  else
  {
    switch (type_)
    {
    case IN_FILE_PROG:
      return u"in_file_prog"_s;
    case ACT_PROG:
      return u"act_prog"_s;
    case SPEECH_PROG:
      return u"speech_prog"_s;
    case RAND_PROG:
      return u"rand_prog"_s;
    case ARAND_PROG:
      return u"arand_prog"_s;
    case FIGHT_PROG:
      return u"fight_prog"_s;
    case HITPRCNT_PROG:
      return u"hitprcnt_prog"_s;
    case DEATH_PROG:
      return u"death_prog"_s;
    case ENTRY_PROG:
      return u"entry_prog"_s;
    case GREET_PROG:
      return u"greet_prog"_s;
    case ALL_GREET_PROG:
      return u"all_greet_prog"_s;
    case GIVE_PROG:
      return u"give_prog"_s;
    case BRIBE_PROG:
      return u"bribe_prog"_s;
    case CATCH_PROG:
      return u"catch_prog"_s;
    case ATTACK_PROG:
      return u"attack_prog"_s;
    case LOAD_PROG:
      return u"load_prog"_s;
    case CAN_SEE_PROG:
      return u"can_see_prog"_s;
    case DAMAGE_PROG:
      return u"damage_prog"_s;
    case COMMAND_PROG:
      return u"command_prog"_s;
    default:
      return u"ERROR_PROG"_s;
    }
  }
}

MobileProgram::MobileProgram(void)
    : is_object_(false)
{
}
