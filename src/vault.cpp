
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fmt/format.h>

#ifdef TRACY
#include <tracy/Tracy.hpp>
#endif

#include "DC/DC.h"
#include "DC/db.h"
#include "DC/handler.h"
#include "DC/interp.h"
#include "DC/clan.h" // clan right
#include "DC/inventory.h"

/*
  void vault_withdraw(quint32 amount, QString owner);
  void vault_deposit(quint32 amount, QString owner);
  void vault_get(QString object_keyword, QString owner);
  void vault_put(QString object_keyword, QString owner);
  void my_vault_access(void);
  void vault_sell(QString object_keyword, QStringList arguments);
  void vault_cost(QString object_keyword, QStringList arguments);
  qint32 vault_search(QString object_keyword);

*/

class vault_search_parameter
{
public:
  vault_search_parameter();
  ~vault_search_parameter();
  vault_search_type type;

  QString str_argument;
  qint32 int_argument;
};

vault_search_parameter::vault_search_parameter()
    : type(vault_search_type::UNDEFINED)
{
}

vault_search_parameter::~vault_search_parameter()
{
}

qint32 total_vaults = {};
qint32 get_line(FILE *fl, QString buf);

CharacterPtr find_owner(QString name);
void vault_log(CharacterPtr ch, QString owner);
QString clanVName(quint64 clan_id);
void vault_search_usage(CharacterPtr ch);

VaultPtr Vaults::has_vault(QString name)
{
  VaultPtr vault;

  for (vault = vault_table; vault; vault = vault->next)
    if (vault && !name.compare(vault->owner, Qt::CaseInsensitive))
      return vault;
  CharacterPtr ch = find_owner(name);
  if (ch && ch->getLevel() >= 10)
  {
    add_new_vault(qPrintable(ch->name()), 0);
    for (vault = vault_table; vault; vault = vault->next)
      if (vault && !name.compare(vault->owner, Qt::CaseInsensitive))
      {
        //	 ch->sendln("A vault was created for you.");
        return vault;
      }
  }
  return 0;
}

void remove_from_object_list(ObjectPtr obj)
{
  ObjectPtr tObj, pObj = {};
  for (tObj = dc_->object_list; tObj; tObj = tObj->next)
  {
    if (tObj == obj)
    {
      if (pObj)
        pObj->next = tObj->next;
      else
        dc_->object_list = tObj->next;
      tObj->next = {};
      break;
    }
    pObj = tObj;
  }
}

