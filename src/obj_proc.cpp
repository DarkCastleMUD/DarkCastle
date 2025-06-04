// File: obj_proc.C
// Usage: This file contains special procedures pertaining to objects, except
// for the boards which are in board.C

#include <vector>
#include <string>
#include <sstream>

#include <cstring> // strstr()

#include <fmt/format.h>

#include "DC/fight.h"
#include "DC/room.h"
#include "DC/DC.h"
#include "DC/connect.h"
#include "DC/timeinfo.h"
#include "DC/utility.h"
#include "DC/character.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/player.h"
#include "DC/sing.h"
#include "DC/interp.h"
#include "DC/magic.h"
#include "DC/act.h"
#include "DC/mobile.h"
#include "DC/spells.h"
#include "DC/returnvals.h"
#include "DC/set.h"
#include "DC/race.h"
#include "DC/const.h"
#include "DC/inventory.h"
#include "DC/guild.h"
#include "DC/wizard.h"

#define EMOTING_FILE "emoting-objects.txt"

extern struct mprog_throw_type *g_mprog_throw_list;

// TODO - go over emoting object stuff and make sure it's as efficient as we can get it

struct obj_emote_data
{
  char *emote_text;
  obj_emote_data *next;
};

struct obj_emote_index
{
  obj_emote_data *data;
  obj_emote_index *next;
  short room_number;
  short emote_index_length;
  short frequency;
};
struct obj_emote_index obj_emote_head = {
    nullptr,
    nullptr,
    -1,
    -1 - 1};

void free_emoting_obj_data(obj_emote_index *myobj)
{
  obj_emote_data *curr_data = nullptr;

  while (myobj->data)
  {
    curr_data = myobj->data;
    myobj->data = curr_data->next;

    dc_free(curr_data->emote_text);
    dc_free(curr_data);
  }
}

void DC::free_emoting_objects_from_memory(void)
{
  obj_emote_index *curr_index = nullptr;

  while (obj_emote_head.next)
  {
    curr_index = obj_emote_head.next;
    obj_emote_head.next = curr_index->next;

    free_emoting_obj_data(curr_index);
    dc_free(curr_index);
  }

  free_emoting_obj_data(&obj_emote_head);
}

void load_emoting_objects()
{
  obj_emote_index *index_cursor = nullptr;
  obj_emote_data *data_cursor = nullptr;
  FILE *fl;
  // short i;
  char fromfile;
  bool done = false,
       done2 = false;
  short offset;

  fl = fopen(EMOTING_FILE, "r");
#ifdef LEAK_CHECK
  obj_emote_head.next = (struct obj_emote_index *)
      calloc(1, sizeof(struct obj_emote_index));
#else
  obj_emote_head.next = (struct obj_emote_index *)
      dc_alloc(1, sizeof(struct obj_emote_index));
#endif
  index_cursor = obj_emote_head.next;
  index_cursor->next = nullptr;
  index_cursor->data = nullptr;
  index_cursor->room_number = DC::NOWHERE;
  index_cursor->emote_index_length = -1;
  index_cursor->frequency = 0;
#ifdef LEAK_CHECK
  data_cursor = (struct obj_emote_data *)
      calloc(1, sizeof(struct obj_emote_index));
#else
  data_cursor = (struct obj_emote_data *)
      dc_alloc(1, sizeof(struct obj_emote_index));
#endif
  index_cursor->data = data_cursor;
  data_cursor->next = nullptr;
  while (!done2)
  {
    index_cursor->room_number = fread_int(fl, 0, 1000000);
    index_cursor->frequency = fread_int(fl, 0, 1000000);
    done = false;
    while (!done)
    {
      // Why are we dc_alloc'ing the space when fread_string is returning us
      // a pointer to the space IT allocs?  Azrack you silly goose.  I fixed it.
      // -pir 05/03/00
      // data_cursor->emote_text = (char *)dc_alloc(100, sizeof(char));
      data_cursor->emote_text = fread_string(fl, 0);
      index_cursor->emote_index_length++;
      if ((offset = 1) && ((fromfile = fgetc(fl)) == 'S') && ((offset = 2) && (fromfile = fgetc(fl)) == '\n'))
      {
        done = true;
      }
      else
      {
#ifdef LEAK_CHECK
        data_cursor->next = (struct obj_emote_data *)
            calloc(1, sizeof(struct obj_emote_data));
#else
        data_cursor->next = (struct obj_emote_data *)
            dc_alloc(1, sizeof(struct obj_emote_data));
#endif
        data_cursor = data_cursor->next;
        data_cursor->next = nullptr;
        // Azrack -- fseek had a -1 * offset * sizeof(char) which is going to send us to EOF immmediately
        // because fseek takes an unsigned int.
        fseek(fl, (-1 * offset * sizeof(char)), SEEK_CUR);
      }
    }
    if ((fromfile = fgetc(fl)) == '$')
    {
      done2 = true;
    }
    else
    {
      fseek(fl, (1 * sizeof(char)), SEEK_CUR);
#ifdef LEAK_CHECK
      index_cursor->next = (struct obj_emote_index *)
          calloc(1, sizeof(struct obj_emote_index));
#else
      index_cursor->next = (struct obj_emote_index *)
          dc_alloc(1, sizeof(struct obj_emote_index));
#endif
      index_cursor = index_cursor->next;
      index_cursor->next = nullptr;
#ifdef LEAK_CHECK
      index_cursor->data = (struct obj_emote_data *)
          calloc(1, sizeof(struct obj_emote_data));
#else
      index_cursor->data = (struct obj_emote_data *)
          dc_alloc(1, sizeof(struct obj_emote_data));
#endif
      index_cursor->room_number = DC::NOWHERE;
      index_cursor->emote_index_length = -1;
      index_cursor->frequency = -1;
      data_cursor = index_cursor->data;
      data_cursor->next = nullptr;
    }
  }
  fclose(fl);
  return;
}

int emoting_object(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  obj_emote_index *index_cursor = nullptr;
  obj_emote_data *data_cursor = nullptr;
  short i = 0;
  if (cmd)
  {
    return eFAILURE;
  }
  if (!obj)
  {
    return eFAILURE;
  }
  if (-1 == obj->in_room)
  {
    return eFAILURE;
  }
  if (!DC::getInstance()->world[obj->in_room].people)
  {
    return eFAILURE;
  }
  ch = DC::getInstance()->world[obj->in_room].people;
  for (index_cursor = &obj_emote_head; index_cursor; index_cursor = index_cursor->next)
  {
    if (real_room(index_cursor->room_number) == obj->in_room)
    {
      if (real_room(index_cursor->room_number) == DC::NOWHERE)
      {
        return eFAILURE;
      }
      i = number<decltype(index_cursor->emote_index_length)>(0, (index_cursor->emote_index_length));
      data_cursor = index_cursor->data;
      for (i = 0; i < number<decltype(index_cursor->emote_index_length)>(0, index_cursor->emote_index_length); i++)
      {
        data_cursor = data_cursor->next;
      }
      if (number<decltype(index_cursor->frequency)>(0, (index_cursor->frequency)) == 0)
      {
        act(data_cursor->emote_text, ch, 0, 0, TO_ROOM, 0);
        act(data_cursor->emote_text, ch, 0, 0, TO_CHAR, 0);
      }
    }
  }
  return eFAILURE;
}

int barbweap(Character *ch, class Object *obj, int cmd, const char *arg,
             Character *invoker)
{ // Cestus
  if (cmd != CMD_PUSH)
    return eFAILURE;
  if (str_cmp(arg, " cestus"))
    return eFAILURE;
  switch (obj->obj_flags.value[3])
  {
  case 4:
  case 5:
    ch->sendln("You twist your wrists quickly causing sharp spikes to spring forth from your weapon!");
    obj->obj_flags.value[3] = 10;
    return eSUCCESS;
  case 10:
    ch->sendln("You twist your wrists quickly, retracting the spikes from your weapon.");
    obj->obj_flags.value[3] = 5;
    return eSUCCESS;
  }
  return eFAILURE;
}
int souldrainer(Character *ch, class Object *obj, int cmd, char *arg,
                Character *invoker)
{
  // class Object *wielded;
  int percent, chance;
  Character *vict;

  if (!(vict = ch->fighting))
  {
    return eFAILURE;
  }
  if (vict->getHP() < 3500)
  {
    percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
    chance = number(0, 101);
    if (chance > (1.3 * percent))
    {
      chance = number(0, 101);
      if (chance > (2 * percent))
      {
        chance = number(0, 101);
        if (chance > (2 * percent))
        {
          chance = number(0, 101);
          if (chance > (2 * percent))
          {
            act("You feel your soul being drained as $n's magics swirl around you",
                ch, 0, vict, TO_VICT, 0);
            act("$N gasps in agony as $n's dark magics grasp at $S soul.",
                ch, 0, vict, TO_ROOM, NOTVICT);
            act("Evil energy surges into you as you wrench at $N's soul.",
                ch, 0, vict, TO_CHAR, 0);
            act("The Darkness has triumphed! You drain away $N's esseence.",
                ch, 0, vict, TO_CHAR, 0);
            act("Everything goes black as your soul is pulled into the abyss.",
                ch, 0, vict, TO_VICT, 0);
            act("$N screams as his soul is destroyed by $n's dark magics",
                ch, 0, vict, TO_ROOM, NOTVICT);
            vict->setHP(-20, ch);
            group_gain(ch, vict);
            fight_kill(ch, vict, TYPE_CHOOSE, 0);
            return eSUCCESS;
          }
          else
          { // Missed the fucker
            act("You braveley resist as $n pulls at the very essence of your soul.",
                ch, 0, vict, TO_VICT, 0);
            act("$N stands his ground as dark magic coils around $n",
                ch, 0, vict, TO_ROOM, NOTVICT);
            act("$N shudders but resists your grasp upon $S soul.",
                ch, 0, vict, TO_CHAR, 0);
          }
        }
      }
    }
  }
  return eFAILURE;
}

int pushwand(Character *ch, class Object *obj, int cmd, const char *arg,
             Character *invoker)
{
  if (cmd != CMD_SAY && cmd != CMD_PUSH)
    return eFAILURE;
  if (cmd == CMD_PUSH)
  {
    if (str_cmp(arg, "wand") && str_cmp(arg, "ivory") && str_cmp(arg, "ebony"))
      return eFAILURE;
    if (GET_STR(ch) < 20)
    {
      ch->sendln("You try to separate the wand pieces, but you find yourself too weak to do so.");
      return eSUCCESS;
    }
    int newspell;
    switch (obj->obj_flags.value[3])
    {
    case 17:
      newspell = 28;
      break;
    case 28:
      newspell = 59;
      break;
    case 59:
      newspell = 113;
      break;
    case 113:
      newspell = 17;
      break;
    default:
      newspell = 17;
      break;
    }
    obj->obj_flags.value[3] = newspell;
    ch->sendln("You push the ivory so that the ivory and ebony separate.\r\nReassembling them, you hear a \"click\" as they snap back into place.");
    return eSUCCESS;
  }
  else if (cmd == CMD_SAY)
  {
    if (str_cmp(arg, "recharge"))
      return eFAILURE;
    class Object *curr;
    for (curr = ch->carrying; curr; curr = curr->next_content)
    {
      if (curr->vnum == 22427) // red dragon snout
      {
        obj->obj_flags.value[2] = 5;
        ch->sendln("The wand absorbs the dragon snout and pulses with energy.");
        obj_from_char(curr);
        extract_obj(curr);
        return eSUCCESS;
      }
    }
    return eFAILURE;
  }
  else
    return eFAILURE;
}

int dawnsword(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd != CMD_SAY)
    return eFAILURE;
  if (str_cmp(arg, " liberate me ab inferis"))
    return eFAILURE;
  if (GET_ALIGNMENT(ch) < 350)
  {
    ch->sendln("Dawn refuses your impure prayer.");
    return eSUCCESS;
  }
  if (isTimer(ch, OBJ_DAWNSWORD))
  {
    ch->send("Dawn needs more time to recharge and is not ready to hear your prayer yet.");
    return eSUCCESS;
  }
  if (!ch->in_room || isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) || isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_MAGIC))
  {
    ch->sendln("Something about this room blocks your command.");
    return eSUCCESS;
  }
  addTimer(ch, OBJ_DAWNSWORD, 24);

  ch->sendln("You whisper a prayer to Dawn and it responds in a brilliant flash of light!");
  Character *v;
  struct affected_type af;
  for (v = DC::getInstance()->world[ch->in_room].people; v; v = v->next_in_room)
  {
    if (v == ch)
      continue;
    if (GET_ALIGNMENT(v) >= 350)
    {
      act("$n whispers a quiet prayer and a glorious flash of holy light explodes from their weapon!", ch, 0, v, TO_VICT, 0);
      continue;
    }
    if (IS_AFFECTED(v, AFF_BLIND))
      continue; // no doubleblind
    act("$n whispers a quiet prayer and a searing blast of white light suddenly blinds you!", ch, 0, v, TO_VICT, 0);
    af.type = SPELL_BLINDNESS;
    af.location = APPLY_HITROLL;
    af.modifier = v->has_skill(SKILL_BLINDFIGHTING) ? skill_success(v, 0, SKILL_BLINDFIGHTING) ? -10 : -20 : -20;
    af.duration = 2;
    af.bitvector = AFF_BLIND;
    affect_to_char(v, &af);
    af.location = APPLY_AC;
    af.modifier = v->has_skill(SKILL_BLINDFIGHTING) ? skill_success(v, 0, SKILL_BLINDFIGHTING) ? 20 : 40 : 40;
    affect_to_char(v, &af);
  }

  return eSUCCESS;
}
int songstaff(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (obj->equipped_by == nullptr || !obj->equipped_by->in_room)
    return eFAILURE;
  ch = obj->equipped_by;
  char buf[MAX_STRING_LENGTH];
  if ((isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE) || isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_MAGIC) || isSet(DC::getInstance()->world[ch->in_room].room_flags, QUIET)))
    return eFAILURE;

  obj->obj_flags.timer--;
  if (obj->obj_flags.timer > 0)
    return eFAILURE;
  obj->obj_flags.timer = 5;

  int heal;
  for (Character *tmp_char = DC::getInstance()->world[ch->in_room].people; tmp_char; tmp_char = tmp_char->next_in_room)
  {
    if (!ARE_GROUPED(ch, tmp_char))
      continue;

    heal = 50 / 2 + ((GET_MAX_MOVE(tmp_char) * 2) / 100) + (number(0, 20) - 10);
    if (heal < 5)
      heal = 5;

    if (IS_PC(tmp_char) && isSet(tmp_char->player->toggles, Player::PLR_DAMAGE))
    {
      if (tmp_char == ch)
      {
        tmp_char->send(QStringLiteral("You feel your Travelling March recover %1 moves for you.\r\n").arg(heal));
      }
      else
      {
        sprintf(buf, "You feel %s's Travelling March recovering %d moves for you.\r\n", GET_NAME(ch), heal);
        tmp_char->send(buf);
      }
    }
    else
    {
      tmp_char->sendln("Your feet feel lighter.");
    }
    tmp_char->incrementMove(heal);
  }

  return eSUCCESS;
}

void check_eq(Character *ch);

int lilithring(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd != CMD_SAY)
    return eFAILURE;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);
  if (str_cmp(arg1, "ateni"))
    return eFAILURE;
  Character *victim;
  if (!(victim = ch->get_char_room_vis(arg2)))
  {
    ch->sendln("Noone here by that name.");
    return eSUCCESS;
  }
  if (IS_PC(victim))
  {
    ch->sendln("The Gods prohibit such evil.");
    return eSUCCESS;
  }
  if (isTimer(ch, OBJ_LILITHRING))
  {
    ch->sendln("The ring remains dark and your command goes unheeded.");
    return eSUCCESS;
  }

  if (circle_follow(victim, ch))
  {
    ch->sendln("Sorry, following in circles can not be allowed.");
    return eSUCCESS;
  }

  if ((!ISSET(victim->mobdata->actflags, ACT_BARDCHARM) && !ISSET(victim->mobdata->actflags, ACT_CHARM)) || victim->getLevel() > 50)
  {
    act("$N's soul is too powerful for you to command.", ch, 0, victim, TO_CHAR, 0);
    return eSUCCESS;
  }
  if (GET_ALIGNMENT(ch) > -350)
  {
    ch->sendln("Your soul is too pure for such an unclean act.");
    return eSUCCESS;
  }

  if (!ch->in_room || isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("Something about this room blocks your command.");
    return eSUCCESS;
  }
  act("You speak the command and $N must comply. Lilith's Ring of Command glows with mirthful malevolence.", ch, 0, victim, TO_CHAR, 0);
  act("$n whispers softly to $N. $N's eyes go blank and they now follow $n.", ch, 0, victim, TO_ROOM, 0);

  addTimer(ch, OBJ_LILITHRING, 24);
  if (victim->master)
    stop_follower(victim, 0);

  remove_memory(victim, 'h');

  add_follower(victim, ch, 0);
  struct affected_type af;

  af.type = OBJ_LILITHRING;
  af.duration = 3;
  af.modifier = 0;
  af.location = 0;
  af.bitvector = AFF_CHARM;
  affect_to_char(victim, &af);

  /* remove any !charmie eq the charmie is wearing */
  check_eq(victim);

  return eSUCCESS;
}

int orrowand(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  switch (cmd)
  {
  case CMD_SAY:
    if (str_cmp(arg, "recharge"))
    {
      return eFAILURE;
    }
    break;
  case CMD_PUSH:
    if (str_cmp(arg, "button"))
    {
      return eFAILURE;
    }
    ch->send("You push an almost invisible button on the wand with an audible *click*.\r\n");
    break;
  default:
    return eFAILURE;
    break;
  }

  if (obj->obj_flags.value[2] >= 5)
  {
    ch->send("The wand cannot be recharged yet.\r\n");
    return eSUCCESS;
  }

  class Object *curr;
  class Object *firstP = nullptr, *secondP = nullptr, *vial = nullptr, *diamond = nullptr;

  for (curr = ch->carrying; curr; curr = curr->next_content)
  {
    if (curr->vnum == 17399)
      diamond = curr;
    else if (curr->vnum == 27903 && firstP != nullptr)
      secondP = curr;
    else if (curr->vnum == 27903)
      firstP = curr;
    else if (curr->vnum == 27904)
      vial = curr;
  }

  if (!firstP || !secondP || !vial || !diamond)
  {
    ch->sendln("Recharge unsuccessful. Missing required components:");
    if (!firstP && 27903)
    {
      auto obj = static_cast<Object *>(DC::getInstance()->obj_index[27903].item);
      ch->send(QStringLiteral("%1\r\n").arg(GET_OBJ_SHORT(obj)));
    }
    if (!secondP && 27903)
    {
      auto obj = static_cast<Object *>(DC::getInstance()->obj_index[27903].item);
      ch->send(QStringLiteral("%1\r\n").arg(GET_OBJ_SHORT(obj)));
    }
    if (!vial && 27904)
    {
      auto obj = static_cast<Object *>(DC::getInstance()->obj_index[27904].item);
      ch->send(QStringLiteral("%1\r\n").arg(GET_OBJ_SHORT(obj)));
    }
    if (!diamond && 17399)
    {
      auto obj = static_cast<Object *>(DC::getInstance()->obj_index[17399].item);
      ch->send(QStringLiteral("%1\r\n").arg(GET_OBJ_SHORT(obj)));
    }
    return eSUCCESS;
  }
  obj_from_char(firstP);
  extract_obj(firstP);
  obj_from_char(secondP);
  extract_obj(secondP);
  obj_from_char(vial);
  extract_obj(vial);
  obj_from_char(diamond);
  extract_obj(diamond);
  obj->obj_flags.value[2] = 5;
  ch->sendln("The wand emits a soft \"beep\" and the display flashes \"Wand Recharged\"");
  return eSUCCESS;
}

