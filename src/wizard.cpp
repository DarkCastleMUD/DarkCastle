/************************************************************************
| $Id: wizard.cpp,v 1.87 2015/06/16 04:10:54 pirahna Exp $
| wizard.C
| Description:  Utility functions necessary for wiz commands.
*/
#include "DC/DC.h"

qint32 getRealSpellDamage(CharacterPtr ch);

qint32 number_or_name(QString *name, qint32 *num)
{
  qint32 i;
  QString ppos;
  QString number;

  if ((ppos = index(*name, '.')) != nullptr)
  {
    *ppos++ = '\0';
    dc_strcpy(number, *name);
    dc_strcpy(*name, ppos);

    for (i = {}; *(number + i); i++)
      if (!isdigit(*(number + i)))
        return {};

    return (atoi(number));
  }

  /* no dot */
  if ((*num = atoi(*name)) > 0)
    return -1;
  else
    return 1;
}

#if (0)
qint32 number_or_name(QString *name, qint32 *num)
{
  quint32 i;
  QString ppos = {};
  QString number;

  for (i = {}; i < dc_strlen(*name); i++)
  {
    if (*name[i] == '.')
    {
      ppos = *name + i;
      break;
    }
  }
  if (ppos)
  {
    *ppos++ = '\0';
    dc_strcpy(number, *name);
    dc_strcpy(*name, ppos);

    for (i = {}; *(number + i); i++)
      if (!isdigit(*(number + i)))
        return {};

    return (atoi(number));
  }

  /* no dot */
  if ((*num = atoi(*name)) > 0)
    return -1;
  else
    return 1;
}
#endif

void do_mload(CharacterPtr ch, qint32 rnum, qint32 cnt)
{
  if (!ch)
    return;
  CharacterPtr mob = {};
  QString buf;
  qint32 i;
  if (cnt == 0)
    cnt = 1;
  for (i = 1; i <= cnt; i++)
  {
    mob = ch->dc_->clone_mobile(rnum);
    char_to_room(mob, ch->in_room);
    selfpurge = false;
    mprog_load_trigger(mob);
    if (selfpurge)
    {
      mob = {};
    }
  }

  if (mob)
  {
    act("$n draws up a swirling column of dust and breathes life into it.",
        ch, 0, 0, TO_ROOM, 0);
    act_to_room("$n has created $N!", ch, 0, mob, 0);
    dc_sprintf(buf, "You create %i %s!\r\n", cnt, mob->short_desc);
    ch->send(buf);
    if (cnt > 1)
    {
      dc_snprintf(buf, MAX_STRING_LENGTH, "%s loads %i copies of mob %lu (%s) at room %d (%s).",
                  qPrintable(ch->name()),
                  cnt,
                  dc_->mob_index[rnum].vnum(),
                  mob->short_desc,
                  dc_->world[ch->in_room].number,
                  dc_->world[ch->in_room].name);
    }
    else
    {
      dc_snprintf(buf, MAX_STRING_LENGTH, "%s loads %i copy of mob %lu (%s) at room %d (%s).",
                  qPrintable(ch->name()),
                  cnt,
                  dc_->mob_index[rnum].vnum(),
                  mob->short_desc,
                  dc_->world[ch->in_room].number,
                  dc_->world[ch->in_room].name);
    }
    dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
  }
  else
  {
    dc_snprintf(buf, MAX_STRING_LENGTH, "%s loads %i copies of mob %lu at room %d (%s).",
                qPrintable(ch->name()),
                cnt,
                dc_->mob_index[rnum].vnum(),
                dc_->world[ch->in_room].number,
                dc_->world[ch->in_room].name);
    dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    ch->sendln("You load the mob(s) but they immediatly destroy themselves.");
  }
}

obj_list_t oload(CharacterPtr ch, qint32 rnum, qint32 cnt, bool random)
{
  ObjectPtr obj = {};
  obj_list_t obj_list = {};
  QString buf;

  if (cnt == 0)
  {
    cnt = 1;
  }

  act_to_room("$n makes a strange magical gesture.", ch, 0, 0, INVIS_NULL);
  for (auto i = 1; i <= cnt; i++)
  {
    obj = clone_object(rnum);
    act_to_room("$n has created $p!", ch, obj, 0, 0);
    if (random == true)
    {
      randomize_object(obj);
    }

    if (obj->obj_flags.type_flag == ITEM_MONEY && !ch->isImplementerPlayer())
    {
      extract_obj(obj);
      ch->send("Denied.\r\n");
    }
    else
    {
      obj_list.insert(obj);
      obj_to_char(obj, ch);
    }
  }

  ch->send(fmt::format("You create {} {}{}.\r\n", cnt, random ? "randomized " : "", qPrintable(obj->short_description())));

  buf = fmt::format("{} loads {} {}{} of obj {} ({}) at room {} ({}).",
                    qPrintable(ch->name()),
                    cnt,
                    random ? "randomized " : "",
                    cnt > 1 ? "copies" : "copy",
                    GET_OBJ_VNUM(obj),
                    qPrintable(obj->short_description()),
                    dc_->world[ch->in_room].number,
                    dc_->world[ch->in_room].name);
  dc_->logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);

  return obj_list;
}

void do_oload(CharacterPtr ch, qint32 rnum, qint32 cnt, bool random)
{
  ObjectPtr obj = {};
  QString buf;
  qint32 i;

  if (cnt == 0)
    cnt = 1;

  act_to_room("$n makes a strange magical gesture.", ch, 0, 0, INVIS_NULL);
  for (i = 1; i <= cnt; i++)
  {
    obj = clone_object(rnum);
    act_to_room("$n has created $p!", ch, obj, 0, 0);
    if (random == true)
    {
      randomize_object(obj);
    }

    if ((obj->obj_flags.type_flag == ITEM_MONEY) &&
        (ch->getLevel() < IMPLEMENTER))
    {
      extract_obj(obj);
      ch->sendln("Denied.");
      return;
    }
    else
    {
      obj_to_char(obj, ch);
    }
  }

  dc_snprintf(buf, MAX_STRING_LENGTH, "You create %i %s%s.\r\n", cnt, random ? "randomized " : "", qPrintable(obj->short_description()));

  ch->send(buf);
  if (cnt > 1)
  {
    dc_snprintf(buf, MAX_STRING_LENGTH, "%s loads %i %scopies of obj %lu (%s) at room %d (%s).",
                qPrintable(ch->name()),
                cnt,
                random ? "randomized " : "",
                GET_OBJ_VNUM(obj),
                qPrintable(obj->short_description()),
                dc_->world[ch->in_room].number,
                dc_->world[ch->in_room].name);
  }
  else
  {
    dc_snprintf(buf, MAX_STRING_LENGTH, "%s loads %i %scopy of obj %lu (%s) at room %d (%s).",
                qPrintable(ch->name()),
                cnt,
                random ? "randomized " : "",
                GET_OBJ_VNUM(obj),
                qPrintable(obj->short_description()),
                dc_->world[ch->in_room].number,
                dc_->world[ch->in_room].name);
  }
  dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
}

//
// (I let him email it to me *wink* -pir)
//
/* Mob stat takes ch (the person who wants to stat the mob) and sends them info about
 * k, the mob or player that is being stated.  It prints it in a format similar to score.
 *  -- Borodin  Dec 23 2001 */

