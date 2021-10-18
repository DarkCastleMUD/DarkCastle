/************************************************************************
| $Id: guild.cpp,v 1.132 2014/07/04 22:00:04 jhhudso Exp $
| guild.C
| This contains all the guild commands - practice, gain, etc..
*/
#include "character.h"
#include "structs.h"
#include "spells.h"
#include "utility.h"
#include "levels.h"
#include "player.h"
#include "db.h" // exp_table
#include "interp.h"
#include <string.h>
#include "returnvals.h"
#include "ki.h"
#include "mobile.h"
#include "room.h"
#include "sing.h"
#include "handler.h"
#include "const.h"

extern vector<profession> professions;

int get_max(CHAR_DATA *ch, int skill);
int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *owner);

int do_practice(CHAR_DATA *ch, char *arg, int cmd)
{
  /* Call "guild" with a null string for an argument.
     This displays the character's skills. */

  if (arg[0] != '\0'){
    send_to_char("You can only practice in a guild.\n\r", ch);
  } else {
    guild(ch, 0, cmd, "", ch);
  }
  return eSUCCESS;
}


int do_profession(char_data *ch, char *args, int cmdnum)
{
	// Command is enabled by giving someone the profession skill
	if (!has_skill(ch, SKILL_PROFESSION)) {
		send_to_char("Huh?\n\r", ch);
		return eFAILURE;
	}

	// You can only use the command in the same room of a mob named guildmaster
	char_data *victim = get_mob_room_vis(ch, "guildmaster");
	if (victim == NULL) {
		csendf(ch, "You can't pick a profession here. You need to find a Guild Master.\n\r");
		return eFAILURE;
	}

	char arg1[MAX_INPUT_LENGTH];
	one_argument(args, arg1);

	// Show syntax if no argument is given
	if (arg1[0] == '\0') {
		// Show list of available professions based on player's class type
		csendf(ch, "As a %s you can pick from the following professions:\n\r", pc_clss_types3[GET_CLASS(ch)]);

		for(vector<profession>::iterator i = professions.begin(); i != professions.end(); ++i) {
			if ((*i).c_class == GET_CLASS(ch)) {
				csendf(ch, "%s\n\r", (*i).Name.c_str());
			}
		}

		csendf(ch, "\n\r"
				   "Syntax: profession [profession name]\n\r"
				   "        profession reset\n\r"
				   "\n\r"
				   "Resetting your profession costs 10000 platinum.\n\r");

		return eSUCCESS;
	}

	bool found = false;
	for(vector<profession>::iterator i = professions.begin(); i != professions.end(); ++i) {
		if ((*i).name == string(arg1)) {
			if ((*i).c_class == GET_CLASS(ch)) {
				found = true;
				break;
			} else {
				csendf(ch, "That profession is not available to your class.\n\r");
				return eFAILURE;
			}
		}
	}

	if (found == false) {
		csendf(ch, "%s not a valid profession.\n\r", arg1);
		return eFAILURE;
	}

	return eSUCCESS;
}

char *per_col(int percent)
{
  if (percent == 0)
    return ( "$B$0");
  if (percent <= 5)
    return ( "$4");
  if (percent <= 10)
    return ( "$4");
  if (percent <= 15)
    return ( "$B$4");
  if (percent <= 20)
    return ( "$B$4");
  if (percent <= 30)
    return ( "$5");
  if (percent <= 40)
    return ( "$5");
  if (percent <=50)
    return ( "$B$5");
  if (percent <= 60)
    return ( "$B$5");
  if (percent <= 70)
    return ( "$2");
  if (percent <= 80)
    return ( "$B$2");
  if(percent <= 85)
    return ( "$B$3");
  if (percent <= 90)
    return ("$B$3");
  if (percent <= 100)
    return ("$B$7");
  return ("$B$1");
}

char *how_good(int percent)
{
  if (percent == 0)
    return ( " not learned$R");
  if (percent <= 5)
    return ( " horrible$R");
  if (percent <= 10)
    return ( " crappy$R");
  if (percent <= 15)
    return ( " meager$R");
  if (percent <= 20)
    return ( " bad$R");
  if (percent <= 30)
    return ( " poor$R");
  if (percent <= 40)
    return ( " decent$R");
  if (percent <= 50)
    return ( " average$R");
  if (percent <= 60)
    return ( " fair$R");
  if (percent <= 70)
    return ( " good$R");
  if (percent <= 80)
    return ( " very good$R");
  if(percent <= 85)
    return ( " excellent$R");
  if (percent <= 90)
    return (" superb$R");
  if (percent <= 100)
    return (" Masterful$R");

  return (" Transcendent$R");
}

// return a 1 if I just learned skill for first time
// else 0
int learn_skill(char_data * ch, int skill, int amount, int maximum)
{
  struct char_skill_data * curr = ch->skills;
  int old;
  while(curr)
    if(curr->skillnum == skill)
      break;
    else curr = curr->next;

  if(curr) 
  {
    old = curr->learned;
    curr->learned += amount;
    if (skill == SKILL_MAGIC_RESIST)
      barb_magic_resist(ch, old, curr->learned);
    if(curr->learned > maximum)
      curr->learned = maximum;
  }
  else 
  {
#ifdef LEAK_CHECK
    curr = (char_skill_data *)calloc(1, sizeof(char_skill_data));
#else
    curr = (char_skill_data *)dc_alloc(1, sizeof(char_skill_data));
#endif
    if (skill == SKILL_MAGIC_RESIST)
      barb_magic_resist(ch, 0, amount);
    curr->skillnum = skill;
    curr->learned = amount;
    curr->next = ch->skills;
    ch->skills = curr;

   // could save processing power by making it's own function since skillnum is already known
   // but *shrug*
    prepare_character_for_sixty(ch);
    return 1;
  }
  return 0;
}

int search_skills2(int arg, class_skill_defines * list_skills)
{
  for(int i = 0; *list_skills[i].skillname != '\n'; i++)
    if(arg == list_skills[i].skillnum)
      return i;

  return -1;
}