int holyavenger(Character *ch, class Object *obj, int cmd, const char *arg,
                Character *invoker)
{
  // class Object *wielded;
  int percent, chance;
  Character *vict;

  if (!(vict = ch->fighting))
  {
    return eFAILURE;
  }
  if (vict->getHP() < 3500)
  {
    percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
    chance = number(1, 100);
    if (chance > (1.3 * percent))
    {
      percent = (100 * vict->getHP()) / GET_MAX_HIT(vict);
      chance = number(1, 100);
      if (chance > (2 * percent))
      {
        chance = number(1, 100);
        if (chance > (2 * percent))
        {
          chance = number(1, 100);
          if (chance > (2 * percent) && !isSet(vict->immune, ISR_SLASH))
          {
            if ((
                    (vict->equipment[WEAR_NECK_1] && vict->equipment[WEAR_NECK_1]->vnum == 518) ||
                    (vict->equipment[WEAR_NECK_2] && vict->equipment[WEAR_NECK_2]->vnum == 518)) &&
                !number(0, 1))
            { // tarrasque's leash..
              act("You attempt to behead $N, but your sword bounces of $S neckwear.", ch, 0, vict, TO_CHAR, 0);
              act("$n attempts to behead $N, but fails.", ch, 0, vict, TO_ROOM, NOTVICT);
              act("$n attempts to behead you, but cannot cut through your neckwear.", ch, 0, vict, TO_VICT, 0);
              return eSUCCESS;
            }
            if (IS_AFFECTED(vict, AFF_NO_BEHEAD))
            {
              act("$N deftly dodges your beheading attempt!", ch, 0, vict, TO_CHAR, 0);
              act("$N deftly dodges $n's attempt to behead $M!", ch, 0, vict, TO_ROOM, NOTVICT);
              act("You deftly avoid $n's attempt to lop your head off!", ch, 0, vict, TO_VICT, 0);
              return eSUCCESS;
            }
            act("You feel your life end as $n's sword SLICES YOUR HEAD OFF!", ch, 0, vict, TO_VICT, 0);
            act("You SLICE $N's head CLEAN OFF $S body!", ch, 0, vict, TO_CHAR, 0);
            act("$n cleanly slices $N's head off $S body!", ch, 0, vict, TO_ROOM, NOTVICT);
            vict->setHP(-20, ch);
            make_head(vict);
            group_gain(ch, vict);
            fight_kill(ch, vict, TYPE_CHOOSE, 0);
            return eSUCCESS | eVICT_DIED; /* Zero means kill it! */
                                          // it died..
          }
          else
          { /* You MISS the fucker! */
            act("You hear the SWOOSH sound of wind as $n's sword attempts to slice off your head!", ch, 0, vict, TO_VICT, 0);
            act("You miss your attempt to behead $N.", ch, 0, vict, TO_CHAR, 0);
            act("$N jumps back as $n makes an attempt to BEHEAD $M!", ch, 0, vict, TO_ROOM, NOTVICT);
          }
        }
      }
    }
  }
  return eFAILURE;
}

int hooktippedsteelhalberd(Character *ch, class Object *obj, int cmd,
                           const char *arg, Character *invoker)
{
  Character *victim;
  if (!(victim = ch->fighting))
    return eFAILURE;
  if (number(1, 101) > 2)
    return eFAILURE;
  int which = number(0, MAX_WEAR - 1);
  if (!victim->equipment[which])
    return eFAILURE; // Lucky
  int i = damage_eq_once(victim->equipment[which]);
  if (!victim->equipment[which])
    return eSUCCESS;
  if (i >= eq_max_damage(victim->equipment[which]))
    eq_destroyed(victim, victim->equipment[which], which);
  else
  {
    if (obj->vnum == 17904)
    {
      act("$n's diamond war club cracks your $p!", ch, victim->equipment[which], victim, TO_VICT, 0);
      act("$n smashes $m diamond war club into $N's $p and cracks it!", ch, victim->equipment[which], victim, TO_ROOM, NOTVICT);
      act("You smash your club into $N's $p, and manage to crack it!", ch, victim->equipment[which], victim, TO_CHAR, 0);
    }
    else
    {
      act("$n's hook-tipped steel halberd tears your $p!", ch, victim->equipment[which], victim, TO_VICT, 0);
      act("$n latches $m hook-tipped steel halberd into $N's $p and tears it!", ch, victim->equipment[which], victim, TO_ROOM, NOTVICT);
      act("You latch your halberd into $N's $p, and manage to tear it!", ch, victim->equipment[which], victim, TO_CHAR, 0);
    }
  }
  return eSUCCESS;
}

// TODO - I think we actually used this for a while but it was too powerful
int drainingstaff(Character *ch, class Object *obj, int cmd, char *arg,
                  Character *invoker)
{
  Character *vict;
  Object *staff;
  int dam;

  if (!(vict = ch->fighting))
  {
    return eFAILURE;
  }

  staff = ch->equipment[WIELD + cmd];
  dam = dice(staff->obj_flags.value[1], staff->obj_flags.value[2]);
  dam += GET_DAMROLL(ch);
  dam = (dam * 2) / 10; // Mages usually have 2-3 attacks
  if (IS_NPC(ch))
  { // NPC'S have no mana, so we'll drain hp instead
    if (dam >= vict->getHP())
    {
      dam = vict->getHP() - 1;
    }
    if (vict->affected_by_spell(SPELL_DIVINE_FURY) && dam > vict->affected_by_spell(SPELL_DIVINE_FURY)->modifier)
      dam = vict->affected_by_spell(SPELL_DIVINE_FURY)->modifier;
    vict->removeHP(dam);
    GET_MANA(ch) += dam;
  }
  else
  { // We have a character... drain mana
    if (dam >= GET_MANA(vict))
    {
      dam = GET_MANA(vict);
    }
    GET_MANA(vict) -= dam;
    GET_MANA(ch) += dam;
  }
  if (dam == 0)
  {
    return eFAILURE;
  }
  if (GET_MANA(ch) > GET_MAX_MANA(ch))
  {
    GET_MANA(ch) = GET_MAX_MANA(ch); // can't go above the MAX_MANA
  }
  return eSUCCESS;
}

int bank(Character *ch, class Object *obj, int cmd, const char *arg,
         Character *invoker)
{
  char buf[MAX_INPUT_LENGTH];
  int32_t x;

  if (cmd != CMD_BALANCE && cmd != CMD_DEPOSIT && cmd != CMD_WITHDRAW)
    return eFAILURE;

  /* balance */
  if (cmd == CMD_BALANCE)
  {
    ch->send(fmt::format(std::locale("en_US.UTF-8"), "You have {:L} $B$5gold$R coins in the bank.\r\n", GET_BANK(ch)));
    return eSUCCESS;
  }

  /* deposit */
  if (cmd == CMD_DEPOSIT)
  {
    if (!IS_NPC(ch) && ch->isPlayerGoldThief())
    {
      ch->sendln("Your criminal acts prohibit it.");
      return eFAILURE;
    }

    one_argument(arg, buf);
    if (!*buf || !(x = atoi(buf)) || x < 0)
    {
      ch->sendln("Deposit what?");
      return eSUCCESS;
    }
    if ((uint32_t)x > ch->getGold())
    {
      ch->sendln("You don't have that much gold!");
      return eSUCCESS;
    }
    if ((uint32_t)x + GET_BANK(ch) > 2000000000)
    {
      ch->sendln("That would bring you over your account maximum!");
      return eSUCCESS;
    }
    ch->removeGold(x);
    GET_BANK(ch) += x;
    ch->send(fmt::format(std::locale("en_US.UTF-8"), "You deposit {:L} $B$5gold$R coins.\r\n", x));
    ch->save();
    return eSUCCESS;
  }

  /* withdraw */
  one_argument(arg, buf);
  if (!*buf || !(x = atoi(buf)) || x < 0)
  {
    ch->sendln("Withdraw what?");
    return eSUCCESS;
  }
  if ((uint32_t)x > GET_BANK(ch))
  {
    ch->sendln("You don't have that much $B$5gold$R in the bank!");
    return eSUCCESS;
  }
  ch->addGold(x);
  GET_BANK(ch) -= x;
  ch->send(fmt::format(std::locale("en_US.UTF-8"), "You withdraw {:L} $B$5gold$R coins.\r\n", x));
  ch->save();
  return eSUCCESS;
}

int casino_atm(Character *ch, class Object *obj, int cmd, const char *arg,
               Character *invoker)
{
  char buf[MAX_INPUT_LENGTH];
  int32_t x;

  if (cmd != CMD_BALANCE && cmd != CMD_DEPOSIT && cmd != CMD_WITHDRAW)
    return eFAILURE;

  /* balance */
  if (cmd == CMD_BALANCE)
  {
    ch->send(fmt::format(std::locale("en_US.UTF-8"), "You have {:L} $B$5gold$R coins in the bank.\r\n", GET_BANK(ch)));
    return eSUCCESS;
  }

  /* deposit */
  if (cmd == CMD_DEPOSIT)
  {
    ch->sendln("You cannot use this for depositing money.");
    return eSUCCESS;
  }

  /* withdraw */
  one_argument(arg, buf);
  if (!*buf || !(x = atoi(buf)) || x < 0)
  {
    ch->sendln("Withdraw what?");
    return eSUCCESS;
  }
  if ((uint32_t)x > GET_BANK(ch))
  {
    ch->sendln("You don't have that much $B$5gold$R in the bank!");
    return eSUCCESS;
  }
  ch->addGold(x);
  GET_BANK(ch) -= x;
  ch->send(fmt::format(std::locale("en_US.UTF-8"), "You withdraw {:L} $B$5gold$R coins.\r\n", x));
  ch->save();
  return eSUCCESS;
}

// if you are in the room with this obj, and you try to go in any
// direction, and you don't have the correct obj in your inventory, it sends
// you to a particular room
// TODO - get it so that it effects groups properly.  Right now, if a group
// leader moves, he goes to the "bad" room, and the rest of the group goes to
// where the exit is supposed to go
// I should probably just change the exit temporarily to point to the "bad" room
// and then change it back after the move.
int returner(Character *ch, class Object *obj, int cmd, char *arg,
             Character *invoker)
{

  if (cmd > CMD_DOWN || cmd < CMD_NORTH)
    return eFAILURE;

  if (!obj)
    return eFAILURE;

  if (!obj->in_room)
    return eFAILURE;

  if (!CAN_GO(ch, cmd - 1))
    return eFAILURE;

  if (ch->in_room != obj->in_room)
    return eFAILURE;

  if (get_obj_in_list_num(2200, ch->carrying))
    return eFAILURE;

  if (move_char(ch, real_room(START_ROOM)) == 0)
    return eFAILURE;

  do_look(ch, "\0", 15);

  return eSUCCESS;
}

#define MAX_GEM_ASSEMBLER_ITEM 10

struct assemble_item
{
  char *finish_to_char;
  char *finish_to_room;
  char *missing_to_char;
  int components[MAX_GEM_ASSEMBLER_ITEM];
  int item;
};

struct assemble_item assemble_items[] = {
    // Item 0, the crystalline tir stuff
    {"A brilliant flash of light erupts from your hands as the gems mold themselves together and form a cohesive and flawless gem.\r\n",
     "A brilliant flash of light erupts from $n's hands as the gems $e holds form a new cohesive and flawless gem.",
     "One of the gems seems to be missing.\r\n",
     {2714, 2602, 12607, -1, -1, -1, -1, -1, -1, -1},
     1506},

    // Item 1, Etala the Shadowblade
    {"Connecting the hilt and gem to the blade, you form a whole sword.\r\n",
     "$n fiddles around with some stuff in $s inventory.",
     "You seem to be missing a piece.\r\n",
     {181, 182, 183, -1, -1, -1, -1, -1, -1, -1},
     184},

    // Item 2, Broadhead arrow from forage items
    {"You carefully sand down the pointy stick, adding to its excellent form.\r\n"
     "Splitting the feathers down, you carefully attach them to the pointy stick.\r\n"
     "Finally, you shape the scorpion stinger into a deadly arrowhead and secure it to the front.\r\n",
     "$n sits down with some junk and tries $s hand at fletching.",
     "You don't have all the items required to fletch an arrow.\r\n",
     {3185, 3186, 3187, -1, -1, -1, -1, -1, -1, -1},
     3188},

    // Item 3, Wolf tooh arrow from forage items
    {"You carefully sand down the pointy stick, adding to its excellent form.\r\n"
     "Splitting the feathers down, you carefully attach them to the pointy stick.\r\n"
     "Finally, you hone the wolf's tooth stinger into a sharp arrowhead and secure it to the front.\r\n",
     "$n sits down with some junk and tries $s hand at fletching.",
     "You don't have all the items required to fletch an arrow.\r\n",
     {3185, 28301, 3187, -1, -1, -1, -1, -1, -1, -1},
     3190},

    // Item 4, Gaiot key in DK
    {"The stone pieces join together to form a small statue of a dragon.\r\n",
     "$n assembles some stones together to form a black statue.\r\n",
     "The pieces click together but fall apart as if something is missing.\r\n",
     {9502, 9503, 9504, 9505, 9506, -1, -1, -1, -1, -1},
     9501},

    // Item 5, ventriloquate dummy - rahz
    {"You are able to put the parts together, and create a ventriloquist's dummy.\r\n",
     "$n manages to the parts together, creating a ventriloquist's dummy.\r\n",
     "The pieces don't seem to fit together quite right.\r\n",
     {17349, 17350, 17351, 17352, 17353, 17354, -1, -1, -1, -1},
     17348},

    // Item 6, the Shield of the Beholder
    {"You place the two gems into the holes on the shield and it seems to hum with power.\r\n",
     "$n places two precious gemstones into a beholder's carapace to create a shield.\r\n",
     "You seem to be missing a piece.\r\n",
     {5260, 5261, 5262, -1, -1, -1, -1, -1, -1, -1},
     5263},

    // Item 7, a curiously notched medallion
    {"With a blinding flash, the gem makes the medallion whole.\r\n",
     "As $n fiddles with the medallion pieces, you are dazed by a bright flash!\n\r",
     "You attempt to assemble the family medallion but something is missing.\r\n",
     {30084, 30085, 30086, 30087, 30088, -1, -1, -1, -1, -1},
     30083},

    // Junk Item.  THIS MUST BE LAST IN THE ARRAY
    {"",
     "",
     "",
     {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
     -1}

};

int hellmouth_thing(Character *ch, class Object *obj, int cmd, const char *arg,
                    Character *invoker)
{
  if (cmd != CMD_SAY)
    return eFAILURE;
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];

  half_chop(arg, arg1, junk);
  if (*junk)
    return eFAILURE;

  if (strncmp(arg1, "vanity", 6))
    return eFAILURE;

  if (invoker->fighting)
    return eFAILURE;

  // send_to_char("The hellmouth reaches out and embraces you. As you become completely\r\n"
  // "immersed in its energy your skin burns, your sight fades into darkness,\r\n"
  // "your nose recoils at the stench of sulphur, and all you can taste is\r\n"
  // "blood.\r\n",invoker);
  //  char_from_room(invoker);
  //  char_to_room(invoker, real_room(4801));
  GET_KI(invoker) -= 50;
  if (GET_KI(invoker) < 0)
    GET_KI(invoker) = 0;
  invoker->setHP(invoker->getHP() / 2);
  GET_MANA(invoker) /= 2;
  invoker->setMove(invoker->getMove() / 2.0);
  //  do_look(invoker, "", CMD_DEFAULT);
  //   send_to_char("In an instant your senses are restored and you are left only temporarily\r\n"
  //                "dazed. Although you appear to be somewhere other than where you were prior\r\n"
  //                "to this experience, your life feels as though it has ebbed to the brink of\r\n"
  // 	        "death and has been only partially restored.\r\n",invoker);

  return eFAILURE; // So normal say function will execute after this
}

int search_assemble_items(int vnum)
{
  // This should never happen
  if (vnum < 1)
  {
    logf(ANGEL, DC::LogChannel::LOG_BUG, "search_assemble_items passed vnumx=%d\n\r", vnum);
    produce_coredump();
    return -1;
  }

  for (int item_index = 0; assemble_items[item_index].item != -1; item_index++)
  {
    // We search until MAX_GEM_ASSEMBLER_ITEM -1 because we don't want to include the last item
    // which is the finished item vnum
    for (int component_index = 0; component_index < MAX_GEM_ASSEMBLER_ITEM; component_index++)
    {
      if (assemble_items[item_index].components[component_index] == vnum)
      {
        return item_index;
      }
    }
  }

  return -1;
}

bool assemble_item_index(Character *ch, int item_index)
{
  // This should never happen
  if (item_index < 0)
  {
    logf(ANGEL, DC::LogChannel::LOG_BUG, "assemble_item_index passed item_index=%d\n\r", item_index);
    produce_coredump();
    return false;
  }

  // Look through all the components of item_index and see if the player has them
  for (int component_index = 0; component_index < MAX_GEM_ASSEMBLER_ITEM; component_index++)
  {

    int component_virt = assemble_items[item_index].components[component_index];
    if (component_virt < 1)
    {
      continue;
    }

    rnum_t component_real = component_virt;
    if (component_real < 0)
    {
      logf(ANGEL, DC::LogChannel::LOG_BUG, "assemble_items[%d], component_index %d refers to invalid rnum %d for vnum %d.",
           item_index, component_index, component_real, component_virt);

      ch->sendln("There was an internal malfunction assembling your item. Contact an Immortal.");
      produce_coredump();
      return true;
    }

    if (get_obj_in_list_num(component_real, ch->carrying) == 0)
    {
      return false;
    }
  }

  // If we get to this point then all components for item_index were found
  int item_vnum = assemble_items[item_index].item;
  rnum_t item_real = item_vnum;
  Object *item = DC::getInstance()->obj_index[item_real].item;

  // Check if the item to be assembled is marked UNIQUE but the player already has one
  if (isSet(item->obj_flags.more_flags, ITEM_UNIQUE))
  {
    if (search_char_for_item(ch, item_real, false))
    {
      ch->sendln("You already have one of those!");
      return true;
    }
  }

  // Send item specific assemble messages to the player and room
  send_to_char(assemble_items[item_index].finish_to_char, ch);
  act(assemble_items[item_index].finish_to_room, ch, 0, 0, TO_ROOM, 0);

  // Remove all the components from the player
  for (int component_index = 0; component_index < MAX_GEM_ASSEMBLER_ITEM; component_index++)
  {
    int component_virt = assemble_items[item_index].components[component_index];
    if (component_virt < 1)
    {
      continue;
    }

    rnum_t component_real = component_virt;
    if (component_real < 0)
    {
      logf(ANGEL, DC::LogChannel::LOG_BUG, "assemble_items index %d, component_index %d refers to invalid rnum %d for vnum %d.",
           item_index, component_index, component_real, component_virt);

      ch->sendln("There was an internal malfunction assembling your item. Contact an Immortal.");
      return true;
    }

    Object *component_obj = get_obj_in_list_num(component_real, ch->carrying);
    obj_from_char(component_obj);
    extract_obj(component_obj);
  }

  // make the new item
  Object *reward_item = DC::getInstance()->clone_object(item_real);
  if (reward_item == 0)
  {
    logf(ANGEL, DC::LogChannel::LOG_BUG, "Unable to clone vnum %d, rnum %d.", item_vnum, item_real);
    ch->sendln("There was an internal malfunction cloning the new item. Contact an Immortal.");
    return true;
  }

  obj_to_char(reward_item, ch);

  return true;
}

