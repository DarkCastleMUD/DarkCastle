/***************************************************************************
 *  file: modify.c                                         Part of DIKUMUD *
 *  Usage: Run-time modification (by users) of game variables              *
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
/* $Id: modify.cpp,v 1.9 2004/05/14 14:25:25 urizen Exp $ */

extern "C"
{
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <connect.h> // descriptor_data
#include <utility.h>
#include <character.h>
#include <mobile.h>
#include <interp.h>
#include <levels.h>
#include <player.h>
#include <obj.h>
#include <handler.h>
#include <db.h>

// TODO - what does this do?  Nothing that I can see....let's remove it....
#define REBOOT_AT    10  /* 0-23, time of optional reboot if -e lib/reboot */

#define TP_MOB    0
#define TP_OBJ    1
#define TP_ERROR  2

void show_string(struct descriptor_data *d, char *input);

extern char menu[];

char *string_fields[] =
{
    "name",
    "short",
    "long",
    "description",
    "title",
    "delete-description",
    "\n"
};

// maximum length for text field x+1 
int length[] =
{
    15,
    60,
    256,
    240,
    60
};


char *skill_fields[] = 
{
    "learned",
    "recognize",
    "\n"
};

// TODO - I'd like to put together some sort of "post office" for sending "mail"
//  to players that are offline.  (they get notified when they login, and have to
//  go pick it up)  Note:  There's a "CON_SEND_MAIL" already defined....not sure
//  why...

/* ************************************************************************
*  modification of malloc'ed strings                                      *
************************************************************************ */

/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str)
{
    char *scan;
    int terminator = 0;
    CHAR_DATA *ch = d->character;

    void board_unlock_board(struct char_data *ch);

    /* determine if ths is the terminal string, and truncate if so */
    scan = str;
    while(*scan) { 
      if((terminator = (*scan == '~') != 0)) { 
        *scan = '\0';
        break;
      } 
      scan++;
    }
 
    if(!(d->astr)) {
      if(strlen(str) > (unsigned) d->max_str) {
	send_to_char("String too long - Truncated.\n\r", d->character);
	*(str + d->max_str) = '\0';
	terminator = 1;
      }
#ifdef LEAK_CHECK
      d->astr = (char *)calloc(strlen(str) + 3, sizeof(char));
#else
      d->astr = (char *)dc_alloc(strlen(str) + 3, sizeof(char));
#endif
      strcpy(d->astr, str);
    }
    else {
      if(strlen(str) + strlen(d->astr) > (unsigned) d->max_str) {
	send_to_char("String too long. Last line skipped.\n\r", d->character);
	terminator = 1;
      }

      else {
	if(!(d->astr = (char *) realloc(d->astr, strlen(d->astr) + 
	    strlen(str) + 3))) {
	  perror("string_add");
	  abort();
	}

	strcat(d->astr, str);
      }
    }

    if(terminator) {
      if (*d->str)
        dc_free(*d->str);
      *d->str = d->astr;
      d->str = 0;
      d->astr = 0;
      if(d->connected == CON_EXDSCR) {
        if(GET_LEVEL(d->character) > 1)
          save_char_obj(d->character);
        SEND_TO_Q(menu, d);
	d->connected = CON_SELECT_MENU;
      }
      if(d->connected == CON_WRITE_BOARD) {
        d->connected = CON_PLAYING;
        if(d->character)
          board_unlock_board(d->character);
      }
      if(d->connected == CON_SEND_MAIL) {
        d->connected = CON_PLAYING;
        send_to_char("Ok.\n\r", ch);
        // send_mail();
      }
      if(d->connected == CON_EDITING ||
         d->connected == CON_EDIT_MPROG) 
      {
        d->connected = CON_PLAYING;
        send_to_char("Ok.\n\r", ch);
      }
    }
    else
       strcat(d->astr, "\n\r");
}


