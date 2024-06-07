
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list>
#include <fmt/format.h>
#include <tracy/Tracy.hpp>

#include "structs.h"
#include "DC/db.h"
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

class vault_search_parameter
{
public:
  vault_search_parameter();
  ~vault_search_parameter();
  vault_search_type type;

  QString str_argument;
  int int_argument;
};

vault_search_parameter::vault_search_parameter()
    : type(vault_search_type::UNDEFINED)
{
}

vault_search_parameter::~vault_search_parameter()
{
}

int total_vaults = 0;
int get_line(FILE *fl, char *buf);

void item_remove(Object *obj, struct vault_data *vault);
void item_add(int vnum, struct vault_data *vault);

Character *find_owner(QString name);
void vault_log(Character *ch, char *owner);
QString clanVName(uint64_t clan_id);
void vault_search_usage(Character *ch);

extern class Object *object_list;

struct vault_data *has_vault(QString name)
{
  struct vault_data *vault;

  for (vault = vault_table; vault; vault = vault->next)
    if (vault && !name.compare(vault->owner, Qt::CaseInsensitive))
      return vault;
  Character *ch = find_owner(name);
  if (ch && ch->getLevel() >= 10)
  {
    add_new_vault(GET_NAME(ch), 0);
    for (vault = vault_table; vault; vault = vault->next)
      if (vault && !name.compare(vault->owner, Qt::CaseInsensitive))
      {
        //	 ch->sendln("A vault was created for you.");
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

void save_vault(QString name)
{
  struct vault_data *vault;
  struct vault_access_data *access;
  struct vault_items_data *items;

  if (name.isEmpty())
  {
    return;
  }
  name = name.toLower();
  name[0] = name[0].toUpper();

  if (!(vault = has_vault(name)))
    return;

  Character *ch = find_owner(name);
  if (ch)
    if (vault->size < ch->getLevel() * 10)
      vault->size = ch->getLevel() * 10;

  QString fname = QStringLiteral("../vaults/%1/%2.vault").arg(name[0]).arg(name);

  QFile file(fname);
  if (!file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text))
  {
    logentry(QStringLiteral("save_vault: could not open vault file for [%1].").arg(fname), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  QTextStream out(&file);
  out << QStringLiteral("S %1\n").arg(vault->size);
  out << QStringLiteral("G %1\n").arg(vault->gold);

  for (items = vault->items; items; items = items->next)
  {
    Object *obj = items->obj ? items->obj : get_obj(items->item_vnum);
    if (obj == 0)
      continue;

    out << QStringLiteral("O %1 %2 %3\n").arg(items->item_vnum).arg(items->count).arg(items->obj ? 1 : 0);
    if (items->obj)
      write_object(items->obj, out);
  }

  for (access = vault->access; access; access = access->next)
    out << QStringLiteral("A %1\n").arg(access->name);

  out << "$\n";
}

void vault_access(Character *ch, const char *who)
{
  struct vault_access_data *access;
  struct vault_data *vault = nullptr;

  if (!vault && !(vault = has_vault(GET_NAME(ch))))
  {
    ch->sendln("You don't seem to have a vault.");
    return;
  }

  if (!*who)
  {
    ch->send("The following people have access to your vault:\r\n");
    for (access = vault->access; access; access = access->next)
      ch->send(QStringLiteral("%1\r\n").arg(access->name));
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
      ch->sendln("No such player.");
      return;
    }
    if (!has_vault_access(GET_NAME(ch), vault))
    {
      ch->sendln("You do not have access to that vault anyway.");
      return;
    }
    access_remove(GET_NAME(ch), vault);
    ch->sendln("You remove your access to that vault.");
    return;
  }

  ch->sendln("You have access to the following vaults:");
  for (vault = vault_table; vault; vault = vault->next)
    if (vault && has_vault_access(GET_NAME(ch), vault))
      ch->send(QStringLiteral("%1\r\n").arg(vault->owner));
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
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(QStringLiteral("%1 doesn't have a vault.\r\n").arg(owner));
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    ch->send(QStringLiteral("You don't have permission to see %1's balance.\r\n").arg(owner));
    return;
  }

  if (self)
    ch->send(QStringLiteral("You have %1 $B$5gold$R coins in your vault.\r\n").arg(vault->gold));
  else
    ch->send(QStringLiteral("There are %1 $B$5gold$R coins in %2's vault.\r\n").arg(vault->gold).arg(owner));
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

  if (ch->isPlayerObjectThief() || (ch->isPlayerGoldThief()))
  {
    ch->sendln("You're too busy running from the law!");
    return eFAILURE;
  }

  if (!*arg)
  {
    ch->send(vault_usage);
    if (ch->getLevel() > IMMORTAL)
      ch->send(imm_vault_usage);
    return eSUCCESS;
  }

  if (!str_cmp(arg1, "clan") && ch->clan)
    strcpy(arg1, clanVName(ch->clan).toStdString().c_str());

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
  else if (ch->getLevel() > IMMORTAL && !strncmp(arg, "stats", strlen(arg)))
  {
    vault_stats(ch, arg1);

    // show vault log
  }
  else if (!strncmp(arg, "log", strlen(arg)))
  {
    if (*arg1)
    {
      if (!strcasecmp(arg1, clanVName(ch->clan).toStdString().c_str()))
      {
        clan_data *clan = get_clan(ch);
        if (clan == nullptr)
        {
          ch->sendln("You are not a member of any clan.");
          return eFAILURE;
        }

        // Clan leader or a clan member with the vaultlog right can view log.
        if ((clan->leader && !strcmp(clan->leader, GET_NAME(ch))) || has_right(ch, CLAN_RIGHTS_VAULTLOG))
        {
          std::stringstream clanName;

          clanName << "clan" << clan->number;
          sprintf(arg1, "%s", clanName.str().c_str());

          vault_log(ch, arg1);
        }
        else
        {
          ch->sendln("You don't have access to view the clan's vault log.");
          return eFAILURE;
        }
      }
      else if (ch->getLevel() >= IMMORTAL)
      {
        vault_log(ch, arg1);
      }
      else
      {
        ch->sendln("Syntax: vault log <clan>");
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
  else if (!isSet(DC::getInstance()->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You don't feel safe enough to manage your valuables.");
    return eSUCCESS;
  }
  else if (!strncmp(arg, "withdraw", strlen(arg)))
  {
    half_chop(arg1, argument, arg2);

    if (arg2)
    {
      if (!str_cmp(arg2, "clan") && ch->clan)
      {
        strcpy(arg2, clanVName(ch->clan).toStdString().c_str());
      }
      else if (!*arg2)
      {
        sprintf(arg2, "%s", GET_NAME(ch));
      }
    }

    bool ok = false;
    auto amount = QString(argument).toUInt(&ok);
    if (!ok)
    {
      amount = 0;
    }
    vault_withdraw(ch, amount, arg2);
  }
  else if (!strncmp(arg, "deposit", strlen(arg)))
  {
    half_chop(arg1, argument, arg2);

    if (arg2)
    {
      if (!str_cmp(arg2, "clan") && ch->clan)
      {
        strcpy(arg2, clanVName(ch->clan).toStdString().c_str());
      }
      else if (!*arg2)
      {
        sprintf(arg2, "%s", GET_NAME(ch));
      }
    }

    bool ok = false;
    auto amount = QString(argument).toUInt(&ok);
    if (!ok)
    {
      amount = 0;
    }
    vault_deposit(ch, amount, arg2);
  }
  else if (!strncmp(arg, "put", strlen(arg)))
  {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      strcpy(arg2, clanVName(ch->clan).toStdString().c_str());
    if (!*argument)
    {
      ch->sendln("What item would you like to place in the vault?");
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
      strcpy(arg2, clanVName(ch->clan).toStdString().c_str());

    if (!*argument)
    {
      ch->sendln("What item would you like to get from the vault?");
      return eSUCCESS;
    }
    else if (!*arg2)
      sprintf(arg2, "%s", GET_NAME(ch));
    vault_get(ch, argument, arg2);
  }
  else
  {
    ch->send(vault_usage);
    if (ch->getLevel() > IMMORTAL)
      ch->send(imm_vault_usage);
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

    if (name && *name && !vault->owner.isEmpty() && !vault->owner.startsWith(name))
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

    snprintf(buf1, sizeof(buf1), "%3d) %-15s $B$5%10lu$R     %5d (%4d  ) %11d/%12d/%16d %6d %s\r\n",
             count, vault->owner.toStdString().c_str(), vault->gold, items, unique, weight, vault->weight, vault->size, accesses, weight != vault->weight ? "$5mismatch$R" : "$1    none$R");
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
  ch->send(QStringLiteral("Total Vaults: %1\r\n").arg(count));
  ch->send(QStringLiteral("Not Showing: %1\r\n").arg(skipped));
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

void rename_vault_owner(QString oldname, QString newname)
{
  int num = 0;

  vault_data *vault = has_vault(newname);
  if (vault)
  {
    remove_vault(newname); // free it up first..
  }

  if ((vault = has_vault(oldname)))
  {
    vault->owner = newname;
    save_vault(newname);

    vlog(QStringLiteral("Vault owner changed from '%1' to '%2'.").arg(oldname).arg(newname), newname);
  }

  if (!vault)
    return;

  for (vault_items_data *items = vault->items; items; items = items->next)
  {
    if (items->obj && isSet(items->obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      QStringList tmp = QString(items->obj->name).trimmed().split(' ');
      if (tmp.length() >= 2)
      {
        tmp.pop_back();
      }
      tmp.push_back(newname);

      items->obj->name = str_hsh(tmp.join(' ').toStdString().c_str());
    }
  }
  save_vault(newname);

  for (vault = vault_table; vault; vault = vault->next, num++)
  {
    if (!num)
      continue; // skip 0 cause its null

    for (vault_access_data *access = vault->access; access; access = access->next)
    {
      if (oldname == access->name)
      {
        vlog(QStringLiteral("Replaced '%1' with '%2' in %3's vault access list.").arg(access->name).arg(newname).arg(vault->owner), vault->owner);
        access->name = newname;
        save_vault(vault->owner);
      }
    }
  }

  add_new_vault(newname.toStdString().c_str(), 1);
  // reload_vaults();
  remove_vault(oldname);
}

void remove_vault(QString name, BACKUP_TYPE backup)
{
  char src_filename[256];
  char dst_dir[256] = {0};
  char syscmd[512];
  struct stat statbuf;

  if (name.isEmpty())
  {
    return;
  }

  name = name.toLower();
  name[0] = name[0].toUpper();

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
         name.toStdString().c_str());
    break;
  }

  snprintf(src_filename, 256, "%s/%c/%s.vault", VAULT_DIR, name.at(0), name.toStdString().c_str());

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

  snprintf(src_filename, 256, "%s/%c/%s.vault.backup", VAULT_DIR, name[0], name.toStdString().c_str());

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

  snprintf(src_filename, 256, "%s/%c/%s.vault.log", VAULT_DIR, name[0], name.toStdString().c_str());

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

  remove_vault_accesses(name.toStdString().c_str());

  struct vault_data *vault, *next_vault, *prev_vault = nullptr;
  struct vault_items_data *items, *titems;
  struct vault_access_data *access, *taccess;

  char buf[MAX_INPUT_LENGTH];
  char h[MAX_INPUT_LENGTH];

  snprintf(h, sizeof(h), "cat %s| grep -iv '^%s$' > %s", VAULT_INDEX_FILE, name.toStdString().c_str(), VAULT_INDEX_FILE_TMP);
  system(h);
  unlink(VAULT_INDEX_FILE);
  rename(VAULT_INDEX_FILE_TMP, VAULT_INDEX_FILE);
  snprintf(buf, sizeof(buf), "Deleting %s's vault.", name.toStdString().c_str());
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
    if (!vault || vault->owner.isEmpty())
      continue;

    if (vault->owner == name)
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

void testing_load_vaults(void)
{
  struct vault_data *vault;
  struct vault_access_data *access;
  struct vault_items_data *items;
  class Object *obj = nullptr;
  struct stat statbuf = {};
  int vnum = 0, full = 0, count = 0;
  uint64_t gold = 0;
  bool saveChanges = false;
  QString buffer;

  QFile vault_index_file(VAULT_INDEX_FILE);
  if (!vault_index_file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
  {
    logentry(QStringLiteral("boot_vaults: could not open vault index file, probably doesn't exist."));
    return;
  }
  QTextStream vault_index_stream(&vault_index_file);
  while (!vault_index_stream.atEnd() && vault_index_stream.readLine() != "$")
  {
    total_vaults++;
  }
  vault_index_stream.seek(0);

  logentry(QStringLiteral("load_vaults: found [%1] player vaults to read.").arg(total_vaults));
  if (total_vaults)
  {
    CREATE(vault_table, struct vault_data, total_vaults);
  }

  QString line = vault_index_stream.readLine();
  while (line != "$")
  {
    saveChanges = false;

    if (!line.isEmpty())
    {
      line[0] = line[0].toUpper();
    }

    QString fname = QStringLiteral("../vaults/%1/%2.vault").arg(line[0]).arg(line);
    QFile vault_file(fname);
    if (!vault_file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
    {
      logentry(QStringLiteral("boot_vaults: unable to open file [%1].").arg(fname), IMMORTAL, LogChannels::LOG_BUG);
      line = vault_index_stream.readLine();
      continue;
    }
    else
    {
      // sprintf(buf, "boot_vaults: sucessfully opened file [%s].", fname.toStdString().c_str());
      //       logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
    }
    QTextStream vault_file_stream(&vault_file);
    CREATE(vault, struct vault_data, 1);

    vault->owner = line;
    vault->size = VAULT_BASE_SIZE;
    vault->gold = 0;
    vault->weight = 0;
    vault->access = nullptr;
    vault->items = nullptr;
    vault->next = nullptr;

    QString type;
    qDebugQTextStreamLine(vault_file_stream, "load_vaults() before reading type");
    vault_file_stream >> type;
    while (type != '$')
    {
      QString value;
      unsigned int size{};
      switch (type[0].toLatin1())
      {
      case 'S':
        vault_file_stream >> vault->size >> Qt::ws;
        vault->size = MIN(vault->size, VAULT_MAX_SIZE);
        break;
      case 'G':
        vault_file_stream >> vault->gold >> Qt::ws;
        break;
      case 'O':
        vault_file_stream >> vnum >> count >> full >> Qt::ws;

        CREATE(items, struct vault_items_data, 1);

        if (!full)
        {
          obj = get_obj(vnum);
          items->obj = nullptr;
        }
        else
        {
          // We discard the line #VNUM
          vault_file_stream.readLine();
          if (real_object(vnum) == 3846 && vault->owner == "Gipsy")
          {
            qDebug("3846");
          }
          qDebug(vault->owner.toStdString().c_str());
          obj = read_object(real_object(vnum), vault_file_stream, true);
          items->obj = obj;
        }

        if (!obj)
        {
          if (full)
          {
            // Skip the rest of the full item
            while (type != 'S')
            {
              QString buffer = vault_file_stream.readLine();
              if (vault_file_stream.atEnd() || buffer.isEmpty())
              {
                break;
              }

              type = buffer[0].toLatin1();
            }
          }

          logentry(QStringLiteral("boot_vaults: bad item vnum (#%1) in vault: %2").arg(vnum).arg(vault->owner), IMMORTAL, LogChannels::LOG_BUG);
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
          sprintf(buf, "boot_vaults: got item [%s(%d)(%d)(%d)] from file [%s].", GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), count, vnum, fname.toStdString().c_str());
          if (items->obj && ((items->obj->obj_flags.wear_flags != get_obj(vnum)->obj_flags.wear_flags) ||
                             (items->obj->obj_flags.size != get_obj(vnum)->obj_flags.size) || (items->obj->obj_flags.eq_level != get_obj(vnum)->obj_flags.eq_level)))
            logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
          */
        }
        break;
      case 'A':
        qDebugQTextStreamLine(vault_file_stream, "load_vaults, before type A vault_file_stream >> value");
        vault_file_stream >> value >> Qt::ws;

        if (!value.isEmpty() && !stat(QStringLiteral("%1/%2/%3").arg(SAVE_DIR).arg(value[0]).arg(value).toStdString().c_str(), &statbuf))
        {
          CREATE(access, struct vault_access_data, 1);
          access->name = value;
          access->next = vault->access;
          vault->access = access;
          // sprintf(buf, "boot_vaults: got access [%s] from file [%s].", access->name, filename);
          //         logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
        }
        else
        {
          vlog(QStringLiteral("Invalid access entry found. Removing %1's access to %2.").arg(value).arg(vault->owner), vault->owner);
          saveChanges = true;
        }

        break;
      default:
        logentry(QStringLiteral("boot_vaults: unknown type [%1] in file [%2].").arg(type).arg(fname), IMMORTAL, LogChannels::LOG_BUG);
        break;
      }

      vault_file_stream >> type;
    }

    vault->next = vault_table;
    vault_table = vault;

    if (saveChanges)
    {
      logf(IMMORTAL, LogChannels::LOG_BUG, "boot_vaults: Saving changes to %s's vault.", vault->owner.toStdString().c_str());
      save_vault(vault->owner);
    }

    line = vault_index_stream.readLine();
  }
}

void add_vault_access(Character *ch, QString name, struct vault_data *vault)
{
  struct vault_access_data *access;
  class Connection d;

  if (!name.isEmpty())
    name[0] = name[0].toUpper();

  if (name == GET_NAME(ch))
  {
    ch->sendln("Don't be a moron, you already have access.");
    return;
  }

  // must be done to clear out "d" before it is used

  if (!get_pc(name))
    if (!(load_char_obj(&d, name)))
    {
      ch->sendln("You can't give access to someone who doesn't exist.");
      return;
    }

  if (has_vault_access(name, vault))
  {
    ch->sendln("That person already has access to your vault.");
    if (d.character)
      free_char(d.character, Trace("add_vault_access 1"));
    return;
  }

  ch->send(QStringLiteral("%1 now has access to your vault.\r\n").arg(name));
  CREATE(access, struct vault_access_data, 1);
  access->name = name;
  access->next = vault->access;
  vault->access = access;

  save_char_obj(ch);
  if (d.character)
    free_char(d.character, Trace("add_vault_access 2"));
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
  char value[128] = {}, line[128] = {}, buf[MAX_STRING_LENGTH] = {}, filename[MAX_INPUT_LENGTH] = {}, type[128] = {}, tmp[10] = {};
  bool saveChanges = false;
  char src_filename[256] = {};

  if (!(index = fopen(VAULT_INDEX_FILE, "r")))
  {
    logentry(QStringLiteral("boot_vaults: could not open vault index file, probably doesn't exist."), IMMORTAL, LogChannels::LOG_BUG);
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
    logentry(QStringLiteral("boot_vaults: could not open vault index file, probably doesn't exist."), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  fscanf(index, "%s\n", line);
  while (*line != '$')
  {
    saveChanges = false;

    *line = UPPER(*line);
    sprintf(filename, "../vaults/%c/%s.vault", UPPER(*line), line);
    if (!(fl = fopen(filename, "r")))
    {
      sprintf(buf, "boot_vaults: unable to open file [%s].", filename);
      logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
      fscanf(index, "%s\n", line);
      continue;
    }
    else
    {
      sprintf(buf, "boot_vaults: sucessfully opened file [%s].", filename);
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
        sscanf(type, "%s %lu", tmp, &gold);
        vault->gold = gold;
        break;
      case 'O':
        if (sscanf(type, "%s %d %d %d", tmp, &vnum, &count, &full) == 4)
        {
          ;
        }
        else
        {
          sprintf(buf, "boot_vaults: Bad 'O' option in file [%s]: %s\r\n", filename, type);
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

          snprintf(buf, sizeof(buf), "boot_vaults: bad item vnum (#%d) in vault: %s", vnum,
                   vault->owner.toStdString().c_str());
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
          snprintf(buf, sizeof(buf), "boot_vaults: got access [%s] from file [%s].", access->name.toStdString().c_str(), filename);
          //        logentry(buf, IMMORTAL, LogChannels::LOG_BUG);
        }
        else
        {
          snprintf(buf, sizeof(buf), "Invalid access entry found. Removing %s's access to %s.", value, vault->owner.toStdString().c_str());
          vlog(buf, vault->owner);
          saveChanges = true;
        }
        break;
      default:
        sprintf(buf, "boot_vaults: unknown type in file [%s].", filename);
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
      logf(IMMORTAL, LogChannels::LOG_BUG, "boot_vaults: Saving changes to %s's vault.", vault->owner.toStdString().c_str());
      save_vault(vault->owner);
    }

    fscanf(index, "%s\n", line);
  }
  fclose(index);
}

void remove_vault_access(Character *ch, QString name, struct vault_data *vault)
{

  name = name.toLower();
  if (!name.isEmpty())
  {
    name[0] = name[0].toUpper();
  }

  if (name == GET_NAME(ch))
  {
    ch->sendln("Don't be a moron, you can't remove your own access.");
    return;
  }

  if (!has_vault_access(name, vault))
  {
    ch->sendln("That person doesn't have access to your vault.");
    return;
  }

  ch->send(QStringLiteral("%1 no longer has access to your vault.\r\n").arg(name));
  access_remove(name, vault);
  save_char_obj(ch);
}

void remove_vault_accesses(QString name)
{
  struct vault_data *vault;
  struct vault_access_data *access, *next_access;
  int num = 0;

  for (vault = vault_table; vault; vault = vault->next, num++)
  {
    if (!num)
      continue; // skip 0 cause its null

    for (access = vault->access; access; access = next_access)
    {
      next_access = access->next;
      if (name == access->name)
      {
        vlog(QStringLiteral("Removed %1's access to %2's vault.").arg(name).arg(vault->owner), vault->owner);
        access_remove(name, vault);
        save_vault(vault->owner);
      }
    }
  }
}

void access_remove(QString name, struct vault_data *vault)
{
  struct vault_access_data *access, *next_access, *prev_access = nullptr;

  for (access = vault->access; access; access = next_access)
  {
    next_access = access->next;

    if (name == access->name)
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

QString clanVName(uint64_t clan_id)
{
  return QStringLiteral("Clan%1").arg(clan_id);
}

bool has_vault_access(Character *ch, struct vault_data *vault)
{
  if (ch == nullptr)
  {
    return false;
  }

  if (ch->getLevel() >= 108)
  {
    return true;
  }

  if (ch->getName() == vault->owner)
  {
    return true;
  }

  if (ch->clan && get_clan(ch->clan) && vault->owner == clanVName(ch->clan) && has_right(ch, CLAN_RIGHTS_VAULT))
  {
    return true;
  }

  // its not their vault so check all the access lines
  for (struct vault_access_data *access = vault->access; access; access = access->next)
  {
    if (ch->getName() == access->name)
    {
      return true;
    }
  }

  return false;
}

bool has_vault_access(QString who, struct vault_data *vault)
{
  Character *ch = find_owner(who);
  if (ch == nullptr)
  {
    return false;
  }

  return has_vault_access(ch, vault);
}

struct vault_items_data *get_unique_item_in_vault(struct vault_data *vault, char *object, int num)
{
  struct vault_items_data *items;
  class Object *obj;
  int i = 1;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    if (obj && isexact(object, GET_OBJ_NAME(obj)))
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
    if (obj && isexact(object, GET_OBJ_NAME(obj)))
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
    if (obj && isexact(object, GET_OBJ_NAME(obj)))
    {
      if (i == num)
        return items;
      else
      {
        for (j = 1; j <= items->count; j++)
          if (isexact(object, GET_OBJ_NAME(obj)))
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

class Object *get_obj_in_vault(struct vault_data *vault, QString object, int num)
{
  struct vault_items_data *items;
  class Object *obj;
  int i = 1, j;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    //    obj = get_obj(items->item_vnum);
    if (obj && isexact(object, GET_OBJ_NAME(obj)))
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

void vault_get(Character *ch, QString object, QString owner)
{
  QString sbuf;
  char obj_list[50][100];
  class Object *obj, *tmp_obj;
  struct vault_items_data *items;
  struct vault_data *vault;
  int self = 0, num = 1, i;

  if (!owner.isEmpty())
  {
    owner = owner.toLower();
    owner[0] = owner[0].toUpper();
  }

  if (owner == GET_NAME(ch))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(QStringLiteral("%1 doesn't have a vault.\r\n").arg(owner));
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    ch->send(QStringLiteral("You don't have permission to take %1's stuff.\r\n").arg(owner));
    return;
  }

  QStringList namelist = object.trimmed().split('.');

  if (namelist.length() != 2)
  {
    num = 1;
  }
  else
  {
    bool ok = false;
    num = namelist.at(0).toInt(&ok);
    if (!ok)
    {
      num = 1;
    }
    namelist.pop_front();
    object = namelist.join('.');
  }

  if (object == "all")
  {
    bool ioverload = false;
    for (items = vault->items, i = 0; items; items = items->next)
    {
      obj = items->obj ? items->obj : get_obj(items->item_vnum);
      if (obj == 0)
        continue;

      for (int j = 0; j < items->count; j++, i++)
      {
        strncpy(obj_list[i], fname(obj->getName()).toStdString().c_str(), sizeof(obj_list[i]));
        if (i > 49)
        {
          ch->send("You can only take out 50 items at a time.\r\n");
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

  if (object.startsWith("all.")) // sscanf(object, "all.%s", object))
  {
    object.remove(0, 4);
    num = 0; // count the number of items that match that keyword so we know how many to get
    for (items = vault->items; items; items = items->next)
    {
      obj = items->obj ? items->obj : get_obj(items->item_vnum);
      //    obj = get_obj(items->item_vnum);
      if (obj == 0)
        continue;

      if (isexact(object, GET_OBJ_NAME(obj)))
        num += items->count;
    }

    if (!num)
    {
      ch->send("There is nothing like that in the vault.\r\n");
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
      ch->sendln("There is nothing like that in the vault.");
      return;
    }

    if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_char_for_item(ch, obj->item_number, false))
    {
      ch->sendln("Why would you want another one of those?");
      return;
    }

    if (!self && (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) || isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL)) && ch->isMortal())
    {
      ch->sendln("That item seems to be bound to the vault.");
      return;
    }

    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
    {
      ch->sendln("You cannot hold any more.");
      if (ch->isMortal())
        return;
      else
        ch->sendln("But since you're an immortal, you get it anyway.");
    }

    if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
    {
      ch->sendln("You cannot carry any more items.");
      return;
    }

    sbuf = QStringLiteral("%1 removed %2(v%3) from %4's vault.").arg(ch->getName()).arg(GET_OBJ_SHORT(obj)).arg(QString::number(GET_OBJ_VNUM(obj))).arg(owner);
    vlog(sbuf, owner);
    act(sbuf, ch, 0, 0, TO_ROOM, GODS);
    ch->send(QStringLiteral("%1 has been removed from the vault.\r\n").arg(GET_OBJ_SHORT(obj)));

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
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(QStringLiteral("%1 doesn't have a vault.\r\n").arg(owner));
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    ch->send(QStringLiteral("You don't have permission to put $B$5gold$R in %1's vault.\r\n").arg(owner));
    return;
  }

  auto max_amount = MIN(ch->getGold(), VAULT_MAX_DEPWITH);
  if (!max_amount)
  {
    ch->send("You don't have any $B$5gold$R to deposit.\r\n");
    return;
  }

  if (amount < 1 || amount > max_amount)
  {
    ch->send(QStringLiteral("Valid amounts are from 1 to %1 $B$5gold$R.\r\n").arg(max_amount));
    return;
  }

  if (ch->getGold() >= amount)
  {
    vault->gold += amount;
    ch->removeGold(amount);
    save_char_obj(ch);
    save_vault(owner);

    vlog(QStringLiteral("%1 added %2 gold to %3's vault.").arg(GET_NAME(ch)).arg(amount).arg(owner), owner);
    ch->send(QStringLiteral("You deposit %1 $B$5gold$R into the vault. Its new balance is %2 $B$5gold$R.\r\nYou have %3 $B$5gold$R left on you.\r\n").arg(amount).arg(vault->gold).arg(ch->getGold()));
  }
  else
  {
    ch->send(QStringLiteral("But you only have %1 $B$5gold$R coins!\r\n").arg(ch->getGold()));
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
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(QStringLiteral("%1 doesn't have a vault.\r\n").arg(owner));
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    ch->send(QStringLiteral("You don't have permission to put $B$5gold$R in %1's vault.\r\n").arg(owner));
    return;
  }

  auto max_amount = MIN(vault->gold, VAULT_MAX_DEPWITH);
  if (amount < 1 || amount > max_amount)
  {
    ch->send(QStringLiteral("Valid amounts are from 1 to %1 $B$5gold$R.\r\n").arg(max_amount));
    return;
  }

  if (vault->gold >= (uint64_t)amount)
  {
    /*    if (amount + ch->getGold() > 2000000000) {
          ch->send("You can't hold that much gold.  The most you could get is %ld.\r\n", 2000000000 - ch->getGold());
          return;
        }*/
    vault->gold -= amount;
    ch->addGold(amount);
    save_char_obj(ch);
    save_vault(owner);
    vlog(QStringLiteral("%1 removed %2 gold from %3's vault.").arg(GET_NAME(ch)).arg(amount).arg(owner), owner);
    ch->send(QStringLiteral("You withdraw %1 $B$5gold$R from the vault. Its new balance is %2 $B$5gold$R.\r\nYou have %3 $B$5gold$R left on you.\r\n").arg(amount).arg(vault->gold).arg(ch->getGold()));
  }
  else
  {
    ch->send(QStringLiteral("The vault only has %1 $B$5gold$R coins in it!\r\n").arg(vault->gold));
  }
}

int can_put_in_vault(class Object *obj, int self, struct vault_data *vault, Character *ch)
{
  //  class Object *tmp_obj;

  if (GET_OBJ_VNUM(obj) == -1)
  {
    ch->send(QStringLiteral("%1 is hardly worth saving.\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (GET_OBJ_VNUM(obj) == 393)
  { // no potatoes in the vaults
    ch->send("It would rot. EWW!\r\n");
    return 0;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_vault_by_vnum(GET_OBJ_VNUM(obj), vault))
  { // Uniques
    ch->send(QStringLiteral("Why would you need another %1?\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL) && !self)
  { // GL
    ch->send(QStringLiteral("%1 is far too valuable to place in someone else's vault.\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (!self && isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) && ch->isMortal())
  { // no_trade
    ch->send(QStringLiteral("%1 seems bound to you.\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
  { // cursed
    ch->send(QStringLiteral("%1 is stuck! Ack!.\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE) || isSet(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  { // nosave
    ch->send(QStringLiteral("%1 doesn't seem to be a permanent part of the world.\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (ARE_CONTAINERS(obj) && obj->contains)
  { // non-empty containers
    ch->send(QStringLiteral("%1 needs to be emptied first.\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
  { // vault is full
    ch->send(QStringLiteral("You can't seem to stuff %1 in the vault.  Its too big!\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }
  if (obj->obj_flags.timer > 0)
  {
    ch->send(QStringLiteral("%1 cannot be placed in the vault right now.\r\n").arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  return 1;
}

void vault_put(Character *ch, QString object, QString owner)
{
  class Object *obj, *tmp_obj;
  struct vault_data *vault;
  char buf[MAX_INPUT_LENGTH];
  int self = 0;

  if (!owner.isEmpty())
  {
    owner = owner.toLower();
    owner[0] = owner[0].toUpper();
  }

  if (owner == GET_NAME(ch))
  {
    self = 1;
  }

  if (!(vault = has_vault(owner)))
  {
    if (self)
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(QStringLiteral("%1 doesn't have a vault.\r\n").arg(owner));
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    ch->send(QStringLiteral("You don't have permission to put things in %1's vault.\r\n").arg(owner));
    return;
  }

  if (object == "all")
  {
    for (obj = ch->carrying; obj; obj = tmp_obj)
    {
      tmp_obj = obj->next_content;
      if (!can_put_in_vault(obj, self, vault, ch))
        continue;

      if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
      {
        ch->send(QStringLiteral("%1 won't fit in the vault!\r\n").arg(GET_OBJ_SHORT(obj)));
        continue;
      }

      QString buffer;
      if (ch->isMortal())
        buffer = QStringLiteral("%1 added %2 to %3's vault.").arg(GET_NAME(ch)).arg(GET_OBJ_SHORT(obj)).arg(owner);
      else
        buffer = QStringLiteral("%1 added %2[%3] to %4's vault.").arg(GET_NAME(ch)).arg(GET_OBJ_SHORT(obj)).arg(GET_OBJ_VNUM(obj)).arg(owner);

      vlog(buffer, owner);
      ch->send(QStringLiteral("%1 has been placed in the vault.\r\n").arg(GET_OBJ_SHORT(obj)));

      QString sbuf;
      QTextStream ssin;
      ssin << GET_OBJ_VNUM(obj);

      sbuf = GET_NAME(ch);
      sbuf += " added ";
      sbuf += GET_OBJ_SHORT(obj);
      sbuf += "[";
      if (ssin.string())
        sbuf += *ssin.string();
      sbuf += "] to ";
      sbuf += owner;
      sbuf += "'s vault.";

      act(sbuf, ch, 0, 0, TO_ROOM, GODS);

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
  else if (object.startsWith("all."))
  {
    object.remove(0, 4);
    for (obj = ch->carrying; obj; obj = tmp_obj)
    {
      tmp_obj = obj->next_content;
      if (!isexact(object, GET_OBJ_NAME(obj)))
        continue;
      if (!can_put_in_vault(obj, self, vault, ch))
        continue;

      if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
      {
        ch->send(QStringLiteral("%1 won't fit in the vault!\r\n").arg(GET_OBJ_SHORT(obj)));
        continue;
      }

      QString buffer;
      if (ch->isMortal())
        buffer = QStringLiteral("%1 added %2 to %3's vault.").arg(GET_NAME(ch)).arg(GET_OBJ_SHORT(obj)).arg(owner);
      else
        buffer = QStringLiteral("%1 added %2[%3] to %4's vault.").arg(GET_NAME(ch)).arg(GET_OBJ_SHORT(obj)).arg(GET_OBJ_VNUM(obj)).arg(owner);

      vlog(buffer, owner);

      ch->send(QStringLiteral("%1 has been placed in the vault.\r\n").arg(GET_OBJ_SHORT(obj)));
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
      ch->sendln("You don't have anything like that.");
      return;
    }

    if (!can_put_in_vault(obj, self, vault, ch))
      return;

    if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
    {
      ch->send(QStringLiteral("%1 won't fit in the vault!\r\n").arg(GET_OBJ_SHORT(obj)));
      return;
    }

    QString buffer;
    if (ch->isMortal())
      buffer = QStringLiteral("%1 added %2 to %3's vault.").arg(GET_NAME(ch)).arg(GET_OBJ_SHORT(obj)).arg(owner);
    else
      buffer = QStringLiteral("%1 added %2[%3] to %4's vault.").arg(GET_NAME(ch)).arg(GET_OBJ_SHORT(obj)).arg(GET_OBJ_VNUM(obj)).arg(owner);

    vlog(buffer, owner);

    ch->send(QStringLiteral("%1 has been placed in the vault.\r\n").arg(GET_OBJ_SHORT(obj)));

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

void vault_list(Character *ch, QString owner)
{
  struct vault_items_data *items;
  struct vault_data *vault;
  class Object *obj;
  int objects = 0, self = 0;
  int diff_len = 0;
  int sectionbuf_len = 0;
  int linebuf_len = 0;

  if (!owner.isEmpty())
  {
    owner = owner.toLower();
    owner[0] = owner[0].toUpper();
  }

  if (owner == GET_NAME(ch))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(QStringLiteral("%1 doesn't have a vault.\r\n").arg(owner));
    return;
  }

  if (!has_vault_access(GET_NAME(ch), vault))
  {
    ch->send(QStringLiteral("You don't have access to %1's vault.\r\n").arg(owner));
    return;
  }

  sorted_vault sv;
  sort_vault(*vault, sv);

  if (sv.weight != vault->weight)
  {
    if (self)
    {
      ch->send(QStringLiteral("Some objects in your vault have changed weight.\r\nYour vault's weight has been recalculated from %1 to %2.\r\n").arg(vault->weight).arg(sv.weight));
    }
    vault->weight = sv.weight;
  }

  if (sv.vault_contents.empty())
  {
    if (self)
    {
      ch->send(QStringLiteral("Your vault is currently empty and can hold %1 pounds.\r\n").arg(vault->size));
    }
    else
    {
      ch->send(QStringLiteral("%1's vault is currently empty.\r\n").arg(owner));
    }
    return;
  }

  if (self)
  {
    ch->send(QStringLiteral("Your vault is at %1 of %2 maximum pounds and contains:\r\n").arg(vault->weight).arg(vault->size));
  }
  else
  {
    ch->send(QStringLiteral("%1's vault is at %2 of %3 maximum pounds and contains:\r\n").arg(owner).arg(vault->weight).arg(vault->size));
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
      ch->send(QStringLiteral("[$5%1$R] ").arg(count));
    }

    ch->send(QStringLiteral("%1$R").arg(GET_OBJ_SHORT(obj)));

    if (obj->obj_flags.type_flag == ITEM_ARMOR ||
        obj->obj_flags.type_flag == ITEM_WEAPON ||
        obj->obj_flags.type_flag == ITEM_FIREWEAPON ||
        obj->obj_flags.type_flag == ITEM_CONTAINER ||
        obj->obj_flags.type_flag == ITEM_INSTRUMENT ||
        obj->obj_flags.type_flag == ITEM_STAFF ||
        obj->obj_flags.type_flag == ITEM_WAND ||
        obj->obj_flags.type_flag == ITEM_LIGHT)
    {
      ch->send(QStringLiteral("%1 $3Lvl: %2$R").arg(item_condition(obj)).arg(obj->obj_flags.eq_level));
    }

    if (ch->getLevel() > IMMORTAL && obj->item_number > 0)
    {
      ch->send(QStringLiteral(" [%1]").arg(DC::getInstance()->obj_index[obj->item_number].virt));
    }
    ch->send("\r\n");
  }
}

void add_new_vault(const char *name, int indexonly)
{
  FILE *vfl, *tvfl, *pvfl;
  struct vault_data *vault;
  char filename[256], line[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  if (!(vfl = fopen(VAULT_INDEX_FILE, "r")))
  {
    logentry(QStringLiteral("add_new_vault: error opening index file."), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (!(tvfl = fopen(VAULT_INDEX_FILE_TMP, "w")))
  {
    logentry(QStringLiteral("add_new_vault: error opening temp index file."), IMMORTAL, LogChannels::LOG_BUG);
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

  sprintf(filename, "../vaults/%c/%s.vault", UPPER(*name), name);
  if (!(pvfl = fopen(filename, "w")))
  {
    logentry(QStringLiteral("add_new_vault: error opening new vault file [%1].").arg(filename), IMMORTAL, LogChannels::LOG_BUG);
    return;
  }

  if (ch)
    fprintf(pvfl, "S %d\n", VAULT_BASE_SIZE * ch->getLevel());
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
    vault->size = VAULT_BASE_SIZE * ch->getLevel();
  else
    vault->size = VAULT_BASE_SIZE;
  vault->weight = 0;
  vault->access = nullptr;
  vault->items = nullptr;
  vault->next = vault_table;
  vault_table = vault;

  save_vault(name);
}

Character *find_owner(QString name)
{
  const auto &character_list = DC::getInstance()->character_list;
  const auto &result = find_if(character_list.begin(), character_list.end(), [&name](const auto &ch)
                               {
	  if (ch->getNameC() == nullptr)
    {
		  produce_coredump(); //Trying to track down bug that causes mob->name to be nullptr
	  }
    else if (name == GET_NAME(ch) && IS_PC(ch))
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

  if (!strcmp(owner, clanVName(ch->clan).toStdString().c_str()))
  {
    strncpy(buf, "The following are your clan's most recent vault log entries (Times are UTC):\n\r", MAX_STRING_LENGTH);
  }
  else
  {
    strncpy(buf, "The following are your most recent vault log entries (Times are UTC):\r\n", MAX_STRING_LENGTH);
  }

  owner[0] = UPPER(owner[0]);

  snprintf(fname, 256, "../vaults/%c/%s.vault.log", *owner, owner);

  std::ifstream fin(fname);
  std::stringstream buffer;
  buffer << buf;
  buffer << fin.rdbuf();

  page_string(ch->desc, const_cast<char *>(buffer.str().c_str()), 1);
}

void vlog(QString message, QString name)
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

  sprintf(fname, "../vaults/%c/%s.vault.log", name[0], name.toStdString().c_str());
  sprintf(nfname, "../vaults/%c/%s.vault.log.tmp", name[0], name.toStdString().c_str());

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
  fprintf(nfile, "%s :: %s\n", buf, message.toStdString().c_str());

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
  struct vault_data *cvault = ch->clan ? has_vault(clanVName(ch->clan).toStdString().c_str()) : 0;
  if (cmd == 59)
  {
    if (!vault)
    {
      ch->sendln("You need to level up some before obtaining a vault.");
      return eSUCCESS;
    }
    else if (vault->size < VAULT_MAX_SIZE)
      sprintf(buf, "$B1)$R Increase the size of vault by 10 lbs: %d platinum.\r\n", VAULT_UPGRADE_COST);
    else
      sprintf(buf, "1) You cannot increase your vault-size further.\r\n");
    ch->sendln("$B$2Paul the sleazy vault salesman tells you, 'How aboot a bigger vault? Size matters, you know'$R");

    ch->send(buf);

    sprintf(buf, "$B2)$R Purchase a clan vault: %s\r\n",
            ch->clan ? cvault ? "Your clan already has a vault" : has_right(ch, CLAN_RIGHTS_VAULT) ? "1000 platinum coins."
                                                                                                   : "You are not authorized to make this purchase."
                     : "You are not a member of any clan.");
    ch->send(buf);

    if (!cvault)
      sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", ch->clan ? "Your clan has no vault." : "You're not in a clan.");
    else if (cvault->size < VAULT_MAX_SIZE)
      sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", has_right(ch, CLAN_RIGHTS_VAULT) ? "200 platinum coins." : "You are not authorized to make this purchase.");
    else
      sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: You cannot increase the vault's size further.\r\n");
    ch->send(buf);
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
        ch->sendln("You need to level up some before obtaining a vault.");
        return eSUCCESS;
      }
      if (vault->size >= VAULT_MAX_SIZE)
      {
        ch->sendln("Your vault's size is already at its maximum capacity.");
        return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < VAULT_UPGRADE_COST)
      {
        ch->sendln("You do not have enough platinum.");
        return eSUCCESS;
      }
      GET_PLATINUM(ch) -= VAULT_UPGRADE_COST;
      vault->size += 10;
      save_char_obj(ch);
      save_vault(vault->owner);
      ch->sendln("$B$2Paul the sleazy vault salesman tells you, '10 lbs added to your vault.$R'");
      return eSUCCESS;
    case 2:
      if (cvault)
      {
        ch->sendln("Your clan already has a vault.");
        return eSUCCESS;
      }
      if (!ch->clan)
      {
        ch->sendln("You're not a member of any clan.");
        return eSUCCESS;
      }
      if (!has_right(ch, CLAN_RIGHTS_VAULT))
      {
        ch->sendln("You are not authorized to make that purchase.");
        return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 1000)
      {
        ch->sendln("You do not have enough platinum.");
        return eSUCCESS;
      }
      GET_PLATINUM(ch) -= 1000;
      add_new_vault(clanVName(ch->clan).toStdString().c_str(), 0);
      save_char_obj(ch);
      cvault = has_vault(clanVName(ch->clan));
      cvault->size = 500;
      save_vault(clanVName(ch->clan));
      ch->sendln("You have purchased a vault for your clan's perusal.");
      return eSUCCESS;
    case 3:
      if (!cvault)
      {
        ch->sendln("Your clan does not have a vault.");
        return eSUCCESS;
      }
      if (!has_right(ch, CLAN_RIGHTS_VAULT))
      {
        ch->sendln("You are not authorized to make that purchase.");
        return eSUCCESS;
      }
      if (cvault->size >= VAULT_MAX_SIZE)
      {
        ch->sendln("The vault is already at its maximum capacity.");
        return eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 200)
      {
        ch->sendln("You do not have enough platinum.");
        return eSUCCESS;
      }
      GET_PLATINUM(ch) -= 200;
      cvault->size += 10;
      save_char_obj(ch);
      save_vault(clanVName(ch->clan));
      ch->sendln("You have added 10 lbs capacity to your clan's vault.");
      return eSUCCESS;
    }
  }
  return eFAILURE;
}

void vault_search_usage(Character *ch)
{
  ch->sendln("Usage: vault search [ keyword <keyword> ] | [ level <levels> ] | ...");
  ch->sendln("keyword <keyword>  -  Single word keyword. Can be used multiple times.");
  ch->sendln("level <levels>     -  Single object level or range of levels.\r\n");
  ch->sendln("Examples:");
  ch->sendln("vault search keyword staff");
  ch->sendln("vault search level 55");
  ch->sendln("vault search keyword staff level 55.");
  ch->sendln("vault search keyword staff level 40-60.\r\n");
}

int vault_search(Character *ch, const char *args)
{
  int objects = 0;
  bool owner_shown = false;
  int vaults_searched = 0;
  int objects_found = 0;
  QString arg1{};
  QString arguments{args};
  std::list<vault_search_parameter>::iterator p;
  bool nomatch;

  std::tie(arg1, arguments) = half_chop(arguments);

  if (arg1.isEmpty())
  {
    vault_search_usage(ch);
    return eFAILURE;
  }

  std::list<vault_search_parameter> search;
  vault_search_parameter parameter;

  // parse our arguments and setup a list of the things we want to search for
  do
  {
    if (arg1 == "keyword")
    {
      std::tie(arg1, arguments) = half_chop(arguments);
      if (arg1.isEmpty())
      {
        ch->sendln("Missing keyword parameter.\r\n");
        vault_search_usage(ch);
        return eFAILURE;
      }
      else
      {
        parameter.type = vault_search_type::KEYWORD;
        parameter.str_argument = arg1;
        search.push_back(parameter);
        // We need
        parameter.type = vault_search_type::UNDEFINED;
      }
    }
    else if (arg1 == "level")
    {
      std::tie(arg1, arguments) = half_chop(arguments);
      if (arg1.isEmpty())
      {
        ch->sendln("Missing level parameter.\r\n");
        vault_search_usage(ch);
        return eFAILURE;
      }
      else
      {
        // start parsing level and level ranges
        size_t pos;
        QString level_string(arg1);

        // if a '-' is not found then assume a single number is specified
        if ((pos = level_string.indexOf('-')) == -1)
        {
          bool ok = false;
          parameter.int_argument = arg1.toInt(&ok);
          // Check if a non numeric value is passed
          if (!ok)
          {
            ch->sendln("Invalid level specified.\r\n");
            vault_search_usage(ch);
            return eFAILURE;
          }

          parameter.type = vault_search_type::LEVEL;
          search.push_back(parameter);
        }
        else
        {
          // Get the first half of a level range
          QString min_level_string = level_string.sliced(0, pos);
          bool ok = false;
          parameter.int_argument = min_level_string.toInt(&ok);
          // Check if a non numeric value is passed
          if (!ok)
          {
            ch->sendln("Invalid minimum level specified.\r\n");
            vault_search_usage(ch);
            return eFAILURE;
          }

          parameter.type = vault_search_type::MIN_LEVEL;
          search.push_back(parameter);

          // Get the second half of a level range
          QString max_level_string = level_string.sliced(pos + 1);
          ok = false;
          parameter.int_argument = max_level_string.toInt(&ok);
          // Check if a non numeric value is passed
          if (!ok)
          {
            ch->sendln("Invalid maximum level specified.\r\n");
            vault_search_usage(ch);
            return eFAILURE;
          }

          parameter.type = vault_search_type::MAX_LEVEL;
          search.push_back(parameter);
        } // end of level range parsing
      } // end of level parsing
    }
    else
    {
      ch->sendln("Invalid argument.\r\n");
      vault_search_usage(ch);
      return eFAILURE;
    }
    std::tie(arg1, arguments) = half_chop(arguments);
  } while (!arg1.isEmpty());

  // now that we know what we're looking for, let's search through all the vaults to find it
  for (vault_data *vault = vault_table; vault; vault = vault->next)
  {
    if (vault && !vault->owner.isEmpty() && has_vault_access(ch, vault))
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
          case vault_search_type::UNDEFINED:
            break;
          case vault_search_type::KEYWORD:
            if (!isexact((*p).str_argument, GET_OBJ_NAME(obj)))
            {
              nomatch = true;
            }
            break;
          case vault_search_type::LEVEL:
            if (obj->obj_flags.eq_level != (*p).int_argument)
            {
              nomatch = true;
            }
            break;
          case vault_search_type::MIN_LEVEL:
            if (obj->obj_flags.eq_level < (*p).int_argument)
            {
              nomatch = true;
            }
            break;
          case vault_search_type::MAX_LEVEL:
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
          ch->send(QStringLiteral("\n\r%1:\n\r").arg(vault->owner));
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

        if (ch->getLevel() > IMMORTAL && obj->item_number > 0)
        {
          ch->send(fmt::format(" [{}]", DC::getInstance()->obj_index[obj->item_number].virt));
        }
        ch->send("\r\n");
      } // for loop of objects
    } // if we have access to vault
  } // for loop of vaults

  ch->send(QStringLiteral("\n\rSearched %1 vaults and found %2 objects.\r\n").arg(vaults_searched).arg(objects_found));

  return eSUCCESS;
}