void boro_mob_stat(CharacterPtr ch, CharacterPtr k)
{
  qint32 i, i2;
  QString buf, buf2;
  QString buf3;
  follow_type *fol;
  ObjectPtr j = {};
  affected_type *aff;

  sprinttype(k->c_class, pc_clss_types, buf2);
  dc_sprintf(buf, "$R(:)========================================================================(:)\r\n"
                  "|=|$3 (%3s) Key$R: %-35s $3VNUM$R: %-5lu $3Room$R: %-5d |=|\r\n"
                  "|=|$3 Short$R: %-50s $3RNUM$R: %-6d |=|\r\n"
                  "|=|$3 Long$R: %s"
                  "(:)====================(:)=================================================(:)\r\n"
                  "|\\|  $4Fighting$R: %-9s|/|  $1Race$R:   %-10s $1HitPts$R: %5d$1/$R(%5d+%-3d) |~|\r\n"
                  "|~|  $4Master$R:   %-9s|o|  $1Class$R:  %-10s $1Mana$R:   %5d$1/$R(%5d+%-3d) |\\|\r\n",

             (k->isPlayer() ? "PC" : "MOB"),
             qPrintable(k->name()),
             (k->isNonPlayer() ? dc_->mob_index[k->mobdata->nr].vnum() : 0),
             (k->in_room == DC::NOWHERE ? 0 : dc_->world[k->in_room].number),
             /* end of first line */

             (!k->short_description().isEmpty() ? qPrintable(k->short_description()) : "None"),
             (qint32)(k->isNonPlayer() ? k->mobdata->nr : 0),
             /* end of second line */

             (!k->long_description().isEmpty() ? qPrintable(k->long_description()) : "None"),
             /* end of third line */

             (k->fighting ? qPrintable(k->fighting->name()) : "Nobody"),
             (races[(qint32)(k->race)].singular_name),
             k->getHP(), hit_limit(k), k->hit_gain_lookup(),
             /* end of fourth line */

             (k->master ? qPrintable(k->master->name()) : "Nobody"),
             buf2, /* this is for the class QString, the sprinttype above inits it. */
             GET_MANA(k), mana_limit(k), k->mana_gain_lookup());
  /* end of the first dc_sprintf */

  ch->send(buf); // this sends to character, now we can overwrite buf

  if (k->isNonPlayer())
  {
    dc_sprintf(buf2, "%s", (k->mobdata->hated.isEmpty() ? "NOBODY" : qPrintable(k->mobdata->hated)));
    dc_sprintf(buf3, "%s", (k->mobdata->fears ? k->mobdata->fears : "NOBODY"));
  }
  else
  {
    dc_sprintf(buf2, "Nobody");
    dc_sprintf(buf3, "Nobody");
  }

  dc_sprintf(buf, "|/|  $4Hates$R:    %-9s|\\|  $1Lvl$R:    %-9llu  $1Fatigue$R:%5d$1/$R(%5d+%-3d) |o|\r\n"
                  "|o|  $4Fears$R:    %-9s|~|  $1Height$R: %-9d  $1Ki$R:     %5d$1/$R(%5d)     |/|\r\n"
                  "|\\|  $4Tracking$R: %-9s|/|  $1Weight$R: %-9d  $1Alignment$R:   %4d          |~|\r\n",

             buf2, // who the mob hates
             k->getLevel(),
             GET_MOVE(k), k->move_limit(), k->move_gain_lookup(),
             /* end of first line */

             buf3, // who the mob fears, if anyone
             GET_HEIGHT(k),
             GET_KI(k), ki_limit(k),
             /* end of second line */

             (k->hunting.isEmpty() ? "NOBODY" : qPrintable(k->hunting)),
             GET_WEIGHT(k),
             k->alignment);

  ch->send(buf);
  /* end of second dc_sprintf */

  switch (k->sex)
  {
  case SEX_NEUTRAL:
    dc_sprintf(buf2, "Neutral");
    break;
  case SEX_MALE:
    dc_sprintf(buf2, "Male");
    break;
  case SEX_FEMALE:
    dc_sprintf(buf2, "Female");
    break;
  default:
    dc_sprintf(buf2, "ILLEGAL-VALUE");
    break;
  }

  dc_sprintf(buf, "|~|  $2Timer$R: %-11d |o|  $1Town$R:   %-5d     $1Sex$R:         %-14s|\\|\r\n"
                  "(:)====================(:)==========(:)====================================(:)\r\n",
             k->timer,
             (ch->isPlayer() ? k->hometown : -1),
             buf2); /* buf is the sex... */
  ch->send(buf);    /* THIRD dc_sprintf */

  if (k->isNonPlayer())
  {
    if (dc_->mob_index[k->mobdata->nr].non_combat_func)
      dc_strcpy(buf2, "Exists");
    else
      dc_strcpy(buf2, "None");

    if (dc_->mob_index[k->mobdata->nr].combat_func)
      dc_strcpy(buf3, "Exists");
    else
      dc_strcpy(buf3, "None");
  }

  dc_sprintf(buf, "|/|  $4FIRE$R: %3d  $1COLD$R: %3d $5NRGY$R: %3d |\\| Mob Non-Combat Spec Proc: %-9s|o|\r\n"
                  "|o|  $2$BACID$R: %3d  $3MAGK$R: %3d $2POIS$R: %3d |~| Mob Combat Spec Proc:     %-9s|/|\r\n",

             k->saves[SAVE_TYPE_FIRE], k->saves[SAVE_TYPE_COLD], k->saves[SAVE_TYPE_ENERGY],
             buf2, /* whether nor not they have a non_combat spec proc */
             /* end of first line */

             k->saves[SAVE_TYPE_ACID], k->saves[SAVE_TYPE_MAGIC], k->saves[SAVE_TYPE_POISON],
             buf3); /* buf2 = whether or not there is a combat spec proc */
  /* end of fourth dc_sprintf */
  ch->send(buf);

  sprinttype(GET_POS(k), Character::position_types, buf2);

  if (k->isNonPlayer())
    sprinttype((k->mobdata->default_pos), Character::position_types, buf3);
  else
    dc_strcpy(buf3, "PC");

  for (i = 0, j = k->carrying; j; j = j->next_content, ++i)
    ;
  dc_sprintf(buf, "|\\|  $3Pos$R: %-9s$3DefPos$R: %-9s|/| $1Thirst$R: %-3d $4Hunger$R: %-3d $6Drunk$R: %-3d |~|\r\n"
                  "|/|  $3Dex$R: %2d    $3Con$R: %2d    $3Str$R: %2d  |\\| $3Carried Weight$R: %-4d $3Inv. Items$R: %-3d|o|\r\n",

             buf2, buf3, // pos and default pos respectively
             k->conditions[THIRST],
             k->conditions[FULL],
             k->conditions[DRUNK],
             // end of first line

             GET_DEX(k), GET_CON(k), GET_STR(k),
             IS_CARRYING_W(k), i);
  /* END OF WEAR_SECOND_WIELD LINE */

  ch->send(buf);

  for (i = 0, i2 = {}; i < MAX_WEAR; i++)
    if (k->equipment[i])
      i2++;

  dc_sprintf(buf, "|~|  $3Int$R: %2d    $3Wis$R: %2d    $3Ac$R: %-3d |o| $3Carried Items$R: %d $3Equipped Items$R: %d |/|\r\n",
             GET_INT(k), GET_WIS(k), GET_ARMOR(k), IS_CARRYING_N(k), i2);
  ch->send(buf);

  dc_sprintf(buf, "|o| $3 Hitroll$R: %3d     $3Damroll$R: %3d  |~|                                    |/|\r\n"
                  "(:)=================================(:)====================================(:)\r\n",
             k->hitroll, k->damroll);
  ch->send(buf);

  sprintbit(k->suscept, isr_bits, buf2);
  sprintbit(k->immune, isr_bits, buf3);
  dc_sprintf(buf, "|/| $7Immune$R: %-63s|o|\r\n"
                  "|o| $7Susceptible$R: %-58s|/|\r\n",
             buf2, buf3); // immune and susceptible bits, first and second.
  ch->send(buf);
  sprintbit(k->affected_by, affected_bits, buf2);
  dc_sprintf(buf, "|\\| $7Affected By$R: %-58s|~|\r\n", buf2); // affected bits.
  ch->send(buf);

  if (k->isNonPlayer()) // AND THIS
    sprintbit(k->mobdata->actflags, action_bits, buf2);
  else
    dc_strcpy(buf2, "Not a mob");
  sprintbit(k->combat, combat_bits, buf3);
  dc_sprintf(buf, "|~| $7NPC flags$R: %-60s|\\|\r\n"
                  "|/| $7Combat flags$R: %-57s|o|\r\n",
             buf2, buf3); // npc flags and combat flags respectively
  ch->send(buf);

  sprintbit(k->resist, isr_bits, buf2);
  dc_sprintf(buf, "|o| $7Resistant$R: %-60s|/|\r\n"
                  "(:)========================================================================(:)\r\n",
             buf2); // the resisted bits
  ch->send(buf);

  dc_strcpy(buf, "$3Title$R: ");
  dc_strcat(buf, (k->title ? k->title : "None"));
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  // Description
  ch->sendln("$3Detailed description$R:");
  if (!k->description().isEmpty())
    ch->send(k->description());
  else
    ch->send("None");

  // LIST OF FOLLOWERS
  ch->sendln("$3Followers$R:");
  for (fol = k->followers; fol; fol = fol->next)
    act_to_character("    $N", ch, 0, fol->follower, 0);

  if (!k->isNonPlayer())
  {
    dc_sprintf(buf, "$3Birth$R: [%d]secs  $3Logon$R:[%d]secs $3Played$R[%d]secs\r\n",
               k->player->time.birth,
               k->player->time.logon,
               (qint32)(k->player->time.played));
    ch->send(buf);

    dc_sprintf(buf, "$3Age$R:[%d] Years [%d] Months [%d] Days [%d] Hours\r\n",
               k->age().year, k->age().month, k->age().day, k->age().hours);
    ch->send(buf);
  }

  if (!k->isNonPlayer())
  {
    dc_sprintf(buf, "$3Coins$R:[%ld]  $3Bank$R:[%d]\r\n", k->getGold(),
               k->player->bank);
    ch->send(buf);
  }

  if (k->isPlayer())
  {
    dc_sprintf(buf, "$3SaveMod$R: FIRE[%d] COLD[%d] ENERGY[%d] ACID[%d] MAGIC[%d] POISON[%d]\r\n",
               k->player->saves_mods[SAVE_TYPE_FIRE],
               k->player->saves_mods[SAVE_TYPE_COLD],
               k->player->saves_mods[SAVE_TYPE_ENERGY],
               k->player->saves_mods[SAVE_TYPE_ACID],
               k->player->saves_mods[SAVE_TYPE_MAGIC],
               k->player->saves_mods[SAVE_TYPE_POISON]);
    ch->send(buf);
  }

  if (!k->isNonPlayer())
  {
    dc_sprintf(buf, "$3WizInvis$R:  %d  ", k->player->wizinvis);
    ch->send(buf);
    dc_sprintf(buf, "$3Holylite$R:  %s  ", ((k->player->holyLite) ? "ON" : "OFF"));
    ch->send(buf);
    dc_sprintf(buf, "$3Stealth$R:  %s\r\n", ((k->player->stealth) ? "ON" : "OFF"));
    ch->send(buf);

    if ((k->player->buildLowVnum == k->player->buildMLowVnum) == k->player->buildOLowVnum &&
        (k->player->buildHighVnum == k->player->buildMHighVnum) == k->player->buildOHighVnum)
    {
      dc_sprintf(buf, "$3Creation Range$R:  %d-%d  \r\n", k->player->buildLowVnum, k->player->buildHighVnum);
      ch->send(buf);
    }
    else
    {
      dc_sprintf(buf, "$3R Range$R:  %d-%d  \r\n", k->player->buildLowVnum, k->player->buildHighVnum);
      ch->send(buf);
      dc_sprintf(buf, "$3M Range$R:  %d-%d  \r\n", k->player->buildMLowVnum, k->player->buildMHighVnum);
      ch->send(buf);
      dc_sprintf(buf, "$3O Range$R:  %d-%d  \r\n", k->player->buildOLowVnum, k->player->buildOHighVnum);
      ch->send(buf);
    }
  }

  if (k->affected)
  {
    ch->sendln("\r\n$3Affecting Spells$R:\r\n--------------");

    for (aff = k->affected; aff; aff = aff->next)
    {
      dc_sprintf(buf, "Spell : '%s'\r\n",
                 aff->type < 300 ? spells[(qint32)aff->type - 1] : skills[(qint32)aff->type - 300]);
      ch->send(buf);
      dc_sprintf(buf, "     Modifies %s by %d points\r\n",
                 apply_types[(qint32)aff->location], aff->modifier);
      ch->send(buf);
      dc_sprintf(buf, "     Expires in %3d hours, Bits set ", aff->duration);
      ch->send(buf);
      sprintbit(aff->bitvector, affected_bits, buf);
      dc_strcat(buf, "\r\n");
      ch->send(buf);
    }
    ch->sendln("");
  }

  if (!k->isNonPlayer())
    display_punishes(ch, k);

  if (k->desc)
  {
    sprinttype(k->desc->connected, connected_types, buf2);
    dc_strcat(buf, "  $3Connected$R: ");
    dc_strcat(buf, buf2);
  }
  ch->send(buf);
}

