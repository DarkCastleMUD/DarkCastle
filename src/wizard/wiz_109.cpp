/********************************
| Level 109 wizard commands
| 11/20/95 -- Azrack
**********************/
#include "wizard.h"
#include <spells.h>
#include <fileinfo.h>
#include <connect.h>
#include <utility.h>
#include <player.h>
#include <levels.h>
#include <mobile.h>
#include <handler.h>
#include <interp.h>
#include <db.h>
#include <returnvals.h>
#include <comm.h>
#include <vault.h>
#include <utility.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#ifdef BANDWIDTH
  #include <bandwidth.h>
#endif

void AuctionHandleDelete(string name);

int do_linkload(struct char_data *ch, char *arg, int cmd)
{
  struct descriptor_data d;
  struct char_data *new_new;
  char buf[100];
  char *c;

  void add_to_bard_list(char_data * ch);

  while(*arg == ' ')
    arg++;
 
  if(!*arg) {
    send_to_char("Linkload whom?\n\r", ch);
    return eFAILURE;
  }

  one_argument(arg, buf);

  if(get_pc(buf)) {
    send_to_char("That person is already on the game!\n\r", ch);
    return eFAILURE;
  }

  c = buf;
  *c = UPPER(*c);
  c++;
  while(*c) { 
    *c = LOWER(*c);
    c++;
  }

  // must be done to clear out "d" before it is used
  memset((char *) &d, 0, sizeof(struct descriptor_data));
  if(!(load_char_obj(&d, buf))) {
    send_to_char("Unable to load! (Character might not exist...)\n\r", ch);
    return eFAILURE; 
  } 

  new_new = d.character;
  new_new->desc = 0;
  new_new->next = character_list;
  character_list = new_new;
  add_to_bard_list(new_new);

  redo_hitpoints(new_new);
  redo_mana(new_new);
  if(!GET_TITLE(new_new))
    GET_TITLE(new_new) = str_dup("is a virgin");
  if(GET_CLASS(new_new) == CLASS_MONK)
    GET_AC(new_new) -= GET_LEVEL(new_new) * 3;
  isr_set(new_new);

  char_to_room(new_new, ch->in_room);
  act("$n gestures sharply and $N comes into existence!", ch,
      0, new_new, TO_ROOM, 0);
  act("You linkload $N.", ch, 0, new_new, TO_CHAR, 0); 
  logf(GET_LEVEL(ch), LOG_GOD, "%s linkloads %s.", GET_NAME(ch), GET_NAME(new_new));
  return eSUCCESS;
}

int do_processes(struct char_data *ch, char *arg, int cmd)
{
  FILE *fl;
  char *tmp;
  char buf[100]; 

  strcpy(buf, "ps -ux > ../lib/whassup.txt");

  system(buf);

  if(!(fl = dc_fopen("../lib/whassup.txt", "a"))) {
    log("Unable to open whassup.txt for adding in do_processes!", IMP,
        LOG_BUG);   
     return eFAILURE;
  }
  if(fprintf(fl, "~\n") < 0) {
    dc_fclose(fl);
    send_to_char("Failure writing to transition file.\n\r", ch);
    return eFAILURE;
  }
 
  dc_fclose(fl);

  if(!(fl = dc_fopen("../lib/whassup.txt", "r"))) {
    log("Unable to open whassup.txt for reading in do_processes!", IMP,
        LOG_BUG);
    return eFAILURE;
  }
  tmp = fread_string(fl, 0);
  dc_fclose(fl);

  send_to_char(tmp, ch);
  FREE(tmp);  
  return eSUCCESS;
}

int do_guide(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char name[100], buf[256];

    one_argument(argument,name);

    if(*name) {
        if(!(victim = get_char_vis(ch, name))) {
            send_to_char("That player is not here.\n\r", ch);
            return eFAILURE;
        }
    } else {
        send_to_char("Who exactly would you like to be a guide?\n\r", ch);
        return eFAILURE;
    }

    if (IS_NPC(victim)) {
      send_to_char("Hmm, mobs wouldn't make very good guides now would they?\r\n", ch);
      return eFAILURE;
    }

    if (!IS_SET(victim->pcdata->toggles, PLR_GUIDE)) {
        sprintf(buf, "%s is now a guide.\r\n", GET_NAME(victim));
        send_to_char(buf, ch);
        send_to_char("You have been selected to be a DC Guide!\r\n", victim);
	SET_BIT(victim->pcdata->toggles, PLR_GUIDE);
	SET_BIT(victim->pcdata->toggles, PLR_GUIDE_TOG);
    } else {
        sprintf(buf, "%s is no longer a guide.\r\n", GET_NAME(victim));
        send_to_char(buf, ch);
        send_to_char("You have been removed as a DC guide.\r\n", victim);
        REMOVE_BIT(victim->pcdata->toggles, PLR_GUIDE);
	REMOVE_BIT(victim->pcdata->toggles, PLR_GUIDE_TOG);
    }

  return eSUCCESS;
}

