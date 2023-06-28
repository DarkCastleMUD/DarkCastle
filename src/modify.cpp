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
/* $Id: modify.cpp,v 1.33 2014/07/04 22:00:04 jhhudso Exp $ */

extern "C"
{
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
}

#include "connect.h" // Connection
#include "utility.h"
#include "character.h"
#include "mobile.h"
#include "interp.h"
#include "levels.h"
#include "player.h"
#include "obj.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include <string>
#include <iostream>
using namespace std;

// TODO - what does this do?  Nothing that I can see....let's remove it....
#define REBOOT_AT 10 /* 0-23, time of optional reboot if -e lib/reboot */

#define TP_MOB 0
#define TP_OBJ 1
#define TP_ERROR 2

void check_for_awaymsgs(Character *ch);
void page_string_dep(class Connection *d, const char *str, int keep_internal);

const char *string_fields[] = {"name", "short", "long", "description", "title", "delete-description", "\n"};

// maximum length for text field x+1
int length[] = {40, 60, 256, 240, 60};

const char *skill_fields[] = {"learned", "recognize", "\n"};

// TODO - I'd like to put together some sort of "post office" for sending "mail"
//  to players that are offline.  (they get notified when they login, and have to
//  go pick it up)  Note:  There's a "CON_SEND_MAIL" already defined....not sure
//  why...