command_return_t mob_stat(CharacterPtr ch, CharacterPtr k)
{
  if (!ch || !k)
  {
    return ReturnValue::eFAILURE;
  }

  qint32 i;
  QString buf;
  follow_type *fol;
  qint32 i2;
  QString buf2;
  ObjectPtr j = {};
  affected_type *aff;

  if (k->isNonPlayer())
  {
    dc_sprintf(buf, "$3%s$R - $3Name$R: [%s]  $3VNum$R: %lu  $3RNum$R: %d  $3In room:$R %d $3Mobile type:$R ",
               (k->isPlayer() ? "PC" : "MOB"), qPrintable(k->name()),
               (k->isNonPlayer() ? dc_->mob_index[k->mobdata->nr].vnum() : 0),
               (k->isNonPlayer() ? k->mobdata->nr : 0),
               k->in_room == DC::NOWHERE ? -1 : dc_->world[k->in_room].number);

    sprinttype(GET_MOB_TYPE(k), mob_types, buf2);
    dc_strcat(buf, buf2);
    dc_strcat(buf, "\r\n");
  }
  else
  {
    dc_sprintf(buf, "$3%s$R - $3Name$R: [%s]  $3In room:$R %d\r\n",
               (k->isPlayer() ? "PC" : "MOB"), qPrintable(k->name()),
               k->in_room == DC::NOWHERE ? -1 : dc_->world[k->in_room].number);
  }
  ch->send(buf);

  dc_strcpy(buf, "$3Short description$R: ");
  dc_strcat(buf, (!k->short_description().isEmpty() ? qPrintable(k->short_description()) : "None"));
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  dc_strcpy(buf, "$3Title$R: ");
  dc_strcat(buf, (k->title ? k->title : "None"));
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  ch->send("$3Long description$R: ");
  if (!k->long_description().isEmpty())
    ch->send(k->long_description());
  else
    ch->send("None");
  ch->sendln("$3Detailed description$R:");
  if (!k->description().isEmpty())
    ch->send(k->description());
  else
    ch->send("None");

  dc_strcpy(buf, "\r\n$3Class$R: ");
  sprinttype(k->c_class, pc_clss_types, buf2);

  dc_strcat(buf, buf2);

  dc_sprintf(buf2, "   $3Level$R:[%llu] $3Alignment$R:[%d] ", k->getLevel(),
             k->alignment);
  dc_strcat(buf, buf2);
  ch->send(buf);
  dc_sprintf(buf, "$3Spelldamage$R:[%d] ", getRealSpellDamage(k));
  ch->send(buf);
  dc_sprintf(buf, "$3Race$R: %s\r\n", races[(qint32)(k->race)].singular_name);
  ch->send(buf);

  if (!k->isNonPlayer())
  {
    dc_sprintf(buf, "$3Birth$R: [%d]secs  $3Logon$R:[%d]secs  $3Played$R[%d]secs\r\n",
               k->player->time.birth,
               k->player->time.logon,
               (qint32)(k->player->time.played));
    ch->send(buf);

    dc_sprintf(buf, "$3Age$R:[%d] Years [%d] Months [%d] Days [%d] Hours\r\n",
               k->age().year, k->age().month, k->age().day, k->age().hours);
    ch->send(buf);
  }
  if (k->isNonPlayer())
  {
    QString mobspec_status;

    if (dc_->mob_index[k->mobdata->nr].mobspec == nullptr)
    {
      mobspec_status = "none";
    }
    else
    {
      mobspec_status = "exists";
    }

    ch->sendln(u"$3Mobspec$R: %1  $3Progtypes$R: %2"_s.arg(mobspec_status).arg(dc_->mob_index[k->mobdata->nr].progtypes));
  }
  dc_sprintf(buf, "$3Height$R:[%d]  $3Weight$R:[%d]  $3Sex$R:[", GET_HEIGHT(k), GET_WEIGHT(k));
  ch->send(buf);

  switch (k->sex)
  {
  case SEX_NEUTRAL:
    ch->send("NEUTRAL]  ");
    break;
  case SEX_MALE:
    ch->send("MALE]  ");
    break;
  case SEX_FEMALE:
    ch->send("FEMALE]  ");
    break;
  default:
    ch->send("ILLEGAL-VALUE!]  ");
    break;
  }

  if (ch->isPlayer())
  {
    dc_sprintf(buf, "$3Hometown$R:[%d]\r\n", k->hometown);
    ch->send(buf);
  }
  else
    ch->sendln("");

  dc_sprintf(buf, "$3Str$R:[%2d]+[%2d]=%2d $3Int$R:[%2d]+[%2d]=%2d $3Wis$R:[%2d]+[%2d]=%2d\r\n"
                  "$3Dex$R:[%2d]+[%2d]=%2d $3Con$R:[%2d]+[%2d]=%2d\r\n",
             GET_RAW_STR(k), GET_STR_BONUS(k), GET_STR(k),
             GET_RAW_INT(k), GET_INT_BONUS(k), GET_INT(k),
             GET_RAW_WIS(k), GET_WIS_BONUS(k), GET_WIS(k),
             GET_RAW_DEX(k), GET_DEX_BONUS(k), GET_DEX(k),
             GET_RAW_CON(k), GET_CON_BONUS(k), GET_CON(k));
  ch->send(buf);

  dc_sprintf(buf, "$3Mana$R:[%5d/%5d+%-4d]  $3Hit$R:[%5d/%5d+%-3d]  $3Move$R:[%5d/%5d+%-3d]  $3Ki$R:[%3d/%3d]\r\n",
             GET_MANA(k), mana_limit(k), k->mana_gain_lookup(),
             k->getHP(), hit_limit(k), k->hit_gain_lookup(),
             GET_MOVE(k), k->move_limit(), k->move_gain_lookup(),
             GET_KI(k), ki_limit(k));
  ch->send(buf);

  dc_sprintf(buf, "$3AC$R:[%d]  $3Exp$R:[%ld]  $3Hitroll$R:[%d]  $3Damroll$R:[%d]  $3Gold$R: [$B$5%ld$R]\r\n",
             GET_ARMOR(k), k->exp, GET_REAL_HITROLL(k), GET_REAL_DAMROLL(k), k->getGold());
  ch->send(buf);

  if (!k->isNonPlayer())
  {
    dc_sprintf(buf, "$3Plats$R:[%d]  $3Bank$R:[%d]  $3Clan$R:[%d]  $3Quest Points$R:[%d]\r\n",
               GET_PLATINUM(k), GET_BANK(k), GET_CLAN(k), GET_QPOINTS(k));
    ch->send(buf);
  }

  dc_sprintf(buf, "$3Position$R: %s  $3Fighting$R: %s  ", qPrintable(k->getPositionQString()),
             ((k->fighting) ? qPrintable(k->fighting->name())
                            : "Nobody"));
  if (k->desc)
  {
    sprinttype(k->desc->connected, connected_types, buf2);
    dc_strcat(buf, "  $3Connected$R: ");
    dc_strcat(buf, buf2);
  }
  ch->send(buf);

  if (k->isNonPlayer())
  {
    dc_strcpy(buf, "$3Default position$R: ");
    dc_strcat(buf, Character::position_to_string(qPrintable(k->mobdata->default_pos)));
    ch->send(buf);
  }

  dc_sprintf(buf, "  $3Timer$R:[%d] \r\n", k->timer);
  ch->send(buf);

  if (k->isNonPlayer())
  {
    dc_sprintf(buf, "$3NPC flags$R: [%d %d]", k->mobdata->actflags[0], k->mobdata->actflags[1]);
    sprintbit(k->mobdata->actflags, action_bits, buf2);
  }
  else
  {
    dc_sprintf(buf, "$3PC flags$R: [%d]", k->player->toggles);
    sprintbit(k->player->toggles, player_bits, buf2);
  }
  dc_strcat(buf, buf2);
  ch->send(buf);

  if (k->isNonPlayer())
  {
    dc_strcpy(buf, "\r\n$3Non-Combat Special Proc$R: ");
    dc_strcat(buf, (dc_->mob_index[k->mobdata->nr].non_combat_func ? "exists  " : "none  "));
    ch->send(buf);
    dc_strcpy(buf, "$3Combat Special Proc$R: ");
    dc_strcat(buf, (dc_->mob_index[k->mobdata->nr].combat_func ? "exists  " : "none  "));
    ch->send(buf);
    dc_strcpy(buf, "$3Mob Progs$R: ");
    dc_strcat(buf, (dc_->mob_index[k->mobdata->nr].mobprogs ? "exists\r\n" : "none\r\n"));
    ch->send(buf);
  }

  if (k->isNonPlayer())
  {
    dc_sprintf(buf, "$3NPC Bare Hand Damage$R: %d$3d$R%d.\r\n",
               k->mobdata->damnodice, k->mobdata->damsizedice);
    ch->send(buf);
  }

  dc_sprintf(buf, "$3Carried weight$R: %d   $3Carried items$R: %d\r\n",
             IS_CARRYING_W(k), IS_CARRYING_N(k));
  ch->send(buf);

  for (i = 0, j = k->carrying; j; j = j->next_content, i++)
    ;
  dc_sprintf(buf, "$3Items in inventory$R: %d  ", i);

  for (i = 0, i2 = {}; i < MAX_WEAR; i++)
    if (k->equipment[i])
      i2++;

  dc_sprintf(buf2, "$3Items in equipment$R: %d\r\n", i2);
  dc_strcat(buf, buf2);
  ch->send(buf);

  dc_sprintf(buf, "$3Save Vs$R: $B$4FIRE[%2d] $7COLD[%2d] $5ENERGY[%2d] $2ACID[%2d] $3MAGIC[%2d] $R$2POISON[%2d]$R\r\n",
             k->saves[SAVE_TYPE_FIRE],
             k->saves[SAVE_TYPE_COLD],
             k->saves[SAVE_TYPE_ENERGY],
             k->saves[SAVE_TYPE_ACID],
             k->saves[SAVE_TYPE_MAGIC],
             k->saves[SAVE_TYPE_POISON]);
  ch->send(buf);

  if (k->isPlayer())
  {
    dc_sprintf(buf, "$3SaveMod$R: $B$4FIRE[%2d] $7COLD[%2d] $5ENERGY[%2d] $2ACID[%2d] $3MAGIC[%2d] $R$2POISON[%2d]$R\r\n",
               k->player->saves_mods[SAVE_TYPE_FIRE],
               k->player->saves_mods[SAVE_TYPE_COLD],
               k->player->saves_mods[SAVE_TYPE_ENERGY],
               k->player->saves_mods[SAVE_TYPE_ACID],
               k->player->saves_mods[SAVE_TYPE_MAGIC],
               k->player->saves_mods[SAVE_TYPE_POISON]);
    ch->send(buf);
  }

  dc_sprintf(buf, "$3Thirst$R: %d  $3Hunger$R: %d  $3Drunk$R: %d\r\n",
             k->conditions[THIRST],
             k->conditions[FULL],
             k->conditions[DRUNK]);
  ch->send(buf);
  dc_sprintf(buf, "$3Melee$R: [%d] $3Spell$R: [%d] $3Song$R: [%d] $3Reflect$R: [%d]\r\n",
             k->melee_mitigation, k->spell_mitigation, k->song_mitigation, k->spell_reflect);
  ch->send(buf);

  dc_sprintf(buf, "$3Tracking$R: '%s'\r\n", ((k->hunting.isEmpty()) ? "NOBODY" : qPrintable(k->hunting)));
  ch->send(buf);

  if (k->isNonPlayer())
  {
    dc_sprintf(buf, "$3Hates$R: '%s'\r\n",
               (k->mobdata->hated.isEmpty() ? "NOBODY" : qPrintable(k->mobdata->hated)));
    ch->send(buf);

    dc_sprintf(buf, "$3Fears$R: '%s'\r\n",
               ((k->mobdata->fears) ? k->mobdata->fears : "NOBODY"));
    ch->send(buf);
  }

  dc_sprintf(buf, "$3Master$R: '%s'\r\n",
             ((k->master) ? qPrintable(k->master->name()) : "NOBODY"));
  ch->send(buf);
  ch->sendln("$3Followers$R:");
  for (fol = k->followers; fol; fol = fol->next)
    act_to_character("    $N", ch, 0, fol->follower, 0);

  // Showing the bitvector
  sprintbit(k->combat, combat_bits, buf);
  ch->send(u"$3Combat flags$R: %1\r\n"_s.arg(buf));

  if (!k->isNonPlayer())
    display_punishes(ch, k);

  sprintbit(k->affected_by, affected_bits, buf);
  ch->send(u"$3Affected by$R: [%d %d] %s\r\n"_s.arg(k->affected_by[0]).arg(k->affected_by[1]).arg(buf));

  sprintbit(k->immune, isr_bits, buf);
  ch->send(u"$3Immune$R: [%d] %s\r\n"_s.arg(k->immune).arg(buf));

  sprintbit(k->suscept, isr_bits, buf);
  ch->send(u"$3Susceptible$R: [%d] %s\r\n"_s.arg(k->suscept).arg(buf));

  sprintbit(k->resist, isr_bits, buf);
  ch->send(u"$3Resistant$R: [%d] %s\r\n"_s.arg(k->resist).arg(buf));

  if (!k->isNonPlayer())
  {
    dc_sprintf(buf, "$3WizInvis$R:  %d  ", k->player->wizinvis);
    ch->send(buf);
    dc_sprintf(buf, "$3Holylite$R:  %s  ", ((k->player->holyLite) ? "ON" : "OFF"));
    ch->send(buf);
    dc_sprintf(buf, "$3Stealth$R:  %s\r\n", ((k->player->stealth) ? "ON" : "OFF"));
    ch->send(buf);
    if ((k->player->buildLowVnum == k->player->buildMLowVnum) == k->player->buildOLowVnum &&
        (k->player->buildHighVnum == k->player->buildMHighVnum) == k->player->buildOHighVnum)
    {
      dc_sprintf(buf, "$3Creation Range$R:  %d-%d  \r\n", k->player->buildLowVnum, k->player->buildHighVnum);
      ch->send(buf);
    }
    else
    {
      dc_sprintf(buf, "$3R Range$R:  %d-%d  ", k->player->buildLowVnum, k->player->buildHighVnum);
      ch->send(buf);
      dc_sprintf(buf, "$3M Range$R:  %d-%d  ", k->player->buildMLowVnum, k->player->buildMHighVnum);
      ch->send(buf);
      dc_sprintf(buf, "$3O Range$R:  %d-%d  \r\n", k->player->buildOLowVnum, k->player->buildOHighVnum);
      ch->send(buf);
    }
  }

  ch->send(u"$3Lag Left$R:  %d\r\n"_s.arg((GET_WAIT(k) ? GET_WAIT(k) : 0)));

  if (k->isPlayer())
  {
    ch->send(u"$3Hp metas$R: %d, $3Mana metas$R: %d, $3Move metas$R: %d, $3Ki metas$R: %d, $3AC metas$R: %d, $3Age metas$R: %d\r\n"_s.arg(GET_HP_METAS(k), GET_MANA_METAS(k), GET_MOVE_METAS(k)).arg(GET_KI_METAS(k)).arg(GET_AC_METAS(k)).arg(GET_AGE_METAS(k)));
    ch->send(u"$3Profession$R: %s (%d)\r\n"_s.arg(find_profession(k->c_class, k->player->profession)).arg(k->player->profession));
  }

  if (k->affected)
  {
    ch->sendln("\r\n$3Affecting Spells$R:\r\n--------------");
    for (aff = k->affected; aff; aff = aff->next)
    {

      QString aff_name = get_skill_name(aff->type);

      if (aff_name.isEmpty())
      {
        if (aff->type == INTERNAL_SLEEPING)
          aff_name = u"Internal Sleeping"_s;
        else if (aff->type == Character::PLAYER_CANTQUIT)
          aff_name = u"CANTQUIT"_s;
        else
          aff_name = u"Unknown!!!"_s;
      }
      ch->sendln(u"Spell : '%1' (%2)"_s.arg(aff_name).arg(aff->type));
      dc_sprintf(buf, "     Modifies %s by %d points\r\n",
                 apply_types[(qint32)aff->location], aff->modifier);
      ch->send(buf);
      dc_sprintf(buf, "     Expires in %3d hours", aff->duration);
      //    dc_strcat(buf,",Bits set ");
      //      ch->send(buf);
      //      sprintbit(aff->bitvector,affected_bits,buf);
      dc_strcat(buf, "\r\n");
      ch->send(buf);
    }
    ch->sendln("");
  }

  if (k->isNonPlayer())
  {
    switch (k->mobdata->mob_flags.type)
    {
    case mob_type_t::MOB_NORMAL:
      break;
    case mob_type_t::MOB_GUARD:
      dc_sprintf(buf, "$3Guard room (v1)$R: [%d]\r\n"
                      " $3Direction (v2)$R: [%d]\r\n"
                      "    $3Unused (v3)$R: [%d]\r\n"
                      "    $3Unused (v4)$R: [%d]\r\n",
                 k->mobdata->mob_flags.value[0], k->mobdata->mob_flags.value[1],
                 k->mobdata->mob_flags.value[2], k->mobdata->mob_flags.value[3]);
      ch->send(buf);
      break;
    case mob_type_t::MOB_CLAN_GUARD:
      dc_sprintf(buf, "$3Guard room (v1)$R: [%d]\r\n"
                      " $3Direction (v2)$R: [%d]\r\n"
                      "  $3Clan num (v3)$R: [%d]\r\n"
                      "    $3Unused (v4)$R: [%d]\r\n",
                 k->mobdata->mob_flags.value[0], k->mobdata->mob_flags.value[1],
                 k->mobdata->mob_flags.value[2], k->mobdata->mob_flags.value[3]);
      ch->send(buf);
      break;
    default:
      dc_sprintf(buf, "$3Values 1-4 : [$R%d$3] [$R%d$3] [$R%d$3] [$R%d$3]$R\r\n",
                 k->mobdata->mob_flags.value[0], k->mobdata->mob_flags.value[1],
                 k->mobdata->mob_flags.value[2], k->mobdata->mob_flags.value[3]);
      ch->send(buf);
      break;
    }
  }

  return ReturnValue::eSUCCESS;
}