int do_advance(struct char_data *ch, char *argument, int cmd)
{
    struct char_data *victim;
    char name[100], level[100], buf[300], passwd[100];
    int adv, new_newlevel;

    void gain_exp(struct char_data *ch, int gain);

    if (IS_NPC(ch))
        return eFAILURE;

    half_chop(argument, name, buf);
    argument_interpreter(buf, level, passwd);

    if(*name) {
        if(!(victim = get_char_vis(ch, name))) {
            send_to_char("That player is not here.\n\r", ch);
            return eFAILURE;
        }
    } else {
        send_to_char("Advance whom?\n\r", ch);
        return eFAILURE;
    }

    if (IS_NPC(victim)) {
        send_to_char("NO! Not on NPC's.\n\r", ch);
        return eFAILURE;
    }


    if(!*level ||
       (new_newlevel = atoi(level)) <= 0 || new_newlevel > IMP) {
        send_to_char( "Level must be 1 to 110.\n\r", ch );
        return eFAILURE;
    }

    if((new_newlevel > MAX_MORTAL) && (new_newlevel < MIN_GOD))
    {
      send_to_char("That level doesn't exist!!\n\r", ch);
      return eFAILURE;
    }

    if (GET_LEVEL(ch) < OVERSEER && new_newlevel >= IMMORTAL) {
        send_to_char( "Limited to levels lower than Titan.\n\r", ch );
        return eFAILURE;
    }

    /* Who the fuck took ths out in the first place? -Sadus */
    if(new_newlevel > GET_LEVEL(ch)) {
      send_to_char("Yeah right.\n\r", ch);
      return eFAILURE;
    }

/* Lower level:  First reset the player to level 1. Remove all special
   abilities (all spells, BASH, STEAL, et).  Reset practices to
   zero.  Then act as though you're raising a first level character to
   a higher level.  Note, currently, an implementor can lower another imp.
   -- Swifttest */

    if (new_newlevel <= GET_LEVEL(victim))
      {
        send_to_char("Warning:  Lowering a player's level!\n\r", ch);

                GET_LEVEL(victim) = 1;
                GET_EXP(victim) = 1;

                victim->max_hit  = 5;  /* These are BASE numbers  */
                victim->raw_hit  = 5;

                GET_HIT(victim) = hit_limit(victim);
                GET_MANA(victim) = mana_limit(victim);
                GET_MOVE(victim) = move_limit(victim);

                advance_level(victim, 0);
                redo_hitpoints(victim);
      }

        adv = new_newlevel - GET_LEVEL(victim);

    send_to_char("You feel generous.\n\r", ch);
    act("$n makes some strange gestures.\n\rA strange feeling comes upon you,"
        "like a giant hand. Light comes\n\rdown from above, grabbing your "
        "body, which begins to pulse\n\rwith coloured lights from inside.\n\rYo"        
        "ur head seems to be filled with deamons\n\rfrom another plane as your"
        " body dissolves\n\rto the elements of time and space itself.\n\rSudde"
        "nly a silent explosion of light snaps\n\ryou back to reality. You fee"
        "l slightly\n\rdifferent.",ch,0,victim,TO_VICT, 0);

    sprintf(buf, "%s advances %s to level %d.", GET_NAME(ch),
            GET_NAME(victim), new_newlevel);
    log(buf, GET_LEVEL(ch), LOG_GOD);

    if(GET_LEVEL(victim) == 0)
      do_start(victim);
    else
      while (GET_LEVEL(victim) < new_newlevel) {
        send_to_char( "You raise a level!!  ", victim);
        GET_LEVEL(victim) += 1;
        advance_level(victim, 0);
      }
    update_wizlist(victim);
  return eSUCCESS;
}