int search_skills(char * arg, class_skill_defines * list_skills)
{
  for(int i = 0; *list_skills[i].skillname != '\n'; i++)
    if(is_abbrev(arg, list_skills[i].skillname))
      return i;

  return -1;
}


class_skill_defines * get_skill_list(char_data * ch)
{
  class_skill_defines * skilllist = NULL;

  switch(GET_CLASS(ch)) {
    case CLASS_THIEF:      skilllist = t_skills;  break;  
    case CLASS_WARRIOR:    skilllist = w_skills;  break;  
    case CLASS_BARBARIAN:  skilllist = b_skills;  break;  
    case CLASS_PALADIN:    skilllist = p_skills;  break;  
    case CLASS_ANTI_PAL:   skilllist = a_skills;  break;  
    case CLASS_RANGER:     skilllist = r_skills;  break;  
    case CLASS_BARD:       skilllist = d_skills;  break;  
    case CLASS_MONK:       skilllist = k_skills;  break;  
    case CLASS_DRUID:      skilllist = u_skills;  break;
    case CLASS_CLERIC:     skilllist = c_skills;  break;
    case CLASS_MAGIC_USER: skilllist = m_skills;  break;
    default:
      break;
  }
  return skilllist;
}

char *attrstring(int attr)
{
  switch(attr) {
    case STRDEX: return "Str/Dex";
    case STRCON: return "Str/Con";
    case STRINT: return "Str/Int";
    case STRWIS: return "Str/Wis";
    case DEXCON: return "Dex/Con";
    case DEXINT: return "Dex/Int";
    case DEXWIS: return "Dex/Wis";
    case CONINT: return "Con/Int";
    case CONWIS: return "Con/Wis";
    case INTWIS: return "Int/Wis";
    default: return "oh shit!";
  }
}

char *attrname(int clss, int attr)
{
  switch (attr)
  {
    case STRDEX:
      if(clss == CLASS_WARRIOR)		return "Offensive";
      else if(clss == CLASS_THIEF)	return "Assassination";
      else if(clss == CLASS_ANTI_PAL)	return "Combat";
      else if(clss == CLASS_MONK)	return "Tiger";
      else if(clss == CLASS_RANGER)	return "Physical Prowess";
      else if(clss == CLASS_BARD)	return "Melee";
      else if(clss == CLASS_DRUID)	return "Retribution";
      else return "ERR1";
    case STRCON:
      if(clss == CLASS_WARRIOR)		return "Defensive";
      else if(clss == CLASS_PALADIN)	return "Judgement";
      else if(clss == CLASS_BARBARIAN)	return "Fury";
      else if(clss == CLASS_RANGER)	return "Feral Summoning";
      else if(clss == CLASS_DRUID)	return "Prevention";
      else if(clss == CLASS_CLERIC)	return "Theurgy";
      else if(clss == CLASS_MAGIC_USER)	return "Conjuration";
      else return "ERR2";
    case STRINT:
      if(clss == CLASS_ANTI_PAL)	return "Thaumaturgy";
      else if(clss == CLASS_BARBARIAN)	return "Aggression";
      else if(clss == CLASS_CLERIC)	return "Castigation";
      else if(clss == CLASS_MAGIC_USER)	return "Evocation";
      else return "ERR3";
    case STRWIS:
      if(clss == CLASS_PALADIN)		return "Consecration";
      else return "ERR4";
    case DEXCON:
      if(clss == CLASS_BARBARIAN)	return "Reflex";
      else if(clss == CLASS_BARD)	return "Assault";
      else return "ERR5";
    case DEXINT:
      if(clss == CLASS_WARRIOR)		return "Strategy";
      else if(clss == CLASS_THIEF)	return "Subtlety";
      else if(clss == CLASS_ANTI_PAL)	return "Treachery";
      else if(clss == CLASS_PALADIN)	return "Battle";
      else if(clss == CLASS_RANGER)	return "Hunter's Lore";
      else if(clss == CLASS_CLERIC)	return "Augury";
      else if(clss == CLASS_MAGIC_USER)	return "Enchantment";
      else return "ERR6";
    case DEXWIS:
      if(clss == CLASS_THIEF)		return "Deception";
      else if(clss == CLASS_BARBARIAN)	return "Arsenal";
      else if(clss == CLASS_MONK)	return "Monkey";
      else if(clss == CLASS_DRUID)	return "Natural";
      else if(clss == CLASS_CLERIC)	return "Divinity";
      else if(clss == CLASS_MAGIC_USER)	return "Invocation";
      else return "ERR7";
    case CONINT:
      if(clss == CLASS_THIEF)		return "Martial";
      else if(clss == CLASS_ANTI_PAL)	return "Desecration";
      else if(clss == CLASS_MONK)	return "Dragon";
      else if(clss == CLASS_BARD)	return "Charm";
      else if(clss == CLASS_DRUID)	return "Elemental";
      else return "ERR8";
    case CONWIS:
      if(clss == CLASS_WARRIOR)		return "Weapons";
      else if(clss == CLASS_ANTI_PAL)	return "Necromancy";
      else if(clss == CLASS_PALADIN)	return "Sanctification";
      else if(clss == CLASS_MONK)	return "Crane";
      else if(clss == CLASS_RANGER)	return "Protective Instincts";
      else if(clss == CLASS_BARD)	return "Enhancement";
      else if(clss == CLASS_DRUID)	return "Malediction";
      else if(clss == CLASS_CLERIC)	return "Intercession";
      else if(clss == CLASS_MAGIC_USER)	return "Abjuration";
      else return "ERR9";
    case INTWIS:
      if(clss == CLASS_PALADIN)		return "Benediction";
      else if(clss == CLASS_RANGER)	return "Natural Affinity";
      else if(clss == CLASS_BARD)	return "Detection";
      else if(clss == CLASS_DRUID)	return "Medicinal";
      else if(clss == CLASS_CLERIC)	return "Restoration";
      else if(clss == CLASS_MAGIC_USER)	return "Divination";
      else return "ERR10";
    default: return "Err";
  }
}

