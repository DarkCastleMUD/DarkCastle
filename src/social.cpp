// social.C
// Description:  Anything to do with socials 

extern "C"
{
  #include <string.h>
  #include <stdlib.h> // qsort()
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <fileinfo.h> // SOCIAL_FILE
#include <structs.h> // MAX_INPUT_LENGTH
#include <room.h>
#include <character.h>
#include <utility.h>
#include <mobile.h>
#include <connect.h>
#include <levels.h>
#include <player.h>
#include <social.h>
#include <handler.h>
#include <act.h>
#include <db.h>
#include <interp.h> // len_cmp
#include <returnvals.h>

extern CWorld world;
 
// storage of socials
struct social_messg * soc_mess_list;  // head of social array
long int num_socials;                 // number of actual socials (50 = 0-49)
long int social_array_size;           // size of actual array (since we allocate in chunks)

struct social_messg *find_social(char *arg);

int check_social( struct char_data *ch, char *pcomm, int length, char *arg )
{
    char buf[MAX_INPUT_LENGTH];
    struct social_messg *action = 0;
    struct char_data *vict;
//    char argz[MAX_INPUT_LENGTH];
  //  pcomm = one_argument(pcomm, argz);
    if (*arg) arg++;
    int i=0;
     while (*(pcomm+i)!='\0')
        if (*(pcomm+i)==' ')
	  { *(pcomm+i) = '\0';   break;}
	else
	  i++;
    if(!(action = find_social(pcomm)))
      return SOCIAL_FALSE;

    if(!IS_NPC(ch) && IS_SET(ch->pcdata->punish, PUNISH_NOEMOTE)) {
	send_to_char( "You are anti-social!\n\r", ch );
	return SOCIAL_TRUE;
    }

    switch(GET_POS(ch)) {
    case POSITION_DEAD:
	send_to_char( "Lie still; you are DEAD.\n\r", ch );
	return SOCIAL_TRUE;

    case POSITION_STUNNED:
	send_to_char( "You are too stunned to do that.\n\r", ch );
	return SOCIAL_TRUE;

    case POSITION_SLEEPING:
	send_to_char( "In your dreams, or what?\n\r", ch );
	return SOCIAL_TRUE;
    }

   if (IS_SET(world[ch->in_room].room_flags, QUIET))
   {
    send_to_char ("SHHHHHH!! Can't you see people are trying to read?\r\n", ch);
    return SOCIAL_TRUE;
   }

   if (action->char_found)
	one_argument(arg, buf);
    else
	*buf = '\0';

    if(!*buf)
    {
	if(action->char_no_arg)
	  act(action->char_no_arg, ch, 0, 0, TO_CHAR, 0);
	if(action->others_no_arg)
	  act(action->others_no_arg, ch, 0, 0, TO_ROOM,
	    (action->hide)?INVIS_NULL:0);
	return SOCIAL_TRUE_WITH_NOISE;
    }

    if(!(vict = get_char_room_vis(ch, buf))) {
	if(action->not_found)
	  act(action->not_found, ch, 0, 0, TO_CHAR, 0);
    }
    else if(vict == ch) {
	if(action->char_auto)
	  act(action->char_auto, ch, 0, 0, TO_CHAR, 0);
	if(action->others_auto)
	  act(action->others_auto, ch, 0, 0, TO_ROOM,
	    (action->hide)?INVIS_NULL:0);
    }
    else if(GET_POS(vict) < action->min_victim_position) {
	act("$N is not in a proper position for that.",
	    ch,0,vict,TO_CHAR, 0);
    }
    else {
	if(action->char_found)
	  act(action->char_found, ch, 0, vict, TO_CHAR, 0);
	if(action->others_found)
	  act(action->others_found, ch, 0, vict, TO_ROOM, 
	    NOTVICT|((action->hide)?INVIS_NULL:0));
	if(action->vict_found)
	  act(action->vict_found, ch, 0, vict, TO_VICT,
	    (action->hide)?INVIS_NULL:0);
    }

    return SOCIAL_TRUE_WITH_NOISE;
}

char *fread_social_string(FILE *fl)
{
  char buf[MAX_STRING_LENGTH], *rslt;

  fgets(buf, MAX_STRING_LENGTH, fl);
  if(feof(fl)) {
    log("Fread_social_string - unexpected EOF.", IMMORTAL, LOG_BUG);
    exit(0);
  }

  if(*buf == '#')
    return(0);

  // strip the \n
  *(buf + strlen(buf) - 1) = '\0';

  rslt = str_dup(buf);
  return(rslt);
}

// read one social
// return TRUE on success
// return FALSE on 'EOF'
int read_social_from_file(long int num_social, FILE * fl)
{
  char tmp[MAX_INPUT_LENGTH];
  int hide, min_pos;

  fscanf(fl, " %s ", tmp);
  if(feof(fl))
    return FALSE;
  fscanf(fl, " %d %d \n", &hide, &min_pos);

  // read strings that will always be there
  soc_mess_list[num_social].name                = str_dup(tmp);
  soc_mess_list[num_social].hide                = hide;
  soc_mess_list[num_social].min_victim_position = min_pos;
  soc_mess_list[num_social].char_no_arg         = fread_social_string(fl);
  soc_mess_list[num_social].others_no_arg       = fread_social_string(fl);
  soc_mess_list[num_social].char_found          = fread_social_string(fl);
  // if no char_found, then the social is done, and the ones below won't be there
  if(!soc_mess_list[num_social].char_found)
    return TRUE;
  soc_mess_list[num_social].others_found        = fread_social_string(fl);
  soc_mess_list[num_social].vict_found          = fread_social_string(fl);
  soc_mess_list[num_social].not_found           = fread_social_string(fl);
  soc_mess_list[num_social].char_auto           = fread_social_string(fl);
  soc_mess_list[num_social].others_auto         = fread_social_string(fl);
  return TRUE;
}

// this function used by qsort to sort the social array
int compare_social_sort(const void * A, const void * B)
{
  int i;
  for (i = 0;;i++)
      if (!(*(((social_messg*)A)->name+i) && *(((social_messg*)B)->name+i)))
	break;
      else if ( *(((social_messg*)A)->name+i) > *(((social_messg*)B)->name+i))
         return 1;
      else if ( *(((social_messg*)A)->name+i) < *(((social_messg*)B)->name+i))
	return -1;
  // Match so far..
  if (strlen(((social_messg*)A)->name) > strlen(((social_messg*)B)->name))
    return 1;
  if (strlen(((social_messg*)A)->name) < strlen(((social_messg*)B)->name))
    return -1;
  return 0;
}

// this function used by qsort to search the social array
int compare_social_search(const void * A, const void * B)
{
  int i;
  for (i = 0;;i++)
      if (!(*(((char*)A)+i) && *(((social_messg*)B)->name+i)))
	break;
      else if ( *(((char*)A)+i) > *(((social_messg*)B)->name+i))
         return 1;
      else if ( *(((char*)A)+i) < *(((social_messg*)B)->name+i))
	return -1;
  // Match so far..
  if (strlen(((char*)A)) > strlen(((social_messg*)B)->name))
    return 1;
  if (strlen(((char*)A)) < strlen(((social_messg*)B)->name))
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
  soc_mess_list = (struct social_messg *) calloc(social_array_size, sizeof(struct social_messg));
#else
  soc_mess_list = (struct social_messg *) dc_alloc(social_array_size, sizeof(struct social_messg));
#endif

  if(!(fl = dc_fopen(SOCIAL_FILE, "r"))) {
    perror("Can't open social file in boot_social_messages");
    abort();
  }

  for(;;) 
  {
    // do we have room?
    if(num_socials >= social_array_size) {
      social_array_size += 10;
      // realloc the list to have enough memory for the new array size
#ifdef LEAK_CHECK
      soc_mess_list = (struct social_messg *) realloc(soc_mess_list, 
                                  (sizeof(struct social_messg)*social_array_size));
#else
      soc_mess_list = (struct social_messg *) dc_realloc(soc_mess_list, 
                                  (sizeof(struct social_messg)*social_array_size));
#endif
      // clear the memory we just alloc'd
      memset( (soc_mess_list + num_socials), 0, (sizeof(struct social_messg)*10) );
    }
    if(!read_social_from_file(num_socials, fl))
      break;
    num_socials++;
  }

  // sort it!
  qsort(soc_mess_list, num_socials, sizeof(social_messg), compare_social_sort);

  dc_fclose(fl);
}

struct social_messg * find_social(char *arg)
{
// now uses a linear search
  int i;

  for(i = 1; i < num_socials; i++)
	if (!compare_social_search((void*)arg,(void*)&soc_mess_list[i]))
	  return &soc_mess_list[i];


  return NULL;
//    sprintf(buf + strlen(buf), "%18s", soc_mess_list[i].name);

  

//  return (social_messg *) bsearch(arg, soc_mess_list, num_socials, sizeof(struct social_messg), compare_social_search);
}
    
void clean_socials_from_memory()
{
   for(int i = 0; i < num_socials; i++)
   {
     if(soc_mess_list[i].name)
        dc_free(soc_mess_list[i].name);
     if(soc_mess_list[i].char_no_arg)
        dc_free(soc_mess_list[i].char_no_arg);
     if(soc_mess_list[i].others_no_arg)
        dc_free(soc_mess_list[i].others_no_arg);
     if(soc_mess_list[i].char_found)
        dc_free(soc_mess_list[i].char_found);
     if(soc_mess_list[i].others_found)
       dc_free(soc_mess_list[i].others_found);
     if(soc_mess_list[i].vict_found)
        dc_free(soc_mess_list[i].vict_found);
     if(soc_mess_list[i].not_found)
        dc_free(soc_mess_list[i].not_found);
     if(soc_mess_list[i].char_auto)
        dc_free(soc_mess_list[i].char_auto);
     if(soc_mess_list[i].others_auto)
        dc_free(soc_mess_list[i].others_auto);
   }

   dc_free(soc_mess_list);
}

int do_social(struct char_data *ch, char *argument, int cmd)
{
  int i;
  char buf[MAX_STRING_LENGTH];  
  *buf = '\0';
 
  for(i = 1; i < num_socials; i++)
  {
    sprintf(buf + strlen(buf), "%18s", soc_mess_list[i].name);
    if (!(i % 4))
    {
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      *buf = '\0';
    }
  }
  if(*buf)
      send_to_char(buf, ch);

  sprintf(buf, "\r\nCurrent Socials:  %d\r\n", i);
  send_to_char(buf, ch);
  return eSUCCESS;
}