void Vaults::save_vault(QString name)
{
  VaultPtr vault;
  vault_access_data *access;
  vault_items_data *items;

  if (name.isEmpty())
  {
    return;
  }
  name = name.toLower();
  name[0] = name[0].toUpper();

  if (!(vault = has_vault(name)))
    return;

  CharacterPtr ch = find_owner(name);
  if (ch)
    if (vault->size < ch->getLevel() * 10)
      vault->size = ch->getLevel() * 10;

  QString fname = u"../vaults/%1/%2.vault"_s.arg(name[0]).arg(name);

  QFile file(fname);
  if (!file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text))
  {
    dc_->logentry(u"save_vault: could not open vault file for [%1]."_s.arg(fname), IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  QTextStream out(&file);
  out << u"S %1\n"_s.arg(vault->size);
  out << u"G %1\n"_s.arg(vault->gold);

  for (items = vault->items; items; items = items->next)
  {
    ObjectPtr obj = items->obj ? items->obj : get_obj(items->item_vnum);
    if (obj == 0)
      continue;

    out << u"O %1 %2 %3\n"_s.arg(items->item_vnum).arg(items->count).arg(items->obj ? 1 : 0);
    if (items->obj)
      write_object(items->obj, out);
  }

  for (access = vault->access; access; access = access->next)
    out << u"A %1\n"_s.arg(access->name);

  out << "$\n";
}

void Character::vault_access(QString who)
{
  vault_access_data *access;
  VaultPtr vault = {};

  if (!vault && !(vault = dc_->vaults_->has_vault(name())))
  {
    sendln("You don't seem to have a vault.");
    return;
  }

  if (who.isEmpty())
  {
    send("The following people have access to your vault:\r\n");
    for (auto &access : vault->access)
      send(u"%1\r\n"_s.arg(access.name));
    return;
  }

  if (dc_->has_vault_access(who, vault))
    remove_vault_access(who, vault);
  else
    add_vault_access(who, vault);
}

void Character::vault_myaccess(QString arg)
{
  VaultPtr vault;

  if (arg[0] != '\0')
  {
    if (!(vault = dc_->vaults_->has_vault(arg)))
    {
      sendln("No such player.");
      return;
    }
    if (!dc_->has_vault_access(qPrintable(this->name()), vault))
    {
      sendln("You do not have access to that vault anyway.");
      return;
    }
    access_remove(qPrintable(this->name()), vault);
    sendln("You remove your access to that vault.");
    return;
  }

  sendln("You have access to the following vaults:");
  for (vault = vault_table; vault; vault = vault->next)
    if (vault && dc_->has_vault_access(this, vault))
      send(u"%1\r\n"_s.arg(vault->owner));
}

void Character::vault_balance(QString owner)
{
  if (owner.isEmpty())
    return;

  owner[0] = owner[0].toUpper();
  bool self = false;
  if (owner == name())
    self = true;

  auto vault = dc_->vaults_->has_vault(owner);
  if (!vault)
  {
    if (self)
      send("You don't have a vault.\r\n");
    else
      send(u"%1 doesn't have a vault.\r\n"_s.arg(owner));
    return;
  }

  if (!dc_->has_vault_access(name(), vault))
  {
    send(u"You don't have permission to see %1's balance.\r\n"_s.arg(owner));
    return;
  }

  if (self)
    send(u"You have %1 $B$5gold$R coins in your vault.\r\n"_s.arg(vault->gold));
  else
    send(u"There are %1 $B$5gold$R coins in %2's vault.\r\n"_s.arg(vault->gold).arg(owner));
}

const QString vault_usage = "Syntax: vault <list | balance> [vault owner]\r\n"
                            "        vault <put | get> <object> [vault owner]\r\n"
                            "        vault <deposit | withdraw> <amount>\r\n"
                            "        vault <access | myaccess> [name to add/remove access]\r\n"
                            "        vault log [vault owner]\r\n"
                            "        vault search [ keyword <keyword> ] | [ level <levels> ] | ...\r\n";

const QString imm_vault_usage = "        vault <stats> [name]\r\n";

command_return_t do_vault(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg{}, arg1{}, arg2 = {};

  half_chop(argument, arg, arg1);

  if (ch->isPlayerObjectThief() || (ch->isPlayerGoldThief()))
  {
    ch->sendln("You're too busy running from the law!");
    return ReturnValue::eFAILURE;
  }

  if (arg.isEmpty())
  {
    ch->send(vault_usage);
    if (ch->getLevel() > IMMORTAL)
      ch->send(imm_vault_usage);
    return ReturnValue::eSUCCESS;
  }

  if (!str_cmp(arg1, "clan") && ch->clan)
    dc_strcpy(arg1, qPrintable(clanVName(ch->clan)));

  // show the contents of your or someone elses vault
  if (!strncmp(arg, "list", dc_strlen(arg)))
  {
    if (arg.isEmpty() 1)
      dc_sprintf(arg1, "%s", qPrintable(ch->name()));
    ch->vault_list(arg1);

    // show how much gold in vault
  }
  else if (!strncmp(arg, "balance", dc_strlen(arg)))
  {
    if (arg.isEmpty() 1)
      dc_sprintf(arg1, "%s", qPrintable(ch->name()));
    ch->vault_balance(arg1);

    // show what vaults I have access to
  }
  else if (!strncmp(arg, "myaccess", dc_strlen(arg)))
  {
    ch->vault_myaccess(arg1);

    // show my current access, add access, or remove access
  }
  else if (!strncmp(arg, "access", dc_strlen(arg)))
  {
    ch->vault_access(arg1);
  }
  else if (ch->getLevel() > IMMORTAL && !strncmp(arg, "stats", dc_strlen(arg)))
  {
    ch->vault_stats(arg1);

    // show vault log
  }
  else if (!strncmp(arg, "log", dc_strlen(arg)))
  {
    if (*arg1)
    {
      if (!strcasecmp(arg1, qPrintable(clanVName(ch->clan))))
      {
        ClanPtr clan = get_clan(ch);
        if (clan == nullptr)
        {
          ch->sendln("You are not a member of any clan.");
          return ReturnValue::eFAILURE;
        }

        // Clan leader or a clan member with the vaultlog right can view log.
        if ((clan->leader && !dc_strcmp(clan->leader, qPrintable(ch->name()))) || has_right(ch, CLAN_RIGHTS_VAULTLOG))
        {
          std::stringstream clanName;

          clanName << "clan" << clan->id_;
          dc_sprintf(arg1, "%s", clanName.str().c_str());

          vault_log(ch, arg1);
        }
        else
        {
          ch->sendln("You don't have access to view the clan's vault log.");
          return ReturnValue::eFAILURE;
        }
      }
      else if (ch->isImmortalPlayer())
      {
        vault_log(ch, arg1);
      }
      else
      {
        ch->sendln("Syntax: vault log <clan>");
        return ReturnValue::eFAILURE;
      }
    }
    else
    {
      dc_sprintf(arg1, "%s", qPrintable(ch->name()));
      vault_log(ch, arg1);
    }
  }
  else if (arg == u"search"_s)
  {
    if (*arg1)
    {
      return vault_search(ch, arg1);
    }
    else
    {
      vault_search_usage(ch);
      return ReturnValue::eFAILURE;
    }
    // putting this here so that anything below it requires you to be in a safe room.
  }
  else if (!isSet(dc_->world[ch->in_room].room_flags, SAFE))
  {
    ch->sendln("You don't feel safe enough to manage your valuables.");
    return ReturnValue::eSUCCESS;
  }
  else if (!strncmp(arg, "withdraw", dc_strlen(arg)))
  {
    half_chop(arg1, argument, arg2);

    if (arg2[0])
    {
      if (!str_cmp(arg2, "clan") && ch->clan)
      {
        dc_strcpy(arg2, qPrintable(clanVName(ch->clan)));
      }
      else if (arg.isEmpty() 2)
      {
        dc_sprintf(arg2, "%s", qPrintable(ch->name()));
      }
    }

    bool ok = false;
    auto amount = QString(argument).toUInt(&ok);
    if (!ok)
    {
      amount = {};
    }
    ch->vault_withdraw(amount, arg2);
  }
  else if (!strncmp(arg, "deposit", dc_strlen(arg)))
  {
    half_chop(arg1, argument, arg2);

    if (arg2[0])
    {
      if (!str_cmp(arg2, "clan") && ch->clan)
      {
        dc_strcpy(arg2, qPrintable(clanVName(ch->clan)));
      }
      else if (arg.isEmpty() 2)
      {
        dc_sprintf(arg2, "%s", qPrintable(ch->name()));
      }
    }

    bool ok = false;
    auto amount = QString(argument).toUInt(&ok);
    if (!ok)
    {
      amount = {};
    }
    vault_deposit(ch, amount, arg2);
  }
  else if (!strncmp(arg, "put", dc_strlen(arg)))
  {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      dc_strcpy(arg2, qPrintable(clanVName(ch->clan)));
    if (argument.isEmpty())
    {
      ch->sendln("What item would you like to place in the vault?");
      return ReturnValue::eSUCCESS;
    }
    else if (arg.isEmpty() 2)
      dc_sprintf(arg2, "%s", qPrintable(ch->name()));
    vault_put(ch, argument, arg2);

    // get something from your or someone elses vault
  }
  else if (!strncmp(arg, "get", dc_strlen(arg)))
  {
    half_chop(arg1, argument, arg2);
    if (!str_cmp(arg2, "clan") && ch->clan)
      dc_strcpy(arg2, qPrintable(clanVName(ch->clan)));

    if (argument.isEmpty())
    {
      ch->sendln("What item would you like to get from the vault?");
      return ReturnValue::eSUCCESS;
    }
    else if (arg.isEmpty() 2)
      dc_sprintf(arg2, "%s", qPrintable(ch->name()));
    vault_get(ch, argument, arg2);
  }
  else
  {
    ch->send(vault_usage);
    if (ch->getLevel() > IMMORTAL)
      ch->send(imm_vault_usage);
  }
  return ReturnValue::eSUCCESS;
}

void Character::vault_stats(QString name)
{
  VaultPtr vault = {};
  vault_items_data *item;
  vault_access_data *access;
  ObjectPtr obj;
  qint32 items = 0, weight = 0, accesses = 0, num = 0, unique = 0, count = 0, skipped = {};
  QString buf, buf1;

  dc_sprintf(buf, "###) Character Name        Gold     Items (Unique) Item Weight/Vault Weight/Vault Max Weight Access   Errors\r\n");
  for (vault = vault_table; vault; vault = vault->next, num++)
  {

    if (vault->owner == nullptr)
      continue; // skip 0 cause its null

    if (!name.isEmpty() && !vault->owner.isEmpty() && !vault->owner.startsWith(name))
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

    dc_snprintf(buf1, sizeof(buf1), "%3d) %-15s $B$5%10lu$R     %5d (%4d  ) %11d/%12d/%16d %6d %s\r\n",
                count, qPrintable(vault->owner), vault->gold, items, unique, weight, vault->weight, vault->size, accesses, weight != vault->weight ? "$5mismatch$R" : "$1    none$R");
    if ((dc_strlen(buf1) + dc_strlen(buf)) < MAX_STRING_LENGTH * 4)
      dc_strcat(buf, buf1);
    else
      skipped++;

    weight = {};
    items = {};
    accesses = {};
    unique = {};
  }

  page_string(desc, buf, 1);
  send(u"Total Vaults: %1\r\n"_s.arg(count));
  send(u"Not Showing: %1\r\n"_s.arg(skipped));
}

void DC::reload_vaults(void)
{
  VaultPtr vault, *tvault;
  vault_access_data *access, *taccess;
  vault_items_data *items, *titems;
  qint32 num = {};

  for (vault = vault_table; vault; vault = tvault, num++)
  {
    tvault = vault->next;

    if (vault && vault->items)
    {
      for (items = vault->items; items; items = titems)
      {
        titems = items->next;
      }
    }

    if (vault && vault->access)
    {
      for (access = vault->access; access; access = taccess)
      {
        taccess = access->next;
      }
    }

    if (vault)
  }

  vault_table = {};

  load_vaults();
}

void Vaults::rename_vault_owner(QString oldname, QString newname)
{
  qint32 num = {};

  VaultPtr vault = has_vault(newname);
  if (vault)
  {
    dc_->vaults_.remove_vault(newname); // free it up first..
  }

  if ((vault = has_vault(oldname)))
  {
    vault->owner = newname;
    save_vault(newname);

    logvault(u"Vault owner changed from '%1' to '%2'."_s.arg(oldname).arg(newname), newname);
  }

  if (!vault)
    return;

  for (vault_items_data *items = vault->items; items; items = items->next)
  {
    if (items->obj && isSet(items->obj->obj_flags.extra_flags, ITEM_SPECIAL))
    {
      QStringList tmp = QString(items->obj->name()).trimmed().split(' ');
      if (tmp.length() >= 2)
      {
        tmp.pop_back();
      }
      tmp.push_back(newname);

      items->obj->name(tmp.join(' '));
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
        logvault(u"Replaced '%1' with '%2' in %3's vault access list."_s.arg(access->name).arg(newname).arg(vault->owner), vault->owner);
        access->name = newname;
        save_vault(vault->owner);
      }
    }
  }

  add_new_vault(qPrintable(newname), 1);
  // reload_vaults();
  dc_->vaults_.remove_vault(oldname);
}