int skillmax(struct char_data *ch, int skill, int eh)
{
  class_skill_defines * skilllist = get_skill_list(ch);
  if (IS_NPC(ch)) return eh;
  if(!skilllist)
    skilllist = g_skills;
  int i = search_skills2(skill, skilllist);
  if (i==-1 && skilllist != g_skills){ skilllist = g_skills; i = search_skills2(skill, g_skills);}
  if (i==-1) return eh;
  if (skilllist[i].maximum < i) return skilllist[i].maximum;
  return eh;
}

char charthing(struct char_data *ch, int known, int skill, int maximum)
{
  if (get_max(ch, skill) <= known) return '*';
  if (known >= maximum/2) return '=';
  return '+';
}

void output_praclist(struct char_data *ch, class_skill_defines *skilllist)
{
  int known, last_profession = 0;
  char buf[MAX_STRING_LENGTH];
  for (int i = 0; *skilllist[i].skillname != '\n'; i++)
  {
    known = has_skill(ch, skilllist[i].skillnum);
    if (!known && GET_LEVEL(ch) < skilllist[i].levelavailable)
      continue;

    if (IS_PC(ch) && skilllist[i].group > 0 && skilllist[i].group != ch->pcdata->profession)
    {
      continue;
    }
    else
    {
      if (last_profession != skilllist[i].group)
      {
        last_profession = skilllist[i].group;
        csendf(ch, "\n\r$B%s Profession Skills:$R\n\r", find_profession(ch->c_class, skilllist[i].group));
        send_to_char ( " Ability:                Current/Practice/Autolearn  Cost:     Group:\r\n", ch);
        send_to_char("--------------------------------------------------------------------------------\r\n", ch);
      }
    }

    int self_learn_max = (float)skilllist[i].maximum * 0.5;
    if (GET_LEVEL(ch) * 2 < self_learn_max)
    {
      self_learn_max = GET_LEVEL(ch) * 2;
    }

    sprintf(buf, " %c%-24s%s%15s $B$0$R%s%3d/%3d/%3d$B$0$R  ", UPPER(*skilllist[i].skillname), (skilllist[i].skillname + 1),
            per_col(known), how_good(known), per_col(known), known, self_learn_max, get_max(ch, skilllist[i].skillnum));
    send_to_char(buf, ch);
    if (skilllist[i].skillnum >= 1 && skilllist[i].skillnum <= MAX_SPL_LIST)
    {
      if (skilllist[i].skillnum == SPELL_PORTAL && GET_CLASS(ch) == CLASS_CLERIC)
        sprintf(buf, "Mana: $B%3d$R ", 150);
      else
        sprintf(buf, "Mana: $B%3d$R ", use_mana(ch, skilllist[i].skillnum));
      send_to_char(buf, ch);
    }
    else if (skilllist[i].skillnum >= SKILL_SONG_BASE && skilllist[i].skillnum <= SKILL_SONG_MAX)
    {
      extern struct song_info_type song_info[];
      csendf(ch, "Ki:   $B%3d$R ", song_info[skilllist[i].skillnum - SKILL_SONG_BASE].min_useski);
    }
    else if (skilllist[i].skillnum >= KI_OFFSET && skilllist[i].skillnum <= KI_OFFSET + MAX_KI_LIST)
    {
      extern struct ki_info_type ki_info[];
      csendf(ch, "Ki:   $B%3d$R ", ki_info[skilllist[i].skillnum - KI_OFFSET].min_useski);
    }
    else if (skilllist[i].skillnum == 318) // scan
    {
      csendf(ch, "Move: $B%3d$R ", 2);
    }
    else if (skilllist[i].skillnum == 320) // switch
    {
      csendf(ch, "Move: $B%3d$R ", 4);
    }
    else if (skilllist[i].skillnum == 319) // consider
    {
      csendf(ch, "Move: $B%3d$R ", 5);
    }
    else if (skilllist[i].skillnum == 368) // release
    {
      csendf(ch, "Move: $B%3d$R ", 25);
    }
    else if (skilllist[i].skillnum == 380) // fire arrows
    {
      csendf(ch, "Mana: $B%3d$R ", 30);
    }
    else if (skilllist[i].skillnum == 381) //ice arrows
    {
      csendf(ch, "Mana: $B%3d$R ", 20);
    }
    else if (skilllist[i].skillnum == 382) //tempest arrows
    {
      csendf(ch, "Mana: $B%3d$R ", 10);
    }
    else if (skilllist[i].skillnum == 383) //granite arrows
    {
      csendf(ch, "Mana: $B%3d$R ", 40);
    }
    else if (skill_cost.find(skilllist[i].skillnum) != skill_cost.end())
    {
      csendf(ch, "Move: $B%3d$R ", skill_cost.find(skilllist[i].skillnum)->second);
    }
    else
      send_to_char("          ", ch);
    if (skilllist[i].attrs)
    {
      if (skilllist != g_skills)
      {
        sprintf(buf, " %s", attrname(GET_CLASS(ch), skilllist[i].attrs));
        send_to_char(buf, ch);
      }
      else
        send_to_char(" General", ch);
    }
    if (skilllist[i].skillnum == SKILL_SONG_DISARMING_LIMERICK)
    {
      send_to_char("$B        #$R\n\r", ch);
    }
    else if (skilllist[i].skillnum == SKILL_SONG_FANATICAL_FANFARE)
    {
      send_to_char("$B  #$R\n\r", ch);
    }
    else if (skilllist[i].skillnum == SKILL_SONG_SEARCHING_SONG)
    {
      send_to_char("$B    #$R\n\r", ch);
    }
    else if (skilllist[i].skillnum == SKILL_SONG_VIGILANT_SIREN)
    {
      send_to_char("$B  #$R\n\r", ch);
    }
    else if (skilllist[i].skillnum == SKILL_SONG_MKING_CHARGE)
    {
      send_to_char("$B      #$R\n\r", ch);
    }
    else if (skilllist[i].skillnum == SKILL_SONG_HYPNOTIC_HARMONY)
    {
      send_to_char("$B        #$R\n\r", ch);
    }
    else if (skilllist[i].skillnum == SKILL_SONG_SHATTERING_RESO)
    {
      send_to_char("$B        #$R\n\r", ch);
    }
    else
      send_to_char("\n\r", ch);
  }
}