void obj_stat(CharacterPtr ch, ObjectPtr j)
{
  ObjectPtr j2 = {};
  QString buf;
  QString buf2;
  QString buf3;
  QString buf4;
  extra_descr_data *desc;
  bool found;
  qint32 i, virt;

  qint32 its;

  /*
    if(isSet(j->obj_flags.extra_flags, ITEM_DARK) && ch->getLevel() < POWER)
    {
      ch->sendln("A magical aura around the item attempts to conceal its secrets.");
      return;
    }
*/

  virt = (j->item_number >= 0) ? dc_->obj_index[j->item_number].vnum() : 0;
  dc_sprintf(buf, "$3Object name$R:[%s]  $3R-number$R:[%d]  $3V-number$R:[%d]  $3Item type$R: ",
             qPrintable(j->name()), j->item_number, virt);
  sprinttype(GET_ITEM_TYPE(j), item_types, buf2);

  dc_strcat(buf, buf2);
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  dc_sprintf(buf, "$3Short description$R: %s\r\n$3Long description$R:\r\n%s\r\n",
             ((j->short_description) ? j->short_description : "None"),
             ((j->long_description) ? j->long_description : "None"));
  ch->send(buf);
  if (j->ex_description)
  {
    dc_strcpy(buf, "$3Extra description keyword(s)$R:\r\n----------\r\n");
    for (desc = j->ex_description; desc; desc = desc->next)
    {
      dc_strcat(buf, desc->keyword);
      dc_strcat(buf, "\r\n");
    }
    dc_strcat(buf, "----------\r\n");
    ch->send(buf);
  }
  else
  {
    dc_strcpy(buf, "$3Extra description keyword(s)$R: None\r\n");
    ch->send(buf);
  }
  ch->send("$3Can be worn on$R:");
  sprintbit(j->obj_flags.wear_flags, QFlagsToStrings<ObjectPositions>(), buf);
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  ch->send("$3Can be worn by$R:");
  sprintbit(j->obj_flags.size, Object::size_bits, buf);
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  ch->send("$3Extra flags$R: ");
  sprintbit(j->obj_flags.extra_flags, Object::extra_bits, buf);
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  ch->send("$3More flags$R: ");
  sprintbit(j->obj_flags.more_flags, Object::more_obj_bits, buf);
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  dc_sprintf(buf, "$3Weight$R: %d  $3Value$R: %d  $3Timer$R: %d  $3Eq Level$R: %llu\r\n",
             j->obj_flags.weight,
             j->obj_flags.cost,
             j->obj_flags.timer,
             j->obj_flags.eq_level);
  ch->send(buf);

  dc_strcpy(buf, "$3In room$R: ");
  if (j->in_room == DC::NOWHERE)
    dc_strcat(buf, "NOWHERE");
  else
  {
    dc_sprintf(buf2, "%d", dc_->world[j->in_room].number);
    dc_strcat(buf, buf2);
  }
  dc_strcat(buf, "  $3In object$R: ");
  dc_strcat(buf, (!j->in_obj ? "None" : fname(qPrintable(j->in_obj->name()))));
  dc_strcat(buf, "  $3Carried by$R: ");
  dc_strcat(buf, (!j->carried_by) ? "Nobody" : qPrintable(j->carried_by->name()));
  dc_strcat(buf, "\r\n");
  ch->send(buf);

  switch (j->obj_flags.type_flag)
  {
  case ITEM_LIGHT:
    dc_sprintf(buf, "$3Colour (v1)$R: %d\r\n"
                    "$3Type   (v2)$R: %d\r\n"
                    "$3Hours  (v3)$R: %d\r\n"
                    "$3Unused (v4)$R: %d",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[2],
               j->obj_flags.value[3]);
    break;
  case ITEM_SCROLL:
    sprinttype(j->obj_flags.value[1] - 1, spells, buf2);
    sprinttype(j->obj_flags.value[2] - 1, spells, buf3);
    sprinttype(j->obj_flags.value[3] - 1, spells, buf4);
    dc_sprintf(buf, "$3Level(v1)$R  : %d\r\n"
                    " $3Spells(v2)$R: %d (%s)\r\n"
                    " $3Spells(v3)$R: %d (%s)\r\n"
                    " $3Spells(v4)$R: %d (%s)",
               j->obj_flags.value[0],
               j->obj_flags.value[1], buf2,
               j->obj_flags.value[2], buf3,
               j->obj_flags.value[3], buf4);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprinttype(j->obj_flags.value[3] - 1, spells, buf2);
    dc_sprintf(buf, "$3Level(v1)$R: %d  $3Spell(v4)$R: %d - %s\r\n"
                    "$3Total Charges(v2)$R: %d   $3Current Charges(v3)$R: %d",
               j->obj_flags.value[0],
               j->obj_flags.value[3],
               buf2,
               j->obj_flags.value[1],
               j->obj_flags.value[2]);
    break;
  case ITEM_WEAPON:
    qint32 get_weapon_damage_type(ObjectPtr wielded);
    its = get_weapon_damage_type(j) - 1000;
    extern QStringList strs_damage_types;
    dc_sprintf(buf, "$3Unused(v1)$R: %d (make 0)\r\n$3Todam(v2)d(v3)$R: %dD%d\r\n$3Type(v4)$R: %d (%s)",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[2],
               j->obj_flags.value[3],
               strs_damage_types[its]);
    break;
  case ITEM_MEGAPHONE:
    dc_sprintf(buf, "Interval(v1): %d\r\nInterval, again(v2): %d", j->obj_flags.value[0], j->obj_flags.value[1]);
    break;
  case ITEM_FIREWEAPON:
    dc_sprintf(buf, "$3Tohit(v1)$R: %d\r\n$3Todam(v2)d<v3)$R: %dD%d\r\n$3Type(v4)$R: %d",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[2],
               j->obj_flags.value[3]);
    break;
  case ITEM_MISSILE:
    dc_sprintf(buf, "$3Damage(v1dv2)$R: %d$3/$R%d\r\n$3Tohit(v3)$R: %d\r\n$3Todam(v4)$R: %d",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[3],
               j->obj_flags.value[2]);
    break;
  case ITEM_ARMOR:
    dc_sprintf(buf, "$3AC-apply(v1)$R: [%d]\r\n"
                    "$3Unused  (v2)$R: [%d] (make 0)\r\n"
                    "$3Unused  (v3)$R: [%d] (make 0)\r\n"
                    "$3Unused  (v4)$R: [%d] (make 0)",
               j->obj_flags.value[0], j->obj_flags.value[1], j->obj_flags.value[2], j->obj_flags.value[3]);
    break;
  case ITEM_POTION:
    sprinttype(j->obj_flags.value[1] - 1, spells, buf2);
    sprinttype(j->obj_flags.value[2] - 1, spells, buf3);
    sprinttype(j->obj_flags.value[3] - 1, spells, buf4);
    dc_sprintf(buf, "$3Level (v1)$R: %d\r\n"
                    " $3Spell(v2)$R: %d (%s)\r\n"
                    " $3Spell(v3)$R: %d (%s)\r\n"
                    " $3Spell(v4)$R: %d (%s)",
               j->obj_flags.value[0],
               j->obj_flags.value[1], buf2,
               j->obj_flags.value[2], buf3,
               j->obj_flags.value[3], buf4);
    break;
  case ITEM_TRAP:
    dc_sprintf(buf, "$3Spell(v1)$R    : %d\r\n"
                    "$3Hitpoints(v2)$R: %d",
               j->obj_flags.value[0],
               j->obj_flags.value[1]);
    break;
  case ITEM_CONTAINER:
    dc_sprintf(buf, "$3Max-contains(v1)$R : %d\r\n"
                    "$3Locktype(v2)$R     : %d\r\n"
                    "$3Key #$R            : %d\r\n"
                    "$3Corpse(v4)$R       : %s",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[2],
               j->obj_flags.value[3] ? "Yes" : "No");
    break;
  case ITEM_DRINKCON:
    sprinttype(j->obj_flags.value[2], drinks, buf2);
    //  dc_strcpy(buf2,drinks[j->obj_flags.value[2]]);
    dc_sprintf(buf, "$3Max-contains(v1)$R: %d\r\n"
                    "$3Contains    (v2)$R: %d\r\n"
                    "$3Liquid      (v3)$R: %s (%d)\r\n"
                    "$3Poisoned    (v4)$R: %d",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               buf2,
               j->obj_flags.value[2],
               j->obj_flags.value[3]);
    break;
  case ITEM_NOTE:
    dc_sprintf(buf, "$3Tounge(v1)$R : %d"
                    "$3Unused(v2)$R : %d"
                    "$3Unused(v3)$R : %d"
                    "$3Unused(v4)$R : %d",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[2],
               j->obj_flags.value[3]);
    break;
  case ITEM_UTILITY:
    dc_sprintf(buf2, "$3Utility Type(v1)$R : %d (%s)\r\n",
               j->obj_flags.value[0],
               j->obj_flags.value[0] >= 0 && j->obj_flags.value[0] <= UTILITY_ITEM_MAX ? utility_item_types[j->obj_flags.value[0]] : "INVALID TYPE");
    if (j->obj_flags.value[0] == UTILITY_CATSTINK)
    {
      dc_sprintf(buf, "%s"
                      "$3Sector(v2)$R : %d (%s)\r\n"
                      "$3Unused(v3)$R : %d "
                      "$3HowMuchLag(v4)$R : %d",
                 buf2,
                 j->obj_flags.value[1],
                 j->obj_flags.value[1] >= 0 && j->obj_flags.value[1] <= SECT_MAX_SECT ? sector_types[j->obj_flags.value[1]] : "INVALID SECTOR TYPE",
                 j->obj_flags.value[2],
                 j->obj_flags.value[3]);
    }
    else if (j->obj_flags.value[0] == UTILITY_MORTAR)
    {
      dc_sprintf(buf, "%s"
                      "$3NumDice(v2)$R : %d "
                      "$3DiceSize (v3)$R : %d "
                      "$3HowMuchLag(v4)$R : %d ",
                 buf2,
                 j->obj_flags.value[1],
                 j->obj_flags.value[2],
                 j->obj_flags.value[3]);
    }
    else
    {
      dc_sprintf(buf, "%s"
                      "$3Unused(v2)$R : %d "
                      "$3Unused(v3)$R : %d "
                      "$3HowMuchLag(v4)$R : %d",
                 buf2,
                 j->obj_flags.value[1],
                 j->obj_flags.value[2],
                 j->obj_flags.value[3]);
    }
    break;
  case ITEM_KEY:
    dc_sprintf(buf, "$3Keytype(v1)$R : %d"
                    "$3Unused (v2)$R : %d"
                    "$3Unused (v3)$R : %d"
                    "$3Unused (v4)$R : %d",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[2],
               j->obj_flags.value[3]);
    break;
  case ITEM_FOOD:
    dc_sprintf(buf, "$3Makes full(v1)$R : %d\r\n"
                    "$3Poisoned  (v4)$R : %d",
               j->obj_flags.value[0],
               j->obj_flags.value[3]);
    break;
  case ITEM_INSTRUMENT:
    dc_sprintf(buf, "$3Song Effect$R:  $3Non-Combat(v1)$R[%d] $3Combat(v2)$R[%d]",
               j->obj_flags.value[0],
               j->obj_flags.value[1]);
    break;
  case ITEM_PORTAL:
    dc_sprintf(buf, "$3ToRoom (v1)$R : %lu\r\n"
                    "$3Type   (v2)$R : ",
               j->getPortalDestinationRoom());
    switch (j->getPortalType())
    {
    case Object::portal_types_t::Player:
      dc_strcat(buf, "0-Player-Portal");
      break;
    case Object::portal_types_t::Permanent:
      dc_strcat(buf, "1-Permanent-Game-Portal");
      break;
    case Object::portal_types_t::Temp:
      dc_strcat(buf, "2-Temp-Game-portal");
      break;
    case Object::portal_types_t::LookOnly:
      dc_strcat(buf, "3-Look-only-portal");
      break;
    case Object::portal_types_t::PermanentNoLook:
      dc_strcat(buf, "4-Perm-No-Look-portal");
      break;
    default:
      dc_strcat(buf, "Unknown!!!");
      break;
    }
    dc_sprintf(buf2, "(can be 0-4)\r\n"
                     "$3Zone   (v3)$R : %d (can 'leave' anywhere from this zone (set to -1 otherwise))\r\n"
                     "$3Flags  (v4)$R : ",
               j->obj_flags.value[2]);
    dc_strcat(buf, buf2);
    sprintbit(j->obj_flags.value[3], portal_bits, buf2);
    dc_strcat(buf, buf2);
    dc_strcat(buf, "\n(0 = nobits, 1 = no_leave, 2 = no_enter)");
    break;
  default:
    dc_sprintf(buf, "Values 0-3 : [%d] [%d] [%d] [%d]",
               j->obj_flags.value[0],
               j->obj_flags.value[1],
               j->obj_flags.value[2],
               j->obj_flags.value[3]);
    break;
  }
  ch->send(buf);

  dc_strcpy(buf, "\r\n$3Equipment Status$R: ");
  if (!j->carried_by)
    dc_strcat(buf, "NONE");
  else
  {
    found = false;
    for (i = {}; i < MAX_WEAR; i++)
    {
      if (j->carried_by->equipment[i] == j)
      {
        sprinttype(i, equipment_types, buf2);
        dc_strcat(buf, buf2);
        found = true;
      }
    }
    if (!found)
      dc_strcat(buf, "Inventory");
  }
  ch->send(buf);

  dc_strcpy(buf, "\r\n$3Non-Combat Special procedure$R : ");
  if (j->item_number >= 0)
    dc_strcat(buf, (dc_->obj_index[j->item_number].non_combat_func ? "exists\r\n" : "none\r\n"));
  else
    dc_strcat(buf, "No\r\n");
  ch->send(buf);
  dc_strcpy(buf, "$3Combat Special procedure$R : ");
  if (j->item_number >= 0)
    dc_strcat(buf, (dc_->obj_index[j->item_number].combat_func ? "exists\r\n" : "none\r\n"));
  else
    dc_strcat(buf, "No\r\n");
  ch->send(buf);
  dc_strcpy(buf, "$3Contains$R :\r\n");
  found = false;
  for (j2 = j->contains; j2; j2 = j2->next_content)
  {
    dc_strcat(buf, qPrintable(fname(j2->name())));
    dc_strcat(buf, "\r\n");
    found = true;
  }
  if (!found)
    dc_strcpy(buf, "$3Contains$R : Nothing\r\n");
  ch->send(buf);

  ch->sendln("$3Can affect character$R :");
  for (i = {}; i < j->num_affects; i++)
  {
    //      sprinttype(j->affected[i].location,apply_types,buf2);
    if (j->affected[i].location < 1000)
      sprinttype(j->affected[i].location, apply_types, buf2);
    else if (!get_skill_name(j->affected[i].location / 1000).isEmpty())
      dc_strcpy(buf2, get_skill_name(j->affected[i].location / 1000).toStdString().c_str());
    else
      dc_strcpy(buf2, "Invalid");

    dc_sprintf(buf, "    $3Affects$R : %s By %d\r\n", buf2, j->affected[i].modifier);
    ch->send(buf);
  }
}