void string_hash_add(struct descriptor_data *d, char *str)
{
    char *scan;
    int terminator = 0;
    CHAR_DATA *ch = d->character;

    scan = str;
    while(*scan) {
      if((terminator = (*scan == '~') != 0)) {
        *scan = '\0';
        break;
      }
      scan++;
    }
    
    if(!(*d->hashstr)) {
      if(strlen(str) > (unsigned) d->max_str) {
	send_to_char("String too long - Truncated.\n\r", d->character);
	*(str + d->max_str) = '\0';
	terminator = 1;
      }
#ifdef LEAK_CHECK
      (*d->hashstr) = (char *)calloc(strlen(str) + 3, sizeof(char));
#else
      (*d->hashstr) = (char *)dc_alloc(strlen(str) + 3, sizeof(char));
#endif
      strcpy(*d->hashstr, str);
    }

    else {
      if(strlen(str) + strlen(*d->hashstr) > (unsigned) d->max_str) {
	send_to_char("String too long. Last line skipped.\n\r", d->character);
	terminator = 1;
      }

      else {
	if(!(*d->hashstr = (char *) realloc(*d->hashstr, strlen(*d->hashstr) +
           strlen(str) + 3))) {
	  perror("string_hash_add: ");
	  abort();
	}

	strcat(*d->hashstr, str);
      }
    }

    if(terminator) {
      scan = str_hsh(*d->hashstr);
      dc_free(*d->hashstr);
      *d->hashstr = scan;
      d->hashstr = 0;
      d->connected = CON_PLAYING;
      send_to_char("Ok.\n\r", ch);
    }
    else
       strcat(*d->hashstr, "\n\r");
}

// TODO - figure out what this is for...kill it if nothing
#undef MAX_STR

/* interpret an argument for do_string */
void quad_arg(char *arg, int *type, char *name, int *field, char *string)
{
    char buf[MAX_STRING_LENGTH];

    /* determine type */
    arg = one_argument(arg, buf);
    if (is_abbrev(buf, "char"))
       *type = TP_MOB;
    else if (is_abbrev(buf, "obj"))
       *type = TP_OBJ;
    else
    {
	*type = TP_ERROR;
	return;
    }

    /* find name */
    arg = one_argument(arg, name);

    /* field name and number */
    arg = one_argument(arg, buf);
    if (!(*field = old_search_block(buf, 0, strlen(buf), string_fields, 0)))
       return;

    /* string */
    for (; isspace(*arg); arg++);
    for (; ( *string = *arg ) != '\0' ; arg++, string++)
	;

    return;
}
    
     