int skills_guild ( struct char_data *ch, char *arg, struct char_data *owner )
{
  char buf[160];
  int known, x;
  int skillnumber;
  int percent;

  if ( IS_NPC ( ch ) ) return eFAILURE;

  class_skill_defines * skilllist = get_skill_list ( ch );

  if ( !skilllist )
    return eFAILURE;  // no skills to train

  if ( !*arg )   // display skills that can be learned
    {
      sprintf ( buf, "You have %d practice sessions left.\n\r", ch->pcdata->practices );
      send_to_char ( buf, ch );
      send_to_char ( "You can practice any of these skills:\n\r\r\n", ch );

      send_to_char ( "$BUniversal skills:$R\r\n", ch );
      send_to_char ( " Ability:                Current/Practice/Autolearn  Cost:     Group:\r\n", ch);
      send_to_char ( "--------------------------------------------------------------------------------\r\n", ch );
      output_praclist ( ch, g_skills );

      sprintf ( buf, "\r\n$B%c%s skills:$R\r\n", UPPER ( *pc_clss_types[GET_CLASS ( ch ) ] ), ( 1 + pc_clss_types[GET_CLASS ( ch ) ] ) );
      send_to_char ( buf, ch );

      if ( GET_CLASS ( ch ) != CLASS_MONK )
        send_to_char ( " Ability:                Current/Practice/Autolearn  Cost:     Group:\r\n", ch);
      else
        send_to_char ( " Ability:                Current/Practice/Autolearn  Cost:     Style:\r\n", ch);

      send_to_char ( "--------------------------------------------------------------------------------\r\n", ch );

      output_praclist ( ch, skilllist );

      send_to_char ( "\r\n", ch );

      if ( GET_CLASS ( ch ) == CLASS_BARD )
        send_to_char ( "$B#$R denotes a song which requires an instrument.\n\r", ch );

      return eSUCCESS;

    }

  if (GET_POS(ch) == POSITION_SLEEPING) {
   send_to_char("You cannot practice in your sleep.\r\n",ch);
   return eSUCCESS;
  }
  skillnumber = search_skills(arg, skilllist);
    
  if (skillnumber == -1) {
    return eFAILURE;
  }

  if(GET_LEVEL(ch) < skilllist[skillnumber].levelavailable) {
    send_to_char("You aren't advanced enough for that yet.\r\n", ch);
    return eSUCCESS;
  }

  x = skilllist[skillnumber].skillnum;
  known = has_skill(ch, x);

  // we can only train them if they already know it, or if we're the trainer for that skill
  if (!known)
  {
   if (GET_CLASS(ch) != GET_CLASS(owner))
   {
	do_say(owner, "I am sorry, I cannot teach you that.  You will have to find another trainer.",9);
       return eSUCCESS;
   }
   else {
      struct skill_quest *sq;
      if ((sq=find_sq(skilllist[skillnumber].skillname)) != NULL && sq->message && IS_SET(sq->clas, 1<<(GET_CLASS(ch)-1)))
      {
	  mprog_driver(sq->message, owner, ch, NULL, NULL, NULL, NULL);
	  switch(skillnumber)
	  {
	      case SPELL_VAMPIRIC_AURA:
	      case SPELL_DIVINE_INTER:
	      case SKILL_BULLRUSH:
	      case SPELL_HOLY_AURA:
	      case SKILL_SPELLCRAFT:
	      case SKILL_COMBAT_MASTERY:
	      case KI_MEDITATION+KI_OFFSET:
              case SPELL_CONJURE_ELEMENTAL:
              case SPELL_RELEASE_ELEMENTAL:
	      case SKILL_CRIPPLE:
	      case SKILL_NAT_SELECT:
		do_say(owner, "Alternately, should you feel that you are not up to the task of an exciting quest, you can seek the Skills Master west of town.",9);
		do_say(owner, "Rumour has it he will teach certain skills for a hefty fee.  He will give you a LIST of what he has to offer.",9);
		return eFAILURE;
	      default: break;
	  }
	  return eSUCCESS;
      }
   }

  // If this is a profession-specific skill and we are a mortal without that profession, disallow
  if ( skilllist[skillnumber].group && IS_PC ( ch ) && skilllist[skillnumber].group != ch->pcdata->profession )
  {
    csendf ( ch, "You must join the %s profession in order to learn that.\n\r", find_profession (ch->c_class, skilllist[skillnumber].group) );
    return eSUCCESS;
  }

  }
  if (ch->pcdata->practices <= 0) {
    send_to_char("You do not seem to be able to practice now.\n\r", ch);
    return eSUCCESS;
  }

  if(known >= get_max(ch, x)) {
     do_emote(owner, "eyes you up and down.", 9);
     do_say(owner, "Taking into account your current attributes, your",9);
     do_say(owner, "maximum proficiency in this ability has been reached.", 9);
     return eSUCCESS;
  }

  if (known >= skilllist[skillnumber].maximum) {
    send_to_char("You are already learned in this area.\n\r", ch);
    return eSUCCESS;
  }
  float maxlearn = (float)skilllist[skillnumber].maximum;
  maxlearn *= 0.5;

  if (known >= ( GET_LEVEL(ch) * 2 )) {
    send_to_char("You are not experienced enough to practice that any further right now.\n\r", ch);
    return eSUCCESS;
  }
  if (known >= maxlearn)
  {
    send_to_char("You cannot learn more here.. you need to go out into the world and use it.\r\n",ch);
    return eSUCCESS;
  }
  switch(skillnumber)
  {
      case SPELL_VAMPIRIC_AURA:
      case SPELL_DIVINE_INTER:
      case SKILL_BULLRUSH:
      case SPELL_HOLY_AURA:
      case SKILL_SPELLCRAFT:
      case SKILL_COMBAT_MASTERY:
      case KI_MEDITATION+KI_OFFSET:
      case SKILL_CRIPPLE:
      case SPELL_CONJURE_ELEMENTAL:
      case SPELL_RELEASE_ELEMENTAL:
      case SKILL_NAT_SELECT:
	do_say(owner, "I cannot teach you that. You need to learn it by yourself.",9);

	return eFAILURE;
      default: break;
  }

   if (!known)
  switch (GET_CLASS(ch))
  {
    case CLASS_WARRIOR:
      do_say(owner, "Yar! I can be teachin' ye that skill myself! It should only take but a moment.",9);
      break;
    case CLASS_BARBARIAN:
      do_say(owner,"Hah! That easy to learn! I teach you meself.",9);
      break;
    case CLASS_THIEF:
      do_say(owner,"So young rogue, you wish to advance your skills.  I can teach you of this particular one myself.",9);
	break;
     case CLASS_MONK:
      do_say(owner,"Ahh, well met grasshopper!  I can teach you of this from my own knowledge.",9);
	break;
     case CLASS_RANGER:
     do_say(owner,"My woodland lore is more than sufficient to teach you this myself young apprentice!",9);
	break;
     case CLASS_ANTI_PAL:
     do_say(owner,"Ahh, young dark lord, it is but a simple matter to learn this ability.  Follow my instruction.",9);
	break;
     case CLASS_PALADIN:
     do_say(owner,"This ability is one that I am capable of teaching you myself young novice.  Observe closely.",9);
	break;
     case CLASS_BARD:
      do_say(owner, "Ahh young prodigy, that is a tune with which I myself am familiar! Allow me to show you...",9);
	break;
     case CLASS_MAGIC_USER:
     do_say(owner,"Ahh young apprentice, this is a simple matter for me to teach you if you are capable of comprehending.",9);
	break;
     case CLASS_CLERIC:
     do_say(owner, "Well met Acolyte!  I shall pray for you to receive knowledge of the blessing you request.",9);
	break;
     case CLASS_DRUID:
     do_say(owner, "Nature be with you, young druid.  I can teach you the ability you seek myself if you are willing.",9);
	break;
  }

  send_to_char("You practice for a while...\n\r", ch);
  ch->pcdata->practices--;

  percent = 1 + (int)int_app[GET_INT(ch)].learn_bonus;

  learn_skill(ch, x, percent, skilllist[skillnumber].maximum);
    
  if (known >= skilllist[skillnumber].maximum) {
    send_to_char("You are now learned in this area.\n\r", ch);
    return eSUCCESS;
  }

  return eSUCCESS;
}

