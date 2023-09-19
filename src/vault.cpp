extern "C"
{
#include <ctype.h>
#include <string.h>
}
#include <fstream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list>
#include <fmt/format.h>

#include "structs.h"
#include "db.h"
#include "utility.h"
#include "vault.h"
#include "room.h"
#include "player.h"
#include "fileinfo.h"
#include "obj.h"
#include "act.h"
#include "handler.h"
#include "levels.h"
#include "connect.h"
#include "returnvals.h"
#include "interp.h"
#include "spells.h"
#include "clan.h" // clan right
#include "inventory.h"

using namespace std;

enum vault_search_type
{
  UNDEFINED,
  KEYWORD,
  LEVEL,
  MIN_LEVEL,
  MAX_LEVEL
};

class vault_search_parameter
{
public:
  vault_search_parameter();
  ~vault_search_parameter();
  vault_search_type type;
  union
  {
    char *str_argument;
    int int_argument;
  };
};

vault_search_parameter::vault_search_parameter()
    : type(UNDEFINED)
{
}

vault_search_parameter::~vault_search_parameter()
{
  if (type == KEYWORD && str_argument != nullptr)
  {
    free(str_argument);
  }
}

extern World world;
int total_vaults = 0;
int get_line(FILE *fl, char *buf);

void item_remove(Object *obj, struct vault_data *vault);
void item_add(int vnum, struct vault_data *vault);

Character *find_owner(const char *name);
void vault_log(Character *ch, char *owner);
char *clanVName(int c);
void vault_search_usage(Character *ch);

extern struct index_data *obj_index;
extern class Object *object_list;

struct vault_data *has_vault(const char *name)
{
  struct vault_data *vault;

  for (vault = vault_table; vault; vault = vault->next)
    if (vault && vault->owner && !strcasecmp(vault->owner, name))
      return vault;
  Character *ch = find_owner(name);
  if (ch && GET_LEVEL(ch) >= 10)
  {
    add_new_vault(GET_NAME(ch), 0);
    for (vault = vault_table; vault; vault = vault->next)
      if (vault && vault->owner && !strcasecmp(vault->owner, name))
      {
        //	 send_to_char("A vault was created for you.\r\n",ch);
        return vault;
      }
  }
  return 0;
}

void remove_from_object_list(Object *obj)
{
  Object *tObj, *pObj = nullptr;
  for (tObj = object_list; tObj; tObj = tObj->next)
  {
    if (tObj == obj)
    {
      if (pObj)
        pObj->next = tObj->next;
      else
        object_list = tObj->next;
      tObj->next = nullptr;
      break;
    }
    pObj = tObj;
  }
}

void save_vault(char *name)
{
  FILE *fl;
  struct vault_data *vault;
  struct vault_access_data *access;
  struct vault_items_data *items;
  char fname[256], buf[MAX_INPUT_LENGTH];

  if (!(vault = has_vault(name)))
    return;

  Character *ch = find_owner(name);
  if (ch)
    if (vault->size < (unsigned)(GET_LEVEL(ch) * 10))
      vault->size = GET_LEVEL(ch) * 10;

  *name = UPPER(*name);
  sprintf(fname, "../vaults/%c/%s.vault", UPPER(*name), name);
  if (!(fl = fopen(fname, "w")))
  {
    sprintf(buf, "save_vaults: could not open vault file for [%s].", fname);
    logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  fprintf(fl, "S %d\n", vault->size);
  fprintf(fl, "G %llu\n", vault->gold);

  for (items = vault->items; items; items = items->next)
  {
    Object *obj = items->obj ? items->obj : get_obj(items->item_vnum);
    if (obj == 0)
      continue;

    fprintf(fl, "O %d %d %d\n", items->item_vnum, items->count, items->obj ? 1 : 0);
    if (items->obj)
      write_object(items->obj, fl);
  }

  for (access = vault->access; access; access = access->next)
    fprintf(fl, "A %s\n", access->name);

  fprintf(fl, "$\n");
  fclose(fl);
}

void vault_access(Character *ch, char *who)
{
  struct vault_access_data *access;
  struct vault_data *vault = nullptr;

  if (!vault && !(vault = has_vault(GET_NAME(ch))))
  {
    send_to_char("You don't seem to have a vault.\r\n", ch);
    return;
  }

  if (!*who)
  {
    csendf(ch, "The following people have access to your vault:\r\n");
    for (access = vault->access; access; access = access->next)
      csendf(ch, "%s\r\n", access->name);
    return;
  }

  if (has_vault_access(who, vault))
    remove_vault_access(ch, who, vault);
  else
    add_vault_access(ch, who, vault);
}

void vault_myaccess(Character *ch, char arg[MAX_INPUT_LENGTH])
{
  struct vault_data *vault;

  if (arg[0] != '\0')
  {
    if (!(vault = has_vault(arg)))
    {
      send_to_char("No such player.\r\n", ch);
      return;
    }
    if (!has_vault_access(GET_NAME(ch), vault))
    {
      send_to_char("You do not have access to that vault anyway.\r\n", ch);
      return;
    }
    access_remove(GET_NAME(ch), vault);
    send_to_char("You remove your access to that vault.\r\n", ch);
    return;
  }

  send_to_char("You have access to the following vaults:\r\n", ch);
  for (vault = vault_table; vault; vault = vault->next)
    if (vault && vault->owner && has_vault_access(GET_NAME(ch), vault))
      csendf(ch, "%s\r\n", vault->owner);
}

void vault_balance(Character *ch, char *owner)
{
  struct vault_data *vault;
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch)))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    csendf(ch, "You don't have permission to see %s's balance.\r\n", owner);
    return;
  }

  if (self)
    csendf(ch, "You have %llu $B$5gold$R coins in your vault.\r\n", vault->gold);
  else
    csendf(ch, "There are %llu $B$5gold$R coins in %s's vault.\r\n", vault->gold, owner);
}

char *vault_usage = "Syntax: vault <list | balance> [vault owner]\r\n"
                    "        vault <put | get> <object> [vault owner]\r\n"
                    "        vault <deposit | withdraw> <amount>\r\n"
                    "        vault <access | myaccess> [name to add/remove access]\r\n"
                    "        vault log [vault owner]\r\n"
                    "        vault search [ keyword <keyword> ] | [ level <levels> ] | ...\r\n";

char *imm_vault_usage = "        vault <stats> [name]\r\n";

