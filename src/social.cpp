// social.C
// Description:  Anything to do with socials

#include <cstring>
#include <cstdlib> // qsort()

#include "DC/fileinfo.h" // SOCIAL_FILE
#include "DC/structs.h"  // MAX_INPUT_LENGTH
#include "DC/room.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/mobile.h"
#include "DC/connect.h"
#include "DC/levels.h"
#include "DC/player.h"
#include "DC/social.h"
#include "DC/handler.h"
#include "DC/act.h"
#include "DC/db.h"
#include "DC/interp.h" // len_cmp
#include "DC/returnvals.h"

// storage of socials
struct social_messg *soc_mess_list; // head of social array
int32_t num_socials;                // number of actual socials (50 = 0-49)
int32_t social_array_size;          // size of actual array (since we allocate in chunks)

struct social_messg *find_social(QString arg);

command_return_t Character::check_social(QString pcomm)
{
  QString arg = {}, buf = {};
  struct social_messg *action = {};
  Character *vict = {};

  std::tie(pcomm, arg) = half_chop(pcomm);

  if (!(action = find_social(pcomm)))
  {
    return SOCIAL_false;
  }

  if (isPlayer() && isSet(player->punish, PUNISH_NOEMOTE))
  {
    this->sendln("You are anti-social!");
    return SOCIAL_true;
  }

  switch (GET_POS(this))
  {
  case position_t::DEAD:
    this->sendln("Lie still; you are DEAD.");
    return SOCIAL_true;

  case position_t::STUNNED:
    this->sendln("You are too stunned to do that.");
    return SOCIAL_true;

  case position_t::SLEEPING:
    this->sendln("In your dreams, or what?");
    return SOCIAL_true;
  }

  if (isSet(DC::getInstance()->world[this->in_room].room_flags, QUIET))
  {
    this->sendln("SHHHHHH!! Can't you see people are trying to read?");
    return SOCIAL_true;
  }

  if (action->char_found)
  {
    std::tie(buf, arg) = half_chop(arg);
  }
  else
  {
    buf = {};
  }

  if (buf.isEmpty())
  {
    if (action->char_no_arg)
    {
      act(action->char_no_arg, this, 0, 0, TO_CHAR, 0);
    }

    if (action->others_no_arg)
    {
      act(action->others_no_arg, this, 0, 0, TO_ROOM, (action->hide) ? INVIS_NULL : 0);
    }
    return SOCIAL_true_WITH_NOISE;
  }

  if (!(vict = get_char_room_vis(buf)))
  {
    if (action->not_found)
    {
      act(action->not_found, this, 0, 0, TO_CHAR, 0);
    }
  }
  else if (vict == this)
  {
    if (action->char_auto)
      act(action->char_auto, this, 0, 0, TO_CHAR, 0);
    if (action->others_auto)
      act(action->others_auto, this, 0, 0, TO_ROOM,
          (action->hide) ? INVIS_NULL : 0);
  }
  else if (GET_POS(vict) < action->min_victim_position)
  {
    act("$N is not in a proper position for that.",
        this, 0, vict, TO_CHAR, 0);
  }
  else
  {
    if (action->char_found)
      act(action->char_found, this, 0, vict, TO_CHAR, 0);
    if (action->others_found)
      act(action->others_found, this, 0, vict, TO_ROOM,
          NOTVICT | ((action->hide) ? INVIS_NULL : 0));
    if (action->vict_found)
      act(action->vict_found, this, 0, vict, TO_VICT,
          (action->hide) ? INVIS_NULL : 0);
  }

  return SOCIAL_true_WITH_NOISE;
}

char *fread_social_string(FILE *fl)
{
  char buf[MAX_STRING_LENGTH], *rslt;

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl))
  {
    logentry(QStringLiteral("Fread_social_string - unexpected EOF."), IMMORTAL, LogChannels::LOG_BUG);
    exit(0);
  }

  if (*buf == '#')
    return (0);

  // strip the \n
  *(buf + strlen(buf) - 1) = '\0';

  rslt = str_dup(buf);
  return (rslt);
}

// read one social
// return true on success
// return false on 'EOF'
int read_social_from_file(int32_t num_social, FILE *fl)
{
  char tmp[MAX_INPUT_LENGTH];
  int hide, min_pos;

  fscanf(fl, " %s ", tmp);
  if (feof(fl))
    return false;
  fscanf(fl, " %d %d \n", &hide, &min_pos);

  // read strings that will always be there
  soc_mess_list[num_social].name = str_dup(tmp);
  soc_mess_list[num_social].hide = hide;
  soc_mess_list[num_social].min_victim_position = static_cast<decltype(soc_mess_list[num_social].min_victim_position)>(min_pos);
  soc_mess_list[num_social].char_no_arg = fread_social_string(fl);
  soc_mess_list[num_social].others_no_arg = fread_social_string(fl);
  soc_mess_list[num_social].char_found = fread_social_string(fl);
  // if no char_found, then the social is done, and the ones below won't be there
  if (!soc_mess_list[num_social].char_found)
    return true;
  soc_mess_list[num_social].others_found = fread_social_string(fl);
  soc_mess_list[num_social].vict_found = fread_social_string(fl);
  soc_mess_list[num_social].not_found = fread_social_string(fl);
  soc_mess_list[num_social].char_auto = fread_social_string(fl);
  soc_mess_list[num_social].others_auto = fread_social_string(fl);
  return true;
}