int do_assemble(Character *ch, char *argument, int cmd)
{
  bool different_item_components = false;
  int vnum, item_index = -1, last_item_index = -1;
  char arg1[MAX_INPUT_LENGTH + 1];
  Object *obj;

  one_argument(argument, arg1);

  // if no arguments are given then look through entire inventory for items to assemble.
  if (arg1[0] == 0)
  {
    // for each inventory item
    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
      // if we can see it
      if (CAN_SEE_OBJ(ch, obj, false))
      {
        vnum = obj->vnum;
        item_index = search_assemble_items(vnum);

        // check if it's a component of an item to be assembled
        if (item_index != -1)
        {
          // check if the current component is from a different item than the previous
          // component that we found
          if (last_item_index == -1)
          {
            last_item_index = item_index;
          }
          else if (last_item_index != item_index)
          {
            different_item_components = true;
          }

          if (different_item_components)
          {
            break;
          }
        }
      }
    }

    // If components from multiple items were found then make the player specify
    // the item that needs to be assembled
    if (different_item_components)
    {
      ch->sendln("Assemble which object?");
      return eFAILURE;
    }
    else if (last_item_index == -1)
    {
      ch->sendln("You don't have anything that can be assembled.");
      return eFAILURE;
    }
    else
    {
      // Attempt to assemble all the components
      if (assemble_item_index(ch, last_item_index) == false)
      {
        csendf(ch, "%s", assemble_items[last_item_index].missing_to_char);
        return eFAILURE;
      }
    }

    return eSUCCESS;
  }

  // if arguments are given, find object and see if it can be assembled.
  obj = get_obj_in_list_vis(ch, arg1, ch->carrying);
  if (obj == nullptr)
  {
    act("You can't find it!", ch, 0, 0, TO_CHAR, 0);
    return eFAILURE;
  }

  vnum = obj->vnum;
  item_index = search_assemble_items(vnum);

  // Object specified is not part of an item that can be assembled
  if (item_index == -1)
  {
    ch->sendln("That item can't be assembled into anything.");
    return eFAILURE;
  }

  // Attempt to assemble all the components
  if (assemble_item_index(ch, item_index) == false)
  {
    csendf(ch, "%s", assemble_items[item_index].missing_to_char);
    return eFAILURE;
  }

  return eSUCCESS;
}

int stupid_button(Character *ch, class Object *obj, int cmd, const char *arg,
                  Character *invoker)
{
  if (cmd != CMD_PUSH)
    return eFAILURE;

  char vict[MAX_STRING_LENGTH];

  one_argument(arg, vict);
  if (strcmp(vict, "button") && strcmp(vict, "red") && strcmp(vict, "big"))
  {
    ch->sendln("Push what?");
    return eFAILURE;
  }

  ch->sendln("You couldn't help but push the $4$Bbutton$R, could you?");
  ch->sendln("The floor drops out beneath you and you find yourself.. er.. somewhere.");
  move_char(ch, real_room(49));
  do_look(ch, "\0", 15);
  return eSUCCESS;
}

// Fear gaze.
int gazeofgaiot(Character *ch, class Object *obj, int cmd, const char *arg,
                Character *invoker)
{
  Character *victim;
  char vict[MAX_INPUT_LENGTH]; // buffer overflow fix

  one_argument(arg, vict);
  if (cmd != CMD_GAZE)
    return eFAILURE;
  if (!ch->equipment[WEAR_FACE] || 9603 != ch->equipment[WEAR_FACE]->vnum)
    return eFAILURE;

  if (ch->affected_by_spell(SKILL_FEARGAZE))
  {
    ch->sendln("You need to build up more hatred before you can unleash it again.");
    return eSUCCESS;
  }

  if (!(victim = ch->get_char_room_vis(vict)))
  {
    if (ch->fighting)
    {
      victim = ch->fighting;
    }
    else
    {
      ch->sendln("Gaze on whom?");
      return eSUCCESS;
    }
  }
  if (!can_attack(ch) || !can_be_attacked(ch, victim))
    return eSUCCESS;
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, NO_MAGIC))
  {
    ch->sendln("That action is impossible to perform in these restrictive confinements.");
    return eSUCCESS;
  }
  if (victim->getLevel() > 70)
  {
    ch->sendln("Some great force prevents you.");
    return eSUCCESS;
  }
  // All is good, set timer and perform it.
  struct affected_type af;
  af.type = SKILL_FEARGAZE;
  af.duration = 30;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = -1;
  affect_to_char(ch, &af);
  act("You eyes glow red from hatred, and you discharge it all on $N.", ch, 0, victim, TO_CHAR, 0);
  act("$n's eyes glow with hatred, and $e directs it all at you. OoO, scary!", ch, 0, victim, TO_VICT, 0);
  while (number(0, 1))
    do_flee(victim, "", 0);
  return eSUCCESS;
}

int pfe_word(Character *ch, class Object *obj, int cmd, const char *arg,
             Character *invoker)
{
  // char buf[MAX_INPUT_LENGTH];
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  class Object *obj_object;
  int j;

  if (!cmd && obj) // This is where we recharge
    if (obj->obj_flags.value[3])
      obj->obj_flags.value[3]--;

  // 11 = say 69 = remove
  if (cmd != CMD_SAY && cmd != CMD_REMOVE)
    return eFAILURE;

  if (!ch)
    return eFAILURE;
  int pos = HOLD;
  if (!ch->equipment[pos] || 3611 != ch->equipment[pos]->vnum)
  {
    pos = HOLD2;
    if (!ch->equipment[pos] || 3611 != ch->equipment[pos]->vnum)
      return eFAILURE;
  }

  half_chop(arg, arg1, junk);
  if (*junk)
    return eFAILURE;

  if (cmd == CMD_SAY)
  {
    if (str_cmp("aslexi", arg1))
      return eFAILURE;

    if (ch->equipment[pos]->obj_flags.value[3])
    {
      ch->sendln("The item seems to be recharging.");
      return eSUCCESS;
    }

    ch->equipment[pos]->obj_flags.value[3] = 600;

    act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("You quietly whisper 'aslexi' into your hands.");

    // cast_protection_from_evil(ch->getLevel(), ch, 0, SPELL_TYPE_SPELL, ch, 0, 50);
    // changed to spell_type_potion so that the align check doesn't happen for this item
    //      cast_protection_from_evil(ch->getLevel(), ch, 0, SPELL_TYPE_POTION, ch, 0,
    // 50);
    if (IS_AFFECTED(ch, AFF_PROTECT_EVIL) || ch->affected_by_spell(SPELL_PROTECT_FROM_GOOD))
    {
      ch->sendln("You already have alignment protection.");
      return eFAILURE;
    }

    struct affected_type af;
    if (!ch->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
    {
      af.type = SPELL_PROTECT_FROM_EVIL;
      af.duration = 5;
      af.modifier = ch->getLevel() + 10;
      af.location = APPLY_NONE;
      af.bitvector = AFF_PROTECT_EVIL;
      affect_to_char(ch, &af);
      ch->sendln("You have a righteous, protected feeling!");
    }

    act("A pulsing aura springs to life around $n!", ch, 0, 0, TO_ROOM, 0);
    return eSUCCESS;
  }
  else // cmd=69 (remove)
  {
    obj_object = ch->get_object_in_equip_vis(arg1, ch->equipment, &j, false);
    if (!obj_object)
      return eFAILURE;

    if (obj_object != ch->equipment[HOLD] &&
        obj_object != ch->equipment[HOLD2])
      return eFAILURE;
    if (obj_object->vnum != 3611)
      return eFAILURE;

    if (ch->affected_by_spell(SPELL_PROTECT_FROM_EVIL))
    {
      affect_from_char(ch, SPELL_PROTECT_FROM_EVIL);
      ch->sendln("The power drains away.");
    }
    // This should remove the pfe unless he has it cast or on EQ
    // We will allow it to return false do the do_remove goes through.
  }
  return eFAILURE;
}

int devilsword(Character *ch, class Object *obj, int cmd, const char *arg,
               Character *invoker)
{
  // char buf[MAX_INPUT_LENGTH];
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  // class Object *obj_object;
  // int j;

  if (cmd != CMD_SAY)
    return eFAILURE;

  if (!ch || !ch->equipment || !ch->equipment[WIELD])
    return eFAILURE;

  if (185 != ch->equipment[WIELD]->vnum)
    return eFAILURE;

  half_chop(arg, arg1, junk);

  if (*junk)
    return eFAILURE;

  if (!str_cmp("infrendeo", arg1))
  {
    act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
    if (ch->equipment[WIELD]->obj_flags.value[3] == 8)
    {
      ch->sendln("Nothing happens.");
      return true;
    }

    act("Venom-flecked fangs grow and bristle from the bedeviled Cestus!", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("Huge fangs spring forth from your weapon!");

    ch->equipment[WIELD]->obj_flags.value[3] = 8;
    return eSUCCESS;
  }
  if (!str_cmp("pulsus", arg1))
  {
    act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
    if (ch->equipment[WIELD]->obj_flags.value[3] == 7)
    {
      ch->sendln("Nothing happens.");
      return eSUCCESS;
    }

    act("The fangs of $n's weapon retract magically into the metal.", ch, 0, 0, TO_ROOM, 0);
    ch->sendln("The fangs retract magically into the metal.");

    ch->equipment[WIELD]->obj_flags.value[3] = 7;
    return eSUCCESS;
  }

  return eFAILURE;
}

// If you have AFFsanct but not the spell, kill it
// This works fine as long as they don't have perma-sanct eq
void remove_eliara(Character *ch)
{

  if (!IS_AFFECTED(ch, AFF_SANCTUARY))
    return;

  if (ch->affected_by_spell(SPELL_SANCTUARY))
    return;

  act("Eliara's glow fades, as she falls dormant once again.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("Eliara's glow fades, as she falls dormant once again.\r\n");
  REMBIT(ch->affected_by, AFF_SANCTUARY);
}

int dancevest(Character *ch, class Object *obj, int cmd, const char *arg,
              Character *invoker)
{
  if (!cmd || cmd != CMD_SAY || !ch || !ch->in_room || str_cmp(arg, " just dance"))
  {
    return eFAILURE;
  }
  do_say(ch, "just dance", CMD_SAY);
  if (obj->obj_flags.timer > 0)
  {
    ch->sendln("The vest remains silent.");
    return eSUCCESS;
  }
  char *command_list[] =
      {
          "dance", // 0
          "shuffle",
          "wiggle",
          "bellydance",
          "bounce",
          "polka", // 5
          "waltz",
          "boogie",
          "headbang",
          "showtune" // 9
      };
  Character *v;
  ch->sendln("As you intone the sacred words, phantom music swells around you and everyone within earshot joins in!");
  for (v = DC::getInstance()->world[ch->in_room].people; v; v = v->next_in_room)
  {
    if (GET_POS(v) != position_t::STANDING)
    {
      continue;
    }
    v->sendln("As phantom music swells around you, you are helpless to resist.  You must obey.");
    char tmp_command[32];
    strcpy(tmp_command, command_list[number(0, 9)]);
    v->command_interpreter(tmp_command);
  }
  obj->obj_flags.timer = 48;
  return eSUCCESS;
}

int durendal(Character *ch, class Object *obj, int cmd, const char *arg,
             Character *invoker)
{

  if (!cmd)
  { // pulse
    if (obj->obj_flags.timer > 0)
    {
      obj->obj_flags.timer--;
    }
    if (obj->obj_flags.timer < 322 && isSet(obj->obj_flags.more_flags, ITEM_TOGGLE))
    {
      REMOVE_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
      if (obj->obj_flags.timer > 0 && obj->equipped_by && obj->equipped_by->in_room)
      {
        obj->equipped_by->sendln("The white fire surrounding Durendal gutters and flickers out.");
        act("The flames surrounding $n's weapon gutters and fade.", obj->equipped_by, 0, 0, TO_ROOM, 0);
      }
    }
    return eSUCCESS;
  }

  if (cmd != CMD_SAY || !ch || !ch->in_room || str_cmp(arg, "Gods forgive me") || obj->equipped_by != ch)
  {
    return eFAILURE;
  }
  if (obj->obj_flags.timer > 0)
  {
    ch->sendln("Your plea goes unanswered. Durendal slumbers.");
    return eSUCCESS;
  }
  if (GET_ALIGNMENT(ch) < 350)
  {
    ch->sendln("Your soul is impure. Durendel ignores your contrition.");
    return eSUCCESS;
  }
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("Something about this room prohibits your prayer from being heard.");
    return eSUCCESS;
  }
  ch->sendln("Upon hearing your plea, Durendal suddenly bursts into flame with a blinding flash of searing white heat!");
  act("$n mutters a quiet prayer and with a blinding flash, their weapon bursts into flame!", ch, 0, 0, TO_ROOM, 0);
  Character *v, *vn;
  for (v = DC::getInstance()->world[ch->in_room].people; v; v = vn)
  {
    vn = v->next_in_room;
    if (GET_ALIGNMENT(v) > -350 || ARE_GROUPED(ch, v))
    {
      continue;
    }
    v->sendln("You feel the evil in your soul being burned away!");
    damage(ch, v, 250, TYPE_COLD, TYPE_UNDEFINED, 0);
    act("The evil in $N's soul is burned away!", ch, 0, v, TO_CHAR, 0);
  }
  SET_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
  obj->obj_flags.timer = 360;
  return eSUCCESS;
}

// When fighting an evil opponent, sancts PC
int eliara_combat(Character *ch, class Object *obj, int cmd, const char *arg,
                  Character *invoker)
{

  Character *vict = nullptr;

  if (cmd)
    return eFAILURE;

  if (!ch || !ch->equipment || !ch->equipment[WIELD])
    return eFAILURE;

  if (30627 != ch->equipment[WIELD]->vnum)
  {
    remove_eliara(ch);
    return eFAILURE;
  }

  if (!(vict = ch->fighting))
    return eFAILURE;
  if (!IS_EVIL(vict))
    return eFAILURE;
  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    return eSUCCESS;

  act("Eliara glows brightly for a moment, its incandescent field of light surrounding $n in a glowing aura.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("Eliara glows brightly surrounding you in its protective aura!");

  SETBIT(ch->affected_by, AFF_SANCTUARY);

  return eSUCCESS;
}

int eliara_non_combat(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{

  if (!ch)
    return eFAILURE;

  if (cmd == CMD_REMOVE && GET_POS(ch) == position_t::FIGHTING && ch->equipment && ch->equipment[WIELD] && ch->equipment[WIELD]->vnum == 30627)
  {
    ch->sendln("Eliara refuses to allow you to remove equipment during battle!");
    return eSUCCESS;
  }

  if (GET_POS(ch) < position_t::STANDING)
    return eFAILURE;

  remove_eliara(ch);

  return eFAILURE;
}

int carriage(Character *ch, class Object *obj, int cmd, char *arg,
             Character *invoker)
{
  // this ain't done yet...it's natashas half written proc

  if (cmd > CMD_DOWN || cmd < CMD_NORTH)
    return eFAILURE;

  if (!obj)
    return eFAILURE;

  if (!obj->in_room)
    return eFAILURE;

  if (ch->in_room != obj->in_room)
    return eFAILURE;

  if (move_char(ch, real_room(START_ROOM)) == 0)
    return eFAILURE;

  do_look(ch, "\0", 15);

  return eSUCCESS;
}

int arenaporter(Character *ch, class Object *obj, int cmd, char *arg,
                Character *invoker)
{
  // only go off when a player types a command
  if (!cmd)
    return eFAILURE;

  // 20% of the time
  if (number(1, 5) == 1)
    return eFAILURE;

  if (!obj || !obj->in_room || !ch)
    return eFAILURE;

  if (ch->in_room != obj->in_room)
    return eFAILURE;

  if (!move_char(ch, real_room(number(17800, 17949))) == 0)
    return eFAILURE;

  if (ch->fighting)
  {
    stop_fighting(ch->fighting);
    stop_fighting(ch);
  }

  ch->sendln("A dimensional hole swallows you.\r\nYou reappear elsewhere.");
  act("$n fades out of existence.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  do_look(ch, "\0", 15);

  // return false so their command goes off
  return eFAILURE;
}

int movingarenaporter(Character *ch, class Object *obj, int cmd, char *arg,
                      Character *invoker)
{
  int32_t room = 17840;

  if (!cmd)
  {
    // move myself 10% of time
    if (number(1, 10) > 1)
      return eFAILURE;

    while (room == 17840)
      room = number(17800, 17849);

    move_obj(obj, real_room(room));
    return eSUCCESS;
  }

  if (!obj || !obj->in_room)
    return eFAILURE;

  if (ch->in_room != obj->in_room)
    return eFAILURE;

  if (!move_char(ch, real_room(number(17800, 17949))) == 0)
    return eFAILURE;

  ch->sendln("A dimensional hole swallows you.\r\nYou reappear elsewhere.");
  act("$n fades out of existence.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  if (ch->fighting)
  {
    stop_fighting(ch->fighting);
    stop_fighting(ch);
  }

  do_look(ch, "\0", 15);

  return eSUCCESS;
}

int restring_machine(Character *ch, class Object *obj, int cmd, const char *arg,
                     Character *invoker)
{
  char name[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  class Object *target_obj = nullptr;

  if (cmd != CMD_RESTRING)
    return eFAILURE;

  act("The machine flashes and shoots sparks and smoke throughout the room.", ch, 0, 0, TO_ROOM, 0);
  ch->send("The machine beeps and emits hollow a voice...\n");

  half_chop(arg, name, buf);

  if (!*arg || !*name || !*buf)
  {
    send_to_char("'Restring Machine v2.1' *beep*'\r\n"
                 "'restring <Item> <Description>' *beep*'\r\n"
                 "'This requires up to 50 platinum.  *beep*'\r\n"
                 "'This will only work on Godload. *beep*'\r\n"
                 "'Be careful and type correctly.  No refunds.'\r\n",
                 ch);
    return eSUCCESS;
  }

  if (!(target_obj = get_obj_in_list_vis(ch, name, ch->carrying)))
  {
    ch->send("'Cannot find this item in your inventory.  *beep*'\n");
    return eSUCCESS;
  }

  if (!isSet(target_obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    ch->send("'The item must be godload.  *beep*'\n");
    return eSUCCESS;
  }

  if (strlen(buf) > 80)
  {
    ch->send("'The description cannot be longer than 80 characters. *beep*'\n");
    return eSUCCESS;
  }

  if (GET_PLATINUM(ch) < (uint32_t)(ch->getLevel()))
  {
    ch->send("'Insufficient platinum.  *beep*'\n");
    return eSUCCESS;
  }

  GET_PLATINUM(ch) -= (ch->getLevel());

  //  dc_free(target_obj->short_description);
  //  target_obj->short_description = (char *) dc_alloc(strlen(buf)+1, sizeof(char));
  //  strcpy(target_obj->short_description, buf);
  char zarg[MAX_STRING_LENGTH];
  sprintf(zarg, "$B$7%s$R", buf);
  target_obj->short_description = str_hsh(zarg);

  send_to_char("\r\n'Beginning magical transformation process. *beep*'\r\n"
               "You put your item into the machine and close the lid.\r\n"
               "Smoke pours out of the machine and sparks fly out blackening the floor.\r\n"
               "Your item looks new!\r\n\r\n",
               ch);

  ch->save();
  return eSUCCESS;
}

int weenie_weedy(Character *ch, class Object *obj, int cmd, const char *arg,
                 Character *invoker)
{
  if (cmd)
    return eFAILURE;

  if (number(1, 1000) > 990)
  {
    if (obj->carried_by)
      send_to_room("Someone's weenie weedy doll says, 'BLARG!'\r\n", obj->carried_by->in_room);
    else if (obj->in_room != DC::NOWHERE)
      send_to_room("a weenie weedy doll says, 'BLARG!'\r\n", obj->in_room);
    else if (obj->in_obj && obj->in_obj->carried_by)
      send_to_room("a muffled 'BLARG!' comes from a weenie weedy doll somewhere nearby.\r\n", obj->in_obj->carried_by->in_room);
  }

  return eSUCCESS;
}

// this should really be a azrack megaphone, but i'm too lazy to figure out how they
// work right now
// TODO - figure out how azrack's megaphone's work
int stupid_message(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  if (cmd)
    return eFAILURE;

  if (!obj || obj->in_room == DC::NOWHERE)
    return eFAILURE;

  if (!DC::getInstance()->zones.value(DC::getInstance()->world[obj->in_room].zone).players)
    return eFAILURE;

  if (number(1, 10) == 1)
    send_to_room("The shadows swirl to reveal a face before you.\r\n"
                 "It speaks suddenly, 'Only with the key can you unlock the masters name' "
                 "and then fades away.\r\n",
                 obj->in_room, true);

  return eSUCCESS;
}

// If there is a player in rooms 8695-8699 (the pillars), then they cause a
// "balance" and we can remove the imp_only flag from the target room
int pagoda_balance(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  if (cmd)
    return eFAILURE;

  Character *vict = nullptr;
  int found = 0;

  for (int i = 8695; i < 8699; i++)
  {
    // TODO - should probably check to make sure these are valid rooms before we
    // use them.  Proc isn't used yet though, so no biggy.
    for (vict = DC::getInstance()->world[real_room(i)].people; vict; vict = vict->next_in_room)
      if (IS_NPC(vict))
        found = 0;
      else
      {
        found = 1;
        break;
      }

    if (!found)
      break;
  }

  // If we aren't true here, then one of the rooms didn't have a PC in it
  if (!found)
    return eFAILURE;

  for (int j = 8695; j < 8699; j++)
    send_to_room("The weight of your body helps shift the balances.\r\n"
                 "You hear the poping of a magical barrier dissapating.\r\n\r\n",
                 real_room(j));

  REMOVE_BIT(DC::getInstance()->world[real_room(8699)].room_flags, IMP_ONLY);
  return eFAILURE;
}

// If players enter the room, pop a "imp_only" flag back on the room.
int pagoda_shield_restorer(Character *ch, class Object *obj, int cmd, const char *arg,
                           Character *invoker)
{
  if (!cmd)
    return eFAILURE; // only restore if a player is in the room

  if (!isSet(DC::getInstance()->world[obj->in_room].room_flags, IMP_ONLY))
  {
    send_to_room("You hear the 'pop' of a magical barrier springing up.\r\n\r\n", obj->in_room);
    SET_BIT(DC::getInstance()->world[obj->in_room].room_flags, IMP_ONLY);
  }

  return eFAILURE;
}

// stupid little item I made for an ex-gf so she could find me when she logged in
int phish_locator(Character *ch, class Object *obj, int cmd, const char *arg,
                  Character *invoker)
{
  Character *victim = nullptr;

  if (cmd != CMD_PUSH) // push
    return eFAILURE;

  if (!strstr(arg, "button"))
    return eSUCCESS;

  act("$n fiddles with a small fish-shaped device.", ch, 0, 0, TO_ROOM, INVIS_NULL);

  if (!(victim = get_char("Pirahna")))
  {
    ch->sendln("The locator beeps angrily and smoke starts to come out.\r\nPirahna is unlocatable.");
    return eSUCCESS;
  }

  ch->sendln("Found him!");

  do_transfer(victim, GET_NAME(ch));
  return eSUCCESS;
}

int generic_push_proc(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{
  Character *victim;
  Character *next_vict;

  if (cmd != CMD_PUSH) // push
    return eFAILURE;

  int obj_vnum = obj->vnum;

  switch (obj_vnum)
  {
  case 26723: // transporter in star-trek
    send_to_room("You hear a chiming electrical noise as the transporter hums to life.\r\n", obj->in_room);
    for (victim = DC::getInstance()->world[obj->in_room].people; victim; victim = next_vict)
    {
      next_vict = victim->next_in_room;
      victim->sendln("Your body is pulled apart and reassembled elsewhere!");
      act("$n slowly fades out of existence.", victim, 0, 0, TO_ROOM, 0);
      move_char(victim, 26802);
      act("$n slowly fades into existence.", victim, 0, 0, TO_ROOM, 0);
      do_look(victim, "", CMD_DEFAULT);
    }
    break;

  default:
    ch->sendln("Whatever you pushed doesn't have an entry in the button push table.  Tell a god.");
    logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "'Push' proc on obj %d without entry in proc table. (push_proc)\r\n", obj_vnum);
    break;
  }

  return eSUCCESS;
}

int portal_word(Character *ch, class Object *obj, int cmd, char *arg,
                Character *invoker)
{
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  Character *victim = nullptr;

  if (!cmd && obj)
  { // This is where we recharge
    if (obj->obj_flags.value[3])
    {
      obj->obj_flags.value[3]--;
      if (0 == obj->obj_flags.value[3])
      {
        if (obj->carried_by)
          obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->equipped_by)
          obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->carried_by)
          obj->in_obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->equipped_by)
          obj->in_obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
      }
    }
  }
  // 11 = say
  if (cmd != CMD_SAY)
    return eFAILURE;
  if (!ch)
    return eFAILURE;
  if (ch->equipment[HOLD] != obj)
    return eFAILURE;

  half_chop(arg, arg1, junk);

  if (str_cmp("magiskhal", arg1))
    return eFAILURE;

  if (ch->equipment[HOLD]->obj_flags.value[3] && !ch->isImmortalPlayer())
  {
    ch->sendln("The item seems to be recharging.");
    return eSUCCESS;
  }
  act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You quietly whisper 'magiskhal' into your hands.");

  if (!(victim = get_char_vis(ch, junk)))
  {
    ch->sendln("The box somehow seems......confused.");
  }
  else
  {
    spell_portal(ch->getLevel(), ch, victim, 0, 0);
    // set charge time
    ch->equipment[HOLD]->obj_flags.value[3] = 600;
  }
  act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
  return eSUCCESS;
}

int full_heal_word(Character *ch, class Object *obj, int cmd, char *arg,
                   Character *invoker)
{
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  Character *victim = nullptr;

  if (!cmd && obj)
  { // This is where we recharge
    if (obj->obj_flags.value[3])
    {
      obj->obj_flags.value[3]--;
      if (0 == obj->obj_flags.value[3])
      {
        if (obj->carried_by)
          obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->equipped_by)
          obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->carried_by)
          obj->in_obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->equipped_by)
          obj->in_obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
      }
    }
  }

  // 11 = say
  if (cmd != CMD_SAY)
    return eFAILURE;
  if (!ch)
    return eFAILURE;
  if (ch->equipment[HOLD] != obj)
    return eFAILURE;

  half_chop(arg, arg1, junk);

  if (str_cmp("heltlaka", arg1))
    return eFAILURE;

  if (ch->equipment[HOLD]->obj_flags.value[3] && !ch->isImmortalPlayer())
  {
    ch->sendln("The item seems to be recharging.");
    return eSUCCESS;
  }
  act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You quietly whisper 'heltlaka' into your hands.");

  if (!(victim = get_char_vis(ch, junk)))
  {
    ch->sendln("The box somehow seems......confused.");
  }
  else
  {
    spell_full_heal(ch->getLevel(), ch, victim, 0, 0);
    spell_full_heal(ch->getLevel(), ch, victim, 0, 0);
    spell_full_heal(ch->getLevel(), ch, victim, 0, 0);
    // set charge time
    ch->equipment[HOLD]->obj_flags.value[3] = 300;
  }
  act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
  return eSUCCESS;
}

int mana_box(Character *ch, class Object *obj, int cmd, char *arg,
             Character *invoker)
{
  if (cmd)
    return eFAILURE;
  ch = obj->equipped_by;
  if (!ch)
    return eFAILURE;
  if (ch->equipment[HOLD] != obj)
    return eFAILURE;

  if ((GET_MANA(ch) + 8) < GET_MAX_MANA(ch))
    GET_MANA(ch) += 8;

  if (0 == number(0, 10))
    ch->sendln("The box's magical power eases your mind.");

  return eSUCCESS;
}

int fireshield_word(Character *ch, class Object *obj, int cmd, char *arg,
                    Character *invoker)
{
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];

  if (!cmd && obj)
  { // This is where we recharge
    if (obj->obj_flags.value[3])
    {
      obj->obj_flags.value[3]--;
      if (0 == obj->obj_flags.value[3])
      {
        if (obj->carried_by)
          obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->equipped_by)
          obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->carried_by)
          obj->in_obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->equipped_by)
          obj->in_obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
      }
    }
  }

  // 11 = say
  if (cmd != CMD_SAY)
    return eFAILURE;
  if (!ch)
    return eFAILURE;
  if (ch->equipment[HOLD] != obj)
    return eFAILURE;

  half_chop(arg, arg1, junk);

  if (str_cmp("feuerschild", arg1))
    return eFAILURE;

  if (ch->equipment[HOLD]->obj_flags.value[3] && !ch->isImmortalPlayer())
  {
    ch->sendln("The item seems to be recharging.");
    return eSUCCESS;
  }
  act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You quietly whisper 'feuerschild' into your hands.");

  spell_fireshield(ch->getLevel(), ch, ch, 0, 0);
  // set charge time
  ch->equipment[HOLD]->obj_flags.value[3] = 900;

  act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
  return eSUCCESS;
}