int do_vault(Character *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg, arg1);

  if (affected_by_spell(ch, FUCK_PTHIEF) || (affected_by_spell(ch, FUCK_GTHIEF)))
  {
    send_to_char("You're too busy running from the law!\r\n", ch);
    return eFAILURE;
  }

  if (!*arg)
  {
    send_to_char(vault_usage, ch);
    if (GET_LEVEL(ch) > IMMORTAL)
      send_to_char(imm_vault_usage, ch);
    return eSUCCESS;
  }

  if (!str_cmp(arg1, "clan") && ch->clan)
    strcpy(arg1, clanVName(ch->clan));

  // show the contents of your or someone elses vault
  if (!strncmp(arg, "list", strlen(arg)))
  {
    if (!*arg1)
      sprintf(arg1, "%s", GET_NAME(ch));
    vault_list(ch, arg1);

    // show how much gold in vault
  }
  else if (!strncmp(arg, "balance", strlen(arg)))
  {
    if (!*arg1)
      sprintf(arg1, "%s", GET_NAME(ch));
    vault_balance(ch, arg1);

    // show what vaults I have access to
  }
  else if (!strncmp(arg, "myaccess", strlen(arg)))
  {
    vault_myaccess(ch, arg1);

    // show my current access, add access, or remove access
  }
  else if (!strncmp(arg, "access", strlen(arg)))
  {
    vault_access(ch, arg1);
  }
  else if (GET_LEVEL(ch) > IMMORTAL && !strncmp(arg, "stats", strlen(arg)))
  {
    vault_stats(ch, arg1);

    // show vault log
  }
  else if (!strncmp(arg, "log", strlen(arg)))
  {
    if (*arg1)
    {
      if (!strcasecmp(arg1, clanVName(ch->clan)))
      {
        clan_data *clan = get_clan(ch);
        if (clan == nullptr)
        {
          send_to_char("You are not a member of any clan.\r\n", ch);
          return eFAILURE;
        }

        // Clan leader or a clan member with the vaultlog right can view log.
        if ((clan->leader && !strcmp(clan->leader, GET_NAME(ch))) || has_right(ch, CLAN_RIGHTS_VAULTLOG))
        {
          stringstream clanName;

          clanName << "clan" << clan->number;
          sprintf(arg1, "%s", clanName.str().c_str());

          vault_log(ch, arg1);
        }
        else
        {
          send_to_char("You don't have access to view the clan's vault log.\r\n", ch);
          return eFAILURE;
        }
      }
      else if (GET_LEVEL(ch) >= IMMORTAL)
      {
        vault_log(ch, arg1);
      }
      else
      {
        send_to_char("Syntax: vault log <clan>\n\r", ch);
        return eFAILURE;
      }
    }
    else
    {
      sprintf(arg1, "%s", GET_NAME(ch));
      vault_log(ch, arg1);
    }
  }
  else if (!strcmp(arg, "search"))
  {
    if (*arg1)
    {
      return vault_search(ch, arg1);
    }
    else
    {
      vault_search_usage(ch);
      return eFAILURE;
    }
    // putting this here so that anything below it requires you to be in a safe room.
  }
  else if (!IS_SET(world[ch->in_room].room_flags, SAFE))
  {
    send_to_char("You don't feel safe enough to manage your valuables.\r\n", ch);
    return eSUCCESS;
  }
  else if (!strncmp(arg, "withdraw", strlen(arg)))
  {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));
    if (!*argument)
    {
      send_to_char("How much $B$5gold$R would you like get from the vault?\r\n", ch);
      return eSUCCESS;
    }
    else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    vault_withdraw(ch, atoi(argument), arg2);
  }
  else if (!strncmp(arg, "deposit", strlen(arg)))
  {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));
    if (!*argument)
    {
      send_to_char("How much $B$5gold$R would you like to place in the vault?\r\n", ch);
      return eSUCCESS;
    }
    else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    vault_deposit(ch, atoi(argument), arg2);

    // put something in your vault
  }
  else if (!strncmp(arg, "put", strlen(arg)))
  {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));
    if (!*argument)
    {
      send_to_char("What item would you like to place in the vault?\r\n", ch);
      return eSUCCESS;
    }
    else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    vault_put(ch, argument, arg2);

    // get something from your or someone elses vault
  }
  else if (!strncmp(arg, "get", strlen(arg)))
  {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan));

    if (!*argument)
    {
      send_to_char("What item would you like to get from the vault?\r\n", ch);
      return eSUCCESS;
    }
    else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    vault_get(ch, argument, arg2);
  }
  else
  {
    send_to_char(vault_usage, ch);
    if (GET_LEVEL(ch) > IMMORTAL)
      send_to_char(imm_vault_usage, ch);
  }
  return eSUCCESS;
}

void vault_stats(Character *ch, char *name)
{
  struct vault_data *vault = nullptr;
  struct vault_items_data *item;
  struct vault_access_data *access;
  class Object *obj;
  int items = 0, weight = 0, accesses = 0, num = 0, unique = 0, count = 0, skipped = 0;
  char buf[MAX_STRING_LENGTH * 4], buf1[MAX_STRING_LENGTH];

  sprintf(buf, "###) Character Name        Gold     Items (Unique) Item Weight/Vault Weight/Vault Max Weight Access   Errors\r\n");
  for (vault = vault_table; vault; vault = vault->next, num++)
  {

    if (vault->owner == nullptr)
      continue; // skip 0 cause its null

    if (name && *name && vault->owner && strncasecmp(vault->owner, name, strlen(name)))
      continue;
    count++;

    for (item = vault->items; item; item = item->next)
    {
      unique++;
      items += item->count;
      obj = item->obj ? item->obj : get_obj(item->item_vnum);
      weight += item->count * GET_OBJ_WEIGHT(obj);
    }

    for (access = vault->access; access; access = access->next)
    {
      accesses++;
    }

    sprintf(buf1, "%3d) %-15s $B$5%10llu$R     %5d (%4d  ) %11d/%12d/%16d %6d %s\r\n",
            count, vault->owner, vault->gold, items, unique, weight, vault->weight, vault->size, accesses, weight != vault->weight ? "$5mismatch$R" : "$1    none$R");
    if ((strlen(buf1) + strlen(buf)) < MAX_STRING_LENGTH * 4)
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

void reload_vaults(void)
{
  struct vault_data *vault, *tvault;
  struct vault_access_data *access, *taccess;
  struct vault_items_data *items, *titems;
  int num = 0;

  for (vault = vault_table; vault; vault = tvault, num++)
  {
    tvault = vault->next;

    if (vault && vault->items)
    {
      for (items = vault->items; items; items = titems)
      {
        titems = items->next;
        free(items);
      }
    }

    if (vault && vault->access)
    {
      for (access = vault->access; access; access = taccess)
      {
        taccess = access->next;
        free(access);
      }
    }

    if (vault)
      free(vault);
  }

  vault_table = nullptr;

  load_vaults();
}

void rename_vault_owner(char *oldname, char *newname)
{
  struct vault_data *vault;
  struct vault_access_data *access;
  struct vault_items_data *items;
  char buf[MAX_INPUT_LENGTH];
  int num = 0;

  if ((vault = has_vault(newname)))
  {
    remove_vault(newname); // free it up first..
  }
  if ((vault = has_vault(oldname)))
  {
    free(vault->owner);
    vault->owner = str_dup(newname);
    save_vault(newname);
    sprintf(buf, "Vault owner changed from '%s' to '%s'.", oldname, newname);
    vlog(buf, newname);
  }

  if (!vault)
    return;

  for (items = vault->items; items; items = items->next)
  {
    if (items->obj && IS_SET(items->obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      char tmp[256];
      sprintf(tmp, "%s", items->obj->name);
      tmp[strlen(tmp) - strlen(oldname) - 1] = '\0';
      sprintf(tmp, "%s %s", tmp, newname);
      items->obj->name = str_hsh(tmp);
    }
  }
  save_vault(newname);

  for (vault = vault_table; vault; vault = vault->next, num++)
  {
    if (!num)
      continue; // skip 0 cause its null

    for (access = vault->access; access; access = access->next)
    {
      if (!strcasecmp(access->name, oldname))
      {
        sprintf(buf, "Replaced '%s' with '%s' in %s's vault access list.", access->name, newname, vault->owner);
        vlog(buf, vault->owner);
        free(access->name);
        access->name = str_dup(newname);
        save_vault(vault->owner);
      }
    }
  }

  add_new_vault(newname, 1);
  // reload_vaults();
  remove_vault(oldname);
}

void remove_vault(char *name, BACKUP_TYPE backup)
{
  char src_filename[256];
  char dst_dir[256] = {0};
  char syscmd[512];
  struct stat statbuf;

  if (name == nullptr)
  {
    return;
  }

  name[0] = UPPER(name[0]);

  switch (backup)
  {
  case SELFDELETED:
    strncpy(dst_dir, "../archive/selfdeleted/", 256);
    break;
  case CONDEATH:
    strncpy(dst_dir, "../archive/condeath/", 256);
    break;
  case ZAPPED:
    strncpy(dst_dir, "../archive/zapped/", 256);
    break;
  case NONE:
    break;
  default:
    logf(108, LogChannels::LOG_GOD, "remove_vault passed invalid BACKUP_TYPE %d for %s.", backup,
         name);
    break;
  }

  snprintf(src_filename, 256, "%s/%c/%s.vault", VAULT_DIR, name[0], name);

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }

  snprintf(src_filename, 256, "%s/%c/%s.vault.backup", VAULT_DIR, name[0], name);

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }

  snprintf(src_filename, 256, "%s/%c/%s.vault.log", VAULT_DIR, name[0], name);

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }

  remove_vault_accesses(name);

  struct vault_data *vault, *next_vault, *prev_vault = nullptr;
  struct vault_items_data *items, *titems;
  struct vault_access_data *access, *taccess;

  char buf[MAX_INPUT_LENGTH];
  char h[MAX_INPUT_LENGTH];

  sprintf(h, "cat %s| grep -iv '^%s$' > %s", VAULT_INDEX_FILE, name, VAULT_INDEX_FILE_TMP);
  system(h);
  unlink(VAULT_INDEX_FILE);
  rename(VAULT_INDEX_FILE_TMP, VAULT_INDEX_FILE);
  sprintf(buf, "Deleting %s's vault.", name);
  logentry(buf, ANGEL, LogChannels::LOG_VAULT);

  if (!(vault = has_vault(name)))
    return;

  if (vault && vault->items)
  {
    for (items = vault->items; items; items = titems)
    {
      titems = items->next;
      if (items->obj)
        extract_obj(items->obj);
      free(items);
    }
  }

  if (vault && vault->access)
  {
    for (access = vault->access; access; access = taccess)
    {
      taccess = access->next;
      free(access);
    }
  }

  for (vault = vault_table; vault; vault = next_vault)
  {
    next_vault = vault->next;
    if (!vault || !vault->owner || !*vault->owner)
      continue;

    if (!strcasecmp(vault->owner, name))
    {
      if (vault == vault_table)
        vault_table = vault->next;
      else
        prev_vault->next = vault->next;
      free(vault);
      vault = nullptr;
      total_vaults--;
      return;
    }
    prev_vault = vault;
  }
}