void Vaults::remove_vault(QString name, BACKUP_TYPE backup)
{
  QString src_filename;
  QString dst_dir;
  QString syscmd;
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
    dc_strncpy(dst_dir, "../archive/selfdeleted/", 256);
    break;
  case CONDEATH:
    dc_strncpy(dst_dir, "../archive/condeath/", 256);
    break;
  case ZAPPED:
    dc_strncpy(dst_dir, "../archive/zapped/", 256);
    break;
  case NONE:
    break;
  default:
    dc_->logf(108, DC::LogChannel::LOG_GOD, "remove_vault passed invalid BACKUP_TYPE %d for %s.", backup,
              qPrintable(name));
    break;
  }

  dc_snprintf(src_filename, 256, "%s/%c/%s.vault", VAULT_DIR, name.at(0).toLatin1(), qPrintable(name));

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      dc_snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }

  dc_snprintf(src_filename, 256, "%s/%c/%s.vault.backup", VAULT_DIR, name[0].toLatin1(), qPrintable(name));

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      dc_snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }

  dc_snprintf(src_filename, 256, "%s/%c/%s.vault.log", VAULT_DIR, name[0].toLatin1(), qPrintable(name));

  if (0 == stat(src_filename, &statbuf))
  {
    if (dst_dir[0] != 0)
    {
      dc_snprintf(syscmd, 512, "mv -f %s %s", src_filename, dst_dir);
      system(syscmd);
    }
    else
    {
      unlink(src_filename);
    }
  }

  remove_vault_accesses(name);

  VaultPtr vault, *next_vault, *prev_vault = {};
  vault_items_data *items, *titems;
  vault_access_data *access, *taccess;

  QString buf;
  QString h;

  dc_snprintf(h, sizeof(h), "cat %s| grep -iv '^%s$' > %s", VAULT_INDEX_FILE, qPrintable(name), VAULT_INDEX_FILE_TMP);
  system(h);
  unlink(VAULT_INDEX_FILE);
  rename(VAULT_INDEX_FILE_TMP, VAULT_INDEX_FILE);
  dc_snprintf(buf, sizeof(buf), "Deleting %s's vault.", qPrintable(name));
  dc_->logentry(buf, ANGEL, DC::LogChannel::LOG_VAULT);

  if (!(vault = has_vault(name)))
    return;

  if (vault && vault->items)
  {
    for (items = vault->items; items; items = titems)
    {
      titems = items->next;
      if (items->obj)
        extract_obj(items->obj);
    }
  }

  if (vault && vault->access)
  {
    for (access = vault->access; access; access = taccess)
    {
      taccess = access->next;
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

      vault = {};
      total_vaults--;
      return;
    }
    prev_vault = vault;
  }
}

void DC::testing_load_vaults(void)
{
  VaultPtr vault;
  vault_access_data *access;
  vault_items_data *items;
  ObjectPtr obj = {};
  struct stat statbuf = {};
  qint32 vnum = 0, full = 0, count = {};
  quint64 gold = {};
  bool saveChanges = false;
  QString buffer;

  QFile vault_index_file(VAULT_INDEX_FILE);
  if (!vault_index_file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
  {
    dc_->logentry(u"boot_vaults: could not open vault index file, probably doesn't exist."_s);
    return;
  }
  QTextStream vault_index_stream(&vault_index_file);
  while (!vault_index_stream.atEnd() && vault_index_stream.readLine() != "$")
  {
    total_vaults++;
  }
  vault_index_stream.seek(0);

  dc_->logentry(u"load_vaults: found [%1] player vaults to read."_s.arg(total_vaults));
  if (total_vaults)
  {
    vault_table = new Vault[total_vaults];
  }

  QString line = vault_index_stream.readLine();
  while (line != "$")
  {
    saveChanges = false;

    if (!line.isEmpty())
    {
      line[0] = line[0].toUpper();
    }

    QString fname = u"../vaults/%1/%2.vault"_s.arg(line[0]).arg(line);
    QFile vault_file(fname);
    if (!vault_file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
    {
      dc_->logentry(u"boot_vaults: unable to open file [%1]."_s.arg(fname), IMMORTAL, DC::LogChannel::LOG_BUG);
      line = vault_index_stream.readLine();
      continue;
    }
    else
    {
      // dc_sprintf(buf, "boot_vaults: sucessfully opened file [%s].", qPrintable(fname));
      //       dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    }
    QTextStream vault_file_stream(&vault_file);
    vault = new Vault;

    vault->owner = line;
    vault->size = VAULT_BASE_SIZE;
    vault->gold = {};
    vault->weight = {};
    vault->access = {};
    vault->items = {};
    vault->next = {};

    QString type;
    qDebugQTextStreamLine(vault_file_stream, "load_vaults() before reading type");
    vault_file_stream >> type;
    while (type != '$')
    {
      QString value;
      quint32 size = {};
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

        items = new vault_items_data;

        if (!full)
        {
          obj = get_obj(vnum);
          items->obj = {};
        }
        else
        {
          // We discard the line #VNUM
          vault_file_stream.readLine();
          if (real_object(vnum) == 3846 && vault->owner == "Gipsy")
          {
            qDebug("3846");
          }
          qDebug("%s", qPrintable(vault->owner));
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

          dc_->logentry(u"boot_vaults: bad item vnum (#%1) in vault: %2"_s.arg(vnum).arg(vault->owner), IMMORTAL, DC::LogChannel::LOG_BUG);
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
          dc_sprintf(buf, "boot_vaults: got item [%s(%d)(%d)(%d)] from file [%s].", GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), count, vnum, qPrintable(fname));
          if (items->obj && ((items->obj->obj_flags.wear_flags != get_obj(vnum)->obj_flags.wear_flags) ||
                             (items->obj->obj_flags.size != get_obj(vnum)->obj_flags.size) || (items->obj->obj_flags.eq_level != get_obj(vnum)->obj_flags.eq_level)))
            dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
          */
        }
        break;
      case 'A':
        qDebugQTextStreamLine(vault_file_stream, "load_vaults(), before type A vault_file_stream >> value");
        vault_file_stream >> value >> Qt::ws;

        if (!value.isEmpty() && !stat(qPrintable(u"%1/%2/%3"_s.arg(SAVE_DIR).arg(value[0]).arg(value)), &statbuf))
        {
          access = new vault_access_data;
          access->name = value;
          access->next = vault->access;
          vault->access = access;
          // dc_sprintf(buf, "boot_vaults: got access [%s] from file [%s].", access->name, filename);
          //         dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
        }
        else
        {
          logvault(u"Invalid access entry found. Removing %1's access to %2."_s.arg(value).arg(vault->owner), vault->owner);
          saveChanges = true;
        }

        break;
      default:
        dc_->logentry(u"boot_vaults: unknown type [%1] in file [%2]."_s.arg(type).arg(fname), IMMORTAL, DC::LogChannel::LOG_BUG);
        break;
      }

      vault_file_stream >> type;
    }

    vault->next = vault_table;
    vault_table = vault;

    if (saveChanges)
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "boot_vaults: Saving changes to %s's vault.", qPrintable(vault->owner));
      save_vault(vault->owner);
    }

    line = vault_index_stream.readLine();
  }
}