void string_hash_add(class Connection *d, char *str)
{
	char *scan;
	int terminator = 0;
	Character *ch = d->character;

	scan = str;
	while (*scan)
	{
		if ((terminator = (*scan == '~') != 0))
		{
			*scan = '\0';
			break;
		}
		scan++;
	}

	if (!(*d->hashstr))
	{
		if (strlen(str) > (unsigned)d->max_str)
		{
			send_to_char("String too long - Truncated.\r\n", d->character);
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

	else
	{
		if (strlen(str) + strlen(*d->hashstr) > (unsigned)d->max_str)
		{
			send_to_char("String too long. Last line skipped.\r\n", d->character);
			terminator = 1;
		}

		else
		{
			if (!(*d->hashstr = (char *)realloc(*d->hashstr, strlen(*d->hashstr) + strlen(str) + 3)))
			{
				perror("string_hash_add: ");
				abort();
			}

			strcat(*d->hashstr, str);
		}
	}

	if (terminator)
	{
		scan = str_hsh(*d->hashstr);
		dc_free(*d->hashstr);
		*d->hashstr = scan;
		d->hashstr = 0;
		d->connected = Connection::states::PLAYING;
		send_to_char("Ok.\r\n", ch);
		check_for_awaymsgs(ch);
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
	for (; isspace(*arg); arg++)
		;
	for (; (*string = *arg) != '\0'; arg++, string++)
		;

	return;
}

/* modification of malloc'ed strings in chars/objects */
command_return_t Character::do_string(QStringList arguments, int cmd)
{
	if (IS_NPC(this))
		return eFAILURE;

	if (arguments.length() < 3)
	{
		send_to_char("Syntax:\n\rstring ('obj'|'char') <name> <field>"
					 " [<string>].\r\n",
					 this);
		return 1;
	}

	QString type = arguments.at(0);
	QString name = arguments.at(1);
	name = name.toLower();
	QString field = arguments.at(2);
	QString value;
	if (arguments.length() > 3)
	{
		value = arguments.at(3);
	}

	if (field != "name" && field != "short" && field != "long" && field != "title" && field != "description" && field != "delete-description")
	{
		send_to_char("No field by that name. Try 'help string'.\r\n", this);
		return 1;
	}

	Character *victim = nullptr;
	if (type == "char")
	{
		/* locate the beast */
		if (!(victim = get_char_vis(this, name)))
		{
			send_to_char("I don't know anyone by that name...\r\n", this);
			return 1;
		}

		if ((GET_LEVEL(victim) > GET_LEVEL(this)) && IS_PC(victim))
		{
			send(QString("%1 can string himself, thank you.\r\n").arg(GET_SHORT(victim)));
			return 1;
		}

		if (field == "name")
		{
			if (victim->isPlayer() && isImplementer() == false)
			{
				send_to_char("You can't change that field for players.", this);
				return 1;
			}

			victim->setName(value);

			if (victim->isPlayer())
			{
				send("WARNING: You have changed the name of a player.\r\n");
			}
		}
		else if (field == "short")
		{
			if (GET_LEVEL(this) < DEITY)
			{
				send("You must be a God to do that.\r\n");
				return eFAILURE;
			}
			QString message = QString("%1 just restrung short on %2").arg(GET_NAME(this)).arg(GET_NAME(victim));
			logentry(message, IMPLEMENTER, LogChannels::LOG_GOD);
			victim->setShortDescription(value);
		}
		else if (field == "long")
		{
			if (victim->isPlayer())
			{
				send("That field is for mobiles only.\r\n");
				return 1;
			}
			victim->setLongDescription(value);
		}
		else if (field == "description")
		{
			victim->setDescription(value);
		}
		else if (field == "title")
		{
			if (victim->isMobile())
			{
				send("Mobiles have no titles.\r\n");
				return eFAILURE;
			}
			victim->setTitle(value);
		}
		else
		{
			send_to_char("That field is undefined for monsters.\r\n", this);
			return 1;
		}
	}
	else if (type == "obj")
	{
		Object *obj = nullptr;
		/* locate the object */
		if ((obj = get_obj_vis(this, name)) == nullptr)
		{
			send_to_char("Can't find such a thing here..\r\n", this);
			return 1;
		}

		if (IS_SET(obj->obj_flags.more_flags, ITEM_NO_RESTRING))
		{
			if (GET_LEVEL(this) < IMPLEMENTER)
			{
				send_to_char("That item is not restringable.\r\n", this);
				return 1;
			}
			else
				send_to_char("That item is NO_RESTRING btw.\r\n", this);
		}

		if (field == "name")
		{
			if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && GET_LEVEL(this) < 110)
			{
				send_to_char("The moose will get you if you do that.\r\n", this);
				return 1;
			}

			// TODO fix
			//  ch->desc->hashstr = &(obj->getName().toStdString().c_str());
		}
		else if (field == "short")
		{
			// TODO FIXME
			// ch->desc->hashstr = &obj->getShortDescriptionC();
		}
		else if (field == "long")
		{
			// TODO FIXME
			// ch->desc->hashstr = &obj->description;
		}
		else if (field == "description")
		{
			// TODO - remove this when the obj pfile saving keeps track of extra descs
			send_to_char("Noone may restring object extra descs at this time. -pir", this);
			return 1;

			if (value.isEmpty())
			{
				send_to_char("You have to supply a keyword.\r\n", this);
				return 1;
			}
			/* try to locate extra description */
			for (extra_descr_data *ed = obj->ex_description;; ed = ed->next)
				if (!ed)
				{ /* the field was not found. create a new_new one. */
#ifdef LEAK_CHECK
					ed = (struct extra_descr_data *)
						calloc(1, sizeof(struct extra_descr_data));
#else
					ed = (struct extra_descr_data *)dc_alloc(1, sizeof(struct extra_descr_data));
#endif
					ed->next = obj->ex_description;
					obj->ex_description = ed;
					ed->keyword = str_hsh(value.toStdString().c_str());
					ed->description = 0;
					// TODO FIXME
					// ch->desc->hashstr = &ed->description;
					send_to_char("New field.\r\n", this);
					break;
				}
				else if (!str_cmp(ed->keyword, value.toStdString().c_str()))
				{
					/* the field exists */
					ed->description = 0;
					// TODO FIXME
					// ch->desc->hashstr = &ed->description;
					send_to_char("Modifying description.\r\n", this);
					break;
				}
			desc->max_str = MAX_STRING_LENGTH;
			/* the stndrd (see below) procedure does not apply here */
			return 1;
		}
		else if (field == "")
		{
			if (value.isEmpty())
			{
				send_to_char("You must supply a field name.\r\n", this);
				return 1;
			}
			/* try to locate field */
			for (extra_descr_data *ed = obj->ex_description;; ed = ed->next)
				if (!ed)
				{
					send_to_char("No field with that keyword.\r\n", this);
					return 1;
				}
				else if (!str_cmp(ed->keyword, value.toStdString().c_str()))
				{
					/* delete the entry in the desr list */
					if (ed == obj->ex_description)
						obj->ex_description = ed->next;
					else
					{
						extra_descr_data *tmp = nullptr;
						for (tmp = obj->ex_description; tmp->next != ed; tmp = tmp->next)
							;
						if (tmp != nullptr)
						{
							tmp->next = ed->next;
						}
					}
					dc_free(ed);
					send_to_char("Field deleted.\r\n", this);
					return 1;
				}
		}
		else
		{
			send_to_char("That field is undefined for objects.\r\n", this);
			return 1;
		}
	}

	send_to_char("Ok.\r\n", this);

	desc->connected = Connection::states::EDITING;
	return 1;
}

/* db stuff *********************************************** */

/* One_Word is like one_argument, execpt that words in quotes "" are */
/* regarded as ONE word                                              */

char *one_word(char *argument, char *first_arg)
{
	int begin, look_at;

	begin = 0;

	do
	{
		for (; isspace(*(argument + begin)); begin++)
			;

		if (*(argument + begin) == '\"')
		{ /* is it a quote */

			begin++;

			for (look_at = 0; (*(argument + begin + look_at) >= ' ') && (*(argument + begin + look_at) != '\"'); look_at++)
				*(first_arg + look_at) = LOWER(*(argument + begin + look_at));

			if (*(argument + begin + look_at) == '\"')
				begin++;
		}
		else
		{

			for (look_at = 0; *(argument + begin + look_at) > ' '; look_at++)
				*(first_arg + look_at) = LOWER(*(argument + begin + look_at));
		}

		*(first_arg + look_at) = '\0';
		begin += look_at;
	} while (fill_word(first_arg));

	return (argument + begin);
}

#define MAX_HELP 1100

void free_help_from_memory()
{
	extern struct help_index_element *help_index;

	for (int i = 0; i < MAX_HELP; i++)
		if (help_index[i].keyword)
			dc_free(help_index[i].keyword);

	dc_free(help_index);
}

struct help_index_element *build_help_index(FILE *fl, int *num)
{
	int nr = -1, issorted, i;
	struct help_index_element *list = 0, mem;
	char buf[81], tmp[81], *scan;
	int32_t pos;

#ifdef LEAK_CHECK
	list = (struct help_index_element *)
		calloc(MAX_HELP, sizeof(struct help_index_element));
#else
	list = (struct help_index_element *)dc_alloc(MAX_HELP, sizeof(struct help_index_element));
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

			if (++nr >= MAX_HELP)
			{
				perror("Too many help keywords.");
				abort();
			}

			list[nr].keyword = str_dup(tmp);
			list[nr].pos = pos;
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
	} while (!issorted);

	*num = nr;
	return (list);
}

#define PAGE_LENGTH 22
#define PAGE_WIDTH 80

/* Traverse down the string until the beginning of the next page has been
 * reached.  Return nullptr if this is the last page of the string.
 */
const char *next_page(const char *str)
{
	int col = 1, line = 1, spec_code = false;
	int chars = 0;
	for (;; str++)
	{
		// If end of string, return nullptr.
		if (*str == '\0')
			return nullptr;

		// Check for $ ANSI codes.  They have to be kept together
		// Might wanna put a && *(str+1) != '$' so that $'s are wrapped...
		else if (*str == '$')
		{
			if (*(str + 1) == '\0')
			{ // this should never happen
				logentry("String ended in $ in next_page", ARCHITECT, LogChannels::LOG_BUG);
				//*str = '\0'; // overwrite the $ so it doesn't mess up anything
				return nullptr;
			}
			str++; // skip the $
				   // This causes the next char to get skipped in the loop iteration
			chars += 7;
		}
		// Check for the begining of an ANSI color code block.
		else if (*str == '\x1B' && !spec_code)
			spec_code = true;

		// Check for the end of an ANSI color code block.
		else if (*str == 'm' && spec_code)
			spec_code = false;

		// If we're at the start of the next page, return this fact.
		// Note, this is done AFTER we check for ansi codes, so that we don't
		// beep color into the pager menu (hopefully)
		else if (line > PAGE_LENGTH)
			return str;
		else if (chars > 2048)
			return str;
		// Check for everything else.
		else if (!spec_code)
		{
			chars += 1;

			// Carriage return puts us in column one.
			if (*str == '\r')
				col = 1;
			// Newline puts us on the next line.
			else if (*str == '\n')
				line++;

			// We need to check here and see if we are over the page width,
			// and if so, compensate by going to the begining of the next line.
			else if (col++ > PAGE_WIDTH)
			{
				col = 1;
				line++;
			}
		}
	}
	return nullptr;
}

// Function that returns the number of pages in the string.
int count_pages(const char *str)
{
	int pages;

	for (pages = 1; (str = next_page(str)); pages++)
		;
	return pages;
}

/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(const char *str, class Connection *d)
{
	int i;

	if (d->showstr_count)
		*(d->showstr_vector) = str;

	for (i = 1; i < d->showstr_count && str; i++)
		str = d->showstr_vector[i] = next_page(str);

	d->showstr_page = 0;
}

void page_string(class Connection *d, const char *str, int keep_internal)
{
	if (!d || !(d->character))
		return;

	if (!str || !*str)
	{
		send_to_char("", d->character);
		return;
	}

	if (IS_PC(d->character) && !IS_SET(d->character->player->toggles, PLR_PAGER))
	{
		page_string_dep(d, str, keep_internal);
		return;
	}

	string print_me = str;
	string tmp;
	size_t pagebreak;

	while (!print_me.empty())
	{
		pagebreak = print_me.find_first_of('\n', 3800); // find the first endline after 3800 chars

		if (string::npos == pagebreak)
			pagebreak = print_me.size(); // if one doesn't exist (string < 3800) just set to max string length
		else if (print_me.at(pagebreak) == '\r')
			pagebreak++; // if its a \r, go 1 greater.

		tmp = print_me.substr(0, pagebreak);
		print_me = print_me.substr(pagebreak, string::npos);

		// if they don't want things paginated
		send_to_char(tmp.c_str(), d->character);
	}
	return;
}

/* The depreciated call that gets the paging ball rolling... */
void page_string_dep(class Connection *d, const char *str, int keep_internal)
{
	if (!d)
		return;
	if (!str || !*str)
	{
		send_to_char("", d->character);
		return;
	}

	CREATE(d->showstr_vector, const char *, d->showstr_count = count_pages(str));

	if (keep_internal)
	{
		d->showstr_head = str_dup(str);
		paginate_string(d->showstr_head, d);
	}
	else
		paginate_string(str, d);

	show_string(d, "");
}

/* The call that displays the next page. */
void show_string(class Connection *d, const char *input)
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

	else if (*buf)
	{
		dc_free(d->showstr_vector);
		d->showstr_vector = 0;
		d->showstr_count = 0;
		if (d->showstr_head)
		{
			dc_free(d->showstr_head);
			d->showstr_head = 0;
		}
		return;
	}
	/* If we're displaying the last page, just send it to the character, and
	 * then free up the space we used.
	 */
	if (d->showstr_page + 1 >= d->showstr_count)
	{
		// send them a carriage return first to make sure it looks right
		send_to_char(d->showstr_vector[d->showstr_page], d->character);
		dc_free(d->showstr_vector);
		d->showstr_vector = 0;
		d->showstr_count = 0;
		if (d->showstr_head)
		{
			dc_free(d->showstr_head);
			d->showstr_head = nullptr;
		}
	}
	/* Or if we have more to show.... */
	else
	{
		strncpy(buffer, d->showstr_vector[d->showstr_page], diff = (d->showstr_vector[d->showstr_page + 1]) - (d->showstr_vector[d->showstr_page]));
		buffer[diff] = '\0';
		send_to_char(buffer, d->character);
		d->showstr_page++;
	}
}