
/* The standard includes */
extern "C"
{
  #include <ctype.h>
  #include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h>
#include <db.h>
#include <utility.h>
#include <vault.h>
#include <room.h>
#include <player.h>
#include <fileinfo.h>
#include <obj.h>
#include <handler.h>
#include <levels.h>
#include <connect.h>
#include <returnvals.h>
#include <interp.h>
#include <spells.h>
#include <clan.h> // clan right

extern CWorld world;
int total_vaults = 0;
int get_line(FILE * fl, char *buf);
char *skip_spaces(char *string);
struct obj_data *get_obj(int vnum);

void item_remove(obj_data *obj, struct vault_data *vault);
void item_add(int vnum, struct vault_data *vault);

void vault_log(char *message, char *name);
struct char_data *find_owner(char *name);
void show_vault_log(CHAR_DATA *ch, char *owner);
int class_restricted(struct char_data *ch, struct obj_data *obj);
int size_restricted(struct char_data *ch, struct obj_data *obj);
int spell_identify(byte level, CHAR_DATA *ch, CHAR_DATA *victim, struct obj_data *obj, int skill);
char *clanVName(int c);

extern struct index_data *obj_index;
extern struct obj_data * search_char_for_item(char_data * ch, int16 item_number, bool wearOnly = FALSE);
extern struct obj_data  *object_list;

struct vault_data *has_vault(char *name) {
  struct vault_data *vault;

  for (vault = vault_table;vault;vault = vault->next)
    if (vault && vault->owner && !strcasecmp(vault->owner, name))
      return vault;
  char_data *ch = find_owner(name);
  if (ch && GET_LEVEL(ch) > 10) {
     add_new_vault(GET_NAME(ch), 0);
     for (vault = vault_table;vault;vault = vault->next)
       if (vault && vault->owner && !strcasecmp(vault->owner, name)) {
//	 send_to_char("A vault was created for you.\r\n",ch);
         return vault;
       }
  }
  return 0;
}

bool fullSave(obj_data *obj)
{
  if (eq_current_damage(obj))
    return 1;

  obj_data *tmp_obj = get_obj(GET_OBJ_VNUM(obj));
  if (strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(tmp_obj)))
    return 1;

  if (strcmp(obj->name, tmp_obj->name)) // GL. and stuff.
    return 1;
  
  if (obj->obj_flags.extra_flags != tmp_obj->obj_flags.extra_flags)
	return 1;

  if (obj->obj_flags.more_flags != tmp_obj->obj_flags.more_flags)
	return 1;

  if (obj->obj_flags.type_flag == ITEM_STAFF && obj->obj_flags.value[1] != obj->obj_flags.value[2])
    return 1;

  if (obj->obj_flags.type_flag == ITEM_WAND && obj->obj_flags.value[1] != obj->obj_flags.value[2])
    return 1;

  if (obj->obj_flags.type_flag == ITEM_DRINKCON && obj->obj_flags.value[0] != obj->obj_flags.value[1])
    return 1;

  return 0;
}


void remove_from_object_list(obj_data *obj)
{
  obj_data *tObj, *pObj = NULL;
  for (tObj = object_list; tObj; tObj = tObj->next)
  {
    if (tObj == obj)
    {
	if (pObj) pObj->next = tObj->next;
	else object_list = tObj->next;
	tObj->next = NULL;
	break;
    }
    pObj = tObj;
  }
}

void copySaveData(obj_data *new_obj, obj_data *obj)
{
  int i;
  if ((i = eq_current_damage(obj)) > 0)
  {
     for (;i>0;i--)
       damage_eq_once(new_obj);
  }

  if (strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(new_obj)))
    GET_OBJ_SHORT(new_obj) = str_hsh(GET_OBJ_SHORT(obj));

  if (strcmp(obj->name, new_obj->name))
    new_obj->name = str_hsh(obj->name);

  if (obj->obj_flags.extra_flags != new_obj->obj_flags.extra_flags)
	new_obj->obj_flags.extra_flags = obj->obj_flags.extra_flags;

  if (obj->obj_flags.more_flags != new_obj->obj_flags.more_flags)
	new_obj->obj_flags.more_flags = obj->obj_flags.more_flags;

  if (obj->obj_flags.type_flag == ITEM_STAFF && obj->obj_flags.value[2] != new_obj->obj_flags.value[2])
  	new_obj->obj_flags.value[2] = obj->obj_flags.value[2];

  if (obj->obj_flags.type_flag == ITEM_WAND && obj->obj_flags.value[1] != new_obj->obj_flags.value[1])
  	new_obj->obj_flags.value[2] = obj->obj_flags.value[2];

  if (obj->obj_flags.type_flag == ITEM_DRINKCON && obj->obj_flags.value[1] != new_obj->obj_flags.value[1])
  	new_obj->obj_flags.value[1] = obj->obj_flags.value[1];

  return;
}

bool fullItemMatch(obj_data *obj, obj_data *obj2)
{
  if (eq_current_damage(obj) != eq_current_damage(obj2))
    return 0;

  if (strcmp(GET_OBJ_SHORT(obj), GET_OBJ_SHORT(obj2)))
    return 0;
  
  if (strcmp(obj->name, obj2->name)) 
    return 0;

  if (obj->obj_flags.extra_flags != obj2->obj_flags.extra_flags)
	return 0;

  if (obj->obj_flags.more_flags != obj2->obj_flags.more_flags)
	return 0;

  if (obj->obj_flags.type_flag == ITEM_STAFF && obj->obj_flags.value[1] != obj2->obj_flags.value[1])
    return 0;

  if (obj->obj_flags.type_flag == ITEM_WAND && obj->obj_flags.value[1] != obj2->obj_flags.value[1])
    return 0;

  if (obj->obj_flags.type_flag == ITEM_DRINKCON && obj->obj_flags.value[0] != obj2->obj_flags.value[0])
    return 0;

  return 1;
}

void save_vault(char *name) {
  FILE *fl;
  struct vault_data *vault;
  struct vault_access_data *access;
  struct vault_items_data *items;
  char fname[256], buf[MAX_INPUT_LENGTH];

  if (!(vault = has_vault(name)))
    return;

  *name = UPPER(*name);
  sprintf(fname, "../vaults/%c/%s.vault", UPPER(*name), name);
  if(!(fl = dc_fopen(fname, "w"))) {
    sprintf(buf, "save_vaults: could not open vault file for [%s].", fname);
    log (buf, IMMORTAL, LOG_BUG);
    return;
  }

  fprintf(fl, "S %d\n", vault->size);
  fprintf(fl, "G %llu\n", vault->gold);

  for (items = vault->items;items;items = items->next)
  {
    fprintf(fl, "O %d %d %d\n", items->item_vnum, items->count, items->obj?1:0);
    if (items->obj)
      write_object(items->obj,fl);
  }

  for (access = vault->access;access;access = access->next)
    fprintf(fl, "A %s\n", access->name);
  
  fprintf(fl, "$\n");
  fclose(fl);
}