void do_start(CharacterPtr ch)
{
  QString buf;

  ch->sendln("This is now your character in Dark Castle MUD");

  if (ch->isNonPlayer())
    return;

  ch->setLevel(1);
  ch->exp = 1;

  if (ch->title == nullptr)
  {
    ch->title = u"is a virgin."_s;
  }

  dc_sprintf(buf, "%s appears with an ear-splitting bang!", qPrintable(ch->shortdesc_or_name()));
  ch->player->poofin = buf;

  dc_sprintf(buf, "%s disappears in a puff of smoke.", qPrintable(ch->shortdesc_or_name()));
  ch->player->poofout = buf;

  ch->raw_hit = 10;
  ch->max_hit = 10;

  switch (GET_CLASS(ch))
  {

  case CLASS_MAGIC_USER:
    ch->player->practices = 6;
    break;

  case CLASS_CLERIC:
    ch->player->practices = 6;
    break;

  case CLASS_THIEF:
    ch->player->practices = 6;
    break;

  case CLASS_WARRIOR:
    ch->player->practices = 6;
    break;
  }

  advance_level(ch, 0);
  redo_hitpoints(ch);

  GET_RDEATHS(ch) = {};
  GET_PDEATHS(ch) = {};
  GET_PKILLS(ch) = {};

  ch->fillHPLimit();
  GET_MANA(ch) = mana_limit(ch);
  ch->setMove(ch->move_limit());

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = {};

  ch->player->time.played = {};
  ch->player->time.logon = time(0);
}