int teleport_word(Character *ch, class Object *obj, int cmd, char *arg,
                  Character *invoker)
{
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  Character *victim = nullptr;

  if (!cmd && obj)
  { // This is where we recharge
    if (obj->obj_flags.value[3])
    {
      obj->obj_flags.value[3]--;
      if (0 == obj->obj_flags.value[3])
      {
        if (obj->carried_by)
          obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->equipped_by)
          obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->carried_by)
          obj->in_obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->equipped_by)
          obj->in_obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
      }
    }
  }

  // 11 = say
  if (cmd != CMD_SAY)
    return eFAILURE;
  if (!ch)
    return eFAILURE;
  if (ch->equipment[HOLD] != obj)
    return eFAILURE;

  half_chop(arg, arg1, junk);

  if (str_cmp("sbiadirsivia", arg1))
    return eFAILURE;

  if (ch->equipment[HOLD]->obj_flags.value[3] && !ch->isImmortalPlayer())
  {
    ch->sendln("The item seems to be recharging.");
    return eSUCCESS;
  }
  if (isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("The box doesn't respond.");
    return eSUCCESS;
  }

  act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You quietly whisper 'sbiadirsivia' into your hands.");

  if (!(victim = ch->get_char_room_vis(junk)))
  {
    ch->sendln("The box somehow seems......confused.");
  }
  else
  {
    spell_teleport(ch->getLevel(), ch, victim, 0, 0);
    // set charge time
    ch->equipment[HOLD]->obj_flags.value[3] = 1000;
  }
  act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
  return eSUCCESS;
}

int alignment_word(Character *ch, class Object *obj, int cmd, char *arg,
                   Character *invoker)
{
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];

  if (!cmd && obj)
  { // This is where we recharge
    if (obj->obj_flags.value[3])
    {
      obj->obj_flags.value[3]--;
      if (0 == obj->obj_flags.value[3])
      {
        if (obj->carried_by)
          obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->equipped_by)
          obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->carried_by)
          obj->in_obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->equipped_by)
          obj->in_obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
      }
    }
  }

  // 11 = say
  if (cmd != CMD_SAY)
    return eFAILURE;
  if (!ch)
    return eFAILURE;
  if (ch->equipment[HOLD] != obj)
    return eFAILURE;

  half_chop(arg, arg1, junk);

  if (str_cmp("moralevalore", arg1))
    return eFAILURE;

  act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You quietly whisper 'moralevalore' into your hands.");
  if (ch->equipment[HOLD]->obj_flags.value[3] && !ch->isImmortalPlayer())
  {
    ch->sendln("The item seems to be recharging.");
    return eSUCCESS;
  }

  if (!strcmp("good", junk))
    GET_ALIGNMENT(ch) = 1000;
  else if (!strcmp("evil", junk))
    GET_ALIGNMENT(ch) = -1000;
  else if (!strcmp("neutral", junk))
    GET_ALIGNMENT(ch) = 0;
  else
    ch->sendln("The box somehow seems......confused.");

  // set charge time
  ch->equipment[HOLD]->obj_flags.value[3] = 500;

  act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
  return eSUCCESS;
}

int protection_word(Character *ch, class Object *obj, int cmd, char *arg,
                    Character *invoker)
{
  char junk[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];

  if (!cmd && obj)
  { // This is where we recharge
    if (obj->obj_flags.value[3])
    {
      obj->obj_flags.value[3]--;
      if (0 == obj->obj_flags.value[3])
      {
        if (obj->carried_by)
          obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->equipped_by)
          obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->carried_by)
          obj->in_obj->carried_by->sendln("Your enchanted box seems to be recharged.");
        else if (obj->in_obj && obj->in_obj->equipped_by)
          obj->in_obj->equipped_by->sendln("Your enchanted box seems to be recharged.");
      }
    }
  }

  // 11 = say
  if (cmd != CMD_SAY)
    return eFAILURE;
  if (!ch)
    return eFAILURE;
  if (ch->equipment[HOLD] != obj)
    return eFAILURE;

  half_chop(arg, arg1, junk);

  if (str_cmp("protezione", arg1))
    return eFAILURE;

  if (ch->equipment[HOLD]->obj_flags.value[3] && !ch->isImmortalPlayer())
  {
    ch->sendln("The item seems to be recharging.");
    return true;
  }
  act("$n mutters something into $s hands.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You quietly whisper 'protezione' into your hands.");

  spell_armor(ch->getLevel(), ch, ch, 0, 0);
  spell_bless(ch->getLevel(), ch, ch, 0, 0);
  spell_protection_from_evil(ch->getLevel(), ch, ch, 0, 0);
  spell_invisibility(ch->getLevel(), ch, ch, 0, 0);
  spell_stone_skin(ch->getLevel(), ch, ch, 0, 0);
  spell_resist_fire(ch->getLevel(), ch, ch, 0, 0);
  spell_resist_cold(ch->getLevel(), ch, ch, 0, 0);
  cast_barkskin(ch->getLevel(), ch, 0, SPELL_TYPE_SPELL, ch, 0, 0);

  // set charge time
  ch->equipment[HOLD]->obj_flags.value[3] = 1000;

  act("The $o in $n's hands glows brightly!", ch, obj, 0, TO_ROOM, 0);
  return eSUCCESS;
}

// Proc for handling any 'pull' actions.  Just put it on the object and put in an entry
// Lots of things in here will crash if you remove the zone and stuff, but you just have
// to assume some of these things will work.  If they don't, we got bigger problems anyway
int pull_proc(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd != CMD_PULL) // pull
    return eFAILURE;

  int obj_vnum = obj->vnum;

  switch (obj_vnum)
  {
  case 9529: // DK lever in captain's room
    // unlock the gate
    REMOVE_BIT(DC::getInstance()->world[9531].dir_option[1]->exit_info, EX_LOCKED);
    REMOVE_BIT(DC::getInstance()->world[9532].dir_option[3]->exit_info, EX_LOCKED);
    send_to_room("You hear a large clicking noise.\r\n", 9531, true);
    send_to_room("You hear a large clicking noise.\r\n", 9532, true);
    send_to_room("You hear a large clicking noise.\r\n", ch->in_room, true);
    break;
  case 29203:
    if (DC::getInstance()->obj_index[29202].qty > 0)
    {
      send_to_room("A compartment in the ceiling opens, but is it empty.\r\n", 29258, true);
      break;
    }
    send_to_room("A compartment in the ceiling opens, and a key drops to the ground.\r\n", 29258, true);
    obj_to_room(DC::getInstance()->clone_object(29202), 29258);
    break;
  default:
    ch->sendln("Whatever you pulled doesn't have an entry in the lever pull table.  Tell a god.");
    logf(IMMORTAL, DC::LogChannel::LOG_WORLD, "'Pull' proc on obj %d without entry in proc table. (pull_proc)\r\n", obj_vnum);
    break;
  }

  return eSUCCESS;
}