int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,        
          struct char_data *owner)
{
  int64 exp_needed;
  int x = 0;

  if(cmd == 171 && !IS_MOB(ch)) {         /*   gain crap...  */

    if((GET_LEVEL(ch) >= IMMORTAL) || (GET_LEVEL(ch) == MAX_MORTAL)) {
      send_to_char("You have already reached the highest level!\n\r", ch);
      return eSUCCESS;
    }

   // TODO - make it so you have to be at YOUR guildmaster to gain

    if (GET_LEVEL(ch) == 50)
    { // To get past 50 you need to know your q skill.
      int skl = -1;
      switch (GET_CLASS(ch))
      {
      case CLASS_MAGE:
        skl = SKILL_SPELLCRAFT;
        break;
      case CLASS_BARBARIAN:
        skl = SKILL_BULLRUSH;
        break;
      case CLASS_PALADIN:
        skl = SPELL_HOLY_AURA;
        break;
      case CLASS_MONK:
        skl = KI_OFFSET + KI_MEDITATION;
        break;
      case CLASS_WARRIOR:
        skl = SKILL_COMBAT_MASTERY;
        break;
      case CLASS_THIEF:
        skl = SKILL_CRIPPLE;
        break;
      case CLASS_RANGER:
        skl = SKILL_NAT_SELECT;
        break;
      case CLASS_CLERIC:
        skl = SPELL_DIVINE_INTER;
        break;
      case CLASS_ANTI_PAL:
        skl = SPELL_VAMPIRIC_AURA;
        break;
      case CLASS_DRUID:
        skl = SPELL_CONJURE_ELEMENTAL;
        break;
      case CLASS_BARD:
        skl = SKILL_SONG_HYPNOTIC_HARMONY;
        break;
      }
      if (!has_skill(ch, skl))
      {
        send_to_char("You need to learn your Quest Skill before you can progress further.\r\n", ch);
        return eSUCCESS;
      }
    }
    exp_needed = exp_table[(int)GET_LEVEL(ch) + 1];

    if(exp_needed > GET_EXP(ch)) {
      x = (int)(exp_needed - GET_EXP(ch));

      csendf(ch, "You need %d experience to level.\n\r", x);
      return eSUCCESS;
    }

    send_to_char ("You raise a level!\n\r", ch);
    logf(IMMORTAL, LOG_MORTAL, "%s (%s) just gained level %d.", GET_NAME(ch), pc_clss_types3[(int)GET_CLASS(ch)], GET_LEVEL(ch)+1);

    GET_LEVEL(ch) +=1;
    advance_level(ch, 0);
    GET_EXP(ch) -= (int)exp_needed;
    int bonus = (GET_LEVEL(ch)-50)*250;
    if (bonus > 0 && MAX_MORTAL == 60)
    {
        char buf[MAX_STRING_LENGTH];
        if (GET_LEVEL(ch) == 60)
	{
	     sprintf(buf, "You have truly reached the highest level of %s mastery.", pc_clss_types3[GET_CLASS(ch)]);
	     do_say(owner, buf, 9);
 	     do_say(owner,"As such, the guild will imbue into you some of our most powerful magic and grant you freedom from hunger and thirst!",9);
//	     send_to_char(buf, ch);
	} else {
	 sprintf(buf, "Well done master %s, the guild has collected a tithe to reward your continued support of our profession.",pc_clss_types3[GET_CLASS(ch)]);

	 do_say(owner, buf, 9);
	sprintf(buf, "Your guildmaster gives you %d platinum coins.\r\n", bonus);
	 send_to_char(buf,ch);
	 GET_PLATINUM(ch) += bonus;
	}
    }
    return eSUCCESS;
  }