void load_vaults(void)
{
  struct vault_data *vault;
  struct vault_access_data *access;
  struct vault_items_data *items;
  class Object *obj = nullptr;
  struct stat statbuf = {};
  FILE *fl = nullptr, *index = nullptr;
  int vnum = 0, full = 0, count = 0;
  uint64_t gold = 0;
  char value[128] = {}, line[128] = {}, buf[MAX_STRING_LENGTH] = {}, fname[MAX_INPUT_LENGTH] = {}, type[128] = {}, tmp[10] = {};
  bool saveChanges = false;
  char src_filename[256] = {};

  if (!(index = fopen(VAULT_INDEX_FILE, "r")))
  {
    logentry("boot_vaults: could not open vault index file, probably doesn't exist.", IMMORTAL, LogChannels::LOG_BUG);
    return;
  }
  fscanf(index, "%s\n", line);
  while (*line != '$')
  {
    total_vaults++;
    sprintf(buf, "%d - %s", total_vaults, line);
    //    logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
    fscanf(index, "%s\n", line);
  }
  fclose(index);

  sprintf(buf, "boot_vaults: found [%d] player vaults to read.", total_vaults);
  // logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
  if (total_vaults)
    CREATE(vault_table, struct vault_data, total_vaults);

  if (!(index = fopen(VAULT_INDEX_FILE, "r")))
  {
    logentry("boot_vaults: could not open vault index file, probably doesn't exist.", IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  fscanf(index, "%s\n", line);
  while (*line != '$')
  {
    saveChanges = false;

    *line = UPPER(*line);
    sprintf(fname, "../vaults/%c/%s.vault", UPPER(*line), line);
    if (!(fl = fopen(fname, "r")))
    {
      sprintf(buf, "boot_vaults: unable to open file [%s].", fname);
      logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
      fscanf(index, "%s\n", line);
      continue;
    }
    else
    {
      sprintf(buf, "boot_vaults: sucessfully opened file [%s].", fname);
      //      logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
    }

    CREATE(vault, struct vault_data, 1);

    vault->owner = str_dup(line);
    vault->size = VAULT_BASE_SIZE;
    vault->gold = 0;
    vault->weight = 0;
    vault->access = nullptr;
    vault->items = nullptr;
    vault->next = nullptr;

    get_line(fl, type);
    while (*type != '$')
    {
      switch (*type)
      {
      case 'S':
        sscanf(type, "%s %d", tmp, &vnum);
        vnum = MIN(vnum, VAULT_MAX_SIZE);
        vault->size = vnum;

        /*
        if (vault->size > 2000) {
            logf(IMMORTAL, LogChannels::LOG_BUG, "boot_vaults: buggy vault size of %d on %s.", vault->size, vault->owner);

            FILE *oldfl;
            char oldfname[MAX_INPUT_LENGTH], oldtype[MAX_INPUT_LENGTH];

            sprintf(oldfname, "../vaults.old/%c/%s.vault", UPPER(*line), line);
            if(!(oldfl = fopen(oldfname, "r"))) {
          sprintf(buf, "boot_vaults: unable to open file [%s].", oldfname);
          logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
            } else {
          get_line(oldfl, oldtype);

          if (*oldtype == 'S') {
              sscanf(oldtype, "%s %d", tmp, &vnum);
              vault->size = vnum;
              logf(IMMORTAL, LogChannels::LOG_BUG, "boot_vaults: Setting %s's vault size to %d.", vault->owner, vault->size);
              saveChanges = true;
          }

          fclose(oldfl);
            }
        }
        */

        break;
      case 'G':
        sscanf(type, "%s %llu", tmp, &gold);
        vault->gold = gold;
        break;
      case 'O':
        if (sscanf(type, "%s %d %d %d", tmp, &vnum, &count, &full) == 4)
        {
          ;
        }
        else
        {
          sprintf(buf, "boot_vaults: Bad 'O' option in file [%s]: %s\r\n", fname, type);
          logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
          break;
        }

        CREATE(items, struct vault_items_data, 1);

        if (!full)
        {
          obj = get_obj(vnum);
          items->obj = nullptr;
        }
        else
        {
          char tmp[MAX_INPUT_LENGTH];
          get_line(fl, tmp);
          obj = read_object(real_object(vnum), fl, true);
          items->obj = obj;
        }

        if (!obj)
        {
          if (full)
          {
            // Skip the rest of the full item
            while (strcmp(type, "S") != 0)
            {
              get_line(fl, type);
            }
          }

          sprintf(buf, "boot_vaults: bad item vnum (#%d) in vault: %s", vnum,
                  vault->owner);
          logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
          saveChanges = true;
        }
        else
        {
          vault->weight += (GET_OBJ_WEIGHT(obj) * count);
          items->item_vnum = vnum;
          items->count = count;
          items->next = vault->items;
          vault->items = items;
          /*
          sprintf(buf, "boot_vaults: got item [%s(%d)(%d)(%d)] from file [%s].", GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), count, vnum, fname);
          if (items->obj && ((items->obj->obj_flags.wear_flags != get_obj(vnum)->obj_flags.wear_flags) ||
                             (items->obj->obj_flags.size != get_obj(vnum)->obj_flags.size) || (items->obj->obj_flags.eq_level != get_obj(vnum)->obj_flags.eq_level)))
            logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
          */
        }
        break;
      case 'A':
        sscanf(type, "%s %s", tmp, value);

        // Confirm if the character exists before giving it access to this vault
        snprintf(src_filename, 256, "%s/%c/%s", SAVE_DIR, value[0], value);

        if (0 == stat(src_filename, &statbuf))
        {
          CREATE(access, struct vault_access_data, 1);
          access->name = str_dup(value);
          access->next = vault->access;
          vault->access = access;
          sprintf(buf, "boot_vaults: got access [%s] from file [%s].", access->name, fname);
          //        logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
        }
        else
        {
          sprintf(buf, "Invalid access entry found. Removing %s's access to %s.", value, vault->owner);
          vlog(buf, vault->owner);
          saveChanges = true;
        }

        break;
      default:
        sprintf(buf, "boot_vaults: unknown type in file [%s].", fname);
        logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
        break;
      }
      get_line(fl, type);
    }

    vault->next = vault_table;
    vault_table = vault;

    fclose(fl);

    if (saveChanges)
    {
      logf(IMMORTAL, LogChannels::LOG_BUG, "boot_vaults: Saving changes to %s's vault.", vault->owner);
      save_vault(vault->owner);
    }

    fscanf(index, "%s\n", line);
  }
  fclose(index);
}

void add_vault_access(Character *ch, char *name, struct vault_data *vault)
{
  struct vault_access_data *access;
  class Connection d;

  name[0] = UPPER(name[0]);

  if (!strcasecmp(name, GET_NAME(ch)))
  {
    send_to_char("Don't be a moron, you already have access.\r\n", ch);
    return;
  }

  // must be done to clear out "d" before it is used

  extern Character *get_pc(char *name);

  if (!get_pc(name))
    if (!(load_char_obj(&d, name)))
    {
      send_to_char("You can't give access to someone who doesn't exist.\r\n", ch);
      return;
    }

  if (has_vault_access(name, vault))
  {
    send_to_char("That person already has access to your vault.\r\n", ch);
    if (d.character)
      free_char(d.character, Trace("add_vault_access 1"));
    return;
  }

  csendf(ch, "%s now has access to your vault.\r\n", name);
  CREATE(access, struct vault_access_data, 1);
  access->name = str_dup(name);
  access->next = vault->access;
  vault->access = access;

  save_char_obj(ch);
  if (d.character)
    free_char(d.character, Trace("add_vault_access 2"));
}

void remove_vault_access(Character *ch, char *name, struct vault_data *vault)
{

  name[0] = UPPER(name[0]);

  if (!strcasecmp(name, GET_NAME(ch)))
  {
    send_to_char("Don't be a moron, you can't remove your own access.\r\n", ch);
    return;
  }

  if (!has_vault_access(name, vault))
  {
    send_to_char("That person doesn't have access to your vault.\r\n", ch);
    return;
  }

  csendf(ch, "%s no longer has access to your vault.\r\n", name);
  access_remove(name, vault);
  save_char_obj(ch);
}

void remove_vault_accesses(char *name)
{
  struct vault_data *vault;
  struct vault_access_data *access, *next_access;
  char buf[MAX_INPUT_LENGTH];
  int num = 0;

  for (vault = vault_table; vault; vault = vault->next, num++)
  {
    if (!num)
      continue; // skip 0 cause its null

    for (access = vault->access; access; access = next_access)
    {
      next_access = access->next;
      if (!strcasecmp(access->name, name))
      {
        sprintf(buf, "Removed %s's access to %s's vault.", name, vault->owner);
        vlog(buf, vault->owner);
        access_remove(name, vault);
        save_vault(vault->owner);
      }
    }
  }
}

void access_remove(char *name, struct vault_data *vault)
{
  struct vault_access_data *access, *next_access, *prev_access = nullptr;

  for (access = vault->access; access; access = next_access)
  {
    next_access = access->next;

    if (!strcmp(access->name, name))
    {
      if (access == vault->access)
        vault->access = access->next;
      else if (prev_access != nullptr)
      {
        prev_access->next = access->next;
      }

      free(access);
      access = nullptr;
      break;
    }
    prev_access = access;
  }
}

char *clanVName(int c)
{ // returns clan1, clan2, etc
  static char buf[512];
  sprintf(buf, "Clan%d", c);
  return &buf[0];
}

int has_vault_access(char *who, struct vault_data *vault)
{
  struct vault_access_data *access;
  Character *ch;

  if ((ch = find_owner(who)) && GET_LEVEL(ch) >= 108)
    return 1;

  // its their vault
  if (!strcasecmp(vault->owner, who))
    return 1;

  if (ch && ch->clan && get_clan(ch->clan) && !str_cmp(clanVName(ch->clan), vault->owner) && has_right(ch, CLAN_RIGHTS_VAULT))
    return 1;

  // its not their vault so check all the access lines
  for (access = vault->access; access; access = access->next)
    if (!strcasecmp(access->name, who))
      return 1;

  return 0;
}

struct vault_items_data *get_unique_item_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  class Object *obj;
  int i = 1;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj)))
    {
      if (i == num)
        return items;
      else
        i++;
    }
  }

  return 0;
}