void vault_access(CHAR_DATA *ch, char *who)
{
  struct vault_access_data *access;
  struct vault_data *vault = NULL;

  if (!vault && !(vault = has_vault(GET_NAME(ch)))) {
    send_to_char("You don't seem to have a vault.\r\n", ch);
    return;
  }

  if (!*who) {
    csendf(ch, "The following people have access to your vault:\r\n");
    for (access = vault->access;access;access = access->next) 
      csendf(ch, "%s\r\n", access->name);
    return;
  }
     
  if (has_vault_access(who, vault))
    remove_vault_access(ch, who, vault);
  else
    add_vault_access(ch, who, vault);
}

void my_vault_access(CHAR_DATA *ch, char arg[MAX_INPUT_LENGTH])
{
  struct vault_data *vault;

  if (arg[0] != '\0')
  {
     if (!(vault = has_vault(arg)))
     {
	send_to_char("No such player.\r\n",ch);
	return;
     }
     if (!has_vault_access(GET_NAME(ch), vault))
     {
	send_to_char("You do not have access to that vault anyway.\r\n",ch);
	return;
     }
     access_remove(GET_NAME(ch), vault);
     send_to_char("You remove your access to that vault.\r\n",ch);
     return;
  }

  send_to_char("You have access to the following vaults:\r\n", ch);
  for (vault = vault_table;vault;vault = vault->next)
    if (vault && vault->owner && has_vault_access(GET_NAME(ch), vault))
      csendf(ch, "%s\r\n", vault->owner);
}

void show_vault_balance(CHAR_DATA *ch, char *owner) {
  struct vault_data *vault;
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch))) self = 1;
          
  if (!(vault = has_vault(owner))) {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else  
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }       
  
  if (!has_vault_access(GET_NAME(ch), vault)) {
    csendf(ch, "You don't have permission to see %s's balance.\r\n", owner);
    return; 
  } 

  if (self)
    csendf(ch, "You have %llu gold coins in your vault.\r\n", vault->gold);
  else
    csendf(ch, "There are %llu gold coins in %s's vault.\r\n", vault->gold, owner);
}

char *vault_usage = "Syntax: vault <list | balance> [name of vault owner]\r\n"
                    "        vault <put | get> <object> [name of vault owner]\r\n"
                    "        vault <deposit | withdraw> <amount>\r\n"
                    "        vault <access | myaccess> [name to add/remove access]\r\n"
                    "        vault log\r\n";

char *imm_vault_usage = "         vault <stats> [name]\r\n";

int do_vault(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg, arg1);

  if (affected_by_spell(ch, FUCK_PTHIEF)) {
    send_to_char("No thieves allowed in the vault area!\r\n", ch);
    return eSUCCESS;
  }

  if (!*arg) {
    send_to_char(vault_usage, ch);
    if (GET_LEVEL(ch) > IMMORTAL) send_to_char(imm_vault_usage, ch);
    return eSUCCESS;
  } 

  if (!str_cmp(arg1, "clan") && ch->clan)
    strcpy(arg1, clanVName(ch->clan));

  // show the contents of your or someone elses vault
  if (!strncmp(arg, "list", strlen(arg))) {
    if (!*arg1) sprintf(arg1, "%s", GET_NAME(ch));
    show_vault(ch, arg1);

  // show how much gold in vault
  } else if (!strncmp(arg, "balance", strlen(arg))) {
    if (!*arg1) sprintf(arg1, "%s", GET_NAME(ch));
    show_vault_balance(ch, arg1);

  // show what vaults I have access to
  } else if (!strncmp(arg, "myaccess", strlen(arg))) {
      my_vault_access(ch,arg1);

  // show my current access, add access, or remove access
  } else if (!strncmp(arg, "access", strlen(arg))) {
      vault_access(ch, arg1);

  } else if (GET_LEVEL(ch) > IMMORTAL && !strncmp(arg, "stats", strlen(arg))) {
      vault_stats(ch, arg1);
 
  // show how much gold in vault
  } else if (!strncmp(arg, "log", strlen(arg))) {
    if(*arg1 && !str_cmp(arg1, "clan") && ch->clan && has_right(ch, CLAN_RIGHTS_VAULT))
       strcpy(arg1, clanVName(ch->clan));
    if(!*arg1 || GET_LEVEL(ch) < IMMORTAL)
       sprintf(arg1, "%s", GET_NAME(ch));
    show_vault_log(ch, arg1);

  // putting this here so that anything below it requires you to be in a safe room.
  } else if (!IS_SET(world[ch->in_room].room_flags, SAFE)) {
    send_to_char("You don't feel safe enough to manage your valuables.\r\n", ch);
    return eSUCCESS;

  } else if (!strncmp(arg, "withdraw", strlen(arg))) {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));
    if (!*argument) {
      send_to_char("How much gold would you like get from the vault?\r\n", ch);
      return eSUCCESS;
    } else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    vault_withdraw(ch, atoi(argument), arg2);

  } else if (!strncmp(arg, "deposit", strlen(arg))) {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));
    if (!*argument) {
      send_to_char("How much gold would you like to place in the vault?\r\n", ch);
      return eSUCCESS;
    } else if (!*arg2) 
      sprintf(arg2, "%s", GET_NAME(ch));
    vault_deposit(ch, atoi(argument), arg2);

  // put something in your vault
  } else if (!strncmp(arg, "put", strlen(arg))) {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));
    if (!*argument) {
      send_to_char("What item would you like to place in the vault?\r\n", ch);
      return eSUCCESS;
    } else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    put_in_vault(ch, argument, arg2);

  // get something from your or someone elses vault
  } else if (!strncmp(arg, "get", strlen(arg))) {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));

    if (!*argument) {
      send_to_char("What item would you like to get from the vault?\r\n", ch);
      return eSUCCESS;
    } else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    get_from_vault(ch, argument, arg2);

  } else {
    send_to_char(vault_usage, ch);
    if (GET_LEVEL(ch) > IMMORTAL) send_to_char(imm_vault_usage, ch);
  }
  return eSUCCESS;
}