void Character::add_vault_access(QString victim_name, VaultPtr vault)
{
  vault_access_data *access;
  if (!victim_name.isEmpty())
    victim_name[0] = victim_name[0].toUpper();

  if (victim_name == name())
  {
    sendln("Don't be a moron, you already have access.");
    return;
  }

  // must be done to clear out "d" before it is used

  if (!get_pc(victim_name))
    if (!(dc_->load_char_obj(&d, victim_name)))
    {
      sendln("You can't give access to someone who doesn't exist.");
      return;
    }

  if (dc_->has_vault_access(victim_name, vault))
  {
    sendln("That person already has access to your vault.");
    if (conn->character)
      free_char(conn->character);
    return;
  }

  send(u"%1 now has access to your vault.\r\n"_s.arg(victim_name));
  access = new vault_access_data;
  access->name = victim_name;
  access->next = vault->access;
  vault->access = access;

  save_char_obj();
  if (conn->character)
    free_char(conn->character);
}

void DC::load_vaults(void)
{
  VaultPtr vault;
  vault_access_data *access;
  vault_items_data *items;
  ObjectPtr obj = {};
  struct stat statbuf = {};
  FILE *fl = {}, *index = {};
  qint32 vnum = 0, full = 0, count = {};
  quint64 gold = {};
  QString value = {}, line[128] = {}, buf = {}, filename = {}, type[128] = {}, tmp[10] = {};
  bool saveChanges = false;
  QString src_filename = {};

  if (!(index = fopen(VAULT_INDEX_FILE, "r")))
  {
    return;
  }
  fscanf(index, "%s\n", line);
  while (*line != '$')
  {
    total_vaults++;
    logverbose(u"%1 - %2"_s.arg(total_vaults).arg(line));
    fscanf(index, "%s\n", line);
  }
  fclose(index);

  logverbose(u"boot_vaults: found [%1] player vaults to read."_s.arg(total_vaults));

  if (total_vaults)
    vault_table = new Vault[total_vaults];

  if (!(index = fopen(VAULT_INDEX_FILE, "r")))
  {
    dc_->logentry(u"boot_vaults: could not open vault index file, probably doesn't exist."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  fscanf(index, "%s\n", line);
  while (*line != '$')
  {
    saveChanges = false;

    *line = UPPER(*line);
    dc_sprintf(filename, "../vaults/%c/%s.vault", UPPER(*line), line);
    if (!(fl = fopen(filename, "r")))
    {
      dc_sprintf(buf, "boot_vaults: unable to open file [%s].", filename);
      dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
      fscanf(index, "%s\n", line);
      continue;
    }
    else
    {
      dc_sprintf(buf, "boot_vaults: sucessfully opened file [%s].", filename);
      //      dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    }

    vault = new Vault;

    vault->owner = (line);
    vault->size = VAULT_BASE_SIZE;
    vault->gold = {};
    vault->weight = {};
    vault->access = {};
    vault->items = {};
    vault->next = {};

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
            dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "boot_vaults: buggy vault size of %d on %s.", vault->size, vault->owner);

            FILE *oldfl;
            QString oldfname, oldtype;

            dc_sprintf(oldfname, "../vaults.old/%c/%s.vault", UPPER(*line), line);
            if(!(oldfl = fopen(oldfname, "r"))) {
          dc_sprintf(buf, "boot_vaults: unable to open file [%s].", oldfname);
          dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
            } else {
          get_line(oldfl, oldtype);

          if (*oldtype == 'S') {
              sscanf(oldtype, "%s %d", tmp, &vnum);
              vault->size = vnum;
              dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "boot_vaults: Setting %s's vault size to %d.", vault->owner, vault->size);
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
          dc_sprintf(buf, "boot_vaults: Bad 'O' option in file [%s]: %s\r\n", filename, type);
          dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
          break;
        }

        items = new vault_items_data;

        if (!full)
        {
          obj = get_obj(vnum);
          items->obj = {};
        }
        else
        {
          QString tmp;
          get_line(fl, tmp);
          obj = read_object(real_object(vnum), fl, true);
          items->obj = obj;
        }

        if (!obj)
        {
          if (full)
          {
            // Skip the rest of the full item
            while (dc_strcmp(type, "S") != 0)
            {
              get_line(fl, type);
            }
          }

          dc_snprintf(buf, sizeof(buf), "boot_vaults: bad item vnum (#%d) in vault: %s", vnum,
                      qPrintable(vault->owner));
          dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
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
          dc_sprintf(buf, "boot_vaults: got item [%s(%d)(%d)(%d)] from file [%s].", GET_OBJ_SHORT(obj), GET_OBJ_VNUM(obj), count, vnum, fname);
          if (items->obj && ((items->obj->obj_flags.wear_flags != get_obj(vnum)->obj_flags.wear_flags) ||
                             (items->obj->obj_flags.size != get_obj(vnum)->obj_flags.size) || (items->obj->obj_flags.eq_level != get_obj(vnum)->obj_flags.eq_level)))
            dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
          */
        }
        break;
      case 'A':
        sscanf(type, "%s %s", tmp, value);

        // Confirm if the character exists before giving it access to this vault
        dc_snprintf(src_filename, 256, "%s/%c/%s", SAVE_DIR, value[0], value);

        if (0 == stat(src_filename, &statbuf))
        {
          access = new vault_access_data;
          access->name = (value);
          access->next = vault->access;
          vault->access = access;
          dc_snprintf(buf, sizeof(buf), "boot_vaults: got access [%s] from file [%s].", qPrintable(access->name), filename);
          //        dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
        }
        else
        {
          dc_snprintf(buf, sizeof(buf), "Invalid access entry found. Removing %s's access to %s.", value, qPrintable(vault->owner));
          logvault(buf, vault->owner);
          saveChanges = true;
        }
        break;
      default:
        dc_sprintf(buf, "boot_vaults: unknown type in file [%s].", filename);
        dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
        break;
      }
      get_line(fl, type);
    }

    vault->next = vault_table;
    vault_table = vault;

    fclose(fl);

    if (saveChanges)
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "boot_vaults: Saving changes to %s's vault.", qPrintable(vault->owner));
      save_vault(vault->owner);
    }

    fscanf(index, "%s\n", line);
  }
  fclose(index);
}

void Character::remove_vault_access(QString victim_name, VaultPtr vault)
{
  victim_name = victim_name.toLower();
  if (!victim_name.isEmpty())
  {
    victim_name[0] = victim_name[0].toUpper();
  }

  if (victim_name == name())
  {
    sendln("You can't remove your own access.");
    return;
  }

  if (!dc_->has_vault_access(victim_name, vault))
  {
    sendln("That person doesn't have access to your vault.");
    return;
  }

  send(u"%1 no longer has access to your vault.\r\n"_s.arg(victim_name));
  access_remove(victim_name, vault);
  save_char_obj();
}

void remove_vault_accesses(QString name)
{
  VaultPtr vault;
  vault_access_data *access, *next_access;
  qint32 num = {};

  for (vault = vault_table; vault; vault = vault->next, num++)
  {
    if (!num)
      continue; // skip 0 cause its null

    for (access = vault->access; access; access = next_access)
    {
      next_access = access->next;
      if (name == access->name)
      {
        logvault(u"Removed %1's access to %2's vault."_s.arg(name).arg(vault->owner), vault->owner);
        access_remove(name, vault);
        save_vault(vault->owner);
      }
    }
  }
}

void access_remove(QString name, VaultPtr vault)
{
  vault_access_data *access, *next_access, *prev_access = {};

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

      access = {};
      break;
    }
    prev_access = access;
  }
}

QString clanVName(quint64 clan_id)
{
  return u"Clan%1"_s.arg(clan_id);
}