int do_zap(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  int room;
  char name[100], buf[500];

  void remove_clan_member(int clannumber, struct char_data * ch);

  one_argument(argument, name);

  if(!(*name)) {
    send_to_char("Zap who??\n\rOh, BTW this deletes anyone "
                 "lower than you.\n\r", ch);
    return eFAILURE;
  }
  
  victim = get_pc_vis(ch, name);

  if (victim) {
    if(!IS_NPC(victim) && (GET_LEVEL(ch) < GET_LEVEL(victim)) &&
       strcmp(GET_NAME(ch), "Apocalypse")) { 
      act("$n casts a massive lightning bolt at you.", ch, 0, victim,
          TO_VICT, 0);
      act("$n casts a massive lightning bolt at $N.", ch, 0, victim,
          TO_ROOM, NOTVICT);
      return eFAILURE;
    }

    if(GET_LEVEL(victim) == IMP) 
    { // Hehe..
          send_to_char("Get stuffed.\n\r", ch);
          return eFAILURE;
    }

    if(!IS_NPC(victim)) {
      sprintf(buf, "A massive bolt of lightning arcs down from the "
              "heavens, striking you\n\rbetween the eyes. You have "
              "been utterly destroyed by %s.\n\r",
             GET_SHORT(ch));
      send_to_char(buf,victim);
    }

    room = victim->in_room;

    sprintf(buf,"A massive bolt of lightning arcs down from the heavens," 
            " striking\n\r%s between the eyes.\n\r  %s has been utterly" 
            " destroyed by %s.\n\r",
            GET_NAME(victim), GET_SHORT(victim),
            GET_SHORT(ch));

    remove_familiars(victim->name, ZAPPED);
    if (cmd == 9) // cmd9 = someone typed it. 10 = rename.
      remove_vault(victim->name, ZAPPED);
 
    GET_LEVEL(victim) = 1;
    update_wizlist(victim);

    if(ch->clan)
      remove_clan_member(ch->clan, ch);

    AuctionHandleDelete(GET_NAME(victim));
    snprintf(buf, 500, "%s has deleted %s.\n\r", ch->name, victim->name);
    remove_character(victim->name, ZAPPED);

    do_quit(victim, "", 666);
    send_to_room(buf, room);
    send_to_all("You hear an ominous clap of thunder in the distance.\n\r");
    log(buf, ANGEL, LOG_GOD);
  }

  else
    send_to_char("Zap who??\n\rOh, BTW this deletes anyone "
                 "lower than you.\n\r", ch);

  return eFAILURE;
}

int do_global(struct char_data *ch, char *argument, int cmd)
{
    int i;
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *point;
    
    if (IS_NPC(ch))
        return eFAILURE;


    for (i = 0; *(argument + i) == ' '; i++);

    if(!*(argument + i))
      send_to_char("What message do you want to send to all players?\n\r", ch);
    else {
        sprintf(buf, "\n\r%s\n\r", argument + i);
        for(point = descriptor_list; point; point = point->next)
           if(!point->connected && point->character)
             send_to_char(buf, point->character);
    }
  return eSUCCESS;
}