void vault_stats(CHAR_DATA *ch, char *name) {
  struct vault_data *vault = NULL;
  struct vault_items_data *item;
  struct vault_access_data *access;
  struct obj_data *obj;
  int items = 0, weight = 0, accesses = 0, num = 0, unique = 0, count = 0, skipped = 0;
  char buf[MAX_STRING_LENGTH*4], buf1[MAX_STRING_LENGTH];

  sprintf(buf, "###) Character Name        Gold     Items (Unique) Weight/  Max   Access\r\n");
  for (vault = vault_table;vault;vault = vault->next, num++) {
    if (!num) continue; // skip 0 cause its null

    if (name && *name && vault->owner && strncasecmp(vault->owner, name, strlen(name))) continue;
    count++;

    for (item = vault->items; item; item = item->next) {
      unique++;
      items += item->count;
      obj = item->obj?item->obj:get_obj(item->item_vnum);
  //    obj = get_obj(item->item_vnum);
      weight += item->count * GET_OBJ_WEIGHT(obj);
    }

    for (access = vault->access;access;access = access->next) {
      accesses++;
    }

    sprintf(buf1, "%3d) %-15s $B$5%10llu$R     %5d (%4d  ) %6d/%5d %6d\r\n",
                   count, vault->owner, vault->gold, items, unique, weight, vault->size, accesses);
    if ((strlen(buf1) + strlen(buf)) < MAX_STRING_LENGTH*4)
      strcat(buf, buf1);
    else
      skipped++;
    
    weight = 0;
    items = 0;
    accesses = 0;
    unique = 0;
  }

  page_string(ch->desc, buf, 1);
  csendf(ch, "Total Vaults: %d\r\n", count);
  csendf(ch, "Not Showing: %d\r\n", skipped);
}

void reload_vaults(void) {
  struct vault_data *vault, *tvault;
  struct vault_access_data *access, *taccess;
  struct vault_items_data *items, *titems;
  int num = 0;

  for (vault = vault_table; vault; vault = tvault, num++) {
    tvault = vault->next;

    if (vault && vault->items) {
      for (items = vault->items; items; items = titems) {
        titems = items->next;
        free(items);
      }
    }

    if (vault && vault->access) {
      for (access = vault->access; access; access = taccess) {
        taccess = access->next;
        free(access);
      }
    }

    if (vault)
      free(vault);
  }

  vault_table = NULL;

  load_vaults();
}

int do_fixvault(char_data *ch, char *argument, int cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
  if (!arg1[0] || !arg2[0])
  {
    send_to_char("Syntax: Fixvault <oldname> <newname>\r\n",ch);
    return eFAILURE;
  }
  rename_vault_owner(arg1, arg2);
  return eSUCCESS;
}

void rename_vault_owner(char *oldname, char *newname) {
  struct vault_data *vault;
  struct vault_access_data *access;
  char buf[MAX_INPUT_LENGTH];
  int num = 0;

  if ((vault = has_vault(newname))) {
    remove_vault(newname); // free it up first..
  }
  if ((vault = has_vault(oldname))) {
    free(vault->owner);
    vault->owner = str_dup(newname);
    save_vault(newname);
    sprintf(buf, "Vault owner changed from '%s' to '%s'.", oldname, newname);
    vault_log(buf, newname);
  }

  for (vault = vault_table;vault;vault = vault->next, num++) {
    if (!num) continue; // skip 0 cause its null

    for (access = vault->access;access;access = access->next) {
      if (!strncasecmp(access->name, oldname, strlen(oldname))) {
        sprintf(buf, "Replaced '%s' with '%s' in %s's vault access list.", access->name, newname, vault->owner);
        vault_log(buf, vault->owner);
        free(access->name);
        access->name = str_dup(newname);
        save_vault(vault->owner);
      }
    }
  }

  add_new_vault(newname, 1);
  //reload_vaults();
  remove_vault(oldname);
}

void remove_vault(char *name) {
  struct vault_data *vault, *next_vault, *prev_vault;
  struct vault_items_data *items, *titems;
  struct vault_access_data *access, *taccess;

  char buf[MAX_INPUT_LENGTH];
  char h[MAX_INPUT_LENGTH];

  name[0] = UPPER(name[0]);

  if (!(vault = has_vault(name)))
    return;

  sprintf(h, "../vaults/%c/%s.vault", *name, name);
  unlink(h);
  sprintf(h, "../vaults/%c/%s.vault.log", *name, name);
  unlink(h);
  sprintf(h, "cat %s| grep -iv '^%s$' > %s", VAULT_INDEX_FILE, name, VAULT_INDEX_FILE_TMP);
  system(h);
  unlink(VAULT_INDEX_FILE);
  rename(VAULT_INDEX_FILE_TMP, VAULT_INDEX_FILE);
  sprintf(buf, "Deleting %s's vault.\r\n", name);
  log(buf, ANGEL, LOG_VAULT);
  
  if (vault && vault->items) {
    for (items = vault->items; items; items = titems) {
      titems = items->next;
      if (items->obj)
	extract_obj(items->obj);
      free(items);
     }
  }

  if (vault && vault->access) {
    for (access = vault->access; access; access = taccess) {
      taccess = access->next;
      free(access);
    }
  }

  for (vault = vault_table; vault ; vault = next_vault) {
    next_vault = vault->next;
    if (!vault || !vault->owner || !*vault->owner) continue;

    if (!strcasecmp(vault->owner, name)) {
      if (vault == vault_table)
        vault_table = vault->next;
      else
        prev_vault->next = vault->next;
      free(vault);
      vault = NULL;
      total_vaults--;
      return;
    }
    prev_vault = vault;
  }
}