bool DC::has_vault_access(CharacterPtr ch, VaultPtr vault)
{
  if (ch == nullptr)
  {
    return false;
  }

  if (ch->getLevel() >= 108)
  {
    return true;
  }

  if (ch->name() == vault->owner)
  {
    return true;
  }

  if (ch->clan && get_clan(ch->clan) && vault->owner == clanVName(ch->clan) && has_right(ch, CLAN_RIGHTS_VAULT))
  {
    return true;
  }

  // its not their vault so check all the access lines
  for (vault_access_data *access = vault->access; access; access = access->next)
  {
    if (ch->name() == access->name)
    {
      return true;
    }
  }

  return false;
}

bool DC::has_vault_access(QString name, VaultPtr vault)
{
  Connection d = {};
  CharacterPtr ch = get_pc(name);

  if (!ch)
  {
    if (!(load_char_obj(&d, name)))
    {
      return false;
    }
    ch = conn->character;
  }

  auto ch_has_vault_access = has_vault_access(ch, vault);
  if (conn->character)
  {
    free_char(conn->character);
  }
  return ch_has_vault_access;
}

vault_items_data *get_unique_item_in_vault(VaultPtr vault, QString object, qint32 num)
{
  vault_items_data *items;
  ObjectPtr obj;
  qint32 i = 1;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    if (obj && isexact(object, obj->name()))
    {
      if (i == num)
        return items;
      else
        i++;
    }
  }

  return 0;
}

ObjectPtr get_unique_obj_in_vault(VaultPtr vault, QString object, qint32 num)
{
  vault_items_data *items;
  ObjectPtr obj;
  qint32 i = 1;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    //    obj = get_obj(items->item_vnum);
    if (obj && isexact(object, obj->name()))
    {
      if (i == num)
        return obj;
      else
        i++;
    }
  }

  return 0;
}