int szrildor_pass(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  class Object *p;
  // 30097
  if (cmd && cmd == CMD_EXAMINE)
  {
    char target[MAX_INPUT_LENGTH];
    one_argument(arg, target);
    if (!str_cmp(target, "daypass") || !str_cmp(target, "pass"))
    {
      char buf[2000];
      sprintf(buf, "There appears to be approximately %d minutes left of time before the pass expires.\r\n", ((1800 - obj->obj_flags.timer) * 4) / 60);
      ch->send(buf);
      return eSUCCESS;
    }
  }

  if (cmd)
    return eFAILURE;

  if (obj->obj_flags.timer == 0)
  { // Just created - check if this is the first pass in existence and if so, repop zone 161
    bool first = true;
    for (p = DC::getInstance()->object_list; p; p = p->next)
    {
      if (p->vnum == 30097 && p != obj && p->obj_flags.timer != 0) // if any exist that are not at 1800 timer
      {
        first = false;
        break;
      }
    }
    if (first && real_room(30000) != DC::NOWHERE)
    {
      int zone = DC::getInstance()->world[real_room(30000)].zone;
      const auto &character_list = DC::getInstance()->character_list;
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
        if (DC::getInstance()->world[tmp_victim->in_room].zone == zone)
        {
          if (IS_NPC(tmp_victim))
          {
            for (int l = 0; l < MAX_WEAR; l++)
            {
              if (tmp_victim->equipment[l])
                extract_obj(unequip_char(tmp_victim, l));
            }

            while (tmp_victim->carrying)
            {
              extract_obj(tmp_victim->carrying);
            }

            extract_char(tmp_victim, true);
          }
        }
      }

      DC::resetZone(DC::getInstance()->world[real_room(30000)].zone);
    }
  }

  obj->obj_flags.timer++;
  class Object *n;
  if (obj->obj_flags.timer >= 1800)
  {
    // once one expires, ALL expire.
    for (p = DC::getInstance()->object_list; p; p = n)
    {
      n = p->next;
      if (p->vnum == 30097)
      {
        Character *v = nullptr;

        if (p->carried_by)
        {
          v = p->carried_by;
        }
        else if (p->in_obj)
        {
          v = p->in_obj->carried_by;
        }

        if (v)
        {
          v->sendln("The Szrildor daypass crumbles into dust.");
          extract_obj(p); // extract handles all variations of obj_from_char etc

          if (IS_PC(v) && v->in_room && real_room(30000) > 0 && DC::getInstance()->world[v->in_room].zone == DC::getInstance()->world[real_room(30000)].zone && v->in_room != real_room(30000) && v->in_room != real_room(30096))
          {
            if (v->getLevel() >= IMMORTAL)
            {
              act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment but due to immortal magics your head becomes clear.", v, 0, 0, TO_CHAR, 0);
              act("$n begins to look blurry for a moment but due to immortal magics they become sharp again.", v, 0, 0, TO_ROOM, 0);
            }
            else
            {
              act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment, then vanish into thin air", v, 0, 0, TO_CHAR, 0);
              act("$n begins to look blurry for a moment, then winks out of existence with a \"pop\"!", v, 0, 0, TO_ROOM, 0);
              move_char(v, real_room(30000));
              do_look(v, "", CMD_LOOK);

              struct mprog_throw_type *throwitem = nullptr;
              throwitem = (struct mprog_throw_type *)dc_alloc(1, sizeof(struct mprog_throw_type));
              throwitem->target_mob_num = 30033;
              strcpy(throwitem->target_mob_name, "");
              throwitem->data_num = 99;
              throwitem->delay = 0;
              throwitem->mob = true; // This is, surprisingly, a mob
              throwitem->actor = v;
              throwitem->obj = nullptr;
              throwitem->vo = nullptr;
              throwitem->rndm = nullptr;
              throwitem->opt = 0;
              throwitem->var = nullptr;
              throwitem->next = g_mprog_throw_list;
              g_mprog_throw_list = throwitem;
            }
          }
        }
      }
    }
  }
  return eSUCCESS;
}

int szrildor_pass_checks(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  // 30096
  if (cmd)
    return eFAILURE;

  int count = 0;
  const auto &character_list = DC::getInstance()->character_list;
  for (const auto &i : character_list)
  {
    if (IS_NPC(i))
      continue;
    if (!i->in_room)
      continue;
    if (real_room(30000) > 0 && DC::getInstance()->world[i->in_room].zone != DC::getInstance()->world[real_room(30000)].zone)
      continue;
    if (i->getLevel() >= 100)
      continue;
    if (i->in_room == real_room(30000))
      continue;
    if (i->in_room == real_room(30096))
      continue;

    if (!search_char_for_item(i, 30097, false) || (++count) > 4)
    {
      act("Jeff arrives and frowns.\r\n$B$7Jeff says, 'Hey! You don't have a pass. Get the heck outta here!'$R", i, 0, 0, TO_CHAR, 0);
      act("Jeff arrives and frowns at $n.\r\n$B$7Jeff says, 'Hey! You don't have a pass. Get the heck outta here!'$R", i, 0, 0, TO_ROOM, 0);

      if (i->getLevel() >= IMMORTAL)
      {
        act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment but due to immortal magics your head becomes clear.", i, 0, 0, TO_CHAR, 0);
        act("$n begins to look blurry for a moment but due to immortal magics they become sharp again.", i, 0, 0, TO_ROOM, 0);
      }
      else
      {
        act("As your pass expires and crumbles to dust, you begin to feel a bit fuzzy for a moment, then vanish into thin air", i, 0, 0, TO_CHAR, 0);
        act("$n begins to look blurry for a moment, then winks out of existence with a \"pop\"!", i, 0, 0, TO_ROOM, 0);
        move_char(i, real_room(30000));
        do_look(i, "", CMD_LOOK);
      }
    }
  }
  return eSUCCESS;
}

int moving_portals(Character *ch, class Object *obj, int cmd,
                   const char *arg, Character *invoker)
{
  char msg1[MAX_STRING_LENGTH], msg2[MAX_STRING_LENGTH];
  int low, high, room, time, sector = 0;
  if (cmd)
    return eFAILURE;

  switch (obj->vnum)
  {
  case 11300:
  case 11301:
  case 11302:
  case 11303: // Sea of Dreams ships
  case 11304:
  case 11305:
    low = 11300;
    high = 11599;
    time = 60;
    sector = SECT_WATER_NOSWIM;
    sprintf(msg1, "The ship sails off into the distance.");
    sprintf(msg2, "A ship sails in.");
    break;
  case 5911: // Carnival gates
    low = 18100;
    high = 18213;
    time = 75;
    sprintf(msg1, "The carnival breaks off and moves off into the distance.");
    sprintf(msg2, "A band of wagons enter, and set up a carnival here.");
    break;
  default:
    return eFAILURE;
  }
  obj->obj_flags.timer--;

  if (obj->obj_flags.timer <= 0)
  {
    obj->obj_flags.timer = time;
    uint64_t tries = 0;
    while ((room = number(low, high)))
    {
      // Give up after 100 tries
      if (tries++ > 100)
      {
        break;
      }
      bool portal = false;
      if (real_room(room) == DC::NOWHERE)
        continue;
      if (sector)
        if (DC::getInstance()->world[real_room(room)].sector_type != sector)
          continue;
      class Object *o;
      for (o = DC::getInstance()->world[real_room(room)].contents; o; o = o->next_content)
      {
        if (o->isPortal())
        {
          portal = true;
        }
      }

      if (!portal)
        break;
    } // Find a room

    // Give up after 100 tries
    if (tries > 100)
    {
      return eFAILURE;
    }

    send_to_room(msg1, obj->in_room, true);
    obj_from_room(obj);
    obj_to_room(obj, room);
    send_to_room(msg2, obj->in_room, true);
    return eSUCCESS;
  }
  return eFAILURE;
}

// searches for if a certain mob is alive.  If so, you cannot use magic in this room.
int no_magic_while_alive(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd)
    return eFAILURE;

  if (obj->in_room == DC::NOWHERE)
    return eFAILURE;

  Character *vict = DC::getInstance()->world[obj->in_room].people;

  for (; vict; vict = vict->next_in_room)
  {
    if (IS_NPC(vict) && (DC::getInstance()->mob_index[vict->mobdata->nr].virt == 9544
                         // to add a new mob to this list, just add || and the next check
                         ))
      break;
  }

  if (vict)
  {
    if (!isSet(DC::getInstance()->world[obj->in_room].room_flags, NO_MAGIC))
      send_to_room("With an audible whoosh, the flow of magic is sucked from the room.\r\n", obj->in_room);
    SET_BIT(DC::getInstance()->world[obj->in_room].room_flags, NO_MAGIC);
  }
  else
  {
    if (isSet(DC::getInstance()->world[obj->in_room].room_flags, NO_MAGIC))
      send_to_room("With a large popping noise, the flow of magic returns to the room.\r\n", obj->in_room);
    REMOVE_BIT(DC::getInstance()->world[obj->in_room].room_flags, NO_MAGIC);
  }
  return eSUCCESS;
}

// Send a message to all rooms on a boat
void send_to_boat(int boat, char *message)
{
  switch (boat)
  {
  case 9531: // dk boat
    send_to_room(message, 9522, true);
    send_to_room(message, 9523, true);
    send_to_room(message, 9524, true);
    send_to_room(message, 9525, true);
    send_to_room(message, 9587, true);
    break;
  default:
    break;
  }
}

// How many stops, order of stops (will reverse order on way back)
int dk_boat[] =
    {8, 9593, 9510, 9509, 9508, 9521, 9507, 9506, 9505};

// boat values [pos in travel list] [timer] [boat-entry-room] [time-between-moves]
// pos in travel list is NEGATIVE value on return trip

// handles the movement of ocean-going boats or ferries
// make sure you also look at "leave_boat_proc" if you use this

// Also, make sure you update "send_to_boat" for the messages
//
int boat_proc(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd && cmd != CMD_ENTER)
    return eFAILURE;

  if (obj->in_room == DC::NOWHERE)
    return eFAILURE; // someone loaded me

  // figure out which boat I am
  int *boat_list = nullptr;
  switch (obj->vnum)
  {
  case 9531:
    boat_list = dk_boat;
    break;
  default:
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Illegal boat proc.  Item %d.", obj->vnum);
    break;
  }

  if (cmd)
  {
    // get on the boat
    act("$n boldly boards $p.", ch, obj, 0, TO_ROOM, 0);
    act("You board $p.", ch, obj, 0, TO_CHAR, 0);
    move_char(ch, obj->obj_flags.value[2]);
    act("$n climbs aboard.", ch, 0, 0, TO_ROOM, 0);
    do_look(ch, "", CMD_DEFAULT);
    return eSUCCESS;
  }

  // timer pulsed.  Update
  obj->obj_flags.value[1]--;

  // boat pulsed.  Time to move
  if (!obj->obj_flags.value[1])
  {
    int move_to;
    // reset timer
    obj->obj_flags.value[1] = obj->obj_flags.value[3];
    if (obj->obj_flags.value[0] < 0) // on way back
    {
      obj->obj_flags.value[0]++;
      move_to = boat_list[(obj->obj_flags.value[0] * -1)];
      if (obj->obj_flags.value[0] == -1) // at beginning
      {
        obj->obj_flags.value[0] = 1;
        send_to_boat(obj->vnum, "The ship docks at its destination.\r\n");
      }
    }
    else
    {
      obj->obj_flags.value[0]++;
      move_to = boat_list[obj->obj_flags.value[0]];
      if (obj->obj_flags.value[0] == boat_list[0]) // at beginning
      {
        obj->obj_flags.value[0] *= -1;
        send_to_boat(obj->vnum, "The ship docks at its destination.\r\n");
      }
    }
    send_to_room("The ship sails away.\r\n", obj->in_room, true);
    send_to_boat(obj->vnum, "The ship sails onwards.\r\n");
    obj_from_room(obj);
    obj_to_room(obj, move_to);
    send_to_room("A ship sails in.\r\n", obj->in_room, true);
  }
  return eSUCCESS;
}

// Depending on which boat we're on, exit the boat if we're not 'at sea'
int leave_boat_proc(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  Object *obj2;
  int i;

  if (cmd != CMD_LEAVE) // leave
    return eFAILURE;

  if (obj->in_room == DC::NOWHERE)
    return eFAILURE; // someone loaded me

  // switch off depending on what item we are
  switch (obj->vnum)
  {
  case 9532: // dk boat ramp
    // find the dk boat (9531)
    i = 9531;
    for (obj2 = DC::getInstance()->object_list; obj2; obj2 = obj2->next)
    {
      if (obj2->vnum == i)
        break;
    }

    if (obj2 == nullptr)
    {
      ch->sendln("Cannot find your boat obj.  BUG.  Tell a god.");
      return eSUCCESS;
    }

    if (obj2->in_room == dk_boat[1])
    {
      act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
      act("You disembark from $p.", ch, obj2, 0, TO_CHAR, 0);
      move_char(ch, obj2->in_room);
      act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
      do_look(ch, "", CMD_DEFAULT);
      return eSUCCESS;
    }

    if (obj2->in_room == dk_boat[dk_boat[0]])
    {
      act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
      act("You disembark from $p.", ch, obj2, 0, TO_CHAR, 0);
      move_char(ch, obj2->in_room);
      act("$n disembarks from $p.", ch, obj2, 0, TO_ROOM, 0);
      do_look(ch, "", CMD_DEFAULT);
      return eSUCCESS;
    }

    ch->sendln("You can't just leave the ship in the middle of the Blood Sea!");
    return eSUCCESS;
    break;
  default:
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Illegal boat proc.  Item %d.", obj->vnum);
    break;
  }

  return eSUCCESS;
}

#define BONEWRACK_ROOM 9597
#define GAIOT_AVATAR 9622

// This proc waits for players to enter the room.  Once they do, it echos messages
// and loads the mob into the room.  Can work for multiple messages with a 'wait' state
// obj values:  [current pulse] [unused] [unused] [unused]
//
int mob_summoner(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  Character *vict;

  if (cmd)
    return eFAILURE;

  if (obj->in_room == DC::NOWHERE)
    return eFAILURE; // someone loaded me

  // see if we have any players in room
  for (vict = DC::getInstance()->world[obj->in_room].people; vict; vict = vict->next_in_room)
    if (IS_PC(vict))
      break;

  // no?  reset pulse state and get out
  if (!vict)
  {
    obj->obj_flags.value[0] = 0;
    return eSUCCESS;
  }

  switch (obj->in_room)
  {
  case BONEWRACK_ROOM:
    // find bonewrack
    vict = get_mob_vnum(9535);
    if (!vict || vict->in_room == BONEWRACK_ROOM)
      return eSUCCESS;

    switch (obj->obj_flags.value[0])
    {
    case 0:
      send_to_room("The shadows in the room begin to shift and slide in tricks of the light.\r\n", BONEWRACK_ROOM, true);
      break;
    case 1:
      send_to_zone("A loud roar echos audibly through the entire kingdom.\r\n", DC::getInstance()->world[obj->in_room].zone);
      break;
    case 2:
      send_to_room("The dragon $B$2Bonewrack$R flies in from above!\n\r", BONEWRACK_ROOM, true);
      move_char(vict, BONEWRACK_ROOM);
      obj->obj_flags.value[0] = 0;
      break;
    default:
      break;
    }
    break;

  case GAIOT_AVATAR:
    // find avatar
    vict = get_mob_vnum(9526);
    if (!vict || vict->in_room == GAIOT_AVATAR)
      return eSUCCESS;

    switch (obj->obj_flags.value[0])
    {
    case 0:
      send_to_room("In the distance a winged creature can be seen flying towards you.\r\n", GAIOT_AVATAR, true);
      break;
    case 1:
      send_to_room("The winged creature flies closer and closer.\r\n", GAIOT_AVATAR, true);
      break;
    case 2:
      send_to_room("The creature shatters in illusion!\n\r", GAIOT_AVATAR, true);
      move_char(vict, GAIOT_AVATAR);
      obj->obj_flags.value[0] = 0;
      break;
    default:
      break;
    }
    break;

  default:
    break;
  }

  obj->obj_flags.value[0]++;

  return eSUCCESS;
}

// Takes care of lighting back up a room when globe of darkness wears off
// values:  [time left] [how dark] [unused] [unused]
//
int globe_of_darkness_proc(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd)
    return eFAILURE;

  if (obj->in_room == DC::NOWHERE)
    return eFAILURE; // someone loaded me

  if (obj->obj_flags.value[0] < 1)
  {
    // time to kill myself
    DC::getInstance()->world[obj->in_room].light += obj->obj_flags.value[1]; // light back up
    send_to_room("The globe of darkness fades brightening the room some.\r\n", obj->in_room, true);
    extract_obj(obj);
  }
  else
    obj->obj_flags.value[0]--;

  return eSUCCESS;
}

int hornoplenty(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd)
    return eFAILURE;

  if (!obj || obj->contains)
    return eFAILURE;

  if (number(0, 100))
    return eFAILURE;

  class Object *newobj = nullptr;
  rnum_t objnum = 3170; // chewy tuber
  if (objnum < 0)
  {
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "Horn o plenty load obj incorrent.");
    return eFAILURE;
  }

  newobj = DC::getInstance()->clone_object(objnum);

  obj_to_obj(newobj, obj);
  return eSUCCESS;
}

int gl_dragon_fire(Character *ch, class Object *obj, int cmd, char *arg,
                   Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)
    return eFAILURE;

  if (number(1, 100) > 5)
    return eFAILURE;

  ch->sendln("The head of your dragon staff animates and breathes $B$4fire$R all around you!");
  act("The head of the dragon staff in $n's hands animates and begins to breath fire!",
      ch, obj, 0, TO_ROOM, 0);

  return cast_fire_breath(10, ch, "", SPELL_TYPE_SPELL, ch->fighting, 0, 0);
}

int dk_rend(Character *ch, class Object *obj, int cmd, const char *arg,
            Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)
    return eFAILURE;

  if (number(1, 100) > 5)
    return eFAILURE;

  act("The $o rips deeply into $N rending $S body painfully!",
      ch, obj, ch->fighting, TO_ROOM, 0);

  GET_HIT(ch->fighting) /= 2;
  if (GET_HIT(ch->fighting) < 1)
    ch->fighting->setHP(1);

  return eSUCCESS;
}

int magic_missile_boots(Character *ch, class Object *obj, int cmd, char *arg,
                        Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WEAR_FEET] != obj)
    return eFAILURE;

  if (number(0, 1))
    return eFAILURE;

  act("The $o around $n's feet glows briefly and releases a magic missle spell!",
      ch, obj, ch->fighting, TO_ROOM, 0);
  ch->sendln("Your boots glow briefly and release a magic missle spell!");

  return spell_magic_missile((ch->getLevel() / 2), ch, ch->fighting, 0, 0);
}

int shield_combat_procs(Character *ch, class Object *obj, int cmd, const char *arg,
                        Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WEAR_SHIELD] != obj)
    return eFAILURE;

  switch (obj->vnum)
  {
  case 2715: // shield of ares
    if (number(0, 3))
      return eFAILURE;

    act("$n's $o glows yellow charging up with electrical energy.", ch, obj, ch->fighting, TO_ROOM, 0);
    ch->sendln("Your shield glows yellow as it charges up with electrical energy.");
    return spell_lightning_bolt((ch->getLevel() / 2), ch, ch->fighting, 0, 0);
    break;
  case 555: // wicked boneshield
    if (number(0, 9))
      return eFAILURE;
    act("The spikes $n's $o glimmer brightly.", ch, obj, ch->fighting, TO_ROOM, 0);
    ch->sendln("The spikes on your shield glimmer brightly.");
    return spell_cause_critical(ch->getLevel(), ch, ch->fighting, 0, 0);
    break;
  case 5208: // thalos beholder shield
    if (number(0, 4))
      return eFAILURE;

    act("$n's $o begins to tremble violently upon contact with $N.", ch, obj, ch->fighting, TO_ROOM, NOTVICT);
    act("$n's $o begins to tremble violently upon contact with you!", ch, obj, ch->fighting, TO_VICT, 0);
    ch->sendln("Your shield begins to violently shake after the hit!");
    return spell_cause_serious((ch->getLevel() / 2), ch, ch->fighting, 0, 0);
    break;

  default:
    break;
  }

  return eFAILURE;
}