/* modification of malloc'ed strings in chars/objects */
int do_string(CHAR_DATA *ch, char *arg, int cmd)
{
  char name[MAX_STRING_LENGTH], string[MAX_STRING_LENGTH];
  char message[100];
  int field, type, ctr;
  CHAR_DATA *mob;
  struct obj_data *obj;
  struct extra_descr_data *ed, *tmp;

  if(IS_NPC(ch))
    return 1;

  quad_arg(arg, &type, name, &field, string);

  if(type == TP_ERROR) {
    send_to_char("Syntax:\n\rstring ('obj'|'char') <name> <field>"
                 " [<string>].\n\r", ch);
    return 1;
  }

  if(!field) {
    send_to_char("No field by that name. Try 'help string'.\n\r", ch);
    return 1;
  }

  if(type == TP_MOB) {
    /* locate the beast */
    if(!(mob = get_char_vis(ch, name))) {
      send_to_char("I don't know anyone by that name...\n\r", ch);
      return 1;
    }

    if((GET_LEVEL(mob) > GET_LEVEL(ch)) && !IS_NPC(mob)) {
      sprintf(message, "%s can string himself, thank you.\n\r",GET_SHORT(mob));
      send_to_char(message, ch);
      return 1;
    }

    switch(field) {
      case 1:
        if(!IS_NPC(mob) && GET_LEVEL(ch) < IMP){
          send_to_char("You can't change that field for players.", ch);
          return 1;
        }
        if(IS_NPC(mob))
          ch->desc->hashstr = &GET_NAME(mob);
        else
          ch->desc->str = &GET_NAME(mob);
        if(!IS_NPC(mob))
          send_to_char("WARNING: You have changed the name of a player.\n\r", ch);
        break;
      case 2:
        if(GET_LEVEL(ch) < POWER) {
          send_to_char("You must be a God to do that.\n\r", ch);
          return 1;
        }
        sprintf(message ,"%s just restrung short on %s", GET_NAME(ch), 
          GET_NAME(mob));
        log(message, IMP, LOG_GOD);
        if(IS_NPC(mob))
          ch->desc->hashstr = &mob->short_desc;
        else
          ch->desc->str = &mob->short_desc;
        break;
      case 3:
        if(!IS_NPC(mob)) {
          send_to_char("That field is for monsters only.\n\r", ch);
          return 1;
        }
        ch->desc->hashstr = &mob->long_desc;
        break;
      case 4:
        if(IS_NPC(mob))
          ch->desc->hashstr = &mob->description;
        else
          ch->desc->str = &mob->description;
        break;
      case 5:
        if(IS_NPC(mob)) {
          send_to_char("Monsters have no titles.\n\r", ch);
          return 1;
        }
        ch->desc->str = &mob->title;
        break;
      default:
        send_to_char("That field is undefined for monsters.\n\r", ch);
        return 1;
    }
  }

  /* type == TP_OBJ */
  else {   
    /* locate the object */
    if(!(obj = get_obj_vis(ch, name))) {
      send_to_char("Can't find such a thing here..\n\r", ch);
      return 1;
    }

    if(IS_SET(obj->obj_flags.more_flags, ITEM_NO_RESTRING))
      if(GET_LEVEL(ch) < IMP)
      {
         send_to_char("That item is not restringable.\r\n", ch);
         return 1;
      }
      else send_to_char("That item is NO_RESTRING btw.\r\n", ch);

    switch(field) {
      case 1:
        if(IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && GET_LEVEL(ch)
           < 110) {
          send_to_char("Auto-mailing Sadus from account to inform "
                       "him that you tried to string gl name.\n\r", ch);
          return 1;  
        } 
        ch->desc->hashstr = &obj->name; break;
      case 2: ch->desc->hashstr = &obj->short_description; break;
      case 3: ch->desc->hashstr = &obj->description; break;
      case 4:
        // TODO - remove this when the obj pfile saving keeps track of extra descs
        send_to_char("Noone may restring object extra descs at this time. -pir", ch);
        return 1;

        if(!*string) {
          send_to_char("You have to supply a keyword.\n\r", ch);
          return 1;
        }
        /* try to locate extra description */
        for(ed = obj->ex_description; ; ed = ed->next)
          if(!ed) {    /* the field was not found. create a new_new one. */
#ifdef LEAK_CHECK
	    ed = (struct extra_descr_data *)
	      calloc(1, sizeof(struct extra_descr_data));
#else
	    ed = (struct extra_descr_data *)
	      dc_alloc(1, sizeof(struct extra_descr_data));
#endif
            ed->next = obj->ex_description;
            obj->ex_description = ed;
            ed->keyword = str_hsh(string);
            ed->description = 0;
            ch->desc->hashstr = &ed->description;
            send_to_char("New field.\n\r", ch);
            break;
          }
          else if(!str_cmp(ed->keyword, string)) {
            /* the field exists */
            ed->description = 0;
            ch->desc->hashstr = &ed->description;
            send_to_char("Modifying description.\n\r", ch);
            break;
          }
          ch->desc->max_str = MAX_STRING_LENGTH;
          /* the stndrd (see below) procedure does not apply here */
          return 1;
          break;
        case 6: 
          if(!*string) {
            send_to_char("You must supply a field name.\n\r", ch);
            return 1;
          }
          /* try to locate field */
          for(ed = obj->ex_description; ; ed = ed->next)
            if(!ed) {
              send_to_char("No field with that keyword.\n\r", ch);
              return 1;
            }
            else if(!str_cmp(ed->keyword, string)) {
              /* delete the entry in the desr list */                     
              if(ed == obj->ex_description)
                obj->ex_description = ed->next;
              else {
                for(tmp = obj->ex_description; tmp->next != ed;
                    tmp = tmp->next);
                  tmp->next = ed->next;
              }
              dc_free(ed);
              send_to_char("Field deleted.\n\r", ch);
              return 1;
            }
	  break;              
        default:
          send_to_char("That field is undefined for objects.\n\r", ch);
          return 1;
      }
  }

  /* there was a string in the argument array */
  if(*string) {
    for(ctr = 0; (unsigned) ctr <= strlen(string); ctr++) {
      if(string[ctr] == '$') {
        string[ctr] = ' ';
      }
    }
 
    if(strlen(string) > (unsigned) length[field - 1]) {
      send_to_char("String too long - truncated.\n\r", ch);
      *(string + length[field - 1]) = '\0';
    }
    if(type == TP_MOB && !IS_NPC(mob)) {
      *ch->desc->str = str_dup(string);
      ch->desc->str = 0;
    }
    else {
      *ch->desc->hashstr = str_hsh(string);
      ch->desc->hashstr = 0;
    }
    send_to_char("Ok.\n\r", ch);
  }

  /* there was no string. enter string mode */
  else { 
    send_to_char("Enter string. Terminate with '~' at the beginning "
                 "of a line.\n\r", ch);
    if(type == TP_MOB && !IS_NPC(mob)) 
#ifdef LEAK_CHECK
      (*ch->desc->str) = (char *)calloc(length[field - 1], sizeof(char));
#else
      (*ch->desc->str) = (char *)dc_alloc(length[field - 1], sizeof(char));
#endif
    else
#ifdef LEAK_CHECK
      (*ch->desc->hashstr) = (char *)calloc(length[field - 1], sizeof(char));
#else
      (*ch->desc->hashstr) = (char *)dc_alloc(length[field - 1], sizeof(char));
#endif
    ch->desc->max_str = length[field - 1];
    ch->desc->connected = CON_EDITING;
  }
  return 1;
}