  if(cmd == 80) {  // remort crap
    int groupnumber;

    if(IS_SET(ch->pcdata->toggles, PLR_CLS_TREE_A)) {
      REMOVE_BIT(ch->pcdata->toggles, PLR_CLS_TREE_A);
      groupnumber = 1;
    } else if(IS_SET(ch->pcdata->toggles, PLR_CLS_TREE_B)) {
      REMOVE_BIT(ch->pcdata->toggles, PLR_CLS_TREE_B);
      groupnumber = 2;
    } else if(IS_SET(ch->pcdata->toggles, PLR_CLS_TREE_C)) {
      REMOVE_BIT(ch->pcdata->toggles, PLR_CLS_TREE_C);
      groupnumber = 3;
    } else {
      send_to_char("You have not even chosen a profession out of which to remort!\n\r", ch);
      return eSUCCESS;
    }

    send_to_char("Your profession skills have been reset.\n\r", ch);
    struct class_skill_defines * skilllist = get_skill_list(ch);
    struct char_skill_data * skill;

    for(int i = 0; *skilllist[i].skillname != '\n'; i++) 
      if(skilllist[i].group == groupnumber) {
        skill = ch->skills;
        while(skill) {
          if(skill->skillnum == skilllist[i].skillnum)
            skill->learned = 0;
          skill = skill->next;
        }
      }

    send_to_char("You have remorted back to level 50.\n\r", ch);
    if(GET_LEVEL(ch) <= MAX_MORTAL) GET_LEVEL(ch) = 50;
    SET_BIT(ch->pcdata->toggles, PLR_REMORTED);

    do_save(ch,"",666);
    return eSUCCESS;
  }  

  if((cmd != 164)) 
   return eFAILURE;
  
  if(IS_MOB(ch)) {
    send_to_char("Why practice?  You're just going to die anyway...\r\n", ch);
    return eFAILURE;
  }

  for(; *arg==' '; arg++);  // skip whitespace
    
  if(!*arg)
  {
    skills_guild(ch, arg, owner);
  }
  else
  {
    if(IS_SET(skills_guild(ch, arg, owner), eSUCCESS))
      return eSUCCESS;
    else if (search_skills(arg,g_skills) != -1)
       do_say(owner,"Seek out the SKILLS MASTER in the forests west of Sorpigal to learn this ability.", 9);
    else send_to_char("You do not know of this ability...\n\r", ch);
  }

  return eSUCCESS;
}

int skill_master(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, 
                 struct char_data * invoker)
{
  char buf[MAX_STRING_LENGTH];
  int number, i, percent;
  int learned = 0;
  class_skill_defines * skilllist = get_skill_list(ch);

  if(IS_MOB(ch)) {
    send_to_char("Why practice?  You're just going to die anyway...\r\n", ch);
    return eFAILURE;
  }

  if ((cmd != 164) && (cmd != 170) && (cmd != 56) && cmd != 59 )
    return eFAILURE;
  
  for(; *arg==' '; arg++);

  int skl = -1;
  switch (GET_CLASS(ch))
  {
    case CLASS_MAGE: skl = SKILL_SPELLCRAFT; break;
    case CLASS_BARBARIAN: skl = SKILL_BULLRUSH; break;
    case CLASS_PALADIN: skl = SPELL_HOLY_AURA; break;
    case CLASS_MONK: skl = KI_OFFSET+KI_MEDITATION; break;
    case CLASS_WARRIOR: skl = SKILL_COMBAT_MASTERY; break;
    case CLASS_THIEF: skl = SKILL_CRIPPLE; break;
    case CLASS_RANGER: skl = SKILL_NAT_SELECT; break;
    case CLASS_CLERIC: skl = SPELL_DIVINE_INTER; break;
    case CLASS_ANTI_PAL: skl = SPELL_VAMPIRIC_AURA; break;
    case CLASS_DRUID: skl = SPELL_CONJURE_ELEMENTAL; break;
    case CLASS_BARD: skl = SKILL_SONG_HYPNOTIC_HARMONY; break;
  }
  if (cmd == 59)
  {
    char buf[MAX_STRING_LENGTH];
   
    sprintf(buf, "This is what is available:\r\n[2000 platinum] %s (type $B$2buy questskill$R to purchase it)\r\n",get_skill_name(skl));
    send_to_char(buf,ch);
    return eSUCCESS;
  }
  if (cmd == 56)
  {
    if (GET_LEVEL(ch) < 50)
    {
	do_say(invoker,"You have not obtained a high enough level to buy anything from me.",9);
        return eSUCCESS;
    }
    if (str_cmp(arg, "questskill"))
    {
      do_say(invoker,"I cannot teach you that. Type LIST to see what is available.",9);
//      do_say(invoker,"I could teach you your Quest Skill, for a price of 2000 platinum coins.",9);
 //     do_say(invoker,"Just \"buy questskill\" to obtain it.",9);
      return eSUCCESS;
    }
    if (has_skill(ch, skl))
    {
      do_say(invoker,"I cannot teach you anything further.",9);
      return eSUCCESS;
    }
    if (GET_PLATINUM(ch) < 2000)
    {
      do_say(invoker,"You can't afford it, you need 2000 platinum!",9);
      return eSUCCESS;
    }
    GET_PLATINUM(ch) -= 2000;
    do_say(invoker, "Okay, you've got a deal!",9);
    learn_skill(ch, skl, 1, 1);

    extern void prepare_character_for_sixty(CHAR_DATA *ch);
    prepare_character_for_sixty(ch);
    sprintf(buf, "$BYou have learned the basics of %s.$R\n\r", get_skill_name(skl));
    send_to_char(buf,ch);

    switch (GET_CLASS(ch))
    {
        case CLASS_DRUID:
        learn_skill(ch, SPELL_RELEASE_ELEMENTAL, 1, 1);
        sprintf(buf, "$BYou have learned the basics of %s.$R\n\r", get_skill_name(SPELL_RELEASE_ELEMENTAL));
        send_to_char(buf, ch);
        break;
    }
    return eSUCCESS;
  }