class Object *get_unique_obj_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  class Object *obj;
  int i = 1;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    //    obj = get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj)))
    {
      if (i == num)
        return obj;
      else
        i++;
    }
  }

  return 0;
}

struct vault_items_data *get_item_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  class Object *obj;
  int i = 1, j;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    //    obj = get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj)))
    {
      if (i == num)
        return items;
      else
      {
        for (j = 1; j <= items->count; j++)
          if (isname(object, GET_OBJ_NAME(obj)))
          {
            if (i == num)
              return items;
            else
              i++;
          }
      }
    }
  }

  return 0;
}

class Object *get_obj_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  class Object *obj;
  int i = 1, j;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    //    obj = get_obj(items->item_vnum);
    if (obj && isname(object, GET_OBJ_NAME(obj)))
    {
      if (i == num)
        return obj;
      else
      {
        for (j = 1; j <= items->count; j++)
          if (i == num)
            return obj;
          else
            i++;
      }
    }
  }

  return 0;
}

class Object *exists_in_vault(struct vault_data *vault, Object *obj)
{
  struct vault_items_data *items;
  if (!obj)
    return 0;
  for (items = vault->items; items; items = items->next)
  {
    if (items->obj == obj)
      return obj;
  }

  return 0;
}

void vault_get(Character *ch, char *object, char *owner)
{
  std::string sbuf;
  char obj_list[50][100];
  class Object *obj, *tmp_obj;
  struct vault_items_data *items;
  struct vault_data *vault;
  int self = 0, num = 1, i;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch)))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    csendf(ch, "You don't have permission to take %s's stuff.\r\n", owner);
    return;
  }

  if (sscanf(object, "%d.%s", &num, object) != 2)
    num = 1;

  if (!strcmp(object, "all"))
  {
    bool ioverload = false;
    for (items = vault->items, i = 0; items; items = items->next)
    {
      obj = items->obj ? items->obj : get_obj(items->item_vnum);
      if (obj == 0)
        continue;

      for (int j = 0; j < items->count; j++, i++)
      {
        strncpy(obj_list[i], fname(obj->name), sizeof(obj_list[i]));
        if (i > 49)
        {
          send_to_char("You can only take out 50 items at a time.\r\n", ch);
          ioverload = true;
          break;
        }
        if (IS_CARRYING_N(ch) + i > CAN_CARRY_N(ch))
        {
          ioverload = true;
          break;
        }
      }
      if (ioverload)
        break;
    }
    int amount = i;
    for (i = 0; i < amount; i++)
      vault_get(ch, obj_list[i], owner);
    return;
  }

  if (sscanf(object, "all.%s", object))
  {
    num = 0; // count the number of items that match that keyword so we know how many to get
    for (items = vault->items; items; items = items->next)
    {
      obj = items->obj ? items->obj : get_obj(items->item_vnum);
      //    obj = get_obj(items->item_vnum);
      if (obj == 0)
        continue;

      if (isname(object, GET_OBJ_NAME(obj)))
        num += items->count;
    }

    if (!num)
    {
      send_to_char("There is nothing like that in the vault.\r\n", ch);
      return;
    }

    // start at end of the list and get each item all the way back to the first one.
    for (i = num; i > 0; i--)
    {
      vault_get(ch, object, owner);
    }
    return;
  }
  else
  {

    if (!(obj = get_obj_in_vault(vault, object, num)))
    {
      send_to_char("There is nothing like that in the vault.\r\n", ch);
      return;
    }

    if (IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_char_for_item(ch, obj->item_number, false))
    {
      send_to_char("Why would you want another one of those?\r\n", ch);
      return;
    }

    if (!self && (IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) || IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL)) && GET_LEVEL(ch) < IMMORTAL)
    {
      send_to_char("That item seems to be bound to the vault.\r\n", ch);
      return;
    }

    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
    {
      send_to_char("You cannot hold any more.\r\n", ch);
      if (GET_LEVEL(ch) < IMMORTAL)
        return;
      else
        send_to_char("But since you're an immortal, you get it anyway.\r\n", ch);
    }

    if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
    {
      send_to_char("You cannot carry any more items.\r\n", ch);
      return;
    }

    std::stringstream ssin;
    ssin << GET_OBJ_VNUM(obj);

    if (GET_LEVEL(ch) < IMMORTAL)
    {
      sbuf = GET_NAME(ch);
      sbuf += " removed ";
      sbuf += GET_OBJ_SHORT(obj);
      sbuf += " from ";
      sbuf += owner;
      sbuf += "'s vault.";
    }
    else
    {
      sbuf = GET_NAME(ch);
      sbuf += " removed ";
      sbuf += GET_OBJ_SHORT(obj);
      sbuf += "[";
      sbuf += ssin.str();
      sbuf += "] from ";
      sbuf += owner;
      sbuf += "'s vault.";
    }

    vlog(sbuf.c_str(), owner);
    csendf(ch, "%s has been removed from the vault.\r\n", GET_OBJ_SHORT(obj));

    sbuf = GET_NAME(ch);
    sbuf += " removed ";
    sbuf += GET_OBJ_SHORT(obj);
    sbuf += "[";
    sbuf += ssin.str();
    sbuf += "] from ";
    sbuf += owner;
    sbuf += "'s vault.";

    act(sbuf.c_str(), ch, 0, 0, TO_ROOM, GODS);

    item_remove(obj, vault);

    if (!fullSave(obj))
      tmp_obj = clone_object(real_object(GET_OBJ_VNUM(obj)));
    else
    {
      tmp_obj = clone_object(real_object(GET_OBJ_VNUM(obj)));
      copySaveData(tmp_obj, obj);

      if (verify_item(&tmp_obj))
      {
        copySaveData(tmp_obj, obj);
      }

      // Jared: Removed for the time being because the item is still
      // used elsewhere
      //      if (!exists_in_vault(vault, obj))
      // extract_obj(obj);
    }
    obj_to_char(tmp_obj, ch);
  }

  save_vault(owner);
  save_char_obj(ch);
}