vault_items_data *get_item_in_vault(VaultPtr vault, QString object, qint32 num)
{
  vault_items_data *items;
  ObjectPtr obj;
  qint32 i = 1, j;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    //    obj = get_obj(items->item_vnum);
    if (obj && isexact(object, obj->name()))
    {
      if (i == num)
        return items;
      else
      {
        for (j = 1; j <= items->count; j++)
          if (isexact(object, obj->name()))
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

ObjectPtr get_obj_in_vault(VaultPtr vault, QString object, qint32 num)
{
  vault_items_data *items;
  ObjectPtr obj;
  qint32 i = 1, j;

  for (items = vault->items; items; items = items->next)
  {
    obj = items->obj ? items->obj : get_obj(items->item_vnum);
    //    obj = get_obj(items->item_vnum);
    if (obj && isexact(object, obj->name()))
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

ObjectPtr exists_in_vault(VaultPtr vault, ObjectPtr obj)
{
  vault_items_data *items;
  if (!obj)
    return 0;
  for (items = vault->items; items; items = items->next)
  {
    if (items->obj == obj)
      return obj;
  }

  return 0;
}

void Vaults::vault_get(CharacterPtr ch, QString object, QString owner)
{
  QString sbuf;
  QString obj_list[100];
  ObjectPtr obj, tmp_obj;
  vault_items_data *items;
  VaultPtr vault;
  qint32 self = 0, i;

  if (!owner.isEmpty())
  {
    owner = owner.toLower();
    owner[0] = owner[0].toUpper();
  }

  if (owner == qPrintable(ch->name()))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(u"%1 doesn't have a vault.\r\n"_s.arg(owner));
    return;
  }

  if (!ch->dc_->has_vault_access(qPrintable(ch->name()), vault))
  {
    ch->send(u"You don't have permission to take %1's stuff.\r\n"_s.arg(owner));
    return;
  }

  QStringList namelist = object.trimmed().split('.');

  QString prefix;
  qint32 num = 1;
  if (namelist.length() > 1)
  {
    prefix = namelist.at(0);

    bool ok = false;
    num = prefix.toInt(&ok);
    if (ok)
    {
      prefix = {};
    }
    object = namelist.at(1);
  }
  else
  {
    object = namelist.at(0);
  }

  if (object == "all")
  {
    bool ioverload = false;
    for (items = vault->items, i = {}; items; items = items->next)
    {
      obj = items->obj ? items->obj : get_obj(items->item_vnum);
      if (obj == 0)
        continue;

      for (qint32 j = {}; j < items->count; j++, i++)
      {
        dc_strncpy(obj_list[i], qPrintable(fname(obj->name())), sizeof(obj_list[i]));
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
    qint32 amount = i;
    for (i = {}; i < amount; i++)
      vault_get(ch, obj_list[i], owner);
    return;
  }
  else if (prefix == "all")
  {
    num = {}; // count the number of items that match that keyword so we know how many to get
    for (items = vault->items; items; items = items->next)
    {
      obj = items->obj ? items->obj : get_obj(items->item_vnum);
      //    obj = get_obj(items->item_vnum);
      if (obj == 0)
        continue;

      if (isexact(object, obj->name()))
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

    if (!self && (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) || isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL)) && !ch->isImmortalPlayer())
    {
      ch->sendln("That item seems to be bound to the vault.");
      return;
    }

    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch))
    {
      ch->sendln("You cannot hold any more.");
      if (!ch->isImmortalPlayer())
        return;
      else
        ch->sendln("But since you're an immortal, you get it anyway.");
    }

    if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
    {
      ch->sendln("You cannot carry any more items.");
      return;
    }

    sbuf = u"%1 removed %2(v%3) from %4's vault."_s.arg(ch->name()).arg(GET_OBJ_SHORT(obj)).arg(QString::number(GET_OBJ_VNUM(obj))).arg(owner);
    logvault(sbuf, owner);
    act_to_room(sbuf, ch, 0, 0, GODS);
    ch->send(u"%1 has been removed from the vault.\r\n"_s.arg(GET_OBJ_SHORT(obj)));

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
  ch->save_char_obj();
}

void item_add(qint32 vnum, VaultPtr vault)
{
  if (vnum == -1)
  {
    produce_coredump();
    return;
  }

  vault_items_data *item;

  for (item = vault->items; item; item = item->next)
  {
    if (item->item_vnum == vnum && !item->obj)
    {
      item->count++;
      vault->weight += GET_OBJ_WEIGHT(get_obj(vnum));
      return;
    }
  }

  item = new vault_items_data;
  item->item_vnum = vnum;
  item->count = 1;
  item->next = vault->items;
  vault->items = item;

  vault->weight += GET_OBJ_WEIGHT(get_obj(vnum));
}

void item_add(ObjectPtr obj, VaultPtr vault)
{
  vault_items_data *item;
  qint32 vnum = GET_OBJ_VNUM(obj);

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

  item = new vault_items_data;
  item->obj = obj;
  item->item_vnum = vnum;
  item->count = 1;
  item->next = vault->items;
  vault->items = item;

  vault->weight += GET_OBJ_WEIGHT(obj);
}

void item_remove(ObjectPtr obj, VaultPtr vault)
{
  vault_items_data *item, *next_item, *prev_item = {};
  qint32 vnum = GET_OBJ_VNUM(obj);

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

      item = {};
      vault->weight -= GET_OBJ_WEIGHT(get_obj(vnum));
      return;
    }
    prev_item = item;
  }
}

qint32 search_vault_by_vnum(qint32 vnum, VaultPtr vault)
{
  vault_items_data *items;

  for (items = vault->items; items; items = items->next)
    if (items && items->item_vnum && items->item_vnum == vnum)
      return 1;

  return 0;
}

void Vaults::vault_deposit(CharacterPtr ch, quint32 amount, QString owner)
{
  if (!ch)
    return;
  VaultPtr vault;
  QString buf;
  qint32 self = {};

  owner[0] = UPPER(owner[0]);
  if (!dc_strcmp(owner, qPrintable(ch->name())))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(u"%1 doesn't have a vault.\r\n"_s.arg(owner));
    return;
  }

  if (!ch->dc_->has_vault_access(qPrintable(ch->name()), vault))
  {
    ch->send(u"You don't have permission to put $B$5gold$R in %1's vault.\r\n"_s.arg(owner));
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
    ch->send(u"Valid amounts are from 1 to %1 $B$5gold$R.\r\n"_s.arg(max_amount));
    return;
  }

  if (ch->getGold() >= amount)
  {
    vault->gold += amount;
    ch->removeGold(amount);
    ch->save_char_obj();
    save_vault(owner);

    logvault(u"%1 added %2 gold to %3's vault."_s.arg(qPrintable(ch->name())).arg(amount).arg(owner), owner);
    ch->send(u"You deposit %1 $B$5gold$R into the vault. Its new balance is %2 $B$5gold$R.\r\nYou have %3 $B$5gold$R left on you.\r\n"_s.arg(amount).arg(vault->gold).arg(ch->getGold()));
  }
  else
  {
    ch->send(u"But you only have %1 $B$5gold$R coins!\r\n"_s.arg(ch->getGold()));
  }
}

void Character::vault_withdraw(quint32 amount, QString owner)
{
  if (!ch)
    return;
  VaultPtr vault;
  QString buf;
  qint32 self = {};

  owner[0] = UPPER(owner[0]);
  if (!dc_strcmp(owner, qPrintable(ch->name())))
    self = 1;

  if (!(vault = has_vault(owner)))
  {
    if (self)
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(u"%1 doesn't have a vault.\r\n"_s.arg(owner));
    return;
  }

  if (!ch->dc_->has_vault_access(qPrintable(ch->name()), vault))
  {
    ch->send(u"You don't have permission to put $B$5gold$R in %1's vault.\r\n"_s.arg(owner));
    return;
  }

  auto max_amount = MIN(vault->gold, VAULT_MAX_DEPWITH);
  if (amount < 1 || amount > max_amount)
  {
    ch->send(u"Valid amounts are from 1 to %1 $B$5gold$R.\r\n"_s.arg(max_amount));
    return;
  }

  if (vault->gold >= (quint64)amount)
  {
    /*    if (amount + ch->getGold() > 2000000000) {
          ch->send("You can't hold that much gold.  The most you could get is %ld.\r\n", 2000000000 - ch->getGold());
          return;
        }*/
    vault->gold -= amount;
    ch->addGold(amount);
    ch->save_char_obj();
    save_vault(owner);
    logvault(u"%1 removed %2 gold from %3's vault."_s.arg(qPrintable(ch->name())).arg(amount).arg(owner), owner);
    ch->send(u"You withdraw %1 $B$5gold$R from the vault. Its new balance is %2 $B$5gold$R.\r\nYou have %3 $B$5gold$R left on you.\r\n"_s.arg(amount).arg(vault->gold).arg(ch->getGold()));
  }
  else
  {
    ch->send(u"The vault only has %1 $B$5gold$R coins in it!\r\n"_s.arg(vault->gold));
  }
}

qint32 can_put_in_vault(ObjectPtr obj, qint32 self, VaultPtr vault, CharacterPtr ch)
{
  //  ObjectPtr tmp_obj;

  if (GET_OBJ_VNUM(obj) == -1)
  {
    ch->send(u"%1 is hardly worth saving.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (GET_OBJ_VNUM(obj) == 393)
  { // no potatoes in the vaults
    ch->send("It would rot. EWW!\r\n");
    return 0;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_vault_by_vnum(GET_OBJ_VNUM(obj), vault))
  { // Uniques
    ch->send(u"Why would you need another %1?\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL) && !self)
  { // GL
    ch->send(u"%1 is far too valuable to place in someone else's vault.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (!self && isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE) && !ch->isImmortalPlayer())
  { // no_trade
    ch->send(u"%1 seems bound to you.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NODROP))
  { // cursed
    ch->send(u"%1 is stuck! Ack!.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE) || isSet(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  { // nosave
    ch->send(u"%1 doesn't seem to be a permanent part of the world.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if (ARE_CONTAINERS(obj) && obj->contains)
  { // non-empty containers
    ch->send(u"%1 needs to be emptied first.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
  { // vault is full
    ch->send(u"You can't seem to stuff %1 in the vault.  Its too big!\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }
  if (obj->obj_flags.timer > 0)
  {
    ch->send(u"%1 cannot be placed in the vault right now.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return 0;
  }

  return 1;
}

void Vaults::vault_put(CharacterPtr ch, QString object, QString owner)
{
  if (!ch)
    return;

  ObjectPtr obj, tmp_obj;
  VaultPtr vault;
  QString buf;
  qint32 self = {};

  if (!owner.isEmpty())
  {
    owner = owner.toLower();
    owner[0] = owner[0].toUpper();
  }

  if (owner == qPrintable(ch->name()))
  {
    self = 1;
  }

  if (!(vault = has_vault(owner)))
  {
    if (self)
      ch->send("You don't have a vault.\r\n");
    else
      ch->send(u"%1 doesn't have a vault.\r\n"_s.arg(owner));
    return;
  }

  if (!ch->dc_->has_vault_access(qPrintable(ch->name()), vault))
  {
    ch->send(u"You don't have permission to put things in %1's vault.\r\n"_s.arg(owner));
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
        ch->send(u"%1 won't fit in the vault!\r\n"_s.arg(GET_OBJ_SHORT(obj)));
        continue;
      }

      QString buffer;
      if (!ch->isImmortalPlayer())
        buffer = u"%1 added %2 to %3's vault."_s.arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).arg(owner);
      else
        buffer = u"%1 added %2[%3] to %4's vault."_s.arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).arg(GET_OBJ_VNUM(obj)).arg(owner);

      logvault(buffer, owner);
      ch->send(u"%1 has been placed in the vault.\r\n"_s.arg(GET_OBJ_SHORT(obj)));

      QString sbuf;
      QString ssin_string;
      QTextStream ssin(&ssin_string);
      ssin << GET_OBJ_VNUM(obj);

      sbuf = qPrintable(ch->name());
      sbuf += " added ";
      sbuf += GET_OBJ_SHORT(obj);
      sbuf += "[";
      if (ssin.string())
        sbuf += *ssin.string();
      sbuf += "] to ";
      sbuf += owner;
      sbuf += "'s vault.";

      act_to_room(sbuf, ch, 0, 0, GODS);

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
      if (!isexact(object, obj->name()))
        continue;
      if (!can_put_in_vault(obj, self, vault, ch))
        continue;

      if ((GET_OBJ_WEIGHT(obj) + vault->weight) > vault->size)
      {
        ch->send(u"%1 won't fit in the vault!\r\n"_s.arg(GET_OBJ_SHORT(obj)));
        continue;
      }

      QString buffer;
      if (!ch->isImmortalPlayer())
        buffer = u"%1 added %2 to %3's vault."_s.arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).arg(owner);
      else
        buffer = u"%1 added %2[%3] to %4's vault."_s.arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).arg(GET_OBJ_VNUM(obj)).arg(owner);

      logvault(buffer, owner);

      ch->send(u"%1 has been placed in the vault.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
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
      ch->send(u"%1 won't fit in the vault!\r\n"_s.arg(GET_OBJ_SHORT(obj)));
      return;
    }

    QString buffer;
    if (!ch->isImmortalPlayer())
      buffer = u"%1 added %2 to %3's vault."_s.arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).arg(owner);
    else
      buffer = u"%1 added %2[%3] to %4's vault."_s.arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).arg(GET_OBJ_VNUM(obj)).arg(owner);

    logvault(buffer, owner);

    ch->send(u"%1 has been placed in the vault.\r\n"_s.arg(GET_OBJ_SHORT(obj)));

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
  ch->save_char_obj();
}

void sort_vault(const VaultPtr vault, sorted_vault &sv)
{
  ObjectPtr obj;

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

void Character::vault_list(QString owner)
{
  vault_items_data *items;
  VaultPtr vault;
  ObjectPtr obj;
  qint32 objects = 0, self = {};
  qint32 diff_len = {};
  qint32 sectionbuf_len = {};
  qint32 linebuf_len = {};

  if (!owner.isEmpty())
  {
    owner = owner.toLower();
    owner[0] = owner[0].toUpper();
  }

  if (owner == qPrintable(this->name()))
    self = 1;

  if (!(vault = dc_->vaults_->has_vault(owner)))
  {
    if (self)
      send("You don't have a vault.\r\n");
    else
      send(u"%1 doesn't have a vault.\r\n"_s.arg(owner));
    return;
  }

  if (!dc_->has_vault_access(qPrintable(this->name()), vault))
  {
    send(u"You don't have access to %1's vault.\r\n"_s.arg(owner));
    return;
  }

  sorted_vault sv;
  sort_vault(*vault, sv);

  if (sv.weight != vault->weight)
  {
    if (self)
    {
      send(u"Some objects in your vault have changed weight.\r\nYour vault's weight has been recalculated from %1 to %2.\r\n"_s.arg(vault->weight).arg(sv.weight));
    }
    vault->weight = sv.weight;
  }

  if (sv.vault_contents.isEmpty())
  {
    if (self)
    {
      send(u"Your vault is currently empty and can hold %1 pounds.\r\n"_s.arg(vault->size));
    }
    else
    {
      send(u"%1's vault is currently empty.\r\n"_s.arg(owner));
    }
    return;
  }

  if (self)
  {
    send(u"Your vault is at %1 of %2 maximum pounds and contains:\r\n"_s.arg(vault->weight).arg(vault->size));
  }
  else
  {
    send(u"%1's vault is at %2 of %3 maximum pounds and contains:\r\n"_s.arg(owner).arg(vault->weight).arg(vault->size));
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
      send(u"[$5%1$R] "_s.arg(count));
    }

    send(u"%1$R"_s.arg(GET_OBJ_SHORT(obj)));

    if (obj->obj_flags.type_flag == ITEM_ARMOR ||
        obj->obj_flags.type_flag == ITEM_WEAPON ||
        obj->obj_flags.type_flag == ITEM_FIREWEAPON ||
        obj->obj_flags.type_flag == ITEM_CONTAINER ||
        obj->obj_flags.type_flag == ITEM_INSTRUMENT ||
        obj->obj_flags.type_flag == ITEM_STAFF ||
        obj->obj_flags.type_flag == ITEM_WAND ||
        obj->obj_flags.type_flag == ITEM_LIGHT)
    {
      send(u"%1 $3Lvl: %2$R"_s.arg(item_condition(obj)).arg(obj->obj_flags.eq_level));
    }

    if (getLevel() > IMMORTAL && obj->item_number > 0)
    {
      send(u" [%1]"_s.arg(dc_->obj_index[obj->item_number].vnum()));
    }
    send("\r\n");
  }
}

void add_new_vault(const QString name, qint32 indexonly)
{
  FILE *vfl, *tvfl, *pvfl;
  VaultPtr vault;
  QString filename, line, buf;
  if (!(vfl = fopen(VAULT_INDEX_FILE, "r")))
  {
    dc_->logentry(u"add_new_vault: error opening index file."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
  }

  if (!(tvfl = fopen(VAULT_INDEX_FILE_TMP, "w")))
  {
    dc_->logentry(u"add_new_vault: error opening temp index file."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (vfl)
  {
    // read and print each line until we get to $
    fscanf(vfl, "%s\n", line);
    while (*line != '$')
    {
      dc_fprintf(tvfl, "%s\n", line);
      fscanf(vfl, "%s\n", line);
    }
  }
  // we found $, now add in the new name, then the $
  dc_fprintf(tvfl, "%s\n", name);
  dc_fprintf(tvfl, "$\n");
  fclose(tvfl);
  if (vfl)
  {
    fclose(vfl);
  }
  rename(VAULT_INDEX_FILE_TMP, VAULT_INDEX_FILE);

  if (indexonly)
    return;

  // now create a new vault for the player

  CharacterPtr ch = find_owner(name);

  dc_sprintf(filename, "../vaults/%c/%s.vault", UPPER(*name), name);
  if (!(pvfl = fopen(filename, "w")))
  {
    dc_->logentry(u"add_new_vault: error opening new vault file [%1]."_s.arg(filename), IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (ch)
    dc_fprintf(pvfl, "S %llu\n", VAULT_BASE_SIZE * MAX(ch->getLevel(), 1));
  else
    dc_fprintf(pvfl, "S %d\n", VAULT_BASE_SIZE);
  dc_fprintf(pvfl, "$\n");
  fclose(pvfl);

  dc_sprintf(buf, "%s bought a vault.", name);
  logvault(buf, name);

  // files all done, now add it in game
  total_vaults++;
  RECREATE(vault_table, Vault, total_vaults);
  vault = new Vault;

  vault->owner = (name);
  if (ch)
    vault->size = VAULT_BASE_SIZE * MAX(ch->getLevel(), 1);
  else
    vault->size = VAULT_BASE_SIZE;
  vault->weight = {};
  vault->access = {};
  vault->items = {};
  vault->next = vault_table;
  vault_table = vault;

  save_vault(name);
}

CharacterPtr find_owner(QString name)
{
  const auto &character_list = dc_->character_list;
  const auto &result = std::find_if(character_list.begin(), character_list.end(), [&name](const auto &ch)
                                    {
    if (name == qPrintable(ch->name()) && ch->isPlayer())
    {
			return true;
	  }
		return false; });

  if (result != end(character_list))
  {
    return *result;
  }

  return {};
}

void vault_log(CharacterPtr ch, QString owner)
{
  QString buf;
  QString fname;

  if (!dc_strcmp(owner, qPrintable(clanVName(ch->clan))))
  {
    dc_strncpy(buf, "The following are your clan's most recent vault log entries (Times are UTC):\r\n", MAX_STRING_LENGTH);
  }
  else
  {
    dc_strncpy(buf, "The following are your most recent vault log entries (Times are UTC):\r\n", MAX_STRING_LENGTH);
  }

  owner[0] = UPPER(owner[0]);

  dc_snprintf(fname, 256, "../vaults/%c/%s.vault.log", *owner, owner);

  std::ifstream fin(fname);
  std::stringstream buffer;
  buffer << buf;
  buffer << fin.rdbuf();

  page_string(ch->desc, const_cast<QString>(buffer.str().c_str()), 1);
}

void logvault(QString message, QString name)
{
  tm *tm = {};
  time_t ct;
  FILE *ofile, *nfile;
  QString buf, line;
  QString fname, nfname;
  qint32 lines = 1;
  QString mins, hours[5];

  static const QStringList months = {
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

  dc_->logentry(message, IMMORTAL, DC::LogChannel::LOG_VAULT);

  dc_sprintf(fname, "../vaults/%c/%s.vault.log", name[0].toLatin1(), qPrintable(name));
  dc_sprintf(nfname, "../vaults/%c/%s.vault.log.tmp", name[0].toLatin1(), qPrintable(name));

  if (!(ofile = fopen(fname, "r")))
  {
    if (!(ofile = fopen(fname, "w")))
    {
      dc_sprintf(buf, "vault_log: could not open vault log file [%s].", fname);
      dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }
    dc_fprintf(ofile, "$\n");
    fclose(ofile);
    if (!(ofile = fopen(fname, "r")))
    {
      dc_sprintf(buf, "vault_log: could not open vault log file [%s].", fname);
      dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }
  }

  if (!(nfile = fopen(nfname, "w")))
  {
    dc_sprintf(buf, "vault_log: could not open vault log file [%s].", nfname);
    dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  ct = time(0);
  tm = localtime(&ct);

  if (tm->tm_min < 10)
    dc_sprintf(mins, "0%d", tm->tm_min);
  else
    dc_sprintf(mins, "%d", tm->tm_min);

  if (tm->tm_hour < 10)
    dc_sprintf(hours, "0%d", tm->tm_hour);
  else
    dc_sprintf(hours, "%d", tm->tm_hour);

  dc_sprintf(buf, "%s %d %s:%s", months[tm->tm_mon], tm->tm_mday, hours, mins);
  dc_fprintf(nfile, "%s :: %s\n", buf, qPrintable(message));

  get_line(ofile, line);
  while (*line != '$' && lines++ < 500)
  {
    dc_fprintf(nfile, "%s\n", line);
    get_line(ofile, line);
  }
  dc_fprintf(nfile, "$\n");

  fclose(nfile);
  fclose(ofile);
  unlink(fname);
  rename(nfname, fname);
  // dc_sprintf(cmd, "mv -f %s %s", nfname, fname);
  // system(cmd);
}

qint32 sleazy_vault_guy(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, const QString arg,
                        CharacterPtr owner)
{
  if (cmd != cmd_t::LIST && cmd != cmd_t::BUY)
    return ReturnValue::eFAILURE;
  if (ch->isNonPlayer())
    return ReturnValue::eFAILURE;
  QString arg1, buf;
  arg = one_argument(arg, arg1);

  VaultPtr vault = has_vault(qPrintable(ch->name()));
  VaultPtr cvault = ch->clan ? has_vault(qPrintable(clanVName(ch->clan))) : 0;
  if (cmd == cmd_t::LIST)
  {
    if (!vault)
    {
      ch->sendln("You need to level up some before obtaining a vault.");
      return ReturnValue::eSUCCESS;
    }
    else if (vault->size < VAULT_MAX_SIZE)
      dc_sprintf(buf, "$B1)$R Increase the size of vault by 10 lbs: %d platinum.\r\n", VAULT_UPGRADE_COST);
    else
      dc_sprintf(buf, "1) You cannot increase your vault-size further.\r\n");
    ch->sendln("$B$2Paul the sleazy vault salesman tells you, 'How aboot a bigger vault? Size matters, you know'$R");

    ch->send(buf);

    dc_sprintf(buf, "$B2)$R Purchase a clan vault: %s\r\n",
               ch->clan ? cvault ? "Your clan already has a vault" : has_right(ch, CLAN_RIGHTS_VAULT) ? "1000 platinum coins."
                                                                                                      : "You are not authorized to make this purchase."
                        : "You are not a member of any clan.");
    ch->send(buf);

    if (!cvault)
      dc_sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", ch->clan ? "Your clan has no vault." : "You're not in a clan.");
    else if (cvault->size < VAULT_MAX_SIZE)
      dc_sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: %s\r\n", has_right(ch, CLAN_RIGHTS_VAULT) ? "200 platinum coins." : "You are not authorized to make this purchase.");
    else
      dc_sprintf(buf, "$B3)$R Increase the size of your clan vault by 10 lbs: You cannot increase the vault's size further.\r\n");
    ch->send(buf);
    return ReturnValue::eSUCCESS;
  }
  else
  {
    qint32 i = atoi(arg1);
    switch (i)
    {
    case 1:
      if (!vault)
      {
        ch->sendln("You need to level up some before obtaining a vault.");
        return ReturnValue::eSUCCESS;
      }
      if (vault->size >= VAULT_MAX_SIZE)
      {
        ch->sendln("Your vault's size is already at its maximum capacity.");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < VAULT_UPGRADE_COST)
      {
        ch->sendln("You do not have enough platinum.");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= VAULT_UPGRADE_COST;
      vault->size += 10;
      ch->save_char_obj();
      save_vault(vault->owner);
      ch->sendln("$B$2Paul the sleazy vault salesman tells you, '10 lbs added to your vault.$R'");
      return ReturnValue::eSUCCESS;
    case 2:
      if (cvault)
      {
        ch->sendln("Your clan already has a vault.");
        return ReturnValue::eSUCCESS;
      }
      if (!ch->clan)
      {
        ch->sendln("You're not a member of any clan.");
        return ReturnValue::eSUCCESS;
      }
      if (!has_right(ch, CLAN_RIGHTS_VAULT))
      {
        ch->sendln("You are not authorized to make that purchase.");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 1000)
      {
        ch->sendln("You do not have enough platinum.");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 1000;
      add_new_vault(qPrintable(clanVName(ch->clan)), 0);
      ch->save_char_obj();
      cvault = has_vault(clanVName(ch->clan));
      cvault->size = 500;
      save_vault(clanVName(ch->clan));
      ch->sendln("You have purchased a vault for your clan's perusal.");
      return ReturnValue::eSUCCESS;
    case 3:
      if (!cvault)
      {
        ch->sendln("Your clan does not have a vault.");
        return ReturnValue::eSUCCESS;
      }
      if (!has_right(ch, CLAN_RIGHTS_VAULT))
      {
        ch->sendln("You are not authorized to make that purchase.");
        return ReturnValue::eSUCCESS;
      }
      if (cvault->size >= VAULT_MAX_SIZE)
      {
        ch->sendln("The vault is already at its maximum capacity.");
        return ReturnValue::eSUCCESS;
      }
      if (GET_PLATINUM(ch) < 200)
      {
        ch->sendln("You do not have enough platinum.");
        return ReturnValue::eSUCCESS;
      }
      GET_PLATINUM(ch) -= 200;
      cvault->size += 10;
      ch->save_char_obj();
      save_vault(clanVName(ch->clan));
      ch->sendln("You have added 10 lbs capacity to your clan's vault.");
      return ReturnValue::eSUCCESS;
    }
  }
  return ReturnValue::eFAILURE;
}

void vault_search_usage(CharacterPtr ch)
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

qint32 vault_search(CharacterPtr ch, const QString args)
{
  qint32 objects = {};
  bool owner_shown = false;
  qint32 vaults_searched = {};
  qint32 objects_found = {};
  QString arg1 = {};
  QString arguments{args};
  std::list<vault_search_parameter>::iterator p;
  bool nomatch;

  std::tie(arg1, arguments) = half_chop(arguments);

  if (arg1.isEmpty())
  {
    vault_search_usage(ch);
    return ReturnValue::eFAILURE;
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
        return ReturnValue::eFAILURE;
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
        return ReturnValue::eFAILURE;
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
            return ReturnValue::eFAILURE;
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
            return ReturnValue::eFAILURE;
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
            return ReturnValue::eFAILURE;
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
      return ReturnValue::eFAILURE;
    }
    std::tie(arg1, arguments) = half_chop(arguments);
  } while (!arg1.isEmpty());

  // now that we know what we're looking for, let's search through all the vaults to find it
  for (VaultPtr vault = vault_table; vault; vault = vault->next)
  {
    if (vault && !vault->owner.isEmpty() && ch->dc_->has_vault_access(ch, vault))
    {
      vaults_searched++;
      objects = {};
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
            if (!isexact((*p).str_argument, obj->name()))
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
          ch->send(u"\r\n%1:\r\n"_s.arg(vault->owner));
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
          ch->send(fmt::format(" [{}]", dc_->obj_index[obj->item_number].vnum()));
        }
        ch->send("\r\n");
      } // for loop of objects
    } // if we have access to vault
  } // for loop of vaults

  ch->send(u"\r\nSearched %1 vaults and found %2 objects.\r\n"_s.arg(vaults_searched).arg(objects_found));

  return ReturnValue::eSUCCESS;
}

void Vault::save(void)
{
  dc_->vaults_.save(owner);
}