  if (!*arg) {
      sprintf(buf,"You have %d practice sessions left.\n\r", ch->pcdata->practices);
      send_to_char(buf, ch);
      send_to_char("You can practice any of these skills:\n\r", ch);
      for(i=0; *g_skills[i].skillname != '\n';i++) 
      {
        int known = has_skill(ch, g_skills[i].skillnum);
        if(GET_LEVEL(ch) < g_skills[i].levelavailable) 
          continue;
        sprintf(buf, " %-20s%14s   (Level %2d)\r\n", g_skills[i].skillname,
                   how_good(known), g_skills[i].levelavailable);
        send_to_char(buf, ch);
      }
      return eSUCCESS;
    }
    number = search_skills(arg,g_skills);
    
    if (number == -1) {
      if(!skilllist) return eFAILURE;
      if(search_skills(arg, skilllist) != -1)
         do_say(invoker,"You must speak with your guildmaster to learn such a complicated ability.", 9);
      else send_to_char("You do not know of this skill...\n\r", ch);
      return eSUCCESS;
    }

    if (ch->pcdata->practices <= 0) {
      send_to_char("You do not seem to be able to practice now.\n\r", ch);
      return eSUCCESS;
    }
    learned = has_skill(ch, g_skills[number].skillnum);

    if (learned >= g_skills[number].maximum) {
      send_to_char("You are already learned in this area.\n\r", ch);
      return eSUCCESS;
    }
    if (learned >= ( GET_LEVEL(ch) * 3 ) ) {
      send_to_char("You aren't experienced enough to practice that any further right now.\n\r", ch);
      return eSUCCESS;
    }

    send_to_char("You practice for a while...\n\r", ch);
    ch->pcdata->practices--;
    
    percent = 1+(int)int_app[GET_INT(ch)].learn_bonus;

    learn_skill(ch, g_skills[number].skillnum, percent, g_skills[number].maximum);
    learned = has_skill(ch, g_skills[number].skillnum);
    
    if (learned >= g_skills[number].maximum) {
      send_to_char("You are now learned in this area.\n\r", ch);
      return eSUCCESS;
    }
    return eSUCCESS;
}

int get_stat(CHAR_DATA *ch, int stat)
{
      switch (stat)
      {
        case STRENGTH:
          return GET_RAW_STR(ch);
          break;
        case INTELLIGENCE:
          return GET_RAW_INT(ch);
          break;
        case WISDOM:
          return GET_RAW_WIS(ch);
          break;
        case DEXTERITY:
          return GET_RAW_DEX(ch);
          break;
        case CONSTITUTION:
          return GET_RAW_CON(ch);
          break;
      };
 return 0;
}

int get_stat_bonus(CHAR_DATA *ch, int stat)
{
  int bonus = 0;
  switch(stat) {
    case STRDEX:
      bonus = MAX(0,get_stat(ch,STRENGTH)-15) + MAX(0,get_stat(ch, DEXTERITY)-15);break;
    case STRCON:
      bonus = MAX(0,get_stat(ch,STRENGTH)-15) + MAX(0,get_stat(ch, CONSTITUTION)-15);break;
    case STRINT:
      bonus = MAX(0,get_stat(ch,STRENGTH)-15) + MAX(0,get_stat(ch, INTELLIGENCE)-15);break;
    case STRWIS:
      bonus = MAX(0,get_stat(ch,STRENGTH)-15) + MAX(0,get_stat(ch, WISDOM)-15);break;
    case DEXCON:
      bonus = MAX(0,get_stat(ch,DEXTERITY)-15) + MAX(0,get_stat(ch, CONSTITUTION)-15);break;
    case DEXINT:
      bonus = MAX(0,get_stat(ch,DEXTERITY)-15) + MAX(0,get_stat(ch, INTELLIGENCE)-15);break;
    case DEXWIS:
      bonus = MAX(0,get_stat(ch,DEXTERITY)-15) + MAX(0,get_stat(ch, WISDOM)-15);break;
    case CONINT:
      bonus = MAX(0,get_stat(ch,CONSTITUTION)-15) + MAX(0,get_stat(ch, INTELLIGENCE)-15);break;
    case CONWIS:
      bonus = MAX(0,get_stat(ch,CONSTITUTION)-15) + MAX(0,get_stat(ch, WISDOM)-15);break;
    case INTWIS:
      bonus = MAX(0,get_stat(ch,INTELLIGENCE)-15) + MAX(0,get_stat(ch, WISDOM)-15);break;
    default: bonus = 0; break;
  }
  
  return bonus;
}


// TODO - go ahead and remove 'learned' from everywhere to use it.
// We can't always pass it in, since in 'group' type spells or
// object affects someone that doesn't have the skill is getting a
// valid amount passed in.