void item_add(int vnum, struct vault_data *vault)
{
  if (vnum == -1)
  {
    produce_coredump();
    return;
  }

  struct vault_items_data *item;

  for (item = vault->items; item; item = item->next)
  {
    if (item->item_vnum == vnum && !item->obj)
    {
      item->count++;
      vault->weight += GET_OBJ_WEIGHT(get_obj(vnum));
      return;
    }
  }

  CREATE(item, struct vault_items_data, 1);
  item->item_vnum = vnum;
  item->count = 1;
  item->next = vault->items;
  vault->items = item;

  vault->weight += GET_OBJ_WEIGHT(get_obj(vnum));
}

void item_add(Object *obj, struct vault_data *vault)
{
  struct vault_items_data *item;
  int vnum = GET_OBJ_VNUM(obj);

  remove_from_object_list(obj);
  for (item = vault->items; item; item = item->next)
  {
    if (item->item_vnum == vnum && item->obj && fullItemMatch(item->obj, obj))
    {
      item->count++;
      vault->weight += GET_OBJ_WEIGHT(obj);
      return;
    }
  }

  CREATE(item, struct vault_items_data, 1);
  item->obj = obj;
  item->item_vnum = vnum;
  item->count = 1;
  item->next = vault->items;
  vault->items = item;

  vault->weight += GET_OBJ_WEIGHT(obj);
}

void item_remove(Object *obj, struct vault_data *vault)
{
  struct vault_items_data *item, *next_item, *prev_item = nullptr;
  int vnum = GET_OBJ_VNUM(obj);

  for (item = vault->items; item; item = next_item)
  {
    next_item = item->next;
    if ((!fullSave(obj) && (!item->obj || !fullSave(item->obj)) && item->item_vnum == vnum) ||
        (item->obj && fullItemMatch(obj, item->obj)))
    {
      if (item->count > 1)
      {
        item->count--;
        vault->weight -= GET_OBJ_WEIGHT(get_obj(vnum));
        return;
      }
      else if (item == vault->items)
        vault->items = item->next;
      else if (prev_item != nullptr)
      {
        prev_item->next = item->next;
      }

      free(item);
      item = nullptr;
      vault->weight -= GET_OBJ_WEIGHT(get_obj(vnum));
      return;
    }
    prev_item = item;
  }
}

int search_vault_by_vnum(int vnum, struct vault_data *vault)
{
  struct vault_items_data *items;

  for (items = vault->items; items; items = items->next)
    if (items && items->item_vnum && items->item_vnum == vnum)
      return 1;

  return 0;
}

void vault_deposit(Character *ch, unsigned int amount, char *owner)
{
  struct vault_data *vault;
  char buf[MAX_INPUT_LENGTH];
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch)))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    csendf(ch, "You don't have permission to put $B$5gold$R in %s's vault.\r\n", owner);
    return;
  }

  if (!amount)
  {
    send_to_char("How much gold would you like to place in the vault?\r\n", ch);
    return;
  }

  if (amount > VAULT_MAX_DEPWITH || amount <= 0)
  {
    csendf(ch, "Valid amounts are from 1 to %d gold.\r\n", VAULT_MAX_DEPWITH);
    return;
  }

  if (ch->getGold() >= amount)
  {
    vault->gold += amount;
    ch->removeGold(amount);
    save_char_obj(ch);
    save_vault(owner);
    sprintf(buf, "%s added %d gold to %s's vault.", GET_NAME(ch), amount, owner);
    vlog(buf, owner);
    csendf(ch, "Done!  The current balance is now %llu $B$5gold$R.\r\n", vault->gold);
  }
  else
  {
    csendf(ch, "But you only have %lld $B$5gold$R coins!\r\n", ch->getGold());
  }
}

void vault_withdraw(Character *ch, unsigned int amount, char *owner)
{
  struct vault_data *vault;
  char buf[MAX_INPUT_LENGTH];
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch)))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    csendf(ch, "You don't have permission to put gold in %s's vault.\r\n", owner);
    return;
  }

  if (!amount)
  {
    send_to_char("How much $B$5gold$R would you like get from the vault?\r\n", ch);
    return;
  }

  if (amount > VAULT_MAX_DEPWITH || amount <= 0)
  {
    csendf(ch, "Valid amounts are from 1 to %d gold.\r\n", VAULT_MAX_DEPWITH);
    return;
  }

  if (vault->gold >= (uint64_t)amount)
  {
    /*    if (amount + ch->getGold() > 2000000000) {
          csendf(ch, "You can't hold that much gold.  The most you could get is %ld.\r\n", 2000000000 - ch->getGold());
          return;
        }*/
    vault->gold -= amount;
    ch->addGold(amount);
    save_char_obj(ch);
    save_vault(owner);
    sprintf(buf, "%s removed %d gold from %s's vault.", GET_NAME(ch), amount, owner);
    vlog(buf, owner);
    csendf(ch, "Done!  The current balance is now %llu gold.\r\n", vault->gold);
  }
  else
  {
    csendf(ch, "The vault only has %llu $B$5gold$R coins in it!\r\n", vault->gold);
  }
}