command_return_t do_repop(CharacterPtr ch, QString arguments, cmd_t cmd)
{
  if (ch->getLevel() < DEITY && !can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You may only repop inside of your room range.");
    return ReturnValue::eFAILURE;
  }

  QString arg1;
  std::tie(arg1, arguments) = half_chop(arguments);

  if (arg1 == "full")
  {
    ch->sendln("Performing full zone reset!");
    QString buf = fmt::format("{} full repopped zone #{}.", qPrintable(ch->name()), dc_->world[ch->in_room].zone);
    dc_->logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
    DC::resetZone(dc_->world[ch->in_room].zone, Zone::ResetType::full);
  }
  else
  {
    ch->sendln("Resetting this entire zone!");
    QString buf = fmt::format("{} repopped zone #{}.", qPrintable(ch->name()), dc_->world[ch->in_room].zone);
    dc_->logentry(buf.c_str(), ch->getLevel(), DC::LogChannel::LOG_GOD);
    DC::resetZone(dc_->world[ch->in_room].zone);
  }

  return ReturnValue::eSUCCESS;
}

command_return_t do_clear(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  qint32 zone = dc_->world[ch->in_room].zone;

  if (ch->getLevel() < DEITY && !can_modify_room(ch, ch->in_room))
  {
    ch->sendln("You may only repop inside of your R range.");
    return ReturnValue::eFAILURE;
  }

  const auto &character_list = dc_->character_list;
  for (const auto &tmp_victim : character_list)
  {
    // This should never happen but it has before so we must investigate without crashing the whole MUD
    if (tmp_victim == 0)
    {
      produce_coredump(tmp_victim);
      continue;
    }
    if (GET_POS(tmp_victim) == position_t::DEAD || tmp_victim->in_room == DC::NOWHERE)
    {
      continue;
    }
    if (dc_->world[tmp_victim->in_room].zone == zone)
    {
      if (tmp_victim->isNonPlayer())
      {
        for (qint32 l = {}; l < MAX_WEAR; l++)
        {
          if (tmp_victim->equipment[l])
            extract_obj(tmp_victim->unequip_char(l));
        }
        while (tmp_victim->carrying)
          extract_obj(tmp_victim->carrying);
        extract_char(tmp_victim, true);
      }
      else
        tmp_victim->sendln("You hear unmatched screams of terror as all mobs are summarily executed!");
    }
  }
  ch->sendln("You have just caused the deion of countless creatures in ths area!");
  dc_sprintf(buf, "%s just CLEARED zone #%d!", qPrintable(ch->name()), zone);
  dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
  return ReturnValue::eSUCCESS;
}

command_return_t do_linkdead(CharacterPtr ch, QString arg, cmd_t cmd)
{
  qint32 x = {};
  QString buf;

  const auto &character_list = dc_->character_list;
  for (const auto &i : character_list)
  {

    if (i->isNonPlayer() || i->desc || !CAN_SEE(ch, i))
      continue;
    x++;

    if (i->player->possesing)
      dc_sprintf(buf, "%14s -- [%d] %s  *possessing*\r\n", qPrintable(i->name()),
                 (qint32)(dc_->world[i->in_room].number), (dc_->world[i->in_room].name));
    else
      dc_sprintf(buf, "%14s -- [%d] %s\r\n", qPrintable(i->name()),
                 (qint32)(dc_->world[i->in_room].number), (dc_->world[i->in_room].name));
    ch->send(buf);
  }

  if (!x)
    ch->sendln("No linkdead players found.");
  return ReturnValue::eSUCCESS;
}

command_return_t do_echo(CharacterPtr ch, QString argument, cmd_t cmd)
{
  qint32 i;
  QString buf;
  CharacterPtr vict;

  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  for (i = {}; *(argument + i) == ' '; i++)
    ;

  if (!*(argument + i))
    ch->sendln("What message do you want to echo to ths room?");

  else
  {
    dc_sprintf(buf, "\r\n%s\r\n", argument + i);
    for (vict = dc_->world[ch->in_room].people_; vict; vict = vict->next_in_room)
      vict->send(buf);
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_restore(CharacterPtr ch, const QString argument, cmd_t cmd)
{
  CharacterPtr victim;
  QString buf;
  ConnectionPtr i;

  void update_pos(CharacterPtr victim);

  if (!ch->has_skill(COMMAND_RESTORE))
  {
    ch->sendln("Huh?");
    return ReturnValue::eFAILURE;
  }

  one_argument(argument, buf);
  if (buf.isEmpty())
    ch->sendln("Whom do you wish to restore?");
  else if (dc_strcmp(buf, "all"))
  {

    if (!(victim = get_char(buf)))
    {
      ch->sendln("No-one by that name in the world.");
      return ReturnValue::eFAILURE;
    }

    GET_MANA(victim) = GET_MAX_MANA(victim);
    victim->fillHP();
    victim->setMove(GET_MAX_MOVE(victim));
    GET_KI(victim) = GET_MAX_KI(victim);

    if (victim->getLevel() >= IMMORTAL)
    {
      GET_COND(victim, FULL) = -1;
      GET_COND(victim, THIRST) = -1;
    }
    else
    {
      if (GET_COND(victim, FULL) != -1)
      {
        GET_COND(victim, FULL) = 25;
        GET_COND(victim, THIRST) = 25;
      }
    }

    dc_sprintf(buf, "%s restored %s.", qPrintable(ch->name()), qPrintable(victim->name()));
    dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);

    update_pos(victim);
    redo_hitpoints(victim);
    redo_mana(victim);
    redo_ki(victim);
    ch->sendln("Done.");
    if (!IS_AFFECTED(ch, AFF_BLIND))
      act("You have been fully healed by $N!",
          victim, 0, ch, TO_CHAR, 0);
    victim->save();
  }
  else
  { /* Restore all */

    if (ch->getLevel() < OVERSEER)
    {
      ch->sendln("You don't have the ability to do that!");
      dc_sprintf(buf, "%s tried to do a restore all!", qPrintable(ch->name()));
      dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);

      return ReturnValue::eFAILURE;
    }
    for (auto &i : dc_->connections_)
      if (i->character != ch && !i->connected)
      {

        victim = i->character;

        GET_MANA(victim) = GET_MAX_MANA(victim);
        victim->fillHP();
        victim->setMove(GET_MAX_MOVE(victim));
        GET_KI(victim) = GET_MAX_KI(victim);

        if (victim->getLevel() >= IMMORTAL)
        {
          GET_COND(victim, FULL) = -1;
          GET_COND(victim, THIRST) = -1;
        }

        update_pos(victim);
        redo_ki(victim);
        redo_hitpoints(victim);
        redo_mana(victim);

        act("You have been fully healed by $N!",
            victim, 0, ch, TO_CHAR, 0);
        victim->save();
      }
    dc_sprintf(buf, "%s did a restore all!", qPrintable(ch->name()));
    dc_->logentry(buf, ch->getLevel(), DC::LogChannel::LOG_GOD);
    ch->sendln("Trying to be Mister Popularity?");
  }
  return ReturnValue::eSUCCESS;
}

// Scavenger hunts..

class hunt_data
{
public:
  hunt_data *next;
  QString huntname;
  qint32 itemnum;
  qint32 time;
  qint32 itemsAvail[50];
};

class hunt_items
{ // Bleh, don't wanna make it go through every item in the game everytime someone checks the list
public:
  hunt_items *next;
  hunt_data *hunt;
  ObjectPtr obj;
  QString mobname;
};

hunt_data *hunt_list = {};
hunt_items *hunt_items_list = {};

void check_end_of_hunt(hunt_data *h, bool forced = false)
{
  hunt_items *i, *p = {}, *in;
  qint32 items = {};

  for (i = hunt_items_list; i; i = in)
  {
    in = i->next;
    if (i->hunt == h)
    {
      if (h->time <= 0)
      {
        // obj_from_char(i->obj);
        if (p)
          p->next = in;
        else
          hunt_items_list = in;

        extract_obj(i->obj);
        i->mobname = {};
        i = {};
        continue;
      }
      else
        items++;
    }
    p = i;
  }
  QString buf;
  if (items == 0)
  {
    if (!forced)
    {
      if (h->huntname)
      {
        if (h->time <= 0)
          dc_sprintf(buf, "\r\n## The time limit on hunt '%s' has expired and all unrecovered prizes have been removed.\r\n", h->huntname);
        else
          dc_sprintf(buf, "\r\n## All prizes have been recovered on hunt '%s'\r\n", h->huntname);
      }
      else
      {
        if (h->time <= 0)
          dc_sprintf(buf, "\r\n## The time limit on the hunt for '%s' has expired and all unrecovered prizes have been removed.\r\n", (dc_->obj_index[real_object(h->itemnum)].item)->short_description);
        else
          dc_sprintf(buf, "\r\n## All prizes have been recovered on the hunt for '%s'\r\n", (dc_->obj_index[real_object(h->itemnum)].item)->short_description);
      }
    }
    else
    {
      if (h->huntname)
      {
        dc_sprintf(buf, "\r\n## Hunt '%s' has been ended.\r\n", h->huntname);
      }
      else
      {
        dc_sprintf(buf, "\r\n## The hunt for '%s' has been ended.\r\n", (dc_->obj_index[real_object(h->itemnum)].item)->short_description);
      }
    }
    send_info(buf);

    hunt_data *hl, *p = {};
    for (hl = hunt_list; hl; hl = hl->next)
    {
      if (hl == h)
      {
        if (p)
          p->next = hl->next;
        else
          hunt_list = hl->next;
        break;
      }
      p = hl;
    }
    h->huntname = {};
    h = {};
  }
}

command_return_t do_huntclear(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString arg1;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "doit"))
  {
    ch->sendln("Syntax: huntclear doit\r\nClears all currently running treasure hunts.");
    return ReturnValue::eSUCCESS;
  }
  else
  {
    hunt_data *h, *hn;
    for (h = hunt_list; h; h = hn)
    {
      hn = h->next;
      h->time = -1;
      check_end_of_hunt(h, true);
    }
    ch->sendln("Done!");
    return ReturnValue::eSUCCESS;
  }
}

void huntclear_item(ObjectPtr obj)
{
  hunt_items *hi, *hin, *hip = {};
  for (hi = hunt_items_list; hi; hi = hin)
  {
    hin = hi->next;
    if (hi->obj == obj)
    {
      if (hip)
        hip->next = hin;
      else
        hunt_items_list = hin;
      hi->mobname = {};
      hi = {};
      continue; // Hopefully there's not two in the list, but just in case.
    }
    hip = hi;
  }
}

qint32 get_rand_obj(hunt_data *h)
{
  qint32 i, v;

  for (i = 49; i > 0; i--)
    if (h->itemsAvail[i] > 0)
      break;

  if (i < 0)
    return -1;

  v = dc_->number(0, i);
  qint32 c = h->itemsAvail[v];
  h->itemsAvail[v] = h->itemsAvail[i];
  h->itemsAvail[i] = -1;

  return c;
}

void init_random_hunt_items(hunt_data *h)
{
  FILE *f;
  if ((f = fopen("huntitems.txt", "r")) == nullptr)
  {
    for (qint32 i = {}; i < 50; i++)
      h->itemsAvail[i] = -1;
    return;
  }
  qint32 a, i;
  bool over = false;
  for (a = {}; a < 50; a++)
  {
    if (!over)
    {
      i = fread_int(f, 0, 32768);
      if (i == 0)
        over = true;
      else
      {
        h->itemsAvail[a] = i;
        continue;
      }
    }
    h->itemsAvail[a] = -1;
  }
  fclose(f);
}

QString last_hunt_time(QString last_hunt)
{
  static QString time_of_last_hunt = {};
  QString buf;

  if (!time_of_last_hunt)
  {
    dc_sprintf(buf, "There have been no hunts since the last reboot.       \r\n");
    time_of_last_hunt = (buf);
  }

  if (last_hunt)
    time_of_last_hunt = last_hunt;

  return time_of_last_hunt;
}

void begin_hunt(qint32 item, qint32 duration, qint32 amount, QString huntname)
{ // time, itme, item
  hunt_data *n;
  QString tmp;
  tm *pTime = {};
  time_t ct;

  auto n = new hunt_data;
  n->next = hunt_list;
  hunt_list = n;
  n->itemnum = item;
  n->time = duration;
  if (huntname)
    n->huntname = (huntname);

  ct = time(0);
  pTime = localtime(&ct);
  tmp = last_hunt_time(nullptr);

  if (nullptr != pTime)
  {
#ifdef __CYGWIN__
    dc_snprintf(tmp, dc_strlen(tmp) + 1, "%d/%d/%d (%d:%02d)\r\n",
                pTime->tm_mon + 1,
                pTime->tm_mday,
                pTime->tm_year + 1900,
                pTime->tm_hour,
                pTime->tm_min);
#else
    dc_snprintf(tmp, dc_strlen(tmp) + 1, "%d/%d/%d (%d:%02d) %s\r\n",
                pTime->tm_mon + 1,
                pTime->tm_mday,
                pTime->tm_year + 1900,
                pTime->tm_hour,
                pTime->tm_min,
                pTime->tm_zone);
#endif
  }

  if (item == 76)
    init_random_hunt_items(n);
  qint32 rnum = real_object(item);
  extern qint32 top_of_mobt;
  if (rnum < 0)
    return;

  for (qint32 i = {}; i < amount; i++)
  {
    qint32 mob = -1;
    CharacterPtr vict;
    while (1)
    {
      mob = dc_->number(1, top_of_mobt);
      qint32 vnum = dc_->mob_index[mob].vnum(); // debug
      if (!(dc_->mob_index[mob].vnum() > 300 &&
            (dc_->mob_index[mob].vnum() < 2300 || dc_->mob_index[mob].vnum() > 2499) &&
            (dc_->mob_index[mob].vnum() < 29200 || dc_->mob_index[mob].vnum() > 29299) &&
            (dc_->mob_index[mob].vnum() < 5600 || dc_->mob_index[mob].vnum() > 5699) &&
            (dc_->mob_index[mob].vnum() < 16200 || dc_->mob_index[mob].vnum() > 16399) &&
            (dc_->mob_index[mob].vnum() < 1900 || dc_->mob_index[mob].vnum() > 1999) &&
            (dc_->mob_index[mob].vnum() < 10500 || dc_->mob_index[mob].vnum() > 10622) &&
            (dc_->mob_index[mob].vnum() < 8500 || dc_->mob_index[mob].vnum() > 8699)))
        continue;

      // Skip mobs marked NO_HUNT
      CharacterPtr m = static_cast<CharacterPtr>(dc_->mob_index[mob].item);
      if (m && m->mobdata && ISSET(m->mobdata->actflags, ACT_NO_HUNT))
      {
        continue;
      }

      if (dc_->mob_index[mob].qty <= 0)
        continue;
      if (!(vict = get_random_mob_vnum(vnum)))
        continue;
      if (dc_->zones.value(dc_->world[vict->in_room].zone).isNoHunt())
        continue;

      if (dc_strlen(vict->short_desc) > 34)
        continue; // They suck

      break;
    }
    ObjectPtr obj = clone_object(rnum);
    obj_to_char(obj, vict);
    hunt_items *ni;
    auto ni = new hunt_items;
    ni->hunt = n;
    ni->obj = obj;
    ni->mobname = (vict->short_desc);
    ni->next = hunt_items_list;
    hunt_items_list = ni;
  }
}

void pick_up_item(CharacterPtr ch, ObjectPtr obj)
{
  hunt_items *i, *p = {}, *in;
  QString buf;
  qint32 gold = {};
  for (i = hunt_items_list; i; i = in)
  {
    in = i->next;
    if (i->obj == obj)
    {
      if (p)
        p->next = in;
      else
        hunt_items_list = in;
      qint32 vnum = dc_->obj_index[obj->item_number].vnum();
      dc_sprintf(buf, "\r\n## %s has been recovered from %s by %s!\r\n",
                 qPrintable(obj->short_description()), i->mobname, qPrintable(ch->name()));
      send_info(buf);
      hunt_data *h = i->hunt;
      ObjectPtr oitem = {}, citem;
      qint32 r1 = {};
      switch (vnum)
      {
      case 76:
        obj_from_char(obj);
        obj_to_room(obj, 6345);
        r1 = real_object(get_rand_obj(h));
        if (r1 > 0)
        {
          oitem = clone_object(r1);
          dc_sprintf(buf, "As if by magic, %s transforms into %s!\r\n",
                     qPrintable(obj->short_description()), oitem->short_description);
          ch->send(buf);
          dc_sprintf(buf, "## %s turned into %s!\r\n",
                     qPrintable(obj->short_description()), oitem->short_description);
          send_info(buf);
          if (isSet(oitem->obj_flags.more_flags, ITEM_UNIQUE))
          {
            if (search_char_for_item(ch, oitem->item_number, false))
            {
              if (isSet(oitem->obj_flags.more_flags, ITEM_24H_SAVE))
              {
                ch->sendln("You already have this item - Timer has been reset!");
                extract_obj(oitem);
                citem = search_char_for_item(ch, oitem->item_number, false);
                citem->save_expiration = time(nullptr) + (60 * 60 * 24);
                break; // Used to crash it.
              }
              else
              {
                ch->sendln("The item's uniqueness causes it to poof into thin air!");
                extract_obj(oitem);
                break; // Used to crash it.
              }
            }
            else
              obj_to_char(oitem, ch);
          }
          else
            obj_to_char(oitem, ch);
        }
        else
        {
          ch->sendln("Brick turned into a non-existent item. Tell an imm.");
          break;
        }
        if (dc_->obj_index[oitem->item_number].vnum() < 27915 || dc_->obj_index[oitem->item_number].vnum() > 27918)
          break;
        else
          obj = oitem; // Gold! Continue on to the next cases.
        /* no break */
      case 27915:
      case 27916:
      case 27917:
      case 27918:
        gold = obj->obj_flags.value[0];
        dc_sprintf(buf, "As if by magic, %s transform into %d gold!\r\n",
                   qPrintable(obj->short_description()), gold);
        ch->send(buf);

        ch->addGold(gold);
        obj_from_char(obj);
        obj_to_room(obj, 6345);
        break;
      }
      i->mobname = {};
      i = {};
      check_end_of_hunt(h);
      continue;
    }
    p = i;
  }
}

void pulse_hunts()
{
  hunt_data *h, *hn;

  for (h = hunt_list; h; h = hn)
  {
    hn = h->next;
    h->time--;
    check_end_of_hunt(h);
  }

  try
  {
    if (dc_->world[6345] == Room())
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "pulse_hunts: room 6345 does not exist.");
      return;
    }

    ObjectPtr obj, onext;
    for (obj = dc_->world[6345].contents; obj; obj = onext)
    {
      onext = obj->next_content;
      extract_obj(obj);
    }
  }
  catch (...)
  {
    dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "pulse_hunts: room 6345 does not exist.");
  }
}

command_return_t do_showhunt(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf;
  hunt_data *h;
  hunt_items *hi;

  if (!hunt_list)
  {
    ch->sendln("There are no active hunts at the moment.");

    ch->send(fmt::format("Last hunt was run: {}\r\n", last_hunt_time(nullptr)));

    ch->send(buf);
  }
  else
    ch->sendln("The following hunts are currently active:");

  for (h = hunt_list; h; h = h->next)
  {
    if (h->huntname)
    {
      ch->send(fmt::format("\r\n{} for '{}'({} minutes remaining):\r\n", h->huntname, (dc_->obj_index[real_object(h->itemnum)].item)->short_description, h->time));
    }
    else
    {
      ch->send(fmt::format("\r\nThe hunt for '{}'({} minutes remaining):\r\n", (dc_->obj_index[real_object(h->itemnum)].item)->short_description, h->time));
    }

    qint32 itemsleft = {};
    for (hi = hunt_items_list; hi; hi = hi->next)
    {
      if (hi->hunt != h)
      {
        continue;
      }

      itemsleft++;
      ch->send(fmt::format("{}| {:35}  ", buf, hi->mobname));

      if ((itemsleft % 2) == 0)
      {
        ch->send(fmt::format("{}|\r\n", buf));
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

command_return_t do_huntstart(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg, arg2, arg3, buf;
#ifdef TWITTER
  twitCurl twitterObj;
  QString authUrl, replyMsg;
  QString myOAuthAccessTokenKey("");
  QString myOAuthAccessTokenSecret("");
  QString userName("");
  QString passWord("");
  QString message("");

  userName = "username";
  passWord = "password";

  twitterObj.setTwitterUsername(userName);
  twitterObj.setTwitterPassword(passWord);

  twitterObj.getOAuth().setConsumerKey(QString("xyz"));
  twitterObj.getOAuth().setConsumerSecret(QString("xyz"));

  twitterObj.getOAuth().setOAuthTokenKey(QString("xyz-xyz"));
  twitterObj.getOAuth().setOAuthTokenSecret(QString("xyz"));

  if (twitterObj.accountVerifyCredGet())
  {
    //    twitterObj.getLastWebResponse( replyMsg );
    dc_sprintf(buf, "twitterClient:: twitCurl::accountVerifyCredGet web response:\n%s\n", replyMsg.c_str());
    dc_->logentry(buf, 100, DC::LogChannel::LOG_GOD);
  }
  else
  {
    twitterObj.getLastCurlError(replyMsg);
    dc_sprintf(buf, "twitterClient:: twitCurl::accountVerifyCredGet error:\n%s\n", replyMsg.c_str());
    dc_->logentry(buf, 100, DC::LogChannel::LOG_GOD);
  }
#endif

  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg2);
  argument = one_argument(argument, arg3);

  if (arg3[0] == '\0')
  {
    ch->sendln("Syntax: huntstart <vnum> <# of items (1-50)> <time limit> [hunt name]");
    return ReturnValue::eSUCCESS;
  }
  qint32 vnum = atoi(arg), num = atoi(arg2), time = atoi(arg3);
  if (vnum <= 0 || real_object(vnum) < 0)
  {
    ch->sendln("Non-existent item.");
    return ReturnValue::eSUCCESS;
  }
  if (num <= 0 || num > 50)
  {
    ch->sendln("Invalid number of items. Maximum of 50 allowed.");
    return ReturnValue::eSUCCESS;
  }
  if (time <= 0)
  {
    ch->sendln("Invalid duration.");
    return ReturnValue::eSUCCESS;
  }
  hunt_data *h;
  for (h = hunt_list; h; h = h->next)
    if (h->itemnum == vnum)
    {
      ch->sendln("A hunt for that item is already ongoing!");
      return ReturnValue::eSUCCESS;
    }
  QString huntname;
  if (argument && *argument)
  {
    dc_strcpy(huntname, argument);
    begin_hunt(vnum, time, num, argument);
  }
  else
  {
    dc_strcpy(huntname, "A hunt");
    begin_hunt(vnum, time, num, 0);
  }

  dc_sprintf(buf, "\r\n## %s has been started! There are a total of %d items and %d minutes to find them all!\r\n## Type 'huntitems' to get the locations!\r\n", huntname, num, time);
  send_info(buf);

#ifdef TWITTER
  QString holding[3] = {
      "Get your ass to MAHS... err, DC.  There's a hunt!",
      "You may be wondering why I've gathered you here today.  There's a hunt!",
      "Aussie Aussie Aussie! Oi Oi Hunt!!"};

  qint32 pos = rand() % 3;

  message = holding[pos];

  if (twitterObj.statusUpdate(message))
  {
    //    twitterObj.getLastWebResponse( replyMsg );
    dc_sprintf(buf, "\ntwitterClient:: twitCurl::statusUpdate web response:\n%s\n", replyMsg.c_str());
    dc_->logentry(buf, 100, DC::LogChannel::LOG_GOD);
  }
  else
  {
    twitterObj.getLastCurlError(replyMsg);
    dc_sprintf(buf, "\ntwitterClient:: twitCurl::statusUpdate error:\n%s\n", replyMsg.c_str());
    dc_->logentry(buf, 100, DC::LogChannel::LOG_GOD);
  }
#endif

  return ReturnValue::eSUCCESS;
}