void load_vaults(void) {
  struct vault_data *vault;
  struct vault_access_data *access;
  struct vault_items_data *items;
  struct obj_data *obj;
  FILE *fl, *index;
  int vnum, full, count;
  long long unsigned int gold = 0;
  char value[128], line[128], buf[MAX_STRING_LENGTH], fname[MAX_INPUT_LENGTH], type[128], tmp[10];

  if(!(index = dc_fopen(VAULT_INDEX_FILE, "r"))) {
    log ("boot_vaults: could not open vault index file, probably doesn't exist.", IMMORTAL, LOG_BUG);
    return;
  }
  fscanf(index, "%s\n", line);
  while (*line != '$') {
    total_vaults++;
    sprintf(buf, "%d - %s", total_vaults, line);
//    log(buf, IMMORTAL, LOG_BUG);
    fscanf(index, "%s\n", line);
  }
  dc_fclose(index);

  sprintf(buf, "boot_vaults: found [%d] player vaults to read.", total_vaults);
  log(buf, IMMORTAL, LOG_BUG);
  if (total_vaults)
  CREATE(vault_table, struct vault_data, total_vaults);

  if(!(index = dc_fopen(VAULT_INDEX_FILE, "r"))) {
    log ("boot_vaults: could not open vault index file, probably doesn't exist.", IMMORTAL, LOG_BUG);
    return;
  }

  fscanf(index, "%s\n", line);
  while (*line != '$') {
    *line = UPPER(*line);
    sprintf(fname, "../vaults/%c/%s.vault", UPPER(*line), line);
    if(!(fl = dc_fopen(fname, "r"))) {
      sprintf(buf, "boot_vaults: unable to open file [%s].", fname);
      log(buf, IMMORTAL, LOG_BUG);
      fscanf(index, "%s\n", line);
      continue;
    } else {
      sprintf(buf, "boot_vaults: sucessfully opened file [%s].", fname);
//      log(buf, IMMORTAL, LOG_BUG);
    }

    CREATE(vault, struct vault_data, 1);

    vault->owner 	= str_dup(line);
    vault->size		= VAULT_BASE_SIZE;
    vault->gold		= 0;
    vault->weight	= 0;
    vault->access 	= NULL;
    vault->items 	= NULL;
    vault->next 	= NULL;

    get_line(fl, type);
    while(*type != '$') {
      switch (*type) {
        case 'S':
          sscanf(type, "%s %d", tmp, &vnum);
          vnum = MIN(vnum, VAULT_MAX_SIZE);
          vault->size	= vnum;
          break;
        case 'G':
          sscanf(type, "%s %llu", tmp, &gold);
          vault->gold	= gold;
          break;
        case 'O':
          if (sscanf(type, "%s %d %d %d", tmp, &vnum, &count, &full) == 4) {
            ;
          } else {
            sprintf(buf, "boot_vaults: Bad 'O' option in file [%s]: %s\r\n", fname, type);
            log(buf, 0, LOG_BUG);
            break;
          }
          CREATE(items, struct vault_items_data, 1);
	  if (!full) {
            obj = get_obj(vnum);
	    items->obj = NULL;
	  }
	  else {
	    char tmp[MAX_INPUT_LENGTH];
	    get_line(fl, tmp);
  	    obj = read_object(real_object(vnum), fl, TRUE);
	    items->obj = obj;
	  }

          if (!obj) {
            sprintf(buf, "boot_vaults: bad item vnum (#%d)", vnum);
            log(buf, 1, LOG_MISC);
            exit(1);
          }
          vault->weight += (GET_OBJ_WEIGHT(obj) * count);
          items->item_vnum  	= vnum;
          items->count	= count;
          items->next  	= vault->items;
          vault->items 	= items;
          sprintf(buf, "boot_vaults: got item [%s(%d)(%d)(%d)] from file [%s].", GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), count, vnum, fname);
	  if (items->obj && ((items->obj->obj_flags.wear_flags != get_obj(vnum)->obj_flags.wear_flags)||
(items->obj->obj_flags.size != get_obj(vnum)->obj_flags.size)||(items->obj->obj_flags.eq_level != get_obj(vnum)->obj_flags.eq_level)))
            log(buf, IMMORTAL, LOG_BUG);
          break;
        case 'A':
          sscanf(type, "%s %s", tmp, value);
          CREATE(access, struct vault_access_data, 1);
          access->name  = str_dup(value);
          access->next  = vault->access;
          vault->access = access;
          sprintf(buf, "boot_vaults: got access [%s] from file [%s].", access->name, fname);
  //        log(buf, IMMORTAL, LOG_BUG);
          break;
        default:
          sprintf(buf, "boot_vaults: unknown type in file [%s].", fname);
          log(buf, IMMORTAL, LOG_BUG);
          break;
      }
      get_line(fl, type);
    }

    vault->next = vault_table;
    vault_table = vault;

    dc_fclose(fl);
    fscanf(index, "%s\n", line);
  }
  dc_fclose(index);
}

void add_vault_access(CHAR_DATA *ch, char *name, struct vault_data *vault) {
  struct vault_access_data *access;
  struct descriptor_data d;
  
  name[0] = UPPER(name[0]);
  
  if (!strcasecmp(name, GET_NAME(ch))) {
    send_to_char("Don't be a moron, you already have access.\r\n", ch);
    return;
  }

  // must be done to clear out "d" before it is used
  memset((char *) &d, 0, sizeof(struct descriptor_data));
  if(!(load_char_obj(&d, name))) {
    send_to_char("You can't give access to someone who doesn't exist.\n\r", ch);
    return;
  }

  if (has_vault_access(name, vault)) {
    send_to_char("That person already has access to your vault.\r\n", ch);
    return;   
  }
 
  csendf(ch, "%s now has access to your vault.\r\n", name);
  CREATE(access, struct vault_access_data, 1);
  access->name 	= str_dup(name);
  access->next 	= vault->access;
  vault->access	= access;

  save_char_obj(ch);
}

void remove_vault_access(CHAR_DATA *ch, char *name, struct vault_data *vault) {

  name[0] = UPPER(name[0]);

  if (!strcasecmp(name, GET_NAME(ch))) {
    send_to_char("Don't be a moron, you can't remove your own access.\r\n", ch);
    return;
  }

  if (!has_vault_access(name, vault)) {
    send_to_char("That person doesn't have access to your vault.\n\r", ch);
    return;
  }

  csendf(ch, "%s no longer has access to your vault.\r\n", name);
  access_remove(name, vault);
  save_char_obj(ch);
}

void remove_vault_accesses(char *name) {
  struct vault_data *vault;
  struct vault_access_data *access, *next_access;
  char buf[MAX_INPUT_LENGTH];
  int num = 0;
  
  for (vault = vault_table;vault;vault = vault->next, num++) {
    if (!num) continue; // skip 0 cause its null

    for (access = vault->access;access;access = next_access) {
      next_access = access->next;
      if (!strncasecmp(access->name, name, strlen(name))) {
        sprintf(buf, "Removed %s's access to %s's vault.", name, vault->owner);
        vault_log(buf, vault->owner);
        access_remove(name, vault);
        save_vault(vault->owner);
      }
    }
  }
}

void access_remove(char *name, struct vault_data *vault) {
  struct vault_access_data *access, *next_access, *prev_access;

  for (access = vault->access; access ; access = next_access) {
    next_access = access->next;

    if (!strcmp(access->name, name)) {
      if (access == vault->access)
        vault->access = access->next;
      else 
        prev_access->next = access->next;
      free(access);
      access = NULL;
      break;
    }
    prev_access = access;
  }
}

char *clanVName(int c)
{ // returns clan1, clan2, etc
  static char buf[512];
  sprintf(buf,"Clan%d",c);
  return &buf[0];
}

int has_vault_access(char *who, struct vault_data *vault) 
{
  struct vault_access_data *access;
  struct char_data *ch;

  if ((ch = find_owner(who)) && GET_LEVEL(ch) >= 109)
    return 1;

  // its their vault
  if (!strcasecmp(vault->owner, who)) 
    return 1;

  if (ch && ch->clan && get_clan(ch->clan) && !str_cmp(clanVName(ch->clan), vault->owner)
	&& has_right(ch, CLAN_RIGHTS_VAULT))
     return 1;

  // its not their vault so check all the access lines
  for (access = vault->access;access;access = access->next) 
     if (!strcasecmp(access->name, who))  
       return 1;

  return 0;
}

struct vault_items_data *get_unique_item_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  struct obj_data *obj;
  int i = 1;
  
  for (items = vault->items;items;items = items->next) {
    obj = items->obj?items->obj:get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj)))
      if (i == num) 
        return items;
      else 
        i++;
  } 
    
  return 0;
} 