// this function used by qsort to sort the social array
int compare_social_sort(const void *A, const void *B)
{
  int i;
  for (i = 0;; i++)
    if (!(*(((social_messg *)A)->name + i) && *(((social_messg *)B)->name + i)))
      break;
    else if (*(((social_messg *)A)->name + i) > *(((social_messg *)B)->name + i))
      return 1;
    else if (*(((social_messg *)A)->name + i) < *(((social_messg *)B)->name + i))
      return -1;
  // Match so far..
  if (strlen(((social_messg *)A)->name) > strlen(((social_messg *)B)->name))
    return 1;
  if (strlen(((social_messg *)A)->name) < strlen(((social_messg *)B)->name))
    return -1;
  return 0;
}

// this function used by qsort to search the social array
int compare_social_search(const void *A, const void *B)
{
  int i;
  for (i = 0;; i++)
    if (!(*(((char *)A) + i) && *(((social_messg *)B)->name + i)))
      break;
    else if (*(((char *)A) + i) > *(((social_messg *)B)->name + i))
      return 1;
    else if (*(((char *)A) + i) < *(((social_messg *)B)->name + i))
      return -1;
  // Match so far..
  if (strlen(((char *)A)) > strlen(((social_messg *)B)->name))
    return 1;
  if (strlen(((char *)A)) < strlen(((social_messg *)B)->name))
    return 0;
  return 0;
  //  return len_cmp( (char *) A, ((social_messg *) B)->name );
}

void boot_social_messages(void)
{
  FILE *fl;

  // initialize our array
  num_socials = 0;
  social_array_size = 450; // Guess on number of socials.  Closer this is to actual without
                           // going over saves on memory and boot up speed since we won't
                           // have to realloc as often.
#ifdef LEAK_CHECK
  soc_mess_list = (struct social_messg *)calloc(social_array_size, sizeof(struct social_messg));
#else
  soc_mess_list = (struct social_messg *)dc_alloc(social_array_size, sizeof(struct social_messg));
#endif

  if (!(fl = fopen(SOCIAL_FILE, "r")))
  {
    perror("Can't open social file in boot_social_messages");
    abort();
  }

  for (;;)
  {
    // do we have room?
    if (num_socials >= social_array_size)
    {
      social_array_size += 10;
      // realloc the list to have enough memory for the new array size
#ifdef LEAK_CHECK
      soc_mess_list = (struct social_messg *)realloc(soc_mess_list,
                                                     (sizeof(struct social_messg) * social_array_size));
#else
      soc_mess_list = (struct social_messg *)dc_realloc(soc_mess_list,
                                                        (sizeof(struct social_messg) * social_array_size));
#endif
      // clear the memory we just alloc'd
      memset((soc_mess_list + num_socials), 0, (sizeof(struct social_messg) * 10));
    }
    if (!read_social_from_file(num_socials, fl))
      break;
    num_socials++;
  }

  // sort it!
  qsort(soc_mess_list, num_socials, sizeof(social_messg), compare_social_sort);

  fclose(fl);
}

struct social_messg *find_social(QString arg)
{
  // now uses a linear search
  int i;

  for (i = 1; i < num_socials; i++)
    if (!compare_social_search((void *)arg.toStdString().c_str(), (void *)&soc_mess_list[i]))
      return &soc_mess_list[i];

  return nullptr;
  //    sprintf(buf + strlen(buf), "%18s", soc_mess_list[i].name);

  //  return (social_messg *) bsearch(arg, soc_mess_list, num_socials, sizeof(struct social_messg), compare_social_search);
}

void DC::clean_socials_from_memory()
{
  if (!soc_mess_list)
    return;

  for (int i = 0; i < num_socials; i++)
  {
    if (soc_mess_list[i].name)
      dc_free(soc_mess_list[i].name);
    if (soc_mess_list[i].char_no_arg)
      dc_free(soc_mess_list[i].char_no_arg);
    if (soc_mess_list[i].others_no_arg)
      dc_free(soc_mess_list[i].others_no_arg);
    if (soc_mess_list[i].char_found)
      dc_free(soc_mess_list[i].char_found);
    if (soc_mess_list[i].others_found)
      dc_free(soc_mess_list[i].others_found);
    if (soc_mess_list[i].vict_found)
      dc_free(soc_mess_list[i].vict_found);
    if (soc_mess_list[i].not_found)
      dc_free(soc_mess_list[i].not_found);
    if (soc_mess_list[i].char_auto)
      dc_free(soc_mess_list[i].char_auto);
    if (soc_mess_list[i].others_auto)
      dc_free(soc_mess_list[i].others_auto);
  }

  dc_free(soc_mess_list);
  soc_mess_list = nullptr;
}

int do_social(Character *ch, char *argument, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  *buf = '\0';

  for (i = 1; i < num_socials; i++)
  {
    sprintf(buf + strlen(buf), "%18s", soc_mess_list[i].name);
    if (!(i % 4))
    {
      strcat(buf, "\r\n");
      ch->send(buf);
      *buf = '\0';
    }
  }
  if (*buf)
    ch->send(buf);

  sprintf(buf, "\r\nCurrent Socials:  %d\r\n", i);
  ch->send(buf);
  return eSUCCESS;
}