int do_shutdown(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_INPUT_LENGTH];
  extern int _shutdown;
  extern int try_to_hotboot_on_crash;
  extern int do_not_save_corpses;
  char **new_argv = 0;

    if (IS_NPC(ch))
        return eFAILURE;

    if(!has_skill(ch, COMMAND_SHUTDOWN)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }

    char arg1[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg1);

    // If there was more than 1 argument, create an argument array
    if (*argument != 0) {
      char argN[MAX_INPUT_LENGTH];
      queue<char *> arg_list;

      while (*argument != 0) {
	argument = one_argumentnolow(argument, argN);
	arg_list.push(strdup(argN));
      }

      if (arg_list.size() > 0) {
	new_argv = new char*[arg_list.size()+1];

	int index = 0;
	while (! arg_list.empty()) {
	  new_argv[index++] = arg_list.front();
	  arg_list.pop();
	}

	new_argv[index] = 0;
      }
    }

    if (!*arg1) {
        send_to_char("Syntax:  shutdown [sub command] [options ...]\n\r"
		     " Sub Commands:\n\r"
		     "--------------\n\r"
                     "   hot - Rerun current DC filename and keep players' links active.\n\r"
		     "         Options: [path/dc executable] [dc options ...]\n\r"
                     "  cold - Go ahead and kill the links.\n\r"
                     " crash - Crash the mud by referencing an invalid pointer.\n\r"
                     "  core - Produce a core file.\n\r"
                     "  auto - Toggle auto-hotboot on crash setting.\n\r"
                     "   die - Kill boot script and crash mud so it won't reboot.\r\n", ch);
        return eFAILURE;
    }

    if(!strcmp(arg1, "cold")) {
        sprintf(buf, "Shutdown by %s.\n\r", GET_SHORT(ch) );
        send_to_all(buf);
        log(buf, ANGEL, LOG_GOD);
        _shutdown = 1;
    }
    else if(!strcmp(arg1, "hot")) {
        do_not_save_corpses = 1;
        sprintf(buf, "Hot reboot by %s.\n\r", GET_SHORT(ch) );
        send_to_all(buf);
        log(buf, ANGEL, LOG_GOD);
        log("Writing sockets to file for hotboot recovery.", 0, LOG_MISC);
	do_force(ch, "all save",123);
        if(!write_hotboot_file(new_argv)) {
           log("Hotboot failed.  Closing all sockets.", 0, LOG_MISC);
           send_to_char("Hot reboot failed.\n\r", ch);
        }
    }
    else if(!strcmp(arg1, "auto")) {
        if(try_to_hotboot_on_crash) {
          send_to_char("Mud will not try to hotboot when it crashes next.\n\r", ch);
          try_to_hotboot_on_crash = 0;
        }
        else {
          send_to_char("Mud will now TRY to hotboot when it crashes next.\n\r", ch);
          try_to_hotboot_on_crash = 1;
        }
    }
    else if(!strcmp(arg1, "crash")) {
        // let's crash the mud!
        char_data * blahblah = NULL;
        int chode = blahblah->in_room;
        chode = 1; // we never get here, but it gets rid of the compile warning
    }
    else if (!strcmp(arg1, "core")) {
      produce_coredump();
      log("Corefile produced.", IMMORTAL, LOG_BUG);
    }
    else if(!strcmp(arg1, "die")) {
        fclose(fopen("died_in_bootup", "w"));
        try_to_hotboot_on_crash = 0;
        // let's crash the mud!
        char_data * blahblah = NULL;
        int chode = blahblah->in_room;
        chode = 1; // we never get here, but it gets rid of the compile warning
    }
    else
        send_to_char("Go shut down someone your own size.\n\r", ch);
  return eSUCCESS;
}

int do_shutdow(struct char_data *ch, char *argument, int cmd)
{
  if(!has_skill(ch, COMMAND_SHUTDOWN)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
  }

  send_to_char("If you want to shut something down - say so!\n\r", ch);
  return eSUCCESS;
}


int do_testport(char_data *ch, char *argument, int cmd)
{
    int errnosave = 0;
    static pid_t child = 0;
    char arg1[MAX_INPUT_LENGTH];

    if (ch == NULL) {
	return eFAILURE;
    }

    if (IS_MOB(ch) || !has_skill(ch, COMMAND_TESTPORT)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }  

    argument = one_argument(argument, arg1);

    if (*arg1 == 0) {
	if (child) {
	    send_to_char("status: running\n\r", ch);
	} else {
	    send_to_char("status: not running\n\r", ch);
	}
	send_to_char("testport <start | stop>\n\r\n\r", ch);
	return eFAILURE;
    }

    if (!str_cmp(arg1, "start")) {
	if (child) {
	    send_to_char("Another testport is already running.\n\r", ch);
	    return eFAILURE;
	}
	child = fork();
	// inside of child of process
	if (child == 0) {
	    chdir("../src/");

	    // Find next available fd
	    FILE *testportlog = fopen("../log/testport.log", "w");
	    if (testportlog != NULL) {
		int fd = fileno(testportlog);

		close(0);
		// Redirect stdout and stderr to go to testportlog's fd instead
		dup2(fd, 1);
		dup2(fd, 2);

		// Close all fds so this child doesnt have access to parent's fds
		for (int i=3; i <= fd; i++) {
		    errno = 0;
		    close(i);
		    if (errno) {
			perror("do_testport");
		    }
		}

		char *myargv[] = {"./dcastle", "-P", "-b", "7000", NULL};
		errno = 0;
		execv("./dcastle", myargv);
	    }
	    exit(0);
	}
	
	logf(105, LOG_MISC, "Starting testport under pid %d", child);
	send_to_char("Testport successfully started.\n\r", ch);
    } else if (!str_cmp(arg1, "stop")) {
	if (!child) {
	    send_to_char("The testport is not running currently.\n\r", ch);
	    return eFAILURE;
	} else {
	    errno = 0;
	    kill(child, 9);
	    errnosave = errno;
	    if (errnosave) {
		csendf(ch, "Error: %s\n\r", strerror(errnosave));

		// Process must have already died because it cant be found now
		if (errnosave == ESRCH) {
		    child = 0;
		}
		return eFAILURE;
	    }
	    logf(105, LOG_MISC, "Shutdown testport under pid %d", child);
	    send_to_char("Testport successfully shutdown.\n\r", ch);
	    child = 0;
	    return eSUCCESS;
	}
    } else {
	send_to_char("Invalid option.\n\r", ch);
    }

    return eSUCCESS;
}