struct obj_data *get_unique_obj_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  struct obj_data *obj;
  int i = 1;

  for (items = vault->items;items;items = items->next) {
    obj = items->obj?items->obj:get_obj(items->item_vnum);
//    obj = get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj)))
      if (i == num) 
        return obj;
      else
        i++;
  }

  return 0;
}

struct vault_items_data *get_item_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  struct obj_data *obj;
  int i = 1, j;

  for (items = vault->items;items;items = items->next) {
    obj = items->obj?items->obj:get_obj(items->item_vnum);
//    obj = get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj)))
      if (i == num) 
        return items;
      else
        for (j = 1; j <= items->count; j++)
          if (isname(object, GET_OBJ_NAME(obj)))
            if (i == num) 
              return items;
            else 
              i++;
  }

  return 0;
}

struct obj_data *get_obj_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  struct obj_data *obj;
  int i = 1, j;

  for (items = vault->items;items;items = items->next) {
    obj = items->obj?items->obj:get_obj(items->item_vnum);
//    obj = get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj))) 
      if (i == num) 
        return obj;
       else 
        for (j = 1; j <= items->count; j++)  
          if (i == num) 
            return obj;
           else 
            i++;
  }

  return 0;
}

struct obj_data *exists_in_vault(struct vault_data *vault, obj_data *obj)
{
  struct vault_items_data *items;
  if (!obj) return 0;
  for (items = vault->items;items;items = items->next) {
      if (items->obj == obj) return obj;
  }

  return 0;
}

void get_from_vault(CHAR_DATA *ch, char *object, char *owner) {
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *obj, *tmp_obj;
  struct vault_items_data *items;
  struct vault_data *vault;
  int self = 0, num = 1, i;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch))) self = 1;

  if (!(vault = has_vault(owner))) {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault)) {
    csendf(ch, "You don't have permission to take %s's stuff.\r\n", owner);
    return;
  }

  if (sscanf(object, "%d.%s", &num, object) != 2)
    num = 1;

  if (sscanf(object, "all.%s", object)) {
    num = 0;  // count the number of items that match that keyword so we know how many to get
    for (items = vault->items; items ; items = items->next) {
      obj = items->obj?items->obj:get_obj(items->item_vnum);
  //    obj = get_obj(items->item_vnum);
      if (isname(object, GET_OBJ_NAME(obj)))
        num += items->count;
    }

    if (!num) {
      send_to_char("There is nothing like that in the vault.\n\r", ch);
      return;
    }

     //start at end of the list and get each item all the way back to the first one.
    for (i = num; i > 0; i--) {
      sprintf(buf, "%d.%s", i, object);
      get_from_vault(ch, buf, owner);
    }
    return;
  } else {

   if (!(obj = get_obj_in_vault(vault, object, num))) {
      send_to_char("There is nothing like that in the vault.\n\r", ch);
      return;
    }

    if (IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_char_for_item(ch, obj->item_number)) { 
      send_to_char("Why would you want another one of those?\r\n", ch);
      return;
    } 
  
    if (!self && (IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) ||
		IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) && GET_LEVEL(ch) < IMMORTAL) {
      send_to_char("That item seems to be bound to the vault.\r\n", ch);
      return;
    } 
  
    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
      send_to_char("You can't hold any more.\r\n", ch);
      return;
    }


    if (GET_LEVEL(ch) < IMMORTAL)
      sprintf(buf, "%s removed %s from %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), owner);
    else
      sprintf(buf, "%s removed %s[%d] from %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), owner);

    vault_log(buf, owner);
    csendf(ch, "%s has been removed from the vault.\r\n", GET_OBJ_SHORT(obj));
  
    item_remove(obj, vault);
   
    if (!fullSave(obj))
      tmp_obj = clone_object(real_object(GET_OBJ_VNUM(obj)));
    else 
    {
      tmp_obj = clone_object(real_object(GET_OBJ_VNUM(obj)));
      copySaveData(tmp_obj,obj);
      if (!exists_in_vault(vault, obj))
	extract_obj(obj);
    }
    obj_to_char(tmp_obj, ch);
  }

  save_vault(owner);
  save_char_obj(ch);
}

void item_add(int vnum, struct vault_data *vault) {
  struct vault_items_data *item;

  for (item = vault->items; item; item = item->next) {
    if (item->item_vnum == vnum && !item->obj) {
      item->count++;
      vault->weight += GET_OBJ_WEIGHT(get_obj(vnum));
      return;
    }
  }


  CREATE(item, struct vault_items_data, 1);
  item->item_vnum = vnum;
  item->count     = 1;
  item->next      = vault->items;
  vault->items    = item;

  vault->weight += GET_OBJ_WEIGHT(get_obj(vnum));
}

void item_add(obj_data *obj, struct vault_data *vault) {
  struct vault_items_data *item;
  int vnum = GET_OBJ_VNUM(obj);

  remove_from_object_list(obj);
  for (item = vault->items; item; item = item->next) {
    if (item->item_vnum == vnum && item->obj && fullItemMatch(item->obj, obj)) {
      item->count++;
      vault->weight += GET_OBJ_WEIGHT(obj);
      return;
    }
  }

  CREATE(item, struct vault_items_data, 1);
  item->obj       = obj;
  item->item_vnum = vnum;
  item->count     = 1;
  item->next      = vault->items;
  vault->items    = item;

  vault->weight += GET_OBJ_WEIGHT(obj);
}

void item_remove(obj_data *obj, struct vault_data *vault) {
  struct vault_items_data *item, *next_item, *prev_item;
  int vnum = GET_OBJ_VNUM(obj);

  for (item = vault->items; item ; item = next_item) {
    next_item = item->next;
    if ((!fullSave(obj) && (!item->obj || !fullSave(item->obj)) && item->item_vnum == vnum) || (item->obj && fullItemMatch(obj, item->obj))) {
      if (item->count > 1) {
        item->count--;
        vault->weight -= GET_OBJ_WEIGHT(get_obj(vnum));
        return;  
      } else if (item == vault->items)
        vault->items = item->next;
      else 
        prev_item->next = item->next;
      free(item);
      item = NULL;
      vault->weight -= GET_OBJ_WEIGHT(get_obj(vnum));
      return;
    }
    prev_item = item;
  }
}

int search_vault_by_vnum(int vnum, struct vault_data *vault) {
  struct vault_items_data *items;

  for (items = vault->items;items;items = items->next)
    if (items && items->item_vnum && items->item_vnum == vnum)
      return 1;

  return 0;
}