int generic_weapon_combat(Character *ch, class Object *obj, int cmd, char *arg,
                          Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (!obj || (ch->equipment[WIELD] != obj &&
               ch->equipment[SECOND_WIELD] != obj))
    return eFAILURE;

  if (!DC::getInstance()->obj_index.contains(obj->vnum))
  {
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "generic_weapon_combat: illegal obj->vnum");
    return eFAILURE;
  }

  switch (obj->vnum)
  {
  case 16903: // valhalla hammer
    if (number(1, 100) > GET_DEX(ch) / 4)
      break;
    ch->sendln("The hammer begins to hum and strikes out with the power of Thor!");
    act("$n's hammer begins to hum and strikes out with the power of Thor!",
        ch, obj, 0, TO_ROOM, 0);
    return spell_lightning_bolt((ch->getLevel() / 2), ch, ch->fighting, 0, 0);

  case 19327: // EC Icicle
    if (number(1, 100) < GET_DEX(ch) / 4)
      break;
    ch->sendln("The Icicle begins to pulse repidly...");
    act("$n's $o begins to pulse rapidly...",
        ch, obj, 0, TO_ROOM, 0);
    return spell_icestorm((ch->getLevel() / 2), ch, ch->fighting, 0, 0);

  default:
    ch->sendln("Weapon with invalid generic_weapon_combat, tell an Immortal.");
    break;
  }
  return eFAILURE;
}

// item players can buy to find the ToHS
int TOHS_locator(Character *ch, class Object *obj, int cmd, const char *arg,
                 Character *invoker)
{
  Object *victim = nullptr;

  if (cmd != CMD_PUSH) // push
    return eFAILURE;

  if (!strstr(arg, "button"))
    return eFAILURE;

  act("$n pushes a small button then holds a looking glass to $s face.", ch, 0, 0, TO_ROOM, INVIS_NULL);
  ch->sendln("You push the small button and then hold the looking glass to your face peering through it.\r\n");

  // 1406 is the portal 'rock' you enter to get to Tohs
  rnum_t searchnum = 1406;

  for (victim = DC::getInstance()->object_list; victim; victim = victim->next)
    if (victim->vnum == searchnum)
      break;

  if (!victim || victim->in_room == DC::NOWHERE) // couldn't find it?!
  {
    ch->sendln("The tower seems to not be there!?!!");
    return eSUCCESS;
  }

  searchnum = ch->in_room;
  move_char(ch, victim->in_room);
  do_look(ch, "", CMD_DEFAULT);
  move_char(ch, searchnum);

  return eSUCCESS;
}

/*int no_magic_item(Character *ch, class Object *obj, cmd, char
*arg, Character *invoker)
{ // mobdata last_direction
   if (cmd)  // Not activated through commands..
     return eFAILURE;
//   if (
}
*/
int gotta_dance_boots(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{
  void make_person_dance(Character * ch);

  if (cmd)
    return eFAILURE;

  if (!obj->equipped_by)
    return eFAILURE;

  if (number(0, 3))
    return eFAILURE;

  act("$n eyes widen and $e begins to shake violently.", obj->equipped_by, 0, 0, TO_ROOM, INVIS_NULL);
  obj->equipped_by->sendln("Your boots grasp violently to your legs and rhythmic urges flood your body.");
  do_say(obj->equipped_by, "I...I.....I've gotta dance!!!!", CMD_DEFAULT);
  make_person_dance(obj->equipped_by);
  obj->equipped_by->sendln("You slump back down, exhausted.");
  if (obj->equipped_by->getLevel() <= MORTAL)
    WAIT_STATE(obj->equipped_by, DC::PULSE_VIOLENCE * 3);

  return eSUCCESS;
}

int random_dir_boots(Character *ch, class Object *obj, int cmd, const char *arg,
                     Character *invoker)
{
  if (cmd)
    return eFAILURE;

  if (!obj->equipped_by)
    return eFAILURE;

  if (number(0, 3))
    return eFAILURE;

  act("$n boots just keep on going!", obj->equipped_by, 0, 0, TO_ROOM, INVIS_NULL);
  obj->equipped_by->sendln("Your boots just keep on running!");

  char dothis[32];

  strcpy(dothis, dirs[number(0, 5)]);

  return obj->equipped_by->command_interpreter(dothis);
}

// WARNING - uses obj_flags.value[3] to store stuff

int noremove_eq(Character *ch, class Object *obj, int cmd, const char *arg,
                Character *invoker)
{
  if (cmd && cmd != CMD_REMOVE)
    return eFAILURE;
  if (!obj->equipped_by)
    return eFAILURE;
  if (!cmd && obj->obj_flags.value[3] > 0)
  {
    obj->obj_flags.value[3]--;
    if (!obj->obj_flags.value[3])
      obj->equipped_by->send(QStringLiteral("The %1 loses it's grip on your body.\r\n").arg(obj->short_description));
    return eSUCCESS;
  }
  if (!cmd && obj->obj_flags.value[3] <= 0)
  {
    if (number(0, 4))
      return eSUCCESS;
    csendf(obj->equipped_by, "The %s clamps down onto your body locking your equipment in place!\r\n",
           obj->short_description);
    obj->obj_flags.value[3] = 5;
    return eSUCCESS;
  }
  if (obj->obj_flags.value[3] > 0)
  {
    obj->equipped_by->send(QStringLiteral("The %1 refuses to let you remove anything!\r\n").arg(obj->short_description));
    return eSUCCESS;
  }
  return eFAILURE;
}

int glove_combat_procs(Character *ch, class Object *obj, int cmd, char *arg,
                       Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WEAR_HANDS] != obj)
    return eFAILURE;

  int dam;

  switch (obj->vnum)
  {
  case 9806: // muddy gloves
    if (number(0, 17))
      return eFAILURE;

    dam = dice(1, ch->getLevel());
    act("The mud on $n's gloves spoils $N's flesh causing boils.", ch, obj, ch->fighting, TO_ROOM, NOTVICT);
    act("The mud on $n's gloves spoils your flesh causing boils.", ch, obj, ch->fighting, TO_VICT, 0);
    ch->sendln("The mud on your gloves spoils the flesh of your enemy.");
    return damage(ch, ch->fighting, dam, TYPE_MAGIC, TYPE_UNDEFINED, 0);
    break;

  case 4818:
    if (number(0, 19))
      return eFAILURE;
    return spell_burning_hands(ch->getLevel(), ch, ch->fighting, nullptr, 50);
  case 4819:
    if (number(0, 19))
      return eFAILURE;
    return spell_chill_touch(ch->getLevel(), ch, ch->fighting, nullptr, 50);
  case 21718:
    if (ch->affected_by_spell(BASE_SETS + SET_SAIYAN))
    {
      if (number(0, 19))
        return eFAILURE;
      return spell_sparks(ch->getLevel(), ch, ch->fighting, nullptr, 0);
    }
    break;
  case 19503: // Gloves of the Dreamer
    if (number(0, 17))
      return eFAILURE;

    act("$n's $o momentarily pulse with a $B$7white light$R.", ch, obj, 0, TO_ROOM, 0);
    ch->sendln("Your gloves momentarily pulse with a $B$7white light$R.");
    return spell_cure_serious(30, ch, ch, 0, 50);
    break;
  case 506:
    if (number(0, 33) || !ch->fighting)
      return eFAILURE;
    act("$n's $o begin pulse with a blinding white light for a moment.", ch, obj, 0, TO_ROOM, 0);
    ch->sendln("Your gloves begin to pulse with a blinding white light for a moment.");
    return spell_souldrain(60, ch, ch->fighting, 0, 100);
  default:
    break;
  }

  return eFAILURE;
}

std::vector<std::string> sword_non_combat;
std::vector<std::string> sword_class_specific_combat;
std::vector<std::string> sword_combat;

void do_talking_init()
{

  std::string buf;

  /* GENERIC NONCOMBAT MESSAGE */
  buf = "Who wrote such crappy dialogue for me? Pirahna?";
  sword_non_combat.push_back(buf);
  buf = "Etala is my bitch.";
  sword_non_combat.push_back(buf);
  buf = "I used to get drunk all the time with the Avenger until it found religion and got all holy.";
  sword_non_combat.push_back(buf);
  buf = "Do you think these gems on my hilt make me look fat?";
  sword_non_combat.push_back(buf);
  buf = "*sigh*";
  sword_non_combat.push_back(buf);
  buf = "I'm bored. Go kill something.....";
  sword_non_combat.push_back(buf);
  buf = "Dude... just this once. Hold me over your head and shout, \"By the power of Grayskull!\"";
  sword_non_combat.push_back(buf);
  buf = "I wish my hilt could do that cool curly thing like in Thundercats.";
  sword_non_combat.push_back(buf);
  buf = "Ah, I remember the old days fighting with Musashi.";
  sword_non_combat.push_back(buf);
  buf = "The guy that bought me got the material really cheap. He said it was a \"steel\". HA!";
  sword_non_combat.push_back(buf);
  buf = "Really... I'm not really with this guy, we just sorta fighting together at the moment.";
  sword_non_combat.push_back(buf);
  buf = "Would you just stab him in the back and put me out of this misery?";
  sword_non_combat.push_back(buf);
  buf = "10,000 $B$5gold$R if you pinch his ass right now.";
  sword_non_combat.push_back(buf);
  buf = "Seriously. How can you stand grouping with this moron?";
  sword_non_combat.push_back(buf);
  buf = "Psst! RK him if he goes AFK.";
  sword_non_combat.push_back(buf);
  buf = "Pretend to be a chick, get his password, log in as him and just sacrifice me. Please! I'm begging.";
  sword_non_combat.push_back(buf);
  buf = "Hey, later on lets stop by and get the latest copy of Sword & Dagger. They've got a hot centerfold this month.";
  sword_non_combat.push_back(buf);
  buf = "I just love that Gogo Yubari character; \"Or is it I... who has penetrated you?\" Classic!";
  sword_non_combat.push_back(buf);
  buf = "When I was a kid, I wanted to grow up to be a scimitar so I could be a pirate.";
  sword_non_combat.push_back(buf);
  buf = "If you don't wipe me down before putting me in the scabbard, I'm going to slice your throat while you're sleeping.";
  sword_non_combat.push_back(buf);
  buf = "I miss the old days with Mom and Dad back when I was just a little dagger.";
  sword_non_combat.push_back(buf);
  buf = "When they say a sword is an extension of your body, they mean your arm, not penis, you ass.";
  sword_non_combat.push_back(buf);
  buf = "I'm the sword that killed Enigo Montoya's father.";
  sword_non_combat.push_back(buf);
  buf = "Dude.... I totally think Bedbug likes you.";
  sword_non_combat.push_back(buf);
  buf = "That whole sword in the stone thing is bull. Like anyone would ever do that.";
  sword_non_combat.push_back(buf);
  buf = "Some swords prefer the waist, and some swords prefer the back. I'm a breast-sword myself.";
  sword_non_combat.push_back(buf);
  buf = "Fuck sakes, with how much you use me, I may as well team up with a spoon and a fork and head to Uncle Juan's!";
  sword_non_combat.push_back(buf);
  buf = "You know that sweet looking sword Mel Gibson holds in Braveheart? Yeah... I've tagged that.";
  sword_non_combat.push_back(buf);
  buf = "Devil-fang Cestus used to complain to me all the time about it's owner. Hairy palms.";
  sword_non_combat.push_back(buf);
  buf = "Today is one of those days when you just wanna go out and disembowel some bunnies.";
  sword_non_combat.push_back(buf);
  buf = "Hey... Lets head over to Hyperborea and listen to Frosty.";
  sword_non_combat.push_back(buf);

  /* GENERIC COMBAT MESSAGES*/
  buf = "After this fight, I want you to use me to play air-guiter. Rock on!";
  sword_combat.push_back(buf);
  buf = "If you're happy and you know it, shove me deep! *shove* *shove* ...";
  sword_combat.push_back(buf);
  buf = "No no idiot, swing at his other side!";
  sword_combat.push_back(buf);
  buf = "I'm going to wear your entrails like a scarf.";
  sword_combat.push_back(buf);
  buf = "At least you can do one thing. If you fought as well as you performed in bed, I'd have a new owner by now.";
  sword_combat.push_back(buf);
  buf = "Ooooooh yeah... tickle a little lower under the hilt... OH! I like that!";
  sword_combat.push_back(buf);
  buf = "You want me to penetrate THAT?!?";
  sword_combat.push_back(buf);
  buf = "Why don't you try shoving your own 'sword' in there first? I don't know where they've been either.";
  sword_combat.push_back(buf);
  buf = "Ohhh gross.... do you know how hard this blood is to get off?";
  sword_combat.push_back(buf);
  buf = "Swordfighting is like a dance and *I* get a partner with two left feet.";
  sword_combat.push_back(buf);
  buf = "Your liver is going to taste lovely with some fava beans and a nice Chianti!";
  sword_combat.push_back(buf);
  buf = "That's MISTER Ghaerad to you, punk!";
  sword_combat.push_back(buf);
}

int chaosblade(Character *ch, class Object *obj, int cmd, const char *arg,
               Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!obj->equipped_by)
    return eFAILURE;

  if ((++obj->obj_flags.timer) > 4)
  {
    int dam = number(175, 250);
    obj->obj_flags.timer = 0;
    if (GET_HIT(obj->equipped_by) * 30 / 1000 > dam)
    {
      dam = GET_HIT(obj->equipped_by) * 30 / 1000;
    }
    if (dam >= GET_HIT(obj->equipped_by))
    {
      dam = GET_HIT(obj->equipped_by) - 1;
    }
    if (dam > 0)
    {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "%d", dam);
      send_damage("The Chaos Blade hungers!  You are drained for | damage.", obj->equipped_by, 0, 0, buf, "The Chaos Blade hungers!  You feel your life force being drained!", TO_CHAR);
      send_damage("The katana in $n's hand pulses with a dull red glow as it drains their life force for | damage!", obj->equipped_by, 0, 0, buf, "The katana in $n's hand pulses with a dull red glow as it drains their life force!", TO_ROOM);
      obj->equipped_by->removeHP(dam);
    }
  }
  return eSUCCESS;
}

int rubybrooch(Character *ch, class Object *obj, int cmd, const char *arg,
               Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!obj->equipped_by)
    return eFAILURE;

  if (obj->obj_flags.timer == 39)
  {
    obj->equipped_by->sendln("You feel the ruby brooch's grip upon your neck loosen slightly.");
  }
  ++obj->obj_flags.timer;

  if ((obj->obj_flags.timer % 4) == 0)
  {
    if ((obj->obj_flags.timer) == 44)
    {
      obj->obj_flags.timer = 40; // 40+ = can be removed
    }
    int dam = number(75, 150);
    if (dam >= GET_HIT(obj->equipped_by))
    {
      dam = GET_HIT(obj->equipped_by) - 1;
    }
    if (dam > 0)
    {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "%d", dam);
      send_damage("The ruby brooch squeezes your neck painfully for | damage!", obj->equipped_by, 0, 0, buf, "The ruby brooch squeezes your neck painfully!", TO_CHAR);
      send_damage("A ruby brooch constricts $n's neck for | damage and they cough violently.", obj->equipped_by, 0, 0, buf, "A ruby brooch constricts $n's neck and they cough violently.", TO_ROOM);
      obj->equipped_by->removeHP(dam);
    }
  }
  return eSUCCESS;
}

int eternitystaff(Character *ch, class Object *obj, int cmd, const char *arg,
                  Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!obj->equipped_by)
    return eFAILURE;

  if ((++obj->obj_flags.timer) > 4)
  {
    int dam = number(175, 200);
    obj->obj_flags.timer = 0;
    if (GET_MANA(obj->equipped_by) * 30 / 1000 > dam)
    {
      dam = GET_MANA(obj->equipped_by) * 30 / 1000;
    }

    if (dam >= GET_MANA(obj->equipped_by))
    {
      dam = GET_MANA(obj->equipped_by) - 1;
    }
    if (dam > 0)
    {
      GET_MANA(obj->equipped_by) -= dam;
      obj->equipped_by->send(QStringLiteral("Your body hemorrhages %1 mana as you struggle to control The Eternity Staff.\r\n").arg(dam));

      act("$n is wracked by magical energies!", obj->equipped_by, 0, 0, TO_ROOM, 0);
    }
  }
  return eSUCCESS;
}