int do_testuser(char_data *ch, char *argument, int cmd)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char savefile[255];
    char bsavefile[255];
    char command[512];
    char username[20];

    if (ch == NULL) {
	return eFAILURE;
    }

    if (IS_MOB(ch) || !has_skill(ch, COMMAND_TESTUSER)) {
        send_to_char("Huh?\r\n", ch);
        return eFAILURE;
    }  

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (*arg1 == 0 || *arg2 == 0) {
	send_to_char("testuser <user> <on|off>\n\r\n\r", ch);
	return eFAILURE;
    }

    if (strlen(arg1) > 19 || _parse_name(arg1, username)) {
      send_to_char("Invalid username passed.\n\r", ch);
      return eFAILURE;
    }

    username[0] = UPPER(username[0]);
    for(unsigned int i=1; i < strlen(username); i++) {
      username[i] = LOWER(username[i]);
    }

    snprintf(savefile, 255, "../save/%c/%s", UPPER(username[0]), username);
    snprintf(bsavefile, 255, "../bsave/%c/%s", UPPER(username[0]), username);

    if (!file_exists(savefile)) {
      send_to_char("Player file not found.\n\r", ch);
      return eFAILURE;
    }

    if (!str_cmp(arg2, "on")) {
      sprintf(command, "cp %s %s", savefile, bsavefile);
    } else if (!str_cmp(arg2, "off")) {
      sprintf(command, "rm %s", bsavefile);
    } else {
      send_to_char("Only on or off are valid second arguments to this command.\n\r", ch);
      return eFAILURE;
    }
    
    logf(110, LOG_GOD, "testuser: %s initiated %s", ch->name, command);

    if (system(command)) {
      send_to_char("Error occurred.\n\r", ch);
    } else {
      send_to_char("Ok.\n\r", ch);
    }

    return eSUCCESS;
}

#ifdef BANDWIDTH
int do_bandwidth(struct char_data *ch, char *argument, int cmd)
{
  csendf(ch, "Bytes sent in %ld seconds: %ld\n\r",
	 get_bandwidth_start(), get_bandwidth_amount());
  return eSUCCESS;
}
#endif

int do_skilledit(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH];
  char type[MAX_INPUT_LENGTH];
  char value[MAX_INPUT_LENGTH];
  char buf[180];
  struct char_skill_data * curr = NULL;

  if(!(*argument)) {
    send_to_char("Syntax:  skilledit <character> <action> <value>\n\r"
                 "Possible actions are:  list, add, delete\n\r", ch);
    return eFAILURE;   
  }  
  half_chop(argument, name, argument);
  half_chop(argument, type, value);
     
  if(!(victim = get_pc_vis(ch, name))) {
    send_to_char("Edit the skills of whom?\r\n", ch);
    return eFAILURE;
  }

  if(isname(type, "list"))
  {
    curr = victim->skills;
    if(!curr) {
      sprintf(buf, "%s has no skills.\r\n", GET_NAME(victim));
      send_to_char(buf, ch);
      return eSUCCESS;
    }

    sprintf(buf, "Skills for %s:\r\n", GET_NAME(victim));
    send_to_char(buf, ch);
    while(curr) {
      sprintf(buf, "  %d  -  %d  [%d] [%d] [%d] [%d] [%d]\r\n", curr->skillnum, curr->learned,
                   curr->unused[0], curr->unused[1], curr->unused[2], curr->unused[3], curr->unused[4]);
      send_to_char(buf, ch);
      curr = curr->next;
    }
  }
  else if(isname(type, "add"))
  {
  }
  else if(isname(type, "delete"))
  {
  }
  else
  {
    sprintf(buf, "Invalid action '%s'.  Must be 'list', 'add', or 'delete'.\r\n", type);
    send_to_char(buf, ch);
  }

  return eSUCCESS;
}

