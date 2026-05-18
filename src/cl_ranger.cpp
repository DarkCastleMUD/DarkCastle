/******************************************************************************
 * $Id: cl_ranger.cpp,v 1.95 2015/05/30 04:48:42 pirahna Exp $ | cl_ranger.C  *
 * Description: Ranger skills/spells                                          *
 *                                                                            *
 * Revision History                                                           *
 * 10/28/2003  Onager   Modified do_tame() to remove hate flag for tamer      *
 * 12/08/2003  Onager   Added eq check to do_tame() to remove !charmie eq     *
 *                      from charmies                                         *
 ******************************************************************************/

#include "DC/DC.h"

qint32 charm_space(qint32 level)
{
  if (level >= 50)
    return 10;
  if (level >= 41)
    return 7;
  if (level >= 31)
    return 5;
  if (level >= 21)
    return 3;
  if (level >= 11)
    return 2;
  return 1;
}

qint32 charm_levels(CharacterPtr ch)
{
  qint32 i = ch->getLevel() / 5;
  qint32 z = 3;
  for (auto &f : ch->followers)
    if (IS_AFFECTED(f, AFF_CHARM))
    {
      z--;
      i -= charm_space(f->getLevel());
    }
  if (z <= 0)
    return -1;
  return i;
}

ReturnValues do_free_animal(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr victim = {};
  QString buf;

  if (!ch->has_skill(SKILL_FREE_ANIMAL))
  {
    ch->sendln(u"Try learning HOW to free an animal first."_s);
    return ReturnValue::eFAILURE;
  }

  arg = one_argument(arg, buf);

  for (auto &k : ch->followers)
    if (k->isNonPlayer() && ISSET(k->affected_by, AFF_CHARM) && isexact(buf, qPrintable(k->name())))
    {
      victim = k;
      break;
    }

  if (!victim)
  {
    ch->sendln(u"They aren't even here!"_s);
    return ReturnValue::eSUCCESS;
  }

  if (!charge_moves(ch, SKILL_FREE_ANIMAL))
    return ReturnValue::eSUCCESS;

  if (!skill_success(ch, victim, SKILL_FREE_ANIMAL))
  {
    act("You give $N a gentle pat to the head, but $E seems unwilling to leave your side.",
        ch, nullptr, victim, TO_CHAR, 0);
    act("$n gives $N a gentle pat to the head, but $E seems unwilling to leave $s side.",
        ch, nullptr, victim, TO_ROOM, INVIS_NULL);
    return ReturnValue::eSUCCESS;
  }

  act("With a gentle pat to the head, you set $N free to roam the wilds again.",
      ch, nullptr, victim, TO_CHAR, 0);
  act("With a gentle pat to the head, $n sets $N free to roam the wilds again.",
      ch, nullptr, victim, TO_ROOM, INVIS_NULL);

  stop_follower(victim);

  return ReturnValue::eSUCCESS;
}

ReturnValues do_tame(CharacterPtr ch, QString arg, cmd_t cmd)
{
  affected_type af;
  CharacterPtr victim;
  QString buf;

  arg = arg.trimmed();

  if (arg.isEmpty())
  {
    ch->sendln(u"Who do you want to tame?"_s);
    return ReturnValue::eFAILURE;
  }

  if (!ch->canPerform(SKILL_TAME, "Try learning HOW to tame first.\r\n"))
  {
    return ReturnValue::eFAILURE;
  }

  one_argument(arg, buf);

  if (!(victim = ch->get_char_room_vis(buf)))
  {
    ch->sendln(u"No one here by that name!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim == ch)
  {
    ch->sendln(u"Tame the wild beast!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isPlayer())
  {
    ch->sendln(u"You find yourself unable to tame this player."_s);
    return ReturnValue::eFAILURE;
  }

  if (IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) ||
      (ch->getLevel() < victim->getLevel()))
  {
    ch->sendln(u"You find yourself unable to tame this creature."_s);
    return ReturnValue::eFAILURE;
  }

  if (circle_follow(victim, ch))
  {
    ch->sendln(u"Sorry, following in circles can not be allowed."_s);
    return ReturnValue::eFAILURE;
  }

  if (charm_levels(ch) - charm_space(victim->getLevel()) < 0)
  {
    ch->sendln(u"How you plan on controlling so many followers?"_s);
    return ReturnValue::eFAILURE;
    /*   CharacterPtr  vict = {};
       for(CharacterPtr *k = ch->followers; k; k = k->next)
         if(k->isNonPlayer() &&k->affected_by_spell( SPELL_CHARM_PERSON))
         {
            vict = k;
            break;
         }
         if (vict) {
      if (vict->in_room == ch->in_room && vict->position > position_t::SLEEPING)
        do_say(vict, "Hey... but what about ME!?");
             remove_memory(vict, 'h');
      if (vict->master) {
             stop_follower(vict, cmd_t::BROKE_CHARM);
             vict->add_memory( qPrintable(ch->name()), 'h');
      }
         }*/
  }

  if (!charge_moves(ch, SKILL_TAME))
    return ReturnValue::eSUCCESS;

  act_to_room("$n holds out $s hand to $N and beckons softly.", ch, nullptr, victim, INVIS_NULL);

  WAIT_STATE(ch, DC::PULSE_VIOLENCE * 1);

  if ((isSet(victim->immune, ISR_CHARM)) ||
      !ISSET(victim->mobdata->actflags, ACT_CHARM))
  {
    act_to_character("$N is wilder than you thought.", ch, nullptr, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (!skill_success(ch, victim, SKILL_TAME) || saves_spell(ch, victim, 0, SAVE_TYPE_MAGIC) >= 0)
  {
    act_to_character("$N is unreceptive to your attempts to tame $M.", ch, nullptr, victim, 0);
    return ReturnValue::eFAILURE;
  }

  if (victim->master)
    stop_follower(victim);

  /* make charmie stop hating tamer */
  remove_memory(victim, 'h', ch);

  add_follower(victim, ch);

  af.type = SPELL_CHARM_PERSON;

  if (GET_INT(victim))
    af.duration = 24 * 18 / GET_INT(victim);
  else
    af.duration = 24 * 18;

  af.modifier = {};
  af.location = {};
  af.bitvector = AFF_CHARM;
  affect_to_char(victim, &af);

  /* remove any !charmie eq the charmie is wearing */
  check_eq(victim);

  act_to_victim("Isn't $n just such a nice person?", ch, 0, victim, 0);
  return ReturnValue::eSUCCESS;
}

qint32 pick_one(qint32 a, qint32 b)
{
  return dc_->number(1, 100) <= 50 ? a : b;
}

qint32 pick_one(qint32 a, qint32 b, qint32 c)
{
  qint32 x = dc_->number(1, 100);

  if (x > 66)
    return a;
  else if (x > 33)
    return b;
  else
    return c;
}

qint32 pick_one(qint32 a, qint32 b, qint32 c, qint32 d)
{
  qint32 x = dc_->number(1, 100);

  if (x > 75)
    return a;
  else if (x > 50)
    return b;
  else if (x > 25)
    return c;
  else
    return d;
}

class forage_lookup
{
public:
  qint32 ovnum;
  qint32 rate[4];
};

forage_lookup forage_lookup_table[SECT_MAX_SECT + 1][6] = {
    // SECT_INSIDE
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_CITY
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_FIELD
    {{6306, {8, 9, 10, 11}},
     {6310, {3, 4, 5, 6}},
     {3171, {36, 34, 32, 30}},
     {3173, {7, 8, 9, 10}},
     {3174, {10, 11, 12, 13}},
     {3176, {36, 34, 32, 30}}},

    // SECT_FOREST
    {{6308, {8, 9, 10, 11}},
     {6311, {3, 4, 5, 6}},
     {3167, {36, 34, 32, 30}},
     {3175, {36, 34, 32, 30}},
     {3185, {10, 11, 12, 13}},
     {27800, {7, 8, 9, 10}}},

    // SECT_HILLS
    {{6306, {8, 9, 10, 11}},
     {6310, {3, 4, 5, 6}},
     {3171, {36, 34, 32, 30}},
     {3173, {7, 8, 9, 10}},
     {3174, {10, 11, 12, 13}},
     {3176, {36, 34, 32, 30}}},

    // SECT_MOUNTAIN
    {{6305, {3, 4, 5, 6}},
     {3177, {36, 34, 32, 30}},
     {3178, {8, 9, 10, 11}},
     {3179, {7, 8, 9, 10}},
     {3184, {36, 34, 32, 30}},
     {4, {10, 11, 12, 13}}},

    // SECT_WATER_SWIM
    {{6303, {8, 9, 10, 11}},
     {3160, {36, 34, 32, 30}},
     {3162, {7, 8, 9, 10}},
     {3163, {10, 11, 12, 13}},
     {3164, {3, 4, 5, 6}},
     {3189, {36, 34, 32, 30}}},

    // SECT_WATER_NOSWIM
    {{6303, {8, 9, 10, 11}},
     {3160, {36, 34, 32, 30}},
     {3162, {7, 8, 9, 10}},
     {3163, {10, 11, 12, 13}},
     {3164, {3, 4, 5, 6}},
     {3189, {36, 34, 32, 30}}},

    // SECT_BEACH
    {{6302, {8, 9, 10, 11}},
     {6309, {3, 4, 5, 6}},
     {6304, {7, 8, 9, 10}},
     {3161, {36, 34, 32, 30}},
     {3169, {36, 34, 32, 30}},
     {3187, {10, 11, 12, 13}}},

    // SECT_PAVED_ROAD
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_DESERT
    {{6307, {3, 4, 5, 6}},
     {3165, {36, 34, 32, 30}},
     {3166, {36, 34, 32, 30}},
     {3172, {8, 9, 10, 11}},
     {3186, {10, 11, 12, 13}},
     {1570, {7, 8, 9, 10}}},

    // SECT_UNDERWATER
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_SWAMP
    {{6301, {8, 9, 10, 11}},
     {6312, {3, 4, 5, 6}},
     {3170, {36, 34, 32, 30}},
     {3180, {7, 8, 9, 10}},
     {3181, {10, 11, 12, 13}},
     {7717, {36, 34, 32, 30}}},

    // SECT_AIR
    {{0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}},
     {0, {0, 0, 0, 0}}},

    // SECT_FROZEN_TUNDRA
    {{28314, {7, 8, 9, 10}},
     {44, {10, 11, 12, 13}},
     {5219, {36, 34, 32, 30}},
     {3168, {8, 9, 10, 11}},
     {28313, {3, 4, 5, 6}},
     {28301, {36, 34, 32, 30}}},

    // SECT_ARCTIC
    {{28314, {7, 8, 9, 10}},
     {44, {10, 11, 12, 13}},
     {5219, {36, 34, 32, 30}},
     {3168, {8, 9, 10, 11}},
     {28313, {3, 4, 5, 6}},
     {28301, {36, 34, 32, 30}}},
};

ReturnValues do_forage(CharacterPtr ch, QString arg, cmd_t cmd)
{
  qint32 learned;
  ObjectPtr new_obj = {};
  affected_type af;

  if (ch->affected_by_spell(SKILL_FORAGE))
  {
    if (ch->dc_->number(0, 1) == 0)
      ch->sendln(u"You already foraged recently.  Give mother nature a break!"_s);
    else
      ch->sendln(u"There's a limit to how often you can play with your nuts.  Give it some time."_s);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_FORAGE))
    return ReturnValue::eSUCCESS;

  learned = ch->has_skill(SKILL_FORAGE);
  if (!learned)
  {
    ch->sendln(u"Not knowing how to forage, you poke at the dirt with a stick, finding nothing."_s);
    return ReturnValue::eFAILURE;
  }

  qint32 pick = dc_->number(1, 100);
  qint32 ovnum;
  qint32 lgroup = {};

  if (learned >= 1 && learned <= 30)
  {
    lgroup = {};
  }
  else if (learned >= 31 && learned <= 60)
  {
    lgroup = 1;
  }
  else if (learned >= 61 && learned <= 90)
  {
    lgroup = 2;
  }
  else if (learned >= 91)
  {
    lgroup = 3;
  }
  qint32 cur_sector = dc_->world[ch->in_room].sector_type;

  // If in a clan or safe room, set sector to inside so we fail forage
  if (isSet(dc_->world[ch->in_room]->room_flags_, CLAN_ROOM) ||
      isSet(dc_->world[ch->in_room]->room_flags_, SAFE) ||
      isSet(dc_->world[ch->in_room]->room_flags_, INDOORS))
  {
    cur_sector = SECT_INSIDE;
  }

  qint32 last = {};

  for (qint32 i = {}; i < 6; i++)
  {
    if (pick > last && pick <= last + forage_lookup_table[cur_sector][i].rate[lgroup])
    {
      ovnum = forage_lookup_table[cur_sector][i].ovnum;

      if ((1 + IS_CARRYING_N(ch)) > CAN_CARRY_N(ch))
      {
        ch->sendln(u"You can't carry that many items!"_s);
        return ReturnValue::eFAILURE;
      }

      new_obj = clone_object(real_object(ovnum));
      break;
    }
    last = last + forage_lookup_table[cur_sector][i].rate[lgroup];
  }

  qint32 recharge;
  if (new_obj)
    recharge = 10 - (learned / 20);
  else
    recharge = 6;

  af.type = SKILL_FORAGE;
  af.duration = recharge;
  af.modifier = {};
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af, DC::PULSE_VIOLENCE);

  ch->skill_increase_check(SKILL_FORAGE, learned, SKILL_INCREASE_HARD);
  if (!new_obj)
  {
    act_to_room("$n forages around for some food, but turns up nothing.", ch, 0, 0, 0);
    act_to_character("You forage around for some food, but turn up nothing.", ch, 0, 0, 0);
    return ReturnValue::eFAILURE;
  }

  act_to_room("$n forages around, turning up $p.", ch, new_obj, 0, 0);
  act_to_character("You forage around, turning up $p.", ch, new_obj, 0, 0);
  obj_to_char(new_obj, ch);
  new_obj->flags_.timer = 4;
  return ReturnValue::eSUCCESS;
}

/* this is sent the arrow QString, and return the arrow type.
   also, checks level to make sure character is high enough
   return 0 for failure */

qint32 parse_arrow(CharacterPtr ch, QString arrow)
{

  if (GET_CLASS(ch) != CLASS_RANGER && ch->getLevel() < 100)
    return 0;

  while (*arrow == ' ')
    arrow++;

  if ((arrow[0] == 'f') && ch->has_skill(SKILL_FIRE_ARROW))
    return 1;
  else if ((arrow[0] == 'i') && ch->has_skill(SKILL_ICE_ARROW))
    return 2;
  else if ((arrow[0] == 't') && ch->has_skill(SKILL_TEMPEST_ARROW))
    return 3;
  else if ((arrow[0] == 'g') && ch->has_skill(SKILL_GRANITE_ARROW))
    return 4;

  return 0;
}

/* go through and find an arrow */
/* return {} if failure
 *  return pointer if success */
ObjectPtr find_arrow(ObjectPtr quiver)
{
  ObjectPtr target;

  ObjectPtr get_obj_in_list(QString, ObjectPtr);

  target = get_obj_in_list(u"arrow"_s, quiver->contains);

  if (!target)
    return {};

  if (!(target->flags_.type_flag == ITEM_MISSILE))
    target = {};

  return target;
}

void do_arrow_miss(CharacterPtr ch, CharacterPtr victim, qint32 dir, ObjectPtr found)
{
  QString buf;

  switch (ch->dc_->number(1, 6))
  {
  case 1:
    ch->sendln(u"You miss your shot."_s);
    break;
  case 2:
    ch->sendln(u"Your arrow wizzes by the target harmlessely."_s);
    break;
  case 3:
    ch->sendln(u"Your pitiful aim spears a poor woodland creature instead.."_s);
    break;
  case 4:
    ch->sendln(u"Your shot misses your target, and goes skittering across the ground."_s);
    break;
  case 5:
    ch->sendln(u"A slight breeze forces your arrow off the mark."_s);
    break;
  case 6:
    ch->sendln(u"Your shot narrowly misses the mark."_s);
    break;
  }

  switch (ch->dc_->number(1, 3))
  {
  case 1:
    if (dir < 0)
    {
      dc_sprintf(buf, "%s wizzes by.\r\n", qPrintable(found->short_description()));
      victim->send(buf);
      dc_sprintf(buf, "%s wizzes by.", qPrintable(found->short_description()));
      act_to_room(buf, victim, nullptr, ch, NOTVICT);
    }
    else
    {
      dc_sprintf(buf, "%s wizzes by from the %s.\r\n", qPrintable(found->short_description()), dirs[rev_dir[dir]]);
      victim->send(buf);
      dc_sprintf(buf, "%s wizzes by from the %s.", qPrintable(found->short_description()), dirs[rev_dir[dir]]);
      act_to_room(buf, victim, nullptr, 0, 0);
    }
    break;
  case 2:
    dc_sprintf(buf, "A quiet whistle sounds as %s flies over your head.", qPrintable(found->short_description()));
    act_to_character(buf, victim, 0, 0, 0);
    dc_sprintf(buf, "A quiet whistle sounds as %s flies over your head.", qPrintable(found->short_description()));
    act_to_room(buf, victim, 0, ch, NOTVICT);
    break;
  case 3:
    if (dir < 0)
    {
      dc_sprintf(buf, "%s narrowly misses your head.\r\n", qPrintable(found->short_description()));
      victim->send(buf);
      dc_sprintf(buf, "%s narrowly misses $n.", qPrintable(found->short_description()));
      act_to_room(buf, victim, nullptr, ch, NOTVICT);
    }
    else
    {
      dc_sprintf(buf, "%s from the %s narrowly misses your head.\r\n", qPrintable(found->short_description()), dirs[rev_dir[dir]]);
      victim->send(buf);
      dc_sprintf(buf, "%s from the %s narrowly misses $n.", qPrintable(found->short_description()), dirs[rev_dir[dir]]);
      act_to_room(buf, victim, nullptr, 0, 0);
    }
    break;
  }
}

qint32 mob_arrow_response(CharacterPtr ch, CharacterPtr victim,
                          qint32 dir)
{
  qint32 dir2 = {};
  ReturnValues retval;

  /* this will make IS_STUPID mobs alot easier to kill with arrows,
     but then again, they _are_ 'stupid'.  Keeps people from tracking
     the waterwheel around shire though.
  */

  if (ISSET(victim->mobdata->actflags, ACT_STUPID))
  {
    if (!number(0, 20))
      do_shout(victim, u"Duh George, someone keeps shooting me!"_s);
    return ReturnValue::eSUCCESS;
  }

  /* make mob hate the person, but _not_ track them, this should
   * help make it harder for people to abuse this to RK people.
   * Not impossible, but harder
   */

  victim->add_memory(qPrintable(ch->name()), 'h');

  /* don't want the mob leaving a fight its already in */
  if (victim->fighting)
    return ReturnValue::eSUCCESS;

  if (dir < 0) // in the same room
    return ReturnValue::eSUCCESS;

  if (ch->dc_->number(0, 1))
  {
    /* Send the mob in a random dir */
    if (ch->dc_->number(1, 2) == 1)
    {
      do_say(victim, "Where are these fricken arrows coming from?!");
      dir2 = dc_->number(0, 5);
      if (CAN_GO(victim, dir2))
        if (EXIT(victim, dir2))
        {
          if (!(ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN) &&
                dc_->zones_.value(dc_->world[EXIT(victim, dir2)->to_room].zone).isTown()) &&
              !isSet(dc_->world[EXIT(victim, dir2)->to_room]->room_flags_, NO_TRACK))
          /* send 1-6 since attempt move --cmd's it */
          {
            auto cmd_dir = getCommandFromDirection(dir2);
            if (cmd_dir)
              return attempt_move(victim, *cmd_dir, 0);
          }
        }
    }
  }
  else
  {
    /* Send the mob after the fucker! */
    if (ch->dc_->number(1, 2) == 1)
    {
      do_say(victim, "There he is!");
    }
    dir2 = rev_dir[dir];
    for (qint32 i = {}; i < 4; i++)
      if (!CAN_GO(ch, dir2))
        dir2 = dc_->number(0, 5);
    if (EXIT(victim, dir2))
    {
      if (CAN_GO(ch, dir2))
        if (!(ISSET(victim->mobdata->actflags, ACT_STAY_NO_TOWN) &&
              dc_->zones_.value(dc_->world[EXIT(victim, dir2)->to_room].zone).isTown()))
          if (!isSet(dc_->world[EXIT(victim, dir2)->to_room]->room_flags_, NO_TRACK))
          {
            /* dir+1 it since attempt_move will --cmd it */
            auto cmd_dir = getCommandFromDirection(dir2);
            if (cmd_dir)
            {
              retval = attempt_move(victim, *cmd_dir, 0);
              if (SOMEONE_DIED(retval))
                return retval;
            }

            if (isSet(retval, ReturnValue::eFAILURE)) // can't go after the archer
              return do_flee(victim, u""_s);
          }
    }
    if (ch->dc_->number(1, 5) == 1)
    {
      if (ch->dc_->number(0, 1))
      {
        do_shout(victim, "Where the fuck are these arrows coming from?!");
      }
      else
      {
        do_shout(victim, "Quit shooting me dammit!");
      }
    }
  }
  return ReturnValue::eSUCCESS;
}

/* no need anymore
ReturnValues do_arrow_damage(CharacterPtr ch, CharacterPtr victim,
                     qint32 dir, qint32 dam, qint32 artype,
                     ObjectPtr found)
{
  QString buf;
  ReturnValues retval;

  void inform_victim(CharacterPtr , CharacterPtr , qint32);

  buf[199] = '\0'; // just cause I'm paranoid

  set_cantquit(ch, victim);

  if(0 > (victim->getHP() - dam))
  { // they aren't going to survive..life sucks
   switch(ch->dc_->number(1,2))
   {
    case 1:
    ch->sendln(u"Your shot impales %1 through the heart!"_s.arg(qPrintable(victim->shortdesc_or_name())));
    dc_sprintf(buf, "%s from the %s drives full force into your chest!\r\n",
       qPrintable(found->short_description()), dirs[rev_dir[dir]]);
    victim->send(buf);
    dc_sprintf(buf, "%s from the %s impales $n through the chest!",
       qPrintable(found->short_description()), dirs[rev_dir[dir]]);
    act_to_room(buf, victim, nullptr, 0,  0);
    break;

    case 2:
    dc_sprintf(buf, "Your %s drives through the eye of %s ending their life.\r\n",
       qPrintable(found->short_description()), qPrintable(victim->shortdesc_or_name()));
    ch->send(buf);
    dc_sprintf(buf, "%s drives right through your left eye!\r\nThe last thing through your mind is.........an arrowhead.\r\n",
       qPrintable(found->short_description()));
    victim->send(buf);
    dc_sprintf(buf, "%s from the %s lands with a solid 'thunk.'\r\n$n falls to the ground, an arrow sticking from $s left eye.",
       qPrintable(found->short_description()), dirs[rev_dir[dir]]);
    act_to_room(buf, victim, nullptr, 0,  0);
    break;
   } // of switch
  }
  else  // they have enough to survive the arrow..lucky bastard
  {
    dc_sprintf(buf, "You hit %s with %s!\r\n", qPrintable(victim->shortdesc_or_name()),
       qPrintable(found->short_description()));
    ch->send(buf);
    dc_sprintf(buf, "You get shot with %s from the %s.  Ouch.",
          qPrintable(found->short_description()), dirs[rev_dir[dir]]);
    act_to_character(buf, victim, 0, 0,  0);
    dc_sprintf(buf, "%s from the %s hits $n!",
          qPrintable(found->short_description()), dirs[rev_dir[dir]]);
    act_to_room(buf, victim, 0, 0,  0);

    switch(artype)
    {
      case 1: send_to_room("It was flaming hot!\r\n", victim->in_room);
              break;
      case 2: send_to_room("It was icy cold!\r\n", victim->in_room);
              break;
      default: break;
    }
    if(victim->isNonPlayer())
      retval = mob_arrow_response(ch, victim, dir);
      if(SOMEONE_DIED(retval)) // mob died somehow while moving
         return retval;
  }

  victim->removeHP(dam);
  update_pos(victim);
  inform_victim(ch, victim, dam);

  if(victim->getHP() > 0)
     victim->setStanding();

// This is a cut & paste from fight.C cause I can't think of a better way to do it

  // Payoff for killing things.
  if(victim->getHP() < 0)
  {
    group_gain(ch, victim);
    fight_kill(ch, victim, TYPE_CHOOSE, 0);
    return (ReturnValue::eSUCCESS | ReturnValue::eVICT_DIED);
  } // End of Hit < 0

  return ReturnValue::eSUCCESS;
}
*/

ReturnValues do_fire(CharacterPtr ch, QString arg, cmd_t cmd)
{
  CharacterPtr victim;
  qint32 dam, dir = -1, artype, cost, retval, victroom;
  ObjectPtr found;
  quint32 cur_room, new_room = {};
  QString direct, arrow,
      target, buf,
      buf2, victname,
      victhshr;
  bool enchantmentused = false;

  victim = {};
  *direct = '\0';
  *arrow = '\0';

  if (!ch->canPerform(SKILL_ARCHERY))
  {
    ch->sendln(u"You've no idea how those pointy things with strings and feathers work."_s);
    return ReturnValue::eFAILURE;
  }

  if (!ch->equipment[WEAR_HOLD])
  {
    ch->sendln(u"You need to be holding a bow, moron."_s);
    return ReturnValue::eFAILURE;
  }

  if (!(ch->equipment[WEAR_HOLD]->flags_.type_flag == ITEM_FIREWEAPON))
  {
    ch->sendln(u"You need to be holding a bow, moron."_s);
    return ReturnValue::eFAILURE;
  }
  /*
   if(GET_POS(ch) == position_t::FIGHTING)
   {
   ch->sendln(u"Aren't you a bit busy with hand to hand combat?"_s);
   return ReturnValue::eFAILURE;
   }*/

  if (ch->shotsthisround > 0)
  {
    ch->sendln(u"Slow down there tiger, you can't fire them that fast!"_s);
    return ReturnValue::eFAILURE;
  }

  /*  Command syntax is: fire [target] <dir> [arrowtype]
   if there is !dir, then check skill level
   if there is !arrow, then choose standard arrowtype */

  while (*arg == ' ')
    arg++;

  if (arg.isEmpty())
  {
    ch->sendln(u"Shoot at whom?"_s);
    return ReturnValue::eFAILURE;
  }
  half_chop(arg, target, buf2);
  half_chop(buf2, direct, arrow);

  direct[MAX_STRING_LENGTH - 1] = '\0';
  arrow[MAX_STRING_LENGTH - 1] = '\0';
  target[MAX_STRING_LENGTH - 1] = '\0';

  /* make safe rooms checks */
  if (isSet(dc_->world[ch->in_room]->room_flags_, SAFE))
  {
    ch->sendln(u"You can't shoot arrows if yer in a safe room silly."_s);
    return ReturnValue::eFAILURE;
  }

  cost = {};
  dir = -1;
  artype = {};

  if (!arrow.isEmpty())
  {
    artype = parse_arrow(ch, arrow);
    if (!artype)
    {
      ch->sendln(u"You don't know of that type of arrow."_s);
      return ReturnValue::eFAILURE;
    }
    switch (artype)
    {
    case 1:
      cost = 30;
      break;
    case 2:
      cost = 20;
      break;
    case 3:
      cost = 10;
      break;
    case 4:
      cost = 40;
      break;
    }
  }
  if (!direct.isEmpty())
  {
    if (direct[0] == 'n')
      dir = {};
    else if (direct[0] == 'e')
      dir = 1;
    else if (direct[0] == 's')
      dir = 2;
    else if (direct[0] == 'w')
      dir = 3;
    else if (direct[0] == 'u')
      dir = 4;
    else if (direct[0] == 'd')
      dir = 5;
    else if (direct[0] == 'f' || direct[0] == 't' || direct[0] == 'g' || direct[0] == 'i')
      artype = parse_arrow(ch, direct);
    else
    {
      ch->sendln(u"Shoot in which direction?"_s);
      return ReturnValue::eFAILURE;
    }

    if (dir >= 0 && !CAN_GO(ch, dir))
    {
      ch->sendln(u"There is nothing to shoot in that direction."_s);
      return ReturnValue::eFAILURE;
    }
    else if (artype)
    {
      switch (artype)
      {
      case 1:
        cost = 30;
        break;
      case 2:
        cost = 20;
        break;
      case 3:
        cost = 10;
        break;
      case 4:
        cost = 40;
        break;
      }
    }
  }

  if ((GET_MANA(ch) < cost) && (ch->getLevel() < ANGEL))
  {
    ch->sendln(u"You don't have enough mana for that arrow."_s);
    return ReturnValue::eFAILURE;
  }

  if (!charge_moves(ch, SKILL_ARCHERY))
    return ReturnValue::eSUCCESS;

  /* check if you can see your target */
  /* put ch in the targets room to check if they are visible */
  cur_room = ch->in_room;

  if (target[0])
  {
    if (!ch->fighting)
    {
      if (dir >= 0)
      {
        if (dc_->world[cur_room].dir_option[dir] && !(dc_->world[cur_room].dir_option[dir]->to_room == INVALID_ROOM) && !isSet(dc_->world[cur_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = dc_->world[cur_room].dir_option[dir]->to_room;
          if (isSet(dc_->world[new_room]->room_flags_, SAFE))
          {
            ch->sendln(u"Don't shoot into a safe room!  You might hit someone!"_s);
            return ReturnValue::eFAILURE;
          }
          char_from_room(ch, false);
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            ch->sendln(u"Error moving you to room in do_fire"_s);
            return ReturnValue::eFAILURE;
          }
          victim = ch->get_char_room_vis(target);
        }
      }

      if (!victim && new_room && artype == 3 && dir >= 0)
      {
        if (dc_->world[new_room].dir_option[dir] && !(dc_->world[new_room].dir_option[dir]->to_room == INVALID_ROOM) && !isSet(dc_->world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = dc_->world[new_room].dir_option[dir]->to_room;
          char_from_room(ch, false);
          if (isSet(dc_->world[new_room]->room_flags_, SAFE))
          {
            ch->sendln(u"Don't shoot into a safe room!  You might hit someone!"_s);
            char_to_room(ch, cur_room);
            return ReturnValue::eFAILURE;
          }
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            ch->sendln(u"Error moving you to room in do_fire"_s);
            return ReturnValue::eFAILURE;
          }
          victim = ch->get_char_room_vis(target);
        }
      }

      if (!victim && new_room && artype == 3 && ch->affected_by_spell(SPELL_FARSIGHT) && dir >= 0)
      {
        if (dc_->world[new_room].dir_option[dir] && !(dc_->world[new_room].dir_option[dir]->to_room == INVALID_ROOM) && !isSet(dc_->world[new_room].dir_option[dir]->exit_info, EX_CLOSED))
        {
          new_room = dc_->world[new_room].dir_option[dir]->to_room;
          char_from_room(ch, false);
          if (isSet(dc_->world[new_room]->room_flags_, SAFE))
          {
            ch->sendln(u"Don't shoot into a safe room!  You might hit someone!"_s);
            char_to_room(ch, cur_room);
            return ReturnValue::eFAILURE;
          }
          if (!char_to_room(ch, new_room))
          {
            /* put ch into a room before we exit */
            char_to_room(ch, cur_room);
            ch->sendln(u"Error moving you to room in do_fire"_s);
            return ReturnValue::eFAILURE;
          }
          victim = ch->get_char_room_vis(target);
        }
      }
      /* put chararacter back */
      char_from_room(ch, false);
      char_to_room(ch, cur_room);
    }
    if (!victim && ch->has_skill(SKILL_ARCHERY) > 80)
    {
      if ((victim = ch->get_char_room_vis(target)))
        dir = -1;
    }
    else if (dir < 0)
    {
      ch->sendln(u"You aren't skilled enough to fire at a target this close."_s);
      return ReturnValue::eFAILURE;
    }
    if (!victim)
    {
      if (dir >= 0)
        ch->sendln(u"You cannot concentrate enough to fire into an adjacent room while fighting."_s);
      else
        ch->sendln(u"You cannot seem to locate your target."_s);
      return ReturnValue::eFAILURE;
    }
  }
  else
  {
    ch->sendln(u"Sorry, you must specify a target."_s);
    return ReturnValue::eFAILURE;
  }

  /* check for accidental targeting of self */
  if (victim == ch)
  {
    ch->sendln(u"You need to type more of the target's name."_s);
    return ReturnValue::eFAILURE;
  }

  /* Protect the newbies! */
  if (victim->isPlayer() && victim->getLevel() < 6)
  {
    ch->sendln(u"Don't shoot at a poor defenseless n00b! :("_s);
    return ReturnValue::eFAILURE;
  }
  /* check if target is fighting */
  /*  if(victim->fighting)
   {
   ch->sendln(u"You can't seem to get a clear line of sight."_s);
   return ReturnValue::eFAILURE;
   }*/

  /* check for arrows here */

  found = {};
  qint32 where = {};
  for (; where < MAX_WEAR; where++)
  {
    if (ch->equipment[where])
      if (IS_CONTAINER(ch->equipment[where]) && isexact("quiver", ch->equipment[where]->name()))
      {
        found = find_arrow(ch->equipment[where]);
        if (found)
        {
          get(ch, found, ch->equipment[where], 0, cmd_t::DEFAULT);
        }
        break;
      }
  }

  if (!found)
  {
    ch->sendln(u"You aren't wearing any quivers with arrows!"_s);
    return ReturnValue::eFAILURE;
  }

  if (victim->isNonPlayer() && dc_->mob_index_[victim->mobdata->nr]->vnum() >= 2300 && dc_->mob_index_[victim->mobdata->nr]->vnum() <= 2399)
  {
    ch->sendln(u"Your arrow is disintegrated by the fortress' enchantments."_s);
    extract_obj(found);
    return ReturnValue::eFAILURE;
  }

  /* go ahead and shoot */

  switch (ch->dc_->number(1, 2))
  {
  case 1:
    if (dir >= 0)
      dc_sprintf(buf, "$n fires an arrow %sward.", dirs[dir]);
    else
      dc_sprintf(buf, "$n fires an arrow.");
    act_to_room(buf, ch, 0, 0, 0);
    break;
  case 2:
    if (dir >= 0)
      dc_sprintf(buf, "$n lets off an arrow to the %s.", dirs[dir]);
    else
      dc_sprintf(buf, "$n lets off an arrow.");
    act_to_room(buf, ch, 0, 0, 0);
    break;
  }

  GET_MANA(ch) -= cost;

  if (!skill_success(ch, victim, SKILL_ARCHERY))
  {
    retval = ReturnValue::eSUCCESS;
    do_arrow_miss(ch, victim, dir, found);
  }
  else
  {
    dam = dice(found->flags_.value[1], found->flags_.value[2]);
    dam += dice(ch->equipment[WEAR_HOLD]->flags_.value[1],
                ch->equipment[WEAR_HOLD]->flags_.value[2]);
    for (qint32 i = {}; i < found->num_affects; i++)
      if (found->affected[i].location == APPLY_DAMROLL && found->affected[i].modifier != 0)
        dam += found->affected[i].modifier;
      else if (found->affected[i].location == APPLY_HIT_N_DAM && found->affected[i].modifier != 0)
        dam += found->affected[i].modifier;

    set_cantquit(ch, victim);
    dc_sprintf(victname, "%s", qPrintable(victim->shortdesc_or_name()));
    victroom = victim->in_room;
    dc_strcpy(victhshr, HSHR(victim));

    if (dir >= 0)
      send_to_room(
          "An arrow flies into the room with incredible speed!\r\n",
          victroom);

    retval = damage(ch, victim, dam, TYPE_PIERCE, SKILL_ARCHERY);

    if (isSet(retval, ReturnValue::eVICT_DIED))
    {
      switch (ch->dc_->number(1, 3))
      {
      case 1:
        if (dir < 0)
        {
          dc_sprintf(buf, "The %s impales %s through the heart!\r\n", qPrintable(found->short_description()), victname);
          send_to_room(buf, victroom);
        }
        else
        {
          ch->sendln(u"Your shot impales %1 through the heart!"_s.arg(victname));
          dc_sprintf(buf, "%s from the %s impales %s through the chest!\r\n", qPrintable(found->short_description()), dirs[rev_dir[dir]], victname);
          send_to_room(buf, victroom);
        }
        break;
      case 2:
        if (dir < 0)
        {
          dc_sprintf(buf, "%s drives through the eye of %s, ending %s life.\r\n", qPrintable(found->short_description()), victname, victhshr);
          send_to_room(buf, victroom);
        }
        else
        {
          dc_sprintf(buf, "Your %s drives through the eye of %s ending %s life.\r\n", qPrintable(found->short_description()), victname, victhshr);
          ch->send(buf);
          dc_sprintf(buf, "%s from the %s lands with a solid 'thunk.'\r\n%s falls to the ground, an arrow sticking from %s left eye.\r\n", qPrintable(found->short_description()), dirs[rev_dir[dir]], victname, victhshr);
          send_to_room(buf, victroom);
        }
        break;
      case 3:
        if (dir < 0)
        {
          dc_sprintf(buf, "The %s rips through %s's throat.  Blood spouts as %s expires with a final gurgle.\r\n", qPrintable(found->short_description()), victname, HSSH(victim));
          send_to_room(buf, victroom);
        }
        else
        {
          dc_sprintf(buf, "Your shot rips through the throat of %s ending their life with a gurgle.\r\n", victname);
          ch->send(buf);
          dc_sprintf(buf, "%s from the %s ripes through the throat of %s!  Blood spouts as %s expires with a final gurgle.\r\n", qPrintable(found->short_description()), dirs[rev_dir[dir]], victname, HSSH(victim));
          send_to_room(buf, victroom);
        }
        break;
      }
    }
    else
    {
      if (dir < 0)
      {
        dc_sprintf(buf, "You get shot with %s.  Ouch.",
                   qPrintable(found->short_description()));
        act_to_character(buf, victim, 0, 0, 0);
        dc_sprintf(buf, "%s hits $n!", qPrintable(found->short_description()));
        act_to_room(buf, victim, 0, ch, NOTVICT);
        dc_sprintf(buf, "You hit %s with %s!\r\n", qPrintable(victim->shortdesc_or_name()),
                   qPrintable(found->short_description()));
        ch->send(buf);
      }
      else
      {
        dc_sprintf(buf, "You hit %s with %s!\r\n", qPrintable(victim->shortdesc_or_name()),
                   qPrintable(found->short_description()));
        ch->send(buf);
        dc_sprintf(buf, "You get shot with %s from the %s.  Ouch.",
                   qPrintable(found->short_description()), dirs[rev_dir[dir]]);
        act_to_character(buf, victim, 0, 0, 0);
        dc_sprintf(buf, "%s from the %s hits $n!",
                   qPrintable(found->short_description()), dirs[rev_dir[dir]]);
        act_to_room(buf, victim, 0, 0, 0);
      }
      victim->setStanding();

      if (victim->isNonPlayer())
        retval = mob_arrow_response(ch, victim, dir);
      if (SOMEONE_DIED(retval))
      { // mob died somehow while moving
        extract_obj(found);
        return retval;
      }
    }

    if (!SOMEONE_DIED(retval))
    {
      cur_room = ch->in_room;
      char_from_room(ch, false);
      if (!char_to_room(ch, victim->in_room))
      {
        char_to_room(ch, cur_room);
        ch->sendln(u"Error moving you to room in do_fire."_s);
        return ReturnValue::eFAILURE;
      }
      retval = weapon_spells(ch, victim, ITEM_MISSILE);
      // just in case
      if (isSet(retval, ReturnValue::eCH_DIED))
      {
        ObjectPtr corpse, next;
        for (corpse = dc_->object_list; corpse; corpse = next)
        {
          next = corpse->next;
          if (IS_OBJ_STAT(corpse, ITEM_PC_CORPSE) && isexact(qPrintable(ch->name()), corpse->name()))
          {
            obj_from_room(corpse);
            obj_to_room(corpse, cur_room);
            save_corpses();
            break;
          }
        }
      }
      else
      {
        char_from_room(ch, false);
        char_to_room(ch, cur_room);
      }
    }

    QString buffer;
    if (!SOMEONE_DIED(retval))
    {
      switch (artype)
      {
      case 1:
        dam = 90;
        dc_snprintf(buffer, 100, "%d", dam);
        send_damage(
            "The flames surrounding the arrow burns your wound for | damage!",
            ch, 0, victim, buffer,
            "The flames surrounding the arrow burns your wound!",
            TO_VICT);
        send_damage(
            "The flames surrounding the arrow burns $n's wound for | damage!",
            victim, 0, 0, buffer,
            "The flames surrounding the arrow burns $n's wound!",
            TO_ROOM);
        retval = damage(ch, victim, dam, TYPE_FIRE, SKILL_FIRE_ARROW);
        ch->skill_increase_check(SKILL_FIRE_ARROW, ch->has_skill(SKILL_FIRE_ARROW), get_difficulty(SKILL_FIRE_ARROW));
        enchantmentused = true;
        break;
      case 2:
        dam = 50;
        dc_snprintf(buffer, 100, "%d", dam);
        send_damage("The stray ice shards impale you for | damage!", ch,
                    0, victim, buffer, "The stray ice shards impale you!",
                    TO_VICT);
        send_damage("The stray ice shards impale $n for | damage!",
                    victim, 0, 0, buffer, "The stray ice shards impale $n!",
                    TO_ROOM);
        if (ch->dc_->number(1, 100) < ch->has_skill(SKILL_ICE_ARROW) / 4)
        {
          act("Your body slows down for a second!", ch, 0, victim,
              TO_VICT, 0);
          act("$n's body seems a bit slower!", victim, 0, 0, TO_ROOM, 0);
          WAIT_STATE(victim, DC::PULSE_VIOLENCE);
        }
        retval = damage(ch, victim, dam, TYPE_COLD, SKILL_ICE_ARROW);
        ch->skill_increase_check(SKILL_ICE_ARROW,
                                 ch->has_skill(SKILL_ICE_ARROW),
                                 get_difficulty(SKILL_ICE_ARROW));
        enchantmentused = true;
        break;
      case 3:
        dam = 30;
        dc_snprintf(buffer, 100, "%d", dam);
        send_damage(
            "The storm cloud enveloping the arrow shocks you for | damage!",
            ch, 0, victim, buffer,
            "The storm cloud enveloping the arrow shocks you!",
            TO_VICT);
        send_damage(
            "The storm cloud enveloping the arrow shocks $n for | damage!",
            victim, 0, ch, buffer,
            "The storm cloud enveloping the arrow shocks $n!",
            TO_ROOM);
        retval = damage(ch, victim, dam, TYPE_ENERGY,
                        SKILL_TEMPEST_ARROW, 0);
        ch->skill_increase_check(SKILL_TEMPEST_ARROW,
                                 ch->has_skill(SKILL_TEMPEST_ARROW),
                                 get_difficulty(SKILL_TEMPEST_ARROW));
        enchantmentused = true;
        break;
      case 4:
        dam = 70;
        dc_snprintf(buffer, 100, "%d", dam);
        send_damage(
            "The magical stones surrounding the arrow smack into you, hard for | damage.",
            ch, 0, victim, buffer,
            "The magical stones surrounding the arrow smack into you, hard.",
            TO_VICT);
        send_damage(
            "The magical stones surrounding the arrow smack hard into $n for | damage.",
            victim, 0, 0, buffer,
            "The magical stones surrounding the arrow smack hard into $n.",
            TO_ROOM);
        if (ch->dc_->number(1, 100) < ch->has_skill(SKILL_GRANITE_ARROW) / 4)
        {
          act("The stones knock you flat on your ass!", ch, 0, victim,
              TO_VICT, 0);
          act("The stones knock $n flat on $s ass!", victim, 0, 0,
              TO_ROOM, 0);
          victim->setSitting();
          WAIT_STATE(victim, DC::PULSE_VIOLENCE);
        }
        retval = damage(ch, victim, dam, TYPE_HIT, SKILL_GRANITE_ARROW,
                        0);
        ch->skill_increase_check(SKILL_GRANITE_ARROW,
                                 ch->has_skill(SKILL_GRANITE_ARROW),
                                 get_difficulty(SKILL_GRANITE_ARROW));
        enchantmentused = true;
        break;
      default:
        break;
      }
    }
  }

  extract_obj(found);

  dc_->shooting_list_.insert(ch);

  if (ch->has_skill(SKILL_ARCHERY) < 51 || enchantmentused)
    ch->shotsthisround += DC::PULSE_VIOLENCE;
  else if (ch->has_skill(SKILL_ARCHERY) < 86)
    ch->shotsthisround += DC::PULSE_VIOLENCE / 2;
  else
    ch->shotsthisround += 3;

  return retval;
}

ReturnValues do_mind_delve(CharacterPtr ch, QString arg, cmd_t cmd)
{
  QString buf;
  CharacterPtr target = {};
  //  qint32 learned, specialization;

  if (arg.isEmpty())
  {
    ch->sendln(u"Delve into whom?"_s);
    return ReturnValue::eFAILURE;
  }

  one_argument(arg, arg);

  /*
    TODO - make this into a skill and put it in

    if(ch->isNonPlayer() || ch->getLevel() >= ARCHANGEL)
       learned = 75;
    else if(!(learned = ch->has_skill( SKILL_MIND_DELVE))) {
       ch->sendln(u"You try to think like a chipmunk and go nuts."_s);
       return ReturnValue::eFAILURE;
    }
    specialization = learned / 100;
    learned = learned % 100;

  */

  target = ch->get_char_room_vis(arg);

  if (ch->getLevel() < target->getLevel())
  {
    ch->sendln(u"You can't seem to understand your target's mental processes."_s);
    return ReturnValue::eFAILURE;
  }

  if (!target->isNonPlayer())
  {
    ch->sendln(u"Ewwwww gross!!!  %1 is imagining you naked on all fours!"_s.arg(qPrintable(target->shortdesc_or_name())));
    return ReturnValue::eFAILURE;
  }

  act_to_character("You enter $S mind...", ch, 0, target, INVIS_NULL);
  ch->sendln(u"%1 seems to hate... %2."_s.arg(qPrintable(target->shortdesc_or_name())).arg(ch->mobdata->hated.isEmpty() ? "Noone!" : ch->mobdata->hated));

  if (ch->master)
    dc_sprintf(buf, "%s seems to really like... %s.\r\n", qPrintable(target->shortdesc_or_name()),
               qPrintable(ch->master->shortdesc_or_name()));
  else
    dc_sprintf(buf, "%s seems to really like... %s.\r\n", qPrintable(target->shortdesc_or_name()),
               "Noone!");
  ch->send(buf);
  return ReturnValue::eSUCCESS;
}

void check_eq(CharacterPtr ch)
{
  qint32 pos;

  for (pos = {}; pos < MAX_WEAR; pos++)
  {
    if (ch->equipment[pos])
      ch->equip_char(ch->unequip_char(pos), pos);
  }
}

ReturnValues do_natural_selection(CharacterPtr ch, QString arg, cmd_t cmd)
{
  qint32 i;
  QString buf;
  affected_typePtr af, cur;

  one_argument(arg, buf);

  qint32 learned = ch->has_skill(SKILL_NAT_SELECT);
  if (ch->isNonPlayer() || !learned)
  {
    ch->sendln(u"You don't know how to use this to your advantage."_s);
    return ReturnValue::eFAILURE;
  }

  if (ch->affected_by_spell(SPELL_NAT_SELECT_TIMER))
  {
    ch->sendln(u"You cannot yet select a new enemy of choice."_s);
    return ReturnValue::eFAILURE;
  }

  cur = ch->affected_by_spell(SKILL_NAT_SELECT);

  for (i = 1; i < 33; i++)
  {
    if (is_abbrev(buf, races[i].singular_name))
    {
      if (cur && cur->modifier == i)
      {
        ch->sendln(u"You are already studying this race."_s);
        return ReturnValue::eFAILURE;
      }
      break;
    }
    if (i == 32)
    {
      ch->sendln(u"Please select a valid race."_s);
      return ReturnValue::eFAILURE;
    }
  }

  if (cur)
    affect_remove(ch, cur, SUPPRESS_ALL);

  af.type = SKILL_NAT_SELECT;
  af.duration = -1;
  af.modifier = i;
  af.location = {};
  af.bitvector = -1;
  affect_to_char(ch, &af);

  af.type = SPELL_NAT_SELECT_TIMER;
  af.duration = 60 - learned / 5;
  af.modifier = {};
  af.location = {};
  af.bitvector = -1;
  affect_to_char(ch, &af);

  ch->send(u"You study the habits of the %s race and select them as your enemy of choice.\r\n"_s.arg(races[i].singular_name));

  return ReturnValue::eSUCCESS;
}