int talkingsword(Character *ch, class Object *obj, int cmd, const char *arg,
                 Character *invoker)
{
  Character *vict = nullptr;
  int unequip = -1;
  static bool init_done = false;
  if (cmd)
  {
    if (cmd == CMD_GAG && (!str_cmp(arg, " sword") || !str_cmp(arg, " ghaerad")) && obj->equipped_by)
    {
      char buf2[MAX_STRING_LENGTH] = "$B$7Ghaerad, Sword of Legends says, '";

      if (isSet(obj->obj_flags.more_flags, ITEM_TOGGLE))
      {
        REMOVE_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
        strcat(buf2, "And I'm back! Couldn\'t live without me eh?'$R\n\r");
        send_to_room(buf2, obj->equipped_by->in_room, true);
      }
      else
      {
        SET_BIT(obj->obj_flags.more_flags, ITEM_TOGGLE);
        strcat(buf2, "Fine, I will keep quiet for a while, but you will miss me!'$R\n\r");
        send_to_room(buf2, obj->equipped_by->in_room, true);
      }
      return eSUCCESS;
    }
    else
    {
      return eFAILURE;
    }
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_TOGGLE))
  {
    return eFAILURE;
  }
  if (!init_done)
  {
    init_done = true;
    do_talking_init();
  }

  if (obj->equipped_by)
    vict = obj->equipped_by;

  if (!vict)
    return eFAILURE;

  if (vict->player && vict->desc == nullptr)
  {
    return eFAILURE;
  }

  if (obj->obj_flags.value[0] > 0)
  {
    obj->obj_flags.value[0]--;
  }

  if (obj->obj_flags.value[0] == 0)
  {
    std::vector<std::string> tmp;
    std::string buf;

    if (GET_POS(vict) == position_t::FIGHTING)
    {
      tmp = sword_combat;
      if (IS_NPC(vict->fighting) && vict->fighting->getLevel() > 99)
      {
        buf = "Are you sure you can win this one? I mean.... I'll be ok, but I'm pretty sure you're screwed.";
        tmp.push_back(buf);
      }
      level_diff_t level_difference = vict->getLevel() - vict->fighting->getLevel();
      if (IS_NPC(vict->fighting) && level_difference > 40)
      {
        buf = "Oh come on... this is fuckin' embarrassing...";
        tmp.push_back(buf);
        unequip = tmp.size() - 1;
      }
      switch (GET_RACE(vict->fighting))
      {
      case RACE_HOBBIT:
        buf = "Oh, that is just gross! Like, seriously, shave your feet some time.";
        tmp.push_back(buf);
        break;
      case RACE_PIXIE:
        buf = "Why are you using me to fight a pixie? Get a fly swatter or something.";
        tmp.push_back(buf);
        break;
      case RACE_ELVEN:
        buf = "I hate elves. Fuckin' skinnier than I am. Go eat a burger.";
        tmp.push_back(buf);
        break;
      case RACE_DWARVEN:
        buf = "Cut him off at the knees! Oh, wait... looks like someone already did.";
        tmp.push_back(buf);
        break;
      case RACE_TROLL:
        buf = "I hate trolls. This smell isn't going to go away for weeks. ARGH!";
        tmp.push_back(buf);
        break;
      }
      switch (GET_CLASS(vict->fighting))
      {
      case CLASS_MONK:
        buf = "LOVE MONK AND STUN!";
        tmp.push_back(buf);
        break;
      case CLASS_WARRIOR:
        buf = "This is horrible... who taught you to fight? That idiot Gireth?";
        tmp.push_back(buf);
        break;
      case CLASS_THIEF:
        buf = "Look who brought a knife to a sword fight? *snicker*";
        tmp.push_back(buf);
        break;
      case CLASS_PALADIN:
        buf = "I hate fighting people in platemail. I get such a headache.";
        tmp.push_back(buf);
        break;
      case CLASS_MAGE:
        buf = "Hey magey, you'd better lose or I'm going to have the gods nerf you again.";
        tmp.push_back(buf);
        break;
      }
    }
    else
    {
      tmp = sword_non_combat;
      if (GET_POS(vict) == position_t::SLEEPING)
      {
        buf = "Hey... someone steal me already... this guy sucks...";
        tmp.push_back(buf);
        buf = "Ohhh, if I only had some vasoline and arms to move...";
        tmp.push_back(buf);
        buf = "Someone get some markers and write crap on him.";
        tmp.push_back(buf);
        buf = "Quick! Take pictures of him with your bare ass next to his head!";
        tmp.push_back(buf);
        buf = "Oh come on.... after all the ideas from DC parties none of you is gonna fuck with him?";
        tmp.push_back(buf);
      }

      if (vict->in_room == START_ROOM) // tavern
      {
        buf = "Are you going to just sit in the Tavern all day? Great... I'm owned by Avalios.";
        tmp.push_back(buf);
      }
      if (isSet(DC::getInstance()->world[vict->in_room].room_flags, SAFE))
      {
        buf = "Oh... I suppose we're just going to sit here and gossip for the next few hours, huh?";
        tmp.push_back(buf);
        buf = "While we're here, why don't we just talk about how badass we are on gossip....";
        tmp.push_back(buf);
      }
      switch (DC::getInstance()->world[vict->in_room].sector_type)
      {
      case SECT_UNDERWATER:
        buf = "Aww man, I hate being under water. I'll rust!";
        tmp.push_back(buf);
        buf = "What the fuck, do I LOOK like a harpoon??";
        tmp.push_back(buf);
        break;
      case SECT_FOREST:
        buf = "If you try to use me like an axe on one of these trees, I am going to shove myself up your ass.";
        tmp.push_back(buf);
        break;
      case SECT_MOUNTAIN:
        buf = "I like mountains. The iron I was made of comes from a mountain. Know what else is made from iron? Trains.";
        tmp.push_back(buf);
        break;
      }
    }

    if (!tmp.empty())
    {
      int rnd = number((quint64)0, (quint64)tmp.size() - 1);
      char buf2[MAX_STRING_LENGTH] = "$B$7Ghaerad, Sword of Legends says, '";
      strcat(buf2, tmp[rnd].c_str());
      strcat(buf2, "'$R\n\r");
      send_to_room(buf2, vict->in_room, true);

      if (rnd == unequip)
      {

        if (vict->equipment[WIELD] && vict->equipment[WIELD]->vnum == 27997)
        {

          act("Your $p unequips itself.",
              vict, vict->equipment[WIELD], 0, TO_CHAR, 0);
          act("$n stops using $p.", vict, vict->equipment[WIELD], 0, TO_ROOM, INVIS_NULL);
          obj_to_char(unequip_char(vict, WIELD), vict);
          if (vict->equipment[SECOND_WIELD])
          {
            act("You move your $p to be your primary weapon.", vict, vict->equipment[SECOND_WIELD], 0, TO_CHAR, INVIS_NULL);
            act("$n moves $s $p to be $s primary weapon.", vict, vict->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
            class Object *weapon;
            weapon = unequip_char(vict, SECOND_WIELD);
            equip_char(vict, weapon, WIELD);
          }
        }
        else if (vict->equipment[SECOND_WIELD] && vict->equipment[SECOND_WIELD]->vnum == 27997)
        {

          act("Your $p unequips itself.",
              vict, vict->equipment[SECOND_WIELD], 0, TO_CHAR, 0);
          act("$n stops using $p.", vict, vict->equipment[SECOND_WIELD], 0, TO_ROOM, INVIS_NULL);
          obj_to_char(unequip_char(vict, SECOND_WIELD), vict);
        }
      }
    }
    obj->obj_flags.value[0] = number(8, 10);
  }

  return eFAILURE;
}

// Fun item to give to mortals...ticks for a while and then when it blows
// up BOOM!!!
int hot_potato(Character *ch, class Object *obj, int cmd, const char *arg,
               Character *invoker)
{
  auto &arena = DC::getInstance()->arena_;
  int dropped = 0;
  Character *vict = nullptr;

  if (obj->equipped_by)
    vict = obj->equipped_by;
  if (obj->carried_by)
    vict = obj->carried_by;
  if (obj->in_obj && obj->in_obj->carried_by)
    vict = obj->in_obj->carried_by;
  if (obj->in_obj && obj->in_obj->equipped_by)
    vict = obj->in_obj->equipped_by;
  if (!vict)
    return eFAILURE;

  if (cmd == CMD_PUSH)
  { // push
    if (!strstr(arg, "potatobutton"))
      return eFAILURE;
    if (obj->obj_flags.value[3] > 0)
    {
      vict->sendln("It's already been started!");
      return eSUCCESS;
    }
    if ((vict->in_room >= 0 && vict->in_room <= DC::getInstance()->top_of_world) &&
        vict->room().isArena() && arena.isPotato() && arena.isOpened())
    {
      vict->sendln("Wait until the potato arena is open before you try blowing yourself up!");
      return eSUCCESS;
    }
    vict->sendln("The potato starts getting really really hot and burns your hands!!");
    obj->obj_flags.value[3] = number(1, 100);
    return eSUCCESS;
  }

  if (obj->obj_flags.value[3] < 0) // not active yet:)
    return eFAILURE;

  if (vict->isMortalPlayer())
  {
    if (cmd == CMD_SLIP)
    {
      vict->sendln("You can't slip anything when you have a hot potato! (sorry)");
      return eSUCCESS;
    }
    if (cmd == CMD_DROP)
    {
      vict->sendln("You can't drop anything when you have a hot potato!");
      return eSUCCESS;
    }
    if (cmd == CMD_DONATE)
    {
      vict->sendln("You can't donate anything when you have a hot potato!");
      return eSUCCESS;
    }
    if (cmd == CMD_QUIT)
    {
      vict->sendln("You can't quit when you have a hot potato!");
      return eSUCCESS;
    }
    if (cmd == CMD_SACRIFICE)
    {
      vict->sendln("You can't junk stuff when you have a hot potato!");
      return eSUCCESS;
    }
    if (cmd == CMD_PUT)
    {
      vict->sendln("You can't 'put' stuff when you have a hot potato!");
      return eSUCCESS;
    }
  }

  if (cmd == CMD_GIVE)
  {
    // make sure vict for GIVE/SLIP is a pc
    char obj[MAX_INPUT_LENGTH];
    char target[MAX_INPUT_LENGTH];
    half_chop(arg, obj, target);
    Character *give_vict;
    if (!(give_vict = ch->get_char_room_vis(target)))
      return eFAILURE; // Not giving to char/mob, so ok
    if (IS_NPC(give_vict) && vict->isMortalPlayer())
    {
      vict->sendln("You can only give things to other players when you have a hot potato!");
      return eSUCCESS;
    }
    if ((vict->in_room >= 0 && vict->in_room <= DC::getInstance()->top_of_world) &&
        vict->room().isArena() && arena.isPotato() && arena.isOpened() && vict->isMortalPlayer())
    {
      vict->sendln("Wait until the potato arena is open before you start passing out the potatos!");
      return eSUCCESS;
    }

    // if it's a player, go ahead
    if (number(1, 100) > 90 && vict->getLevel() < 100)
      dropped = 1;
    else
      return eFAILURE;
  }

  if (cmd)
    if (cmd != CMD_GIVE || dropped != 1)
      return eFAILURE;

  if (obj->obj_flags.value[3] > 0 && dropped == 0)
  {
    obj->obj_flags.value[3]--;
    if (obj->obj_flags.value[3] % 3 == 0)
      send_to_room("You smell a delicious baked potato and hear a faint *beep*.\r\n", vict->in_room, true);
  }
  else
  {
    if (dropped == 1)
    {
      vict->sendln("OOPS!!! The hot potato burned you and you dropped it!!!");
      act("$n screams in agony as they are burned by the potato and DROPS it!", vict, 0, 0, TO_ROOM, 0);
    }

    if (IS_PC(vict))
      for (Connection *i = DC::getInstance()->descriptor_list; i; i = i->next)
        if (i->character && i->character->in_room != vict->in_room && !i->connected)
          i->character->sendln("You hear a large BOOM from somewhere in the distance.");

    act("The hot potato $n is carrying beeps one final time.\r\n"
        "\n\r$B"
        "BBBB     OOO     OOO    M   M   !!   !!\n\r"
        "B   B   O   O   O   O   MM MM   !!   !!\n\r"
        "BBBB    O   O   O   O   M M M   !!   !!\n\r"
        "B   B   O   O   O   O   M   M \n\r"
        "BBBB     OOO     OOO    M   M   !!   !!\n\r"
        "\n\r$R"
        "Small pieces of $n and mashed potato splatter everywhere!!!\n\r"
        "$n has been KILLED!!",
        vict, 0, 0, TO_ROOM, 0);

    if (IS_NPC(vict))
    {
      act("$n gets back up.", vict, 0, 0, TO_ROOM, 0);
      do_say(vict, "HA!  Fooled ya!", CMD_DEFAULT);
      extract_obj(obj);
      return eSUCCESS;
    }

    vict->setHP(-1);
    update_pos(vict);
    send_to_char("$B"
                 "BBBB     OOO     OOO    M   M !!   !!\n\r"
                 "B   B   O   O   O   O   MM MM !!   !!\n\r"
                 "BBBB    O   O   O   O   M M M !!   !!\n\r"
                 "B   B   O   O   O   O   M   M \n\r"
                 "BBBB     OOO     OOO    M   M !!   !!\n\r\n\r"
                 "$R",
                 vict);
    send_to_char("The baked potato you are carrying EXPLODES!!!\n\r"
                 "You have been KILLED!\n\r",
                 vict);
    extract_obj(obj);
    if (!vict->room().isArena())
      fight_kill(vict, vict, TYPE_PKILL, KILL_POTATO);
    else if (arena.isPotato())
      fight_kill(vict, vict, TYPE_ARENA_KILL, KILL_MASHED);
    else
      fight_kill(vict, vict, TYPE_ARENA_KILL, KILL_POTATO);
    return eSUCCESS | eCH_DIED;
  }

  if (dropped == 1)
    return eSUCCESS;
  else
    return eFAILURE;
}

// proc for mortar shells - see object.cpp
int exploding_mortar_shells(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  int dam = 0;
  char buf[MAX_STRING_LENGTH];
  Character *victim = nullptr;
  Character *next_v = nullptr;

  if (cmd)
    return eFAILURE;

  if (obj->in_room <= 0)
  {
    logentry(QStringLiteral("Mortar round without a room?"), IMMORTAL, DC::LogChannel::LOG_BUG);
    extract_obj(obj);
    return eFAILURE;
  }

  send_to_room("The mortar shell explodes ripping the area to shreds!\r\n", obj->in_room);

  for (int i = 0; i < 6; i++)
    if (DC::getInstance()->world[obj->in_room].dir_option[i] && DC::getInstance()->world[obj->in_room].dir_option[i]->to_room)
      send_to_room("You hear a loud boom.\r\n", DC::getInstance()->world[obj->in_room].dir_option[i]->to_room);

  for (victim = DC::getInstance()->world[obj->in_room].people; victim; victim = next_v)
  {
    next_v = victim->next_in_room;
    if (IS_NPC(victim)) // only hurts players
      continue;

    dam = dice(obj->obj_flags.value[1], obj->obj_flags.value[2]);
    victim->removeHP(dam);
    sprintf(buf, "Pieces of shrapnel rip through your skin inflicting %d damage!\r\n", dam);
    victim->send(buf);
    if (victim->getHP() < 1)
    {
      victim->sendln("You have been KILLED!!");
      fight_kill(victim, victim, TYPE_PKILL, KILL_MORTAR);
    }
  }

  extract_obj(obj);
  return eSUCCESS;
}

// 565
int godload_banshee(Character *ch, class Object *obj, int cmd,
                    const char *arg, Character *invoker)
{
  Character *vict;
  if (cmd)
    return eFAILURE;
  if (!(vict = ch->fighting))
    return eFAILURE;
  if (number(1, 101) > 12)
    return eFAILURE;
  act("$n's instrument takes on a life of its own, sending out a piercing wail.", ch, 0, vict, TO_ROOM, 0);
  ch->sendln("Your instrument sends out a piercing wail.");
  return song_whistle_sharp(51, ch, "", vict, 50);
}
// 511
int godload_claws(Character *ch, class Object *obj, int cmd,
                  char *arg, Character *invoker)
{
  Character *vict;
  if (cmd)
    return eFAILURE;
  if (!(vict = ch->fighting))
    return eFAILURE;
  if (number(1, 101) > 5)
    return eFAILURE;
  act("$n's claws glow icy blue.", ch, 0, vict, TO_ROOM, 0);
  ch->sendln("Your claws glow icy blue.");
  return spell_chill_touch(51, ch, vict, 0, 50);
}