void skill_increase_check(char_data *ch, int skill, int learned, int difficulty)
{
  int chance, maximum;

  if (IS_MOB(ch))
  {
    return;
  }

  if (ch->in_room < 1)
  {
    return;
  }

  if (IS_SET(world[ch->in_room].room_flags, NOLEARN))
  {
    return;
  }

  if (IS_SET(world[ch->in_room].room_flags, SAFE))
  {
    return;
  }

  if (difficulty < 1)
  {
    logf(IMMORTAL, LOG_BUG, "%s had an invalid skill level of %d in skill %d.", GET_NAME(ch), difficulty, skill);
    return; // Skill w/out difficulty.
  }

  if (skill == SKILL_DODGE && affected_by_spell(ch, SKILL_DEFENDERS_STANCE))
  {
    return;
  }

  if (skill == SKILL_COMBAT_MASTERY && number(0, 8) > 0)
  {
    return;
  }

  learned = has_skill(ch, skill);

  // If we don't know the skill yet then we can't learn it any more
  if (learned < 1)
  {
    return; // get out if i don't have the skill
  }

  // If we've already learned it 2*level or more then we can't learn it anymore until we gain more levels
  if (learned >= (GET_LEVEL(ch) * 2))
  {
    return;
  }

  class_skill_defines *skilllist = get_skill_list(ch);
  if (!skilllist)
  {
    return; // class has no skills by default
  }

  maximum = 0;
  int i;
  for (i = 0; *skilllist[i].skillname != '\n'; i++)
  {
    if (skilllist[i].skillnum == skill)
    {
      maximum = skilllist[i].maximum;
      break;
    }
  }

  if (maximum < 1)
  {
    skilllist = g_skills;
    for (i = 0; *skilllist[i].skillname != '\n'; i++)
    {
      if (skilllist[i].skillnum == skill)
      {
        maximum = skilllist[i].maximum;
        break;
      }
    }
  }

  if (maximum < 1)
  {
    return;
  }

  float percent = maximum * 0.75;

  if (skilllist[i].attrs)
  {
    percent += maximum / 100.0 * get_stat_bonus(ch, skilllist[i].attrs);
  }

  percent = MIN(maximum, percent);
  percent = MAX(maximum * 0.75, percent);

  if (learned >= (int)percent)
  {
    return;
  }

  chance = number(1, 101);
  if (learned < 15)
  {
    chance += 5;
  }

  int oi = 101;
  if (difficulty > 500)
  {
    oi = 100;
    difficulty -= 500;
  }

  switch (difficulty)
  {
  case SKILL_INCREASE_EASY:
    if (oi == 101)
    {
      oi = 94;
    }

    oi -= int_app[GET_INT(ch)].easy_bonus;
    break;
  case SKILL_INCREASE_MEDIUM:
    if (oi == 101)
    {
      oi = 96;
    }

    oi -= int_app[GET_INT(ch)].medium_bonus;
    break;
  case SKILL_INCREASE_HARD:
    if (oi == 101)
    {
      oi = 98;
    }

    oi -= int_app[GET_INT(ch)].hard_bonus;
    break;
  default:
    log("Illegal difficulty value sent to skill_increase_check", IMMORTAL, LOG_BUG);
    break;
  }

  if (oi > chance)
  {
    return;
  }
  // figure out the name of the affect (if any)
  const char *skillname = get_skill_name(skill);

  if (!skillname)
  {
    csendf(ch, "Attempt to increase an unknown skill %d.  Tell a god. (bug)\r\n", skill);
    logf(IMMORTAL, LOG_BUG, "skill_increase_check(%s, skill=%d, learned=%d, difficulty=%d): Attempt to increase an unknown skill.", GET_NAME(ch), skill, learned, difficulty);
    return;
  }

  // increase the skill by one
  learn_skill(ch, skill, 1, get_max(ch, skill));
  learned = has_skill(ch, skill);
  csendf(ch, "$R$B$5You feel more competent in your %s ability. It increased to %d out of %d.$R\r\n", skillname, learned, get_max(ch, skill));
}

void verify_max_stats(CHAR_DATA *ch)
{
          struct char_skill_data * curr = ch->skills;
          while(curr) {

	   if (get_max(ch, curr->skillnum) && get_max(ch, curr->skillnum) < curr->learned)
		curr->learned = get_max(ch, curr->skillnum);
	
	   curr = curr->next;
          }
  
}

int get_max(CHAR_DATA *ch, int skill)
{
  class_skill_defines *skilllist = get_skill_list(ch);
  if (!skilllist)
    skilllist = g_skills;
 
  int maximum = 0;
  int i = 0;
  for (; *skilllist[i].skillname != '\n'; i++)
  {
    if (skilllist[i].skillnum == skill)
    {
      maximum = skilllist[i].maximum;
      break;
    }
  }

  if (!maximum && skilllist != g_skills)
  {
    skilllist = g_skills;
    for (i = 0; *skilllist[i].skillname != '\n'; i++)
      if (skilllist[i].skillnum == skill)
      {
        maximum = skilllist[i].maximum;
        break;
      }
  }

  float percent = maximum * 0.75;

  if (maximum && skilllist[i].attrs)
    percent += maximum / 100.0 * get_stat_bonus(ch, skilllist[i].attrs);

  percent = MIN(maximum, percent);
  percent = MAX(maximum * 0.75, percent);

  return (int)percent;
}

void check_maxes(CHAR_DATA *ch)
{
   int maximum;
   class_skill_defines * skilllist = get_skill_list(ch);
   if(!skilllist)
     return;  // class has no skills by default
   maximum = 0;
   int i;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     {
       maximum = skilllist[i].maximum;
       float percent = maximum*0.75;
       if (skilllist[i].attrs)
            percent += maximum/100.0 * get_stat_bonus(ch,skilllist[i].attrs);

       percent = MIN(maximum, percent);
       percent = MAX(maximum*0.75, percent);

       percent = (int)percent;

       if (has_skill(ch,skilllist[i].skillnum) > percent)
       {
	  struct char_skill_data * curr = ch->skills;

  	  while(curr)
    	  if(curr->skillnum == skilllist[i].skillnum)
      	  break;
    	  else curr = curr->next;

	  if (curr) curr->learned = (int)percent;
       }
   }
   skilllist = g_skills;
   for(i = 0; *skilllist[i].skillname != '\n'; i++)
     {
       maximum = skilllist[i].maximum;
       float percent = maximum*0.75;
       if (skilllist[i].attrs)
            percent += maximum/100.0 * get_stat_bonus(ch,skilllist[i].attrs);

       percent = MIN(maximum, percent);
       percent = MAX(maximum*0.75, percent);

       percent = (int)percent;

       if (has_skill(ch,skilllist[i].skillnum) > percent)
       {
	  struct char_skill_data * curr = ch->skills;

  	  while(curr)
    	  if(curr->skillnum == skilllist[i].skillnum)
      	  break;
    	  else curr = curr->next;

	  if (curr) curr->learned = (int)percent;
       }
   }

}