/* db stuff *********************************************** */


/* One_Word is like one_argument, execpt that words in quotes "" are */
/* regarded as ONE word                                              */

char *one_word(char *argument, char *first_arg )
{
    int found, begin, look_at;

    found = begin = 0;

    do
    {
	for ( ;isspace(*(argument + begin)); begin++);

	if (*(argument+begin) == '\"') {  /* is it a quote */

	    begin++;

	    for (look_at=0; (*(argument+begin+look_at) >= ' ') && 
		(*(argument+begin+look_at) != '\"') ; look_at++)
		*(first_arg + look_at) = LOWER(*(argument + begin + look_at));

	    if (*(argument+begin+look_at) == '\"')
		begin++;

	} else {

	    for (look_at=0; *(argument+begin+look_at) > ' ' ; look_at++)
		*(first_arg + look_at) = LOWER(*(argument + begin + look_at));

	}

	*(first_arg + look_at) = '\0';
	begin += look_at;
    }
    while (fill_word(first_arg));

    return(argument+begin);
}

#define MAX_HELP  1100

void free_help_from_memory()
{
  extern struct help_index_element *help_index;

  for(int i = 0; i < MAX_HELP; i++)
    if(help_index[i].keyword)
      dc_free(help_index[i].keyword);

  dc_free(help_index);
}

struct help_index_element *build_help_index(FILE *fl, int *num)
{
    int nr = -1, issorted, i;
    struct help_index_element *list = 0, mem;
    char buf[81], tmp[81], *scan;
    long pos;

#ifdef LEAK_CHECK
    list = (struct help_index_element *)
      calloc(MAX_HELP, sizeof(struct help_index_element));
#else
    list = (struct help_index_element *)
      dc_alloc(MAX_HELP, sizeof(struct help_index_element));
#endif

    for (;;)
    {
	pos = ftell(fl);
	fgets(buf, 81, fl);
	*(buf + strlen(buf) - 1) = '\0';
	scan = buf;
	for (;;)
	{
	    /* extract the keywords */
	    scan = one_word(scan, tmp);

	    if (!*tmp)
		break;

	    if ( ++nr >= MAX_HELP )
	    {
		perror( "Too many help keywords." );
		abort();
	    }

	    list[nr].keyword	= str_dup(tmp);
	    list[nr].pos	= pos;
	}

	/* skip the text */
	do
	  fgets(buf, 81, fl);
	while (*buf != '#');
	if (*(buf + 1) == '~')
	    break;
    }

    /* we might as well sort the stuff */
    do
    {
	issorted = 1;
	for (i = 0; i < nr; i++)
	    if (str_cmp(list[i].keyword, list[i + 1].keyword) > 0)
	    {
		mem = list[i];
		list[i] = list[i + 1];
		list[i + 1] = mem;
		issorted = 0;
	    }
    }
    while (!issorted);

    *num = nr;
    return(list);
}


#define PAGE_LENGTH     22
#define PAGE_WIDTH      80

/* Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
char *next_page(char *str)
{
  int col = 1, line = 1, spec_code = FALSE;
  if (*str == '\0') return NULL;
  for (;; str++) {
    // If end of string, return NULL. 
    if (*str == '\0')
      return str; // Test fix, see if it works better now..

    // Check for $ ANSI codes.  They have to be kept together
    // Might wanna put a && *(str+1) != '$' so that $'s are wrapped...
    else if (*str == '$') {
      if(*(str+1) == '\0') { // this should never happen
        log("String ended in $ in next_page", ANGEL, LOG_BUG);
        *str = '\0'; // overwrite the $ so it doesn't mess up anything
        return NULL;
      }
      str++; // skip the $
             // This causes the next char to get skipped in the loop iteration
    }

    // Check for the begining of an ANSI color code block. 
    else if (*str == '\x1B' && !spec_code)
      spec_code = TRUE;

    // Check for the end of an ANSI color code block. 
    else if (*str == 'm' && spec_code)
      spec_code = FALSE;

    // If we're at the start of the next page, return this fact. 
    // Note, this is done AFTER we check for ansi codes, so that we don't
    // beep color into the pager menu (hopefully)
    else if (line > PAGE_LENGTH)
      return str;

    // Check for everything else. 
    else if (!spec_code) {
      // Carriage return puts us in column one. 
      if (*str == '\r')
        col = 1;
      // Newline puts us on the next line. 
      else if (*str == '\n')
        line++;

      // We need to check here and see if we are over the page width,
      // and if so, compensate by going to the begining of the next line.
      else if (col++ > PAGE_WIDTH) {
        col = 1;
        line++;
      }
    }
  }
}

// Function that returns the number of pages in the string. 
int count_pages(char *str)
{
  int pages;

  for (pages = 1; (str = next_page(str)); pages++);
  return pages;
}


/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(char *str, struct descriptor_data *d)
{
  int i;

  if (d->showstr_count)
    *(d->showstr_vector) = str;

  for (i = 1; i < d->showstr_count && str; i++)
    str = d->showstr_vector[i] = next_page(str);

  d->showstr_page = 0;
}


/* The call that gets the paging ball rolling... */
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  if (!d)
    return;

  if (!str || !*str) {
    send_to_char("", d->character);
    return;
  }

  // if they don't want things paginated
  if(!IS_MOB(d->character) && IS_SET(d->character->pcdata->toggles, PLR_PAGER))
  {
     send_to_char(str, d->character);
     return;
  }

  CREATE(d->showstr_vector, char *, d->showstr_count = count_pages(str));

  if (keep_internal) {
    d->showstr_head = str_dup(str);
    paginate_string(d->showstr_head, d);
  } else
    paginate_string(str, d);

  show_string(d, "");
}


/* The call that displays the next page. */
void show_string(struct descriptor_data *d, char *input)
{
  char buffer[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  int diff;

  one_argument(input, buf);

  if (LOWER(*buf) == 'r')
    d->showstr_page = MAX(0, d->showstr_page - 1);

  /* B is for back, so back up two pages internally so we can display the
   * correct page here.
   */
  else if (LOWER(*buf) == 'b')
    d->showstr_page = MAX(0, d->showstr_page - 2);

  /* Feature to 'goto' a page.  Just type the number of the page and you
   * are there!
   */
  else if (isdigit(*buf))
    d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));

  else if (*buf) {
    dc_free(d->showstr_vector);
    d->showstr_vector = 0;
    d->showstr_count = 0;
    if(d->showstr_head) {
        dc_free(d->showstr_head);
        d->showstr_head = 0;
    }
    return;
  }
  /* If we're displaying the last page, just send it to the character, and
   * then free up the space we used.
   */
  if (d->showstr_page + 1 >= d->showstr_count) {
    // send them a carriage return first to make sure it looks right
    send_to_char(d->showstr_vector[d->showstr_page], d->character);
    dc_free(d->showstr_vector);
    d->showstr_vector = 0;
    d->showstr_count = 0;
    if (d->showstr_head) {
      dc_free(d->showstr_head);
      d->showstr_head = NULL;
    }
  }
  /* Or if we have more to show.... */
  else {
    strncpy(buffer, d->showstr_vector[d->showstr_page],
            diff = ((int) d->showstr_vector[d->showstr_page + 1])
            - ((int) d->showstr_vector[d->showstr_page]));
    buffer[diff] = '\0';
    send_to_char(buffer, d->character);
    d->showstr_page++;
  }
}