// 556
int godload_defender(Character *ch, class Object *obj, int cmd,
                     const char *arg,
                     Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);
  if (str_cmp(arg1, "arad") || str_cmp(arg2, "tor"))
    return eFAILURE;
  if (isTimer(ch, SPELL_PROTECT_FROM_EVIL))
  {
    ch->sendln("The defender flickers, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_PROTECT_FROM_EVIL, 24);
  return spell_protection_from_evil(50, ch, ch, 0, 150);
}

// 500
int godload_stargazer(Character *ch, class Object *obj, int cmd,
                      const char *arg,
                      Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  //  arg = one_argument(arg, arg2);
  // arg = one_argument(arg, arg3);
  if (str_cmp(arg1, "cabed"))
    return eFAILURE;
  if (isTimer(ch, SPELL_MANA))
  {
    ch->sendln("The robe glows, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_MANA, 6);
  ch->sendln("Your robes glow brightly!");
  return spell_mana(50, ch, ch, 0, 100);
}

// 534
int godload_cassock(Character *ch, class Object *obj, int cmd,
                    const char *arg,
                    Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  //  arg = one_argument(arg, arg2);
  // arg = one_argument(arg, arg3);
  if (str_cmp(arg1, "alata"))
    return eFAILURE;
  if (isTimer(ch, SPELL_GROUP_SANC))
  {
    ch->sendln("The cassock hums, but ends soon after it starts.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_GROUP_SANC, 36);
  ch->sendln("Your cassocks begin to hum loudly!");
  return spell_group_sanc((uint8_t)50, ch, ch, 0, 100);
}

// 526
int godload_armbands(Character *ch, class Object *obj, int cmd,
                     const char *arg, Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  if (str_cmp(arg1, "vanesco"))
    return eFAILURE;
  if (isTimer(ch, SPELL_TELEPORT))
  {
    ch->sendln("The armbands crackle, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_TELEPORT, 24);
  ch->sendln("Your armbands crackle, and you phase out of existence.");
  act("$n phases out of existence.", ch, 0, 0, TO_ROOM, 0);
  return spell_teleport(50, ch, ch, 0, 100);
}

// 548
int godload_gaze(Character *ch, class Object *obj, int cmd,
                 const char *arg,
                 Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  if (str_cmp(arg1, "iudicium"))
    return eFAILURE;
  if (isTimer(ch, SPELL_KNOW_ALIGNMENT))
  {
    ch->sendln("The gaze gazes stoically, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_KNOW_ALIGNMENT, 24);
  ch->sendln("The gaze reacts to your words, and you feel ready to judge.");
  return spell_know_alignment(50, ch, ch, 0, 150);
}

// 514
int godload_wailka(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);

  if (str_cmp(arg1, "suloaki"))
    return eFAILURE;
  if (isTimer(ch, SPELL_PARALYZE) || isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("The ring hums, but nothing happens.");
    return eSUCCESS;
  }
  Character *vict;
  if ((vict = ch->get_char_room_vis(arg2)) == nullptr)
  {
    ch->sendln("You need to tell the item who.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_PARALYZE, 12);
  ch->sendln("Your ring radiates evil, and does your bidding.");
  return spell_paralyze(50, ch, vict, 0, 50);
}

// 517
int godload_choker(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "burzum"))
    return eFAILURE;
  if (isTimer(ch, SPELL_GLOBE_OF_DARKNESS))
  {
    ch->sendln("The choker glows black for a moment, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_GLOBE_OF_DARKNESS, 12);
  ch->sendln("Your choker glows black, and dampens all light in the room.");
  return spell_globe_of_darkness(50, ch, ch, 0, 150);
}

// 519
int godload_lorne(Character *ch, class Object *obj, int cmd, const char *arg,
                  Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "incende"))
    return eFAILURE;
  if (isTimer(ch, SPELL_CONT_LIGHT))
  {
    ch->sendln("The necklace shines, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_CONT_LIGHT, 12);
  ch->sendln("The necklace shines brightly.");
  return spell_cont_light(50, ch, ch, 0, 150);
}

// 528
int godload_leprosy(Character *ch, class Object *obj, int cmd, char *arg,
                    Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WEAR_FEET] != obj)
    return eFAILURE;

  if (number(0, 1))
    return eFAILURE;

  act("$n's $o release a cloud of disease!",
      ch, obj, ch->fighting, TO_ROOM, 0);
  ch->sendln("Your feet releases a cloud of disease!");

  return spell_harm(ch->getLevel(), ch, ch->fighting, 0, 150);
}

// 540
int godload_quiver(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "quant'naith"))
    return eFAILURE;
  if (isTimer(ch, SPELL_MISANRA_QUIVER))
  {
    ch->sendln("The quiver glitters, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_MISANRA_QUIVER, 24);
  class Object *obj2;
  int i;
  ch->sendln("The quiver glitters, and hums.");
  for (i = 0; i < 25; i++)
  {
    if ((obj->obj_flags.weight + 1) < obj->obj_flags.value[0])
    {
      obj2 = DC::getInstance()->clone_object(597);
      if (!obj_to_obj(obj2, obj))
      {
        ch->sendln("Some arrows appear in your quiver.");
        ch->sendln("The quiver flickers brightly, and ends unfinished.");
        return eSUCCESS;
      }
    }
    else
    {
      ch->sendln("Some arrows appear in your quiver.");
      ch->sendln("The quiver flickers brightly, and ends unfinished.");
      return eSUCCESS;
    }
  }
  ch->sendln("Some arrows appear in your quiver.");
  return eSUCCESS;
}

int godload_aligngood(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "aglar"))
    return eFAILURE;
  if (isTimer(ch, SPELL_ALIGN_GOOD))
  {
    ch->sendln("The fire burns, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_ALIGN_GOOD, 48);
  GET_ALIGNMENT(ch) += 400;
  if (GET_ALIGNMENT(ch) > 1000)
    GET_ALIGNMENT(ch) = 1000;
  ch->sendln("You are purified by the light of the fire.");
  return eSUCCESS;
}

int godload_alignevil(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "dagnir"))
    return eFAILURE;
  if (isTimer(ch, SPELL_ALIGN_EVIL))
  {
    ch->sendln("The blackened heart croaks, but nothing happens.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_ALIGN_EVIL, 48);
  GET_ALIGNMENT(ch) -= 400;
  if (GET_ALIGNMENT(ch) < -1000)
    GET_ALIGNMENT(ch) = -1000;
  ch->sendln("You are burnt by the heart's darkness.");
  return eSUCCESS;
}

int godload_tovmier(Character *ch, class Object *obj, int cmd, const char *arg,
                    Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH];
  if (cmd != CMD_PULL || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "staff") && str_cmp(arg1, "tovmier"))
    return eFAILURE;

  ch->sendln("You twist the handle of the staff.");
  for (int i = 0; i < obj->num_affects; i++)
    if (obj->affected[i].location == WEP_DISPEL_EVIL)
    {
      obj->affected[i].location = WEP_DISPEL_GOOD;
      return eSUCCESS;
    }
    else if (obj->affected[i].location == WEP_DISPEL_GOOD)
    {
      obj->affected[i].location = WEP_DISPEL_EVIL;
      return eSUCCESS;
    }
  ch->sendln("Something's bugged with this staff. Report it.");
  return eSUCCESS;
}

int godload_hammer(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  if (cmd != CMD_TREMOR || !ch)
    return eFAILURE;

  if (!is_wearing(ch, obj))
    return eFAILURE;
  if (isTimer(ch, SPELL_EARTHQUAKE))
  {
    ch->sendln("The hammer glows, but nothing happens.");
    return eSUCCESS;
  }
  act("$n smashes $s hammer into the ground causing a tectonic blast.", ch, 0, 0, TO_ROOM, 0);
  ch->sendln("You smash your hammer into the ground, causing it to shake violently.");
  addTimer(ch, SPELL_EARTHQUAKE, 24);
  int retval = spell_earthquake(50, ch, ch, 0, 100);
  if (!SOMEONE_DIED(retval))
    retval |= spell_earthquake(50, ch, ch, 0, 100);
  return retval;
}

int angie_proc(Character *ch, class Object *obj, int cmd, const char *arg, Character *invoker)
{
  if (cmd != CMD_OPEN || ch->in_room != 29263)
    return eFAILURE;
  char arg1[MAX_INPUT_LENGTH];
  arg = one_argument(arg, arg1);

  if (str_cmp(arg1, "door"))
    return eFAILURE;
  if (!isSet(DC::getInstance()->world[ch->in_room].dir_option[0]->exit_info, EX_CLOSED))
    return eFAILURE;
  REMOVE_BIT(DC::getInstance()->world[ch->in_room].dir_option[0]->exit_info, EX_CLOSED);
  REMOVE_BIT(DC::getInstance()->world[29265].dir_option[2]->exit_info, EX_CLOSED);
  act("$n turns the doorknob, there is a loud click, and a blinding explosion knocks you on your ass.", ch, nullptr, nullptr, TO_ROOM, 0);
  act("You turn the doorknob, there is a loud click, and a blinding explosion knocks you on your ass.", ch, nullptr, nullptr, TO_CHAR, 0);
  Character *a, *b, *c;
  b = initiate_oproc(nullptr, obj);
  for (a = DC::getInstance()->world[ch->in_room].people; a; a = c)
  {
    c = a->next_in_room; // 'cause mobs get freed
    if (a == b)
      continue;
    damage(b, a, 1000, TYPE_HIT, 50000, 0);
  }

  end_oproc(b);
  extract_obj(obj);
  return eSUCCESS;
}

int godload_phyraz(Character *ch, class Object *obj, int cmd, const char *arg,
                   Character *invoker)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  if (cmd != CMD_SAY || !is_wearing(ch, obj))
    return eFAILURE;
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);

  if (str_cmp(arg1, "katascopse"))
    return eFAILURE;
  if (isTimer(ch, SPELL_WIZARD_EYE))
  {
    ch->sendln("The ball stays murky.");
    return eSUCCESS;
  }
  addTimer(ch, SPELL_WIZARD_EYE, 24);
  Character *vict = get_char_vis(ch, arg2);
  if (!vict)
  {
    ch->sendln("The scrying ball stays murky.");
    return eSUCCESS;
  }
  ch->sendln("You tell the scrying ball your bidding, and an image appears.");
  return spell_wizard_eye(100, ch, vict, 0, 100);
}

void destroy_spellcraft_glyphs(Character *ch)
{
  class Object *tmp_obj, *loop_obj;

  for (tmp_obj = ch->carrying; tmp_obj; tmp_obj = tmp_obj->next_content)
  {
    if (GET_ITEM_TYPE(tmp_obj) == ITEM_CONTAINER)
      for (loop_obj = tmp_obj->contains; loop_obj; loop_obj = loop_obj->next_content)
        if (tmp_obj->vnum == 6351 || tmp_obj->vnum == 6352 || tmp_obj->vnum == 6353)
          move_obj(loop_obj, ch);
    if (tmp_obj->vnum == 6351 || tmp_obj->vnum == 6352 || tmp_obj->vnum == 6353)
      obj_from_char(tmp_obj);
  }
  ch->spellcraftglyph = 0;
}

int spellcraft_glyphs(Character *ch, class Object *obj, int cmd, const char *argi, Character *invoker)
{
  char target[MAX_STRING_LENGTH], arg[MAX_STRING_LENGTH];
  class Object *sunglyph, *bookglyph, *heartglyph;

  if (cmd != CMD_PUT)
    return eFAILURE; // put

  argi = one_argument(argi, arg);
  argi = one_argument(argi, target);

  sunglyph = get_obj_in_list_vis(ch, 6351, ch->carrying);
  bookglyph = get_obj_in_list_vis(ch, 6352, ch->carrying);
  heartglyph = get_obj_in_list_vis(ch, 6353, ch->carrying);

  if (!str_cmp(arg, "power"))
  {
    if (ch->in_room != 14060)
    {
      ch->sendln("There's no place around to put this special item.");
      return eFAILURE;
    }

    if (sunglyph == nullptr)
    {
      ch->sendln("Put what where?");
      return eFAILURE;
    }
    if (!str_cmp(target, "sun"))
    {
      send_to_room("The sun glows brightly as it releases the energy inside the glyph.\r\n", ch->in_room);
      obj_from_char(sunglyph);
      SET_BIT(ch->spellcraftglyph, 1);
    }
    else if (!str_cmp(target, "book"))
    {
      send_to_room("The sun glows a bright red and shatters the glyphs!\n\r", ch->in_room);
      destroy_spellcraft_glyphs(ch);
    }
    else if (!str_cmp(target, "heart"))
    {
      send_to_room("The sun glows a bright red and shatters the glyphs!\n\r", ch->in_room);
      destroy_spellcraft_glyphs(ch);
    }
    else
    {
      ch->sendln("Put it where?");
      return eFAILURE;
    }
  }
  else if (!str_cmp(arg, "wisdom"))
  {
    if (ch->in_room != 14060)
    {
      ch->sendln("There's no place around to put this special item.");
      return eFAILURE;
    }
    if (bookglyph == nullptr)
    {
      ch->sendln("Put what where?");
      return eFAILURE;
    }
    if (!str_cmp(target, "sun"))
    {
      send_to_room("The book slams shut creating a sonic wave that shatters the glyphs!\n\r", ch->in_room);
      destroy_spellcraft_glyphs(ch);
    }
    else if (!str_cmp(target, "book"))
    {
      send_to_room("The book closes over the glyph, becoming slightly warm.\r\n", ch->in_room);
      obj_from_char(bookglyph);
      SET_BIT(ch->spellcraftglyph, 2);
    }
    else if (!str_cmp(target, "heart"))
    {
      send_to_room("The book slams shut creating a sonic wave that shatters the glyphs!\n\r", ch->in_room);
      destroy_spellcraft_glyphs(ch);
    }
    else
    {
      ch->sendln("Put it where?");
      return eFAILURE;
    }
  }
  else if (!str_cmp(arg, "will"))
  {
    if (ch->in_room != 14060)
    {
      ch->sendln("There's no place around to put this special item.");
      return eFAILURE;
    }

    if (heartglyph == nullptr)
    {
      ch->sendln("Put what where?");
      return eFAILURE;
    }
    if (!str_cmp(target, "sun"))
    {
      send_to_room("The heart beats seemingly uncontrollibly and shatters the glyphs!\n\r", ch->in_room);
      destroy_spellcraft_glyphs(ch);
    }
    else if (!str_cmp(target, "book"))
    {
      send_to_room("The heart beats seemingly uncontrollibly and shatters the glyphs!\n\r", ch->in_room);
      destroy_spellcraft_glyphs(ch);
    }
    else if (!str_cmp(target, "heart"))
    {
      send_to_room("You place the glyph next to the heart, and it slowly begins to pulse.\r\n", ch->in_room);
      obj_from_char(heartglyph);
      SET_BIT(ch->spellcraftglyph, 4);
    }
    else
    {
      ch->sendln("Put it where?");
      return eFAILURE;
    }
  }
  else
    return eFAILURE;
  //      ch->sendln("Which glyph?");
  if (ch->spellcraftglyph == 7)
  {
    if (GET_CLASS(ch) == CLASS_MAGIC_USER && ch->getLevel() >= 50 && !ch->has_skill(SKILL_SPELLCRAFT))
    {
      send_to_room("The glyph receptacles glow an eerie pale white.\n\rThe book shoots out a beams of light from the pages.\r\n", ch->in_room);
      ch->sendln("A beam of light hits you in the head!\n\rYou have learned spellcraft!");
      ch->learn_skill(SKILL_SPELLCRAFT, 1, 1);
    }
  }
  /*   if(ch->spellcraftglyph > 8) {
        send_to_room("The glyph containers ", ch->in_room);
        destroy_spellcraft_glyphs(ch); //just in case
        return eFAILURE;
     }
  */
  return eSUCCESS;
}

int godload_grathelok(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{
  Character *vict;

  if (!(vict = ch->fighting))
    return eFAILURE;

  if (vict->getHP() > 100)
    return eFAILURE;

  switch (number(0, 1))
  {
  case 0:
    act("$n's blow rips your leg from your body.  Extreme pain is yours to know until you hit the ground mercifully dead.", ch, 0, vict, TO_VICT, 0);
    act("You attack takes off $N's leg at the knee!  Ahh the blood!", ch, 0, vict, TO_CHAR, 0);
    act("$n fierce swing takes off $N's leg at the knee leaving a bloody stump and a brief scream.", ch, 0, vict, TO_ROOM, NOTVICT);
    make_leg(vict);
    break;
  case 1:
    act("$n's attack takes off your arm at the shoulder.  You stare in shock at the fountaining blood before $e crushes your skull.", ch, 0, vict, TO_VICT, 0);
    act("You violently rip off $S arm with the attack before caving in $N's forehead.", ch, 0, vict, TO_CHAR, 0);
    act("With a grunt of exertion, $n swings with enough force to rip $N's arm off!", ch, 0, vict, TO_ROOM, NOTVICT);
    make_arm(vict);
    break;
  }
  vict->setHP(-20, ch);
  group_gain(ch, vict);
  fight_kill(ch, vict, TYPE_CHOOSE, 0);
  return eSUCCESS | eVICT_DIED;
}
int goldenbatleth(Character *ch, class Object *obj, int cmd, const char *arg,
                  Character *invoker)
{
  Character *vict;

  if (!(vict = ch->fighting))
    return eFAILURE;

  if (vict->getHP() > 40)
    return eFAILURE;

  switch (number(0, 1))
  {
  case 0:
    act("$n's blow rips your leg from your body.  Extreme pain is yours to know until you hit the ground mercifully dead.", ch, 0, vict, TO_VICT, 0);
    act("You attack takes off $N's leg at the knee!  Ahh the blood!", ch, 0, vict, TO_CHAR, 0);
    act("$n fierce swing takes off $N's leg at the knee leaving a bloody stump and a brief scream.", ch, 0, vict, TO_ROOM, NOTVICT);
    make_leg(vict);
    break;
  case 1:
    act("$n's attack takes off your arm at the shoulder.  You stare in shock at the fountaining blood before $e crushes your skull.", ch, 0, vict, TO_VICT, 0);
    act("You violently rip off $S arm with the attack before caving in $N's forehead.", ch, 0, vict, TO_CHAR, 0);
    act("With a grunt of exertion, $n swings with enough force to rip $N's arm off!", ch, 0, vict, TO_ROOM, NOTVICT);
    make_arm(vict);
    break;
  }
  vict->setHP(-20, ch);
  group_gain(ch, vict);
  fight_kill(ch, vict, TYPE_CHOOSE, 0);
  return eSUCCESS | eVICT_DIED;
}

int godload_jaelgreth(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)
    return eFAILURE;

  if (number(1, 100) > 5)
    return eFAILURE;

  ch->sendln("You thrust your sacrificial blade into your victim, leeching their lifeforce!");
  act("$n's dagger sinks into your flesh, and you feel your life force being drained!",
      ch, obj, ch->fighting, TO_VICT, 0);
  Character *victim = ch->fighting;

  int dam = 100;

  if (victim->affected_by_spell(SPELL_DIVINE_INTER) && dam > victim->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
    dam = victim->affected_by_spell(SPELL_DIVINE_INTER)->modifier;

  victim->removeHP(dam, ch);
  ch->addHP(dam, victim);

  if (ch->getHP() > GET_MAX_HIT(ch))
  {
    ch->fillHP();
  }

  dam = MIN(100, GET_MANA(victim));
  if (dam < 0)
    dam = 0;

  GET_MANA(victim) -= dam;
  GET_MANA(ch) += dam;

  if (GET_MANA(ch) > GET_MAX_MANA(ch))
    GET_MANA(ch) = GET_MAX_MANA(ch);

  update_pos(victim);

  if (GET_POS(victim) == position_t::DEAD)
  {
    act("$n is DEAD!!", victim, 0, 0, TO_ROOM, INVIS_NULL);
    group_gain(ch, victim);
    if (IS_PC(victim))
      victim->sendln("You have been KILLED!!\n\r");
    fight_kill(ch, victim, TYPE_CHOOSE, 0);
    return eSUCCESS | eVICT_DIED;
  }
  return eSUCCESS;
}

int godload_foecrusher(Character *ch, class Object *obj, int cmd, const char *arg,
                       Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)
    return eFAILURE;

  if (number(1, 100) > 5)
    return eFAILURE;

  int dam = 80;
  switch (number(1, 2))
  {
  case 1:
    act("$n smashes $s hammer into your face!",
        ch, obj, ch->fighting, TO_VICT, 0);
    act("You smash your hammer into $N's face!",
        ch, obj, ch->fighting, TO_CHAR, 0);
    dam = 80;
    break;
  case 2:
    dam = 130;
    act("The force of the blow dealt by $n's Foecrusher inflicts heavy damage on you.",
        ch, obj, ch->fighting, TO_VICT, 0);
    act("The force of your hammer's blow inflicts heavy damage on $N.",
        ch, obj, ch->fighting, TO_CHAR, 0);
    break;
  }
  Character *victim = ch->fighting;
  dam = number(50, dam);

  if (victim->affected_by_spell(SPELL_DIVINE_INTER) && dam > victim->affected_by_spell(SPELL_DIVINE_INTER)->modifier)
    dam = victim->affected_by_spell(SPELL_DIVINE_INTER)->modifier;
  victim->removeHP(dam);

  update_pos(victim);

  if (GET_POS(victim) == position_t::DEAD)
  {
    act("$n is DEAD!!", victim, 0, 0, TO_ROOM, INVIS_NULL);
    group_gain(ch, victim);
    if (IS_PC(victim))
      victim->sendln("You have been KILLED!!\n\r");
    fight_kill(ch, victim, TYPE_CHOOSE, 0);
    return eSUCCESS | eVICT_DIED;
  }
  return eSUCCESS;
}

int godload_hydratail(Character *ch, class Object *obj, int cmd, const char *arg,
                      Character *invoker)
{
  if (cmd)
    return eFAILURE;
  if (!ch || !ch->fighting)
    return eFAILURE;
  if (ch->equipment[WIELD] != obj &&
      ch->equipment[SECOND_WIELD] != obj)
    return eFAILURE;

  if (number(1, 100) > 10)
    return eFAILURE;

  int damtype = 0;
  char dammsg[MAX_STRING_LENGTH];
  int dam = number(50, 100);
  std::string damtypeStr;
  sprintf(dammsg, "$B%d$R", dam);

  switch (number(1, 4))
  {
  case 1:
    damtypeStr = "$4$Bfiery$R";
    damtype = TYPE_FIRE;
    break;
  case 2:
    damtypeStr = "$3$Bchilling$R";
    damtype = TYPE_COLD;
    break;
  case 3:
    damtypeStr = "$2$Bacidic$R";
    damtype = TYPE_ACID;
    break;
  case 4:
    damtypeStr = "$5$Bshocking$R";
    damtype = TYPE_ENERGY;
    break;
  }
  Character *victim = ch->fighting;

  std::stringstream strwithdam;
  std::stringstream strwithoutdam;

  strwithdam << "A head on $n's whip lashes out of its own accord unleashing a " << damtypeStr << " blast at you for | damage!";
  strwithoutdam << "A head on $n's whip lashes out of its own accord unleashing a " << damtypeStr << " blast at you!";
  send_damage(strwithdam.str().c_str(), ch, obj, ch->fighting, dammsg, strwithoutdam.str().c_str(), TO_VICT);

  strwithdam.str("");
  strwithoutdam.str("");

  strwithdam << "A head on your whip lashes out of its own accord unleashing a " << damtypeStr << " blast at $N for | damage!";
  strwithoutdam << "A head on your whip lashes out of its own accord unleashing a " << damtypeStr << " blast at $N!";
  send_damage(strwithdam.str().c_str(), ch, obj, ch->fighting, dammsg, strwithoutdam.str().c_str(), TO_CHAR);

  return damage(ch, victim, dam, damtype, 0, 0);
}