int can_put_in_vault(class Object *obj, int self, struct vault_data *vault, Character *ch)
{
  //  class Object *tmp_obj;

  if (GET_OBJ_VNUM(obj) == -1)
  {
    csendf(ch, "%s is hardly worth saving.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (GET_OBJ_VNUM(obj) == 393)
  { // no potatoes in the vaults
    csendf(ch, "It would rot. EWW!\r\n");
    return 0;
  }

  if (IS_SET(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_vault_by_vnum(GET_OBJ_VNUM(obj), vault))
  { // Uniques
    csendf(ch, "Why would you need another %s?\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_SPECIAL) && !self)
  { // GL
    csendf(ch, "%s is far too valuable to place in someone else's vault.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (!self && IS_SET(obj->obj_flags.more_flags, ITEM_NO_TRADE) && GET_LEVEL(ch) < IMMORTAL)
  { // no_trade
    csendf(ch, "%s seems bound to you.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP))
  { // cursed
    csendf(ch, "%s is stuck! Ack!.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (IS_SET(obj->obj_flags.extra_flags, ITEM_NOSAVE) || IS_SET(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  { // nosave
    csendf(ch, "%s doesn't seem to be a permanent part of the world.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if (ARE_CONTAINERS(obj) && obj->contains)
  { // non-empty containers
    csendf(ch, "%s needs to be emptied first.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
  { // vault is full
    csendf(ch, "You can't seem to stuff %s in the vault.  Its too big!\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }
  if (obj->obj_flags.timer > 0)
  {
    csendf(ch, "%s cannot be placed in the vault right now.\r\n", GET_OBJ_SHORT(obj));
    return 0;
  }

  return 1;
}

void vault_put(Character *ch, char *object, char *owner)
{
  class Object *obj, *tmp_obj;
  struct vault_data *vault;
  char buf[MAX_INPUT_LENGTH];
  int self = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch)))
  {
    self = 1;
  }

  if (!(vault = has_vault(owner)))
  {
    if (self)
      csendf(ch, "You don't have a vault.\r\n");
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    csendf(ch, "You don't have permission to put things in %s's vault.\r\n", owner);
    return;
  }

  if (!strcmp(object, "all"))
  {
    for (obj = ch->carrying; obj; obj = tmp_obj)
    {
      tmp_obj = obj->next_content;
      if (!can_put_in_vault(obj, self, vault, ch))
        continue;

      if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
      {
        csendf(ch, "%s won't fit in the vault!\r\n", GET_OBJ_SHORT(obj));
        continue;
      }

      if (GET_LEVEL(ch) < IMMORTAL)
        snprintf(buf, MAX_INPUT_LENGTH, "%s added %s to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), owner);
      else
        snprintf(buf, MAX_INPUT_LENGTH, "%s added %s[%d] to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), owner);

      vlog(buf, owner);
      csendf(ch, "%s has been placed in the vault.\r\n", GET_OBJ_SHORT(obj));

      std::string sbuf;
      std::stringstream ssin;
      ssin << GET_OBJ_VNUM(obj);

      sbuf = GET_NAME(ch);
      sbuf += " added ";
      sbuf += GET_OBJ_SHORT(obj);
      sbuf += "[";
      sbuf += ssin.str();
      sbuf += "] to ";
      sbuf += owner;
      sbuf += "'s vault.";

      act(sbuf.c_str(), ch, 0, 0, TO_ROOM, GODS);

      if (!fullSave(obj) && GET_OBJ_VNUM(obj) > 0)
      {
        item_add(GET_OBJ_VNUM(obj), vault);
        extract_obj(obj);
      }
      else
      {
        obj_from_char(obj);
        item_add(obj, vault);
      }
    }
  }
  else if (sscanf(object, "all.%s", object))
  {
    for (obj = ch->carrying; obj; obj = tmp_obj)
    {
      tmp_obj = obj->next_content;
      if (!isname(object, GET_OBJ_NAME(obj)))
        continue;
      if (!can_put_in_vault(obj, self, vault, ch))
        continue;

      if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
      {
        csendf(ch, "%s won't fit in the vault!\r\n", GET_OBJ_SHORT(obj));
        continue;
      }

      if (GET_LEVEL(ch) < IMMORTAL)
        snprintf(buf, MAX_INPUT_LENGTH, "%s added %s to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), owner);
      else
        snprintf(buf, MAX_INPUT_LENGTH, "%s added %s[%d] to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), owner);

      vlog(buf, owner);
      csendf(ch, "%s has been placed in the vault.\r\n", GET_OBJ_SHORT(obj));
      if (!fullSave(obj) && GET_OBJ_VNUM(obj) > 0)
      {
        item_add(GET_OBJ_VNUM(obj), vault);
        extract_obj(obj);
      }
      else
      {
        obj_from_char(obj);
        item_add(obj, vault);
      }
    }
  }
  else
  {
    if (!(obj = get_obj_in_list_vis(ch, object, ch->carrying)))
    {
      send_to_char("You don't have anything like that.\r\n", ch);
      return;
    }

    if (!can_put_in_vault(obj, self, vault, ch))
      return;

    if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
    {
      csendf(ch, "%s won't fit in the vault!\r\n", GET_OBJ_SHORT(obj));
      return;
    }

    if (GET_LEVEL(ch) < IMMORTAL)
      snprintf(buf, MAX_INPUT_LENGTH, "%s added %s to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), owner);
    else
      snprintf(buf, MAX_INPUT_LENGTH, "%s added %s[%d] to %s's vault.", GET_NAME(ch), GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), owner);

    vlog(buf, owner);
    csendf(ch, "%s has been placed in the vault.\r\n", GET_OBJ_SHORT(obj));

    if (!fullSave(obj) && GET_OBJ_VNUM(obj) > 0)
    {
      item_add(GET_OBJ_VNUM(obj), vault);
      extract_obj(obj);
    }
    else
    {
      obj_from_char(obj);
      item_add(obj, vault);
    }
  }
  save_vault(owner);
  save_char_obj(ch);
}

void sort_vault(const vault_data &vault, sorted_vault &sv)
{
  class Object *obj;

  for (vault_items_data *items = vault.items; items; items = items->next)
  {
    obj = items->obj;
    if (obj == nullptr)
    {
      obj = get_obj(items->item_vnum);
    }

    if (GET_OBJ_SHORT(obj) != nullptr)
    {
      auto &o = sv.vault_content_qty[GET_OBJ_SHORT(obj)];
      o.first = obj;
      if (o.second == 0)
      {
        // sv.vault_contents.push_back(GET_OBJ_SHORT(obj));
        sv.vault_contents.insert(sv.vault_contents.begin(), GET_OBJ_SHORT(obj));
      }
      o.second += items->count;
      sv.weight += (obj->obj_flags.weight * items->count);
    }
  }
}

void vault_list(Character *ch, char *owner)
{
  struct vault_items_data *items;
  struct vault_data *vault;
  class Object *obj;
  int objects = 0, self = 0;
  char sectionbuf[MAX_STRING_LENGTH * 4];
  char linebuf[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  int diff_len = 0;
  int sectionbuf_len = 0;
  int linebuf_len = 0;

  owner[0] = UPPER(owner[0]);
  if (!strcmp(owner, GET_NAME(ch)))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      csendf(ch, "You don't have a vault.\r\n", ch);
    else
      csendf(ch, "%s doesn't have a vault.\r\n", owner);
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    csendf(ch, "You don't have access to %s's vault.\r\n", owner);
    return;
  }

  sorted_vault sv;
  sort_vault(*vault, sv);

  if (sv.weight != vault->weight)
  {
    if (self)
    {
      ch->send(fmt::format("Some objects in your vault have changed weight.\r\nYour vault's weight has been recalculated from {} to {}.\r\n", vault->weight, sv.weight));
    }
    vault->weight = sv.weight;
  }

  if (sv.vault_contents.empty())
  {
    if (self)
    {
      csendf(ch, "Your vault is currently empty and can hold %d pounds.\r\n", vault->size);
    }
    else
    {
      csendf(ch, "%s's vault is currently empty.\r\n", owner);
    }
    return;
  }

  if (self)
  {
    ch->send(fmt::format("Your vault is at {} of {} maximum pounds and contains:\r\n", vault->weight, vault->size));
  }
  else
  {
    ch->send(fmt::format("{}'s vault is at {} of {} maximum pounds and contains:\r\n", owner, vault->weight, vault->size));
  }

  // We are showing the last item in vault first because items were inserted at the
  // front of the sv.vault_contents container
  for (const auto &o_short_description : sv.vault_contents)
  {
    const auto &o = sv.vault_content_qty[o_short_description];
    const auto &obj = o.first;
    const auto &count = o.second;

    if (count > 1)
    {
      ch->send(fmt::format("[$5{}$R] ", count));
    }

    ch->send(fmt::format("{}$R", GET_OBJ_SHORT(obj)));

    if (obj->obj_flags.type_flag == ITEM_ARMOR ||
        obj->obj_flags.type_flag == ITEM_WEAPON ||
        obj->obj_flags.type_flag == ITEM_FIREWEAPON ||
        obj->obj_flags.type_flag == ITEM_CONTAINER ||
        obj->obj_flags.type_flag == ITEM_INSTRUMENT ||
        obj->obj_flags.type_flag == ITEM_STAFF ||
        obj->obj_flags.type_flag == ITEM_WAND ||
        obj->obj_flags.type_flag == ITEM_LIGHT)
    {
      ch->send(fmt::format("{} $3Lvl: {}$R", item_condition(obj), obj->obj_flags.eq_level));
    }

    if (GET_LEVEL(ch) > IMMORTAL && obj->item_number > 0)
    {
      ch->send(fmt::format(" [{}]", obj_index[obj->item_number].virt));
    }
    ch->send("\r\n");
  }
}

void add_new_vault(char *name, int indexonly)
{
  FILE *vfl, *tvfl, *pvfl;
  struct vault_data *vault;
  char fname[256], line[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];

  if (!(vfl = fopen(VAULT_INDEX_FILE, "r")))
  {
    logentry("add_new_vault: error opening index file.", IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (!(tvfl = fopen(VAULT_INDEX_FILE_TMP, "w")))
  {
    logentry("add_new_vault: error opening temp index file.", IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  // read and print each line until we get to $
  fscanf(vfl, "%s\n", line);
  while (*line != '$')
  {
    fprintf(tvfl, "%s\n", line);
    fscanf(vfl, "%s\n", line);
  }
  // we found $, now add in the new name, then the $
  fprintf(tvfl, "%s\n", name);
  fprintf(tvfl, "$\n");
  fclose(tvfl);
  fclose(vfl);
  rename(VAULT_INDEX_FILE_TMP, VAULT_INDEX_FILE);

  if (indexonly)
    return;

  // now create a new vault for the player

  Character *ch = find_owner(name);

  sprintf(fname, "../vaults/%c/%s.vault", UPPER(*name), name);
  if (!(pvfl = fopen(fname, "w")))
  {
    sprintf(buf, "add_new_vault: error opening new vault file [%s].", fname);
    logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (ch)
    fprintf(pvfl, "S %d\n", VAULT_BASE_SIZE * GET_LEVEL(ch));
  else
    fprintf(pvfl, "S %d\n", VAULT_BASE_SIZE);
  fprintf(pvfl, "$\n");
  fclose(pvfl);

  sprintf(buf, "%s bought a vault.", name);
  vlog(buf, name);

  // files all done, now add it in game
  total_vaults++;
  RECREATE(vault_table, struct vault_data, total_vaults);
  CREATE(vault, struct vault_data, 1);

  vault->owner = str_dup(name);
  if (ch)
    vault->size = VAULT_BASE_SIZE * GET_LEVEL(ch);
  else
    vault->size = VAULT_BASE_SIZE;
  vault->weight = 0;
  vault->access = nullptr;
  vault->items = nullptr;
  vault->next = vault_table;
  vault_table = vault;

  save_vault(name);
}

Character *find_owner(const char *name)
{
  const auto &character_list = DC::getInstance()->character_list;
  const auto &result = find_if(character_list.begin(), character_list.end(), [&name](const auto &ch)
                               {
	  if (ch->name == nullptr)
    {
		  produce_coredump(); //Trying to track down bug that causes mob->name to be nullptr
	  }
    else if (!strcmp(name, GET_NAME(ch)) && IS_PC(ch))
    {
			return true;
	  }
		return false; });

  if (result != end(character_list))
  {
    return *result;
  }

  return nullptr;
}

void vault_log(Character *ch, char *owner)
{
  char buf[MAX_STRING_LENGTH];
  char fname[256];

  if (!strcmp(owner, clanVName(ch->clan)))
  {
    strncpy(buf, "The following are your clan's most recent vault log entries (Times are UTC):\n\r", MAX_STRING_LENGTH);
  }
  else
  {
    strncpy(buf, "The following are your most recent vault log entries (Times are UTC):\r\n", MAX_STRING_LENGTH);
  }

  owner[0] = UPPER(owner[0]);

  snprintf(fname, 256, "../vaults/%c/%s.vault.log", *owner, owner);

  ifstream fin(fname);
  stringstream buffer;
  buffer << buf;
  buffer << fin.rdbuf();

  page_string(ch->desc, const_cast<char *>(buffer.str().c_str()), 1);
}

void vlog(const char *message, const char *name)
{
  struct tm *tm = nullptr;
  time_t ct;
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

  logentry(message, IMMORTAL, LogChannels::LOG_VAULT);

  sprintf(fname, "../vaults/%c/%s.vault.log", *name, name);
  sprintf(nfname, "../vaults/%c/%s.vault.log.tmp", *name, name);

  if (!(ofile = fopen(fname, "r")))
  {
    if (!(ofile = fopen(fname, "w")))
    {
      sprintf(buf, "vault_log: could not open vault log file [%s].", fname);
      logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
      return;
    }
    fprintf(ofile, "$\n");
    fclose(ofile);
    if (!(ofile = fopen(fname, "r")))
    {
      sprintf(buf, "vault_log: could not open vault log file [%s].", fname);
      logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
      return;
    }
  }

  if (!(nfile = fopen(nfname, "w")))
  {
    sprintf(buf, "vault_log: could not open vault log file [%s].", nfname);
    logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  ct = time(0);
  tm = localtime(&ct);

  if (tm->tm_min < 10)
    sprintf(mins, "0%d", tm->tm_min);
  else
    sprintf(mins, "%d", tm->tm_min);

  if (tm->tm_hour < 10)
    sprintf(hours, "0%d", tm->tm_hour);
  else
    sprintf(hours, "%d", tm->tm_hour);

  sprintf(buf, "%s %d %s:%s", months[tm->tm_mon], tm->tm_mday, hours, mins);
  fprintf(nfile, "%s :: %s\n", buf, message);

  get_line(ofile, line);
  while (*line != '$' && lines++ < 500)
  {
    fprintf(nfile, "%s\n", line);
    get_line(ofile, line);
  }
  fprintf(nfile, "$\n");

  fclose(nfile);
  fclose(ofile);
  unlink(fname);
  rename(nfname, fname);
  // sprintf(cmd, "mv -f %s %s", nfname, fname);
  // system(cmd);
}

int sleazy_vault_guy(Character *ch, class Object *obj, int cmd, const char *arg,
                     Character *owner)
{
  if (cmd != 59 && cmd != 56)
    return eFAILURE;
  if (IS_NPC(ch))
    return eFAILURE;
  char arg1[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  arg = one_argument(arg, arg1);

  struct vault_data *vault = has_vault(GET_NAME(ch));
  struct vault_data *cvault = ch->clan ? has_vault(clanVName(ch->clan)) : 0;
  if (cmd == 59)
  {
    if (!vault)
    {
      send_to_char("You need to level up some before obtaining a vault.\r\n", ch);
      return eSUCCESS;
    }
    else if (vault->size < VAULT_MAX_SIZE)
      sprintf(buf, "$B1)$R Increase the size of vault by 10 lbs: %d platinum.\r\n", VAULT_UPGRADE_COST);
    else
      sprintf(buf, "1) You cannot increase your vault-size further.\r\n");
    send_to_char("$B$2Paul the sleazy vault salesman tells you, 'How aboot a bigger vault? Size matters, you know'$R\r\n", ch);

    send_to_char(buf, ch);

    sprintf(buf, "$B2)$R Purchase a clan vault: %s\r\n",
            ch->clan ? cvault ? "Your clan already has a vault" : has_right(ch, CLAN_RIGHTS_VAULT) ? "1000 platinum coins."
                                                                                                   : "You are not authorized to make this purchase."
                     : "You are not a member of any clan.");
    send_to_char(buf, ch);

    if (!cvault)
      sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", ch->clan ? "Your clan has no vault." : "You're not in a clan.");
    else if (cvault->size < VAULT_MAX_SIZE)
      sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", has_right(ch, CLAN_RIGHTS_VAULT) ? "200 platinum coins." : "You are not authorized to make this purchase.");
    else
      sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: You cannot increase the vault's size further.\r\n");
    send_to_char(buf, ch);
    return eSUCCESS;
  }
  else
  {
    int i = atoi(arg1);
    switch (i)
    {
    case 1:
      if (!vault)
      {
        send_to_char("You need to level up some before obtaining a vault.\r\n", ch);
        return eSUCCESS;
      }
      if (vault->size >= VAULT_MAX_SIZE)
      {
        send_to_char("Your vault's size is already at its maximum capacity.\r\n", ch);
        return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < VAULT_UPGRADE_COST)
      {
        send_to_char("You do not have enough platinum.\r\n", ch);
        return eSUCCESS;
      }
      GET_PLATINUM(ch) -= VAULT_UPGRADE_COST;
      vault->size += 10;
      save_char_obj(ch);
      save_vault(vault->owner);
      send_to_char("$B$2Paul the sleazy vault salesman tells you, '10 lbs added to your vault.$R'\r\n", ch);
      return eSUCCESS;
    case 2:
      if (cvault)
      {
        send_to_char("Your clan already has a vault.\r\n", ch);
        return eSUCCESS;
      }
      if (!ch->clan)
      {
        send_to_char("You're not a member of any clan.\r\n", ch);
        return eSUCCESS;
      }
      if (!has_right(ch, CLAN_RIGHTS_VAULT))
      {
        send_to_char("You are not authorized to make that purchase.\r\n", ch);
        return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 1000)
      {
        send_to_char("You do not have enough platinum.\r\n", ch);
        return eSUCCESS;
      }
      GET_PLATINUM(ch) -= 1000;
      add_new_vault(clanVName(ch->clan), 0);
      save_char_obj(ch);
      cvault = has_vault(clanVName(ch->clan));
      cvault->size = 500;
      save_vault(clanVName(ch->clan));
      send_to_char("You have purchased a vault for your clan's perusal.\r\n", ch);
      return eSUCCESS;
    case 3:
      if (!cvault)
      {
        send_to_char("Your clan does not have a vault.\r\n", ch);
        return eSUCCESS;
      }
      if (!has_right(ch, CLAN_RIGHTS_VAULT))
      {
        send_to_char("You are not authorized to make that purchase.\r\n", ch);
        return eSUCCESS;
      }
      if (cvault->size >= VAULT_MAX_SIZE)
      {
        send_to_char("The vault is already at its maximum capacity.\r\n", ch);
        return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 200)
      {
        send_to_char("You do not have enough platinum.\r\n", ch);
        return eSUCCESS;
      }
      GET_PLATINUM(ch) -= 200;
      cvault->size += 10;
      save_char_obj(ch);
      save_vault(clanVName(ch->clan));
      send_to_char("You have added 10 lbs capacity to your clan's vault.\r\n", ch);
      return eSUCCESS;
    }
  }
  return eFAILURE;
}

void vault_search_usage(Character *ch)
{
  send_to_char("Usage: vault search [ keyword <keyword> ] | [ level <levels> ] | ...\r\n", ch);
  send_to_char("keyword <keyword>  -  Single word keyword. Can be used multiple times.\r\n", ch);
  send_to_char("level <levels>     -  Single object level or range of levels.\r\n\r\n", ch);
  send_to_char("Examples:\r\n", ch);
  send_to_char("vault search keyword staff\r\n", ch);
  send_to_char("vault search level 55\r\n", ch);
  send_to_char("vault search keyword staff level 55.\r\n", ch);
  send_to_char("vault search keyword staff level 40-60.\r\n\r\n", ch);
}

int vault_search(Character *ch, const char *args)
{
  int objects = 0;
  bool owner_shown = false;
  int vaults_searched = 0;
  int objects_found = 0;
  string arg1{};
  string arguments{args};
  list<vault_search_parameter>::iterator p;
  bool nomatch;

  tie(arg1, arguments) = half_chop(arguments);
  if (arg1.empty())
  {
    vault_search_usage(ch);
    return eFAILURE;
  }

  list<vault_search_parameter> search;
  vault_search_parameter parameter;

  // parse our arguments and setup a list of the things we want to search for
  do
  {
    if (arg1 == "keyword")
    {
      tie(arg1, arguments) = half_chop(arguments);
      if (arg1.empty())
      {
        send_to_char("Missing keyword parameter.\r\n\r\n", ch);
        vault_search_usage(ch);
        return eFAILURE;
      }
      else
      {
        parameter.type = KEYWORD;
        parameter.str_argument = str_dup(arg1.c_str());
        search.push_back(parameter);
        // We need
        parameter.type = UNDEFINED;
      }
    }
    else if (arg1 == "level")
    {
      tie(arg1, arguments) = half_chop(arguments);
      if (arg1.empty())
      {
        send_to_char("Missing level parameter.\r\n\r\n", ch);
        vault_search_usage(ch);
        return eFAILURE;
      }
      else
      {
        // start parsing level and level ranges
        size_t pos;
        string level_string(arg1);

        // if a '-' is not found then assume a single number is specified
        if ((pos = level_string.find('-')) == string::npos)
        {
          parameter.int_argument = atoi(arg1.c_str());
          // Check if a non numeric value is passed
          if (parameter.int_argument == 0 && level_string != "0")
          {
            send_to_char("Invalid level specified.\r\n\r\n", ch);
            vault_search_usage(ch);
            return eFAILURE;
          }

          parameter.type = LEVEL;
          search.push_back(parameter);
        }
        else
        {
          // Get the first half of a level range
          string min_level_string = level_string.substr(0, pos);
          parameter.int_argument = atoi(min_level_string.c_str());
          // Check if a non numeric value is passed
          if (parameter.int_argument == 0 && min_level_string != "0")
          {
            send_to_char("Invalid minimum level specified.\r\n\r\n", ch);
            vault_search_usage(ch);
            return eFAILURE;
          }

          parameter.type = MIN_LEVEL;
          search.push_back(parameter);

          // Get the second half of a level range
          string max_level_string = level_string.substr(pos + 1);
          parameter.int_argument = atoi(max_level_string.c_str());
          // Check if a non numeric value is passed
          if (parameter.int_argument == 0 && max_level_string != "0")
          {
            send_to_char("Invalid maximum level specified.\r\n\r\n", ch);
            vault_search_usage(ch);
            return eFAILURE;
          }

          parameter.type = MAX_LEVEL;
          search.push_back(parameter);
        } // end of level range parsing
      }   // end of level parsing
    }
    else
    {
      send_to_char("Invalid argument.\r\n\r\n", ch);
      vault_search_usage(ch);
      return eFAILURE;
    }
    tie(arg1, arguments) = half_chop(arguments);
  } while (!arg1.empty());

  // now that we know what we're looking for, let's search through all the vaults to find it
  for (vault_data *vault = vault_table; vault; vault = vault->next)
  {
    if (vault && vault->owner && has_vault_access(GET_NAME(ch), vault))
    {
      vaults_searched++;
      objects = 0;
      owner_shown = false;

      sorted_vault sv;
      sort_vault(*vault, sv);

      for (const auto &o_short_description : sv.vault_contents)
      {
        const auto &o = sv.vault_content_qty[o_short_description];
        const auto &obj = o.first;

        nomatch = false;
        // look through each search parameter to see if any of them don't match the current object
        for (p = search.begin(); p != search.end(); ++p)
        {
          switch ((*p).type)
          {
          case UNDEFINED:
            break;
          case KEYWORD:
            if (!isname((*p).str_argument, GET_OBJ_NAME(obj)))
            {
              nomatch = true;
            }
            break;
          case LEVEL:
            if (obj->obj_flags.eq_level != (*p).int_argument)
            {
              nomatch = true;
            }
            break;
          case MIN_LEVEL:
            if (obj->obj_flags.eq_level < (*p).int_argument)
            {
              nomatch = true;
            }
            break;
          case MAX_LEVEL:
            if (obj->obj_flags.eq_level > (*p).int_argument)
            {
              nomatch = true;
            }
            break;
          }

          // if something didn't match break out of the parameter search so we can move on to another object
          if (nomatch)
          {
            break;
          }
        } // end of parameter for loop

        // something did't match so move on to another object
        if (nomatch)
        {
          continue;
        }

        if (!owner_shown)
        {
          owner_shown = true;
          csendf(ch, "\n\r%s:\n\r", vault->owner);
        }

        const auto &count = o.second;
        objects_found += count;
        if (count > 1)
        {
          ch->send(fmt::format("[$5{}$R] ", count));
        }

        ch->send(fmt::format("{}$R", GET_OBJ_SHORT(obj)));

        if (obj->obj_flags.type_flag == ITEM_ARMOR ||
            obj->obj_flags.type_flag == ITEM_WEAPON ||
            obj->obj_flags.type_flag == ITEM_FIREWEAPON ||
            obj->obj_flags.type_flag == ITEM_CONTAINER ||
            obj->obj_flags.type_flag == ITEM_INSTRUMENT ||
            obj->obj_flags.type_flag == ITEM_STAFF ||
            obj->obj_flags.type_flag == ITEM_WAND ||
            obj->obj_flags.type_flag == ITEM_LIGHT)
        {
          ch->send(fmt::format("{} $3Lvl: {}$R", item_condition(obj), obj->obj_flags.eq_level));
        }

        if (GET_LEVEL(ch) > IMMORTAL && obj->item_number > 0)
        {
          ch->send(fmt::format(" [{}]", obj_index[obj->item_number].virt));
        }
        ch->send("\r\n");
      } // for loop of objects
    }   // if we have access to vault
  }     // for loop of vaults

  csendf(ch, "\n\rSearched %d vaults and found %d objects.\r\n",
         vaults_searched, objects_found);

  return eSUCCESS;
}