void vault_deposit(CHAR_DATA *ch, unsigned int amount, char *owner) {
  struct vault_data *vault;
  char buf[MAX_INPUT_LENGTH];
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch))) self = 1;

  if (!(vault = has_vault(owner))) { 
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault)) {
    csendf(ch, "You don't have permission to put gold in %s's vault.\r\n", owner);
    return;
  }

  if (!amount) {
    send_to_char("How much gold would you like to place in the vault?\r\n", ch);
    return;
  }

  if (amount > VAULT_MAX_DEPWITH || amount <= 0) {
    csendf(ch, "Valid amounts are from 1 to %d gold.\r\n", VAULT_MAX_DEPWITH);
    return;
  }
  
  if (GET_GOLD(ch) >= amount) {
    vault->gold += amount;
    GET_GOLD(ch) -= amount;
    save_char_obj(ch);
    save_vault(owner);
    sprintf(buf, "%s added %d gold to %s's vault.", GET_NAME(ch), amount, owner);
    vault_log(buf, owner);
    csendf(ch, "Done!  The current balance is now %llu gold.\r\n", vault->gold);
  } else {
    csendf(ch, "But you only have %ld gold coins!\r\n", GET_GOLD(ch));
  }
}

void vault_withdraw(CHAR_DATA *ch, unsigned int amount, char *owner) {
  struct vault_data *vault;
  char buf[MAX_INPUT_LENGTH];
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch))) self = 1;

  if (!(vault = has_vault(owner))) {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault)) {
    csendf(ch, "You don't have permission to put gold in %s's vault.\r\n", owner);
    return;
  }

  if (!amount) {
    send_to_char("How much gold would you like get from the vault?\r\n", ch);
    return;
  }

  if (amount > VAULT_MAX_DEPWITH || amount <= 0) {
    csendf(ch, "Valid amounts are from 1 to %d gold.\r\n", VAULT_MAX_DEPWITH);
    return;
  }

  if (vault->gold >= (long long unsigned int)amount) {
    if (amount + GET_GOLD(ch) > 2000000000) {
      csendf(ch, "You can't hold that much gold.  The most you could get is %ld.\r\n", 2000000000 - GET_GOLD(ch));
      return;
    }
    vault->gold -= amount;
    GET_GOLD(ch) += amount;
    save_char_obj(ch);
    save_vault(owner);
    sprintf(buf, "%s removed %d gold from %s's vault.", GET_NAME(ch), amount, owner);
    vault_log(buf, owner);
    csendf(ch, "Done!  The current balance is now %llu gold.\r\n", vault->gold);
  } else {
    csendf(ch, "The vault only has %llu gold coins in it!\r\n", vault->gold);
  }
}

int can_put_in_vault(struct obj_data *obj, int self, struct vault_data *vault, struct char_data *ch) {
//  struct obj_data *tmp_obj;
 
  if (GET_OBJ_VNUM(obj) == NOWHERE) {
    csendf(ch, "%s is hardly worth saving.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (GET_OBJ_VNUM(obj) == 393) { // no potatoes in the vaults
    csendf(ch, "It would rot. EWW!\r\n");
    return 0;
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_vault_by_vnum(GET_OBJ_VNUM(obj), vault)) { // Uniques
    csendf(ch, "Why would you need another %s?\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && !self) { // GL
    csendf(ch, "%s is far too valuable to place in someone else's vault.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (!self && IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) && GET_LEVEL(ch) < IMMORTAL) { // no_trade
    csendf(ch, "%s seems bound to you.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }
 
  if (IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP))  { //cursed
    csendf(ch, "%s is stuck! Ack!.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_NOSAVE))   { //nosave
    csendf(ch, "%s doesn't seem to be a permanent part of the world.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (obj->obj_flags.type_flag == ITEM_KEY)  { // keys
    csendf(ch, "Its not nice to horde all %s.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (obj->obj_flags.type_flag == ITEM_CONTAINER && obj->contains)  { // non-empty containers
    csendf(ch, "%s needs to be emptied first.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size) { // vault is full
    csendf(ch, "You can't seem to stuff %s in the vault.  Its too big!\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  return 1;
}

void put_in_vault(CHAR_DATA *ch, char *object, char *owner) {
  struct obj_data *obj, *tmp_obj;
  struct vault_data *vault;
  char buf[MAX_INPUT_LENGTH];
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch))) self = 1;

  if (!(vault = has_vault(owner))) {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault)) {
    csendf(ch, "You don't have permission to put things in %s's vault.\r\n", owner);
    return;
  }

  if (!strcmp(object, "all")) {
    for (obj = ch->carrying; obj ; obj = tmp_obj) {
      tmp_obj = obj->next_content;
      if (!can_put_in_vault(obj, self, vault, ch)) 
        continue;
      
      if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size) {
        csendf(ch, "%s won't fit in the vault!\r\n", GET_OBJ_SHORT(obj));
        continue;
      }

    if (GET_LEVEL(ch) < IMMORTAL)
      sprintf(buf, "%s added %s to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), owner);
    else
      sprintf(buf, "%s added %s[%d] to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), owner);

      vault_log(buf, owner);
      csendf(ch, "%s has been placed in the vault.\r\n", GET_OBJ_SHORT(obj));
      if (!fullSave(obj))
         { item_add(GET_OBJ_VNUM(obj), vault); extract_obj(obj); }
      else { obj_from_char(obj); item_add(obj, vault); }
    }
  } else if (sscanf(object, "all.%s", object)) {
    for (obj = ch->carrying; obj ; obj = tmp_obj) {
      tmp_obj = obj->next_content;
      if (!isname(object, GET_OBJ_NAME(obj))) 
        continue;
      if (!can_put_in_vault(obj, self, vault, ch)) 
        continue;

      if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size) {
        csendf(ch, "%s won't fit in the vault!\r\n", GET_OBJ_SHORT(obj));
        continue;
      }

      if (GET_LEVEL(ch) < IMMORTAL)
	sprintf(buf, "%s added %s to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), owner);
      else
	sprintf(buf, "%s added %s[%d] to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), owner);

      vault_log(buf, owner);
      csendf(ch, "%s has been placed in the vault.\r\n", GET_OBJ_SHORT(obj));
      if (!fullSave(obj)) {
        item_add(GET_OBJ_VNUM(obj), vault); extract_obj(obj); }
      else { obj_from_char(obj); item_add(obj, vault); }
    }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, object, ch->carrying))) {
      send_to_char("You don't have anything like that.\n\r", ch);
      return;
    }
  
    if (!can_put_in_vault(obj, self, vault, ch)) 
      return;

    if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size) {
      csendf(ch, "%s won't fit in the vault!\r\n", GET_OBJ_SHORT(obj));
      return;
    }

    if (GET_LEVEL(ch) < IMMORTAL)
      sprintf(buf, "%s added %s to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), owner);
    else
      sprintf(buf, "%s added %s[%d] to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), owner);

    vault_log(buf, owner);
    csendf(ch, "%s has been placed in the vault.\r\n", GET_OBJ_SHORT(obj));
  
    if (!fullSave(obj)) {
      item_add(GET_OBJ_VNUM(obj), vault); extract_obj(obj); }
    else { obj_from_char(obj); item_add(obj, vault);  }
  }
  save_vault(owner);
  save_char_obj(ch);
}


void show_vault(CHAR_DATA *ch, char *owner) {
  struct vault_items_data *items;
  struct vault_data *vault;
  struct obj_data *obj;
  int objects = 0, self = 0;
  char buf[MAX_STRING_LENGTH*4];
  char buf1[MAX_INPUT_LENGTH];

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch))) self = 1;

  if (!(vault = has_vault(owner))) {
    if (self)
      csendf(ch, "You don't have a vault.\r\n", ch);
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }
  
  if (!has_vault_access(GET_NAME(ch), vault)) {
    csendf(ch, "You don't have access to %s's vault.\r\n", owner);
    return;
  }

  if (self)
    sprintf(buf, "Your vault is at %d of %d maximum pounds and contains:\r\n", vault->weight, vault->size);
  else
    sprintf(buf, "%s's vault is at %d of %d maximum pounds and contains:\r\n", owner, vault->weight, vault->size);
   
  
  for (items = vault->items;items;items = items->next) {
    buf1[0] = '\0';
    if (items->count > 1) {
      sprintf(buf1, "[$5%d$R] ", items->count);
      strcat(buf, buf1);
    }

//    obj = get_obj(items->item_vnum);
    obj = items->obj?items->obj:get_obj(items->item_vnum);
    sprintf(buf1, "%s ", GET_OBJ_SHORT(obj));

      if (obj->obj_flags.type_flag == ITEM_ARMOR ||
          obj->obj_flags.type_flag == ITEM_WEAPON ||
          obj->obj_flags.type_flag == ITEM_FIREWEAPON ||
          obj->obj_flags.type_flag == ITEM_CONTAINER ||
          obj->obj_flags.type_flag == ITEM_INSTRUMENT ||
          obj->obj_flags.type_flag == ITEM_STAFF ||
          obj->obj_flags.type_flag == ITEM_WAND ||
          obj->obj_flags.type_flag == ITEM_LIGHT)
      { 
 extern char *item_condition(struct obj_data *obj);

    sprintf(buf1, "%s %s $3Lvl: %d$R",buf1,
			item_condition(obj), obj->obj_flags.eq_level);
       }
    if (GET_LEVEL(ch) > IMMORTAL) {
      sprintf(buf1, "%s [%d]", buf1,items->item_vnum);
    }
    objects = 1;
    strcat(buf1,"\r\n");
    if (strlen(buf1) + strlen(buf) < MAX_STRING_LENGTH*4 - 200)
      strcat(buf,buf1);
    else {
      strcat(buf, "Overflow!!!\r\n"); break; }
  }

  if (!objects)
    if (self)
      csendf(ch, "Your vault is currently empty and can hold %d pounds.\r\n", vault->size);
    else
      csendf(ch, "%s's vault is currently empty.\r\n", owner);
  else
    page_string(ch->desc, buf, 1);
}

void add_new_vault(char *name, int indexonly) {
  FILE *vfl, *tvfl, *pvfl;
  struct vault_data *vault;
  char fname[256], line[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];

  if (!(vfl = dc_fopen(VAULT_INDEX_FILE, "r" ))) {
    log("add_new_vault: error opening index file.", IMMORTAL, LOG_BUG);
    return;
  }

  if (!(tvfl = fopen( VAULT_INDEX_FILE_TMP, "w"))) {
    log("add_new_vault: error opening temp index file.", IMMORTAL, LOG_BUG);
    return;
  }

  // read and print each line until we get to $
  fscanf(vfl, "%s\n", line);
  while (*line != '$') {
    fprintf(tvfl, "%s\n", line);
    fscanf(vfl, "%s\n", line);
  }
  // we found $, now add in the new name, then the $
  fprintf(tvfl, "%s\n", name);
  fprintf(tvfl, "$\n");
  dc_fclose(tvfl);
  dc_fclose(vfl);
  rename(VAULT_INDEX_FILE_TMP, VAULT_INDEX_FILE);

  if (indexonly) return;


  // now create a new vault for the player
  sprintf(fname, "../vaults/%c/%s.vault", UPPER(*name), name);
  if (!(pvfl = dc_fopen(fname, "w"))) {
    sprintf(buf, "add_new_vault: error opening new vault file [%s].", fname);
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }

  fprintf(pvfl, "S %d\n", VAULT_BASE_SIZE);
  fprintf(pvfl, "$\n");
  dc_fclose(pvfl);

  sprintf(buf, "%s bought a vault.", name);
  vault_log(buf, name);

  // files all done, now add it in game
  total_vaults++;
  RECREATE(vault_table, struct vault_data, total_vaults);
  CREATE(vault, struct vault_data, 1);

  vault->owner 	= str_dup(name);
  vault->size	= VAULT_BASE_SIZE;
  vault->weight	= 0;
  vault->access	= NULL;
  vault->items 	= NULL;
  vault->next = vault_table;
  vault_table = vault;

  save_vault(name);
}

struct char_data *find_owner(char *name) {
  extern char_data * character_list;
  struct char_data *ch;
  for(ch = character_list; ch; ch = ch->next) 
    if (!strcmp(name, GET_NAME(ch)))
      return ch;

  return 0;
}

struct obj_data *get_obj(int vnum) 
{
  int num = real_object(vnum);
  
  return ((struct obj_data *)obj_index[num].item); 
}

void show_vault_log(CHAR_DATA *ch, char *owner)
{
  char buf1[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char fname[256];

  if(!strcmp(owner,clanVName(ch->clan))) {
     sprintf(buf, "The following are your clans most recent vault log entries(Times are EST):\n\r");
  }
  else sprintf(buf, "The following are your most recent vault log entries (Times are EST):\r\n");

  owner[0] = UPPER(owner[0]);

  sprintf(fname, "../vaults/%c/%s.vault.log", *owner, owner);
  vault_log_to_string(fname, buf1);

  strcat(buf, buf1);

  page_string(ch->desc, buf, 1);
}

void vault_log(char *message, char *name) {
  struct tm *tm = NULL;
  long ct;
  FILE *ofile, *nfile;
  char buf[MAX_INPUT_LENGTH], line[MAX_INPUT_LENGTH];
  char fname[256], nfname[256];
  int lines = 1;
  char mins[5], hours[5];

  static char *months[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
  };

  log(message, IMMORTAL, LOG_VAULT);

  sprintf(fname, "../vaults/%c/%s.vault.log",  *name, name);
  sprintf(nfname, "../vaults/%c/%s.vault.log.tmp", *name, name);

  if(!(ofile = dc_fopen(fname, "r"))) {
    if(!(ofile = dc_fopen(fname, "w"))) {
      sprintf(buf, "vault_log: could not open vault log file [%s].", fname);
      log(buf, IMMORTAL, LOG_BUG);
      return;
    }
    fprintf(ofile, "$\n");
    fclose(ofile);
    if(!(ofile = dc_fopen(fname, "r"))) {
      sprintf(buf, "vault_log: could not open vault log file [%s].", fname);
      log(buf, IMMORTAL, LOG_BUG);
      return;
    }
  }

  if(!(nfile = dc_fopen(nfname, "w"))) {
    sprintf(buf, "vault_log: could not open vault log file [%s].", nfname);
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }
  
  ct   = time(0);
  tm = localtime(&ct);

  if (tm->tm_min < 10) 
    sprintf(mins, "0%d", tm->tm_min);
  else 
    sprintf(mins, "%d", tm->tm_min);

  if (tm->tm_hour < 10) 
    sprintf(hours, "0%d", tm->tm_hour);
  else 
    sprintf(hours, "%d", tm->tm_hour);

  
  sprintf(buf, "%s %d %s:%s", months[tm->tm_mon],tm->tm_mday,hours,mins);
  fprintf(nfile, "%s :: %s\n", buf, message);
  
  get_line(ofile, line);
  while (*line != '$' && lines++ < 500) {
    fprintf(nfile, "%s\n", line);
    get_line(ofile, line);
  }
  fprintf(nfile, "$\n");
  
  fclose(nfile);
  fclose(ofile);
  unlink(fname);
  rename(nfname, fname);
 // sprintf(cmd, "mv %s %s", nfname, fname);
 // system(cmd);
}

int vault_log_to_string(const char *name, char *buf) {
    FILE *fl;
    char tmp[512];

    *buf = '\0';

    if (!(fl = dc_fopen(name, "r"))) {
      perror("vault_log_to_string");
      return(-1);
    }

    do {
      fgets(tmp, 510, fl);

      if (!feof(fl)) {
        if (strlen(buf) + strlen(tmp) + 2 > MAX_STRING_LENGTH) {
          log("fl->strng: string too big (vault_log_to_string)", 0, LOG_BUG);
          *buf = '\0';
          return(-1);
        }

        tmp[strlen(tmp) -1] = '\0';
        strcat(tmp, "\r\n");
        strcat(buf, tmp);
      }
    } while (!feof(fl));

    dc_fclose(fl);

    return(0);
}


int sleazy_vault_guy(struct char_data *ch, struct obj_data *obj, int cmd, char *arg,
          struct char_data *owner)
{
  if (cmd != 59 && cmd != 56) return eFAILURE;
  if (IS_NPC(ch)) return eFAILURE;
  char arg1[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
  arg = one_argument(arg, arg1);
  
  struct vault_data *vault = has_vault(GET_NAME(ch));
  struct vault_data *cvault = ch->clan?has_vault(clanVName(ch->clan)):0;
  if (cmd == 59)
  {
      if (!vault)
      {
	send_to_char("You need to level up some before obtaining a vault.\r\n",ch);
	return eSUCCESS;
      }
      else if (vault->size < VAULT_MAX_SIZE)
         sprintf(buf, "$B1)$R Increase the size of vault by 10 lbs: %d platinum.\r\n",VAULT_UPGRADE_COST);
      else
	sprintf(buf, "1) You cannot increase your vault-size further.\r\n");
      send_to_char("$B$2Paul the sleazy vault salesman tells you, 'How aboot a bigger vault? Size matters, you know'$R\r\n",ch);

      send_to_char(buf,ch);

      sprintf(buf,"$B2)$R Purchase a clan vault: %s\r\n",
		ch->clan?cvault?"Your clan already has a vault":has_right(ch, CLAN_RIGHTS_VAULT)?"1000 platinum coins.":"You are not authorized to make this purchase.":"You are not a member of any clan.");
      send_to_char(buf,ch);

      if (!cvault)
        sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", ch->clan?"Your clan has no vault.":"You're not in a clan.");
      else if (cvault->size < VAULT_MAX_SIZE)
        sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", has_right(ch, CLAN_RIGHTS_VAULT)?"200 platinum coins.":"You are not authorized to make this purchase.");
      else
        sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: You cannot increase the vault's size further.\r\n");
      send_to_char(buf,ch);
      return eSUCCESS;
  } else {
     int i = atoi(arg1); 
     switch (i)
     {
	case 1:
		if (!vault) 
	        {
	  	  send_to_char("You need to level up some before obtaining a vault.\r\n",ch);
		  return eSUCCESS;
	        }
		if (vault->size >= VAULT_MAX_SIZE)
		{
		  send_to_char("Your vault's size is already at its maximum capacity.\r\n",ch);
		  return eSUCCESS;
		}
		if (GET_PLATINUM(ch) < VAULT_UPGRADE_COST)
		{
		  send_to_char("You do not have enough platinum.\r\n",ch);
		  return eSUCCESS;
		}
		GET_PLATINUM(ch) -= VAULT_UPGRADE_COST;
		vault->size += 10;
		save_char_obj(ch);
		save_vault(vault->owner);
		send_to_char("$B$2Paul the sleazy vault salesman tells you, '10 lbs added to your vault.$R'\r\n",ch);
		return eSUCCESS;
	case 2: 
		if (cvault) 
	        {
	  	  send_to_char("Your clan already has a vault.\r\n",ch);
		  return eSUCCESS;
	        }
		if (!ch->clan)
		{
		  send_to_char("You're not a member of any clan.\r\n",ch);
		  return eSUCCESS;
		}
		if (!has_right(ch, CLAN_RIGHTS_VAULT))
		{
		  send_to_char("You are not authorized to make that purchase.\r\n",ch);
		  return eSUCCESS;
		}
		if (GET_PLATINUM(ch) < 1000)
		{
		  send_to_char("You do not have enough platinum.\r\n",ch);
		  return eSUCCESS;
		}
		GET_PLATINUM(ch) -= 1000;
		add_new_vault(clanVName(ch->clan),0);
		save_char_obj(ch);
		save_vault(clanVName(ch->clan));
		send_to_char("You have purchased vault for your clan's perusal.\r\n",ch);
		return eSUCCESS;
	case 3:
		if (!cvault) 
	        {
	  	  send_to_char("Your clan does not have a vault.\r\n",ch);
		  return eSUCCESS;
	        }
		if (!has_right(ch, CLAN_RIGHTS_VAULT))
		{
		  send_to_char("You are not authorized to make that purchase.\r\n",ch);
		  return eSUCCESS;
		}
		if (cvault->size >= VAULT_MAX_SIZE)
		{
		  send_to_char("The vault is already at its maximum capacity.\r\n",ch);
		  return eSUCCESS;
		}
		if (GET_PLATINUM(ch) < 200)
		{
		  send_to_char("You do not have enough platinum.\r\n",ch);
		  return eSUCCESS;
		}
		GET_PLATINUM(ch) -= 200;
		cvault->size += 10;
		save_char_obj(ch);
		save_vault(clanVName(ch->clan));
		send_to_char("You have added 10 lbs capacity to your clan's vault.\r\n",ch);
		return eSUCCESS;
		
     }
  }
  return eFAILURE;
}
