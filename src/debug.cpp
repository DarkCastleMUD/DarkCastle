#include <unistd.h>

#include <iostream>
#include <QMap>
#include <filesystem>
#include <QQueue>
#include <cassert>

#include <QtSql/QSqlRelationalTableModel>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlField>
#include <QtSql/QSqlError>

#include "DC/db.h"
#include "DC/DC.h"
#include "DC/const.h"

#include "DC/DC.h"
#include "DC/interp.h"

void load_char_obj_error(FILE *fpsave, QString strsave);
void store_to_char(char_file_u4 *st, CharacterPtr ch);
qint32 store_to_char_variable_data(CharacterPtr ch, FILE *fpsave);
ObjectPtr my_obj_store_to_char(CharacterPtr ch, FILE *fpsave, ObjectPtr last_cont);
qsizetype fread_to_tilde(FILE *fpsave, QString filename);
bool read_pc_or_mob_data(CharacterPtr ch, FILE *fpsave, QString filename);
void load_vaults();

extern VaultPtr vault_table;
extern Leaderboard leaderboard;

bool verbose_mode = false;

void test_handle_ansi(QString test)
{
  // std::cerr <<  "Testing '" << test << "'" << std::endl;
  CharacterPtr ch = new Character(DC::getInstance());
  ch->player = new Player;
  ch->setType(Character::Type::Player);
  SET_BIT(ch->player->toggles, Player::PLR_ANSI);
  ch->setLevel(1);

  // QString str1 = "$b$B$1test$R $ $$ $$$ $$$";
  QString str1 = test;
  QString str2;
  memset(str2, 1024, 0);
  dc_strncpy(str2, str1.toStdString().c_str(), 1024);
  QString result1 = handle_ansi(str1, ch);
  QString result2 = QString(handle_ansi_(str2, ch));
  // std::cerr <<  "Result1: [" << result1 << "]" << std::endl;
  // std::cerr <<  "Result2: [" << result2 << "]" << std::endl;
  assert(handle_ansi(str1, ch) == QString(handle_ansi_(str2, ch)));
}

bool test_rolls(quint8 total)
{
  qint32 x, a, b;
  stat_data stats;

  quint64 attempts = {};
  while (1)
  {
    attempts++;
    for (x = {}; x <= 4; x++)
    {
      a = dice(3, 6);
      b = dice(6, 3);
      stats.str[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.dex[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.con[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.tel[x] = MAX(12 + number(0, 1), MAX(a, b));
      a = dice(3, 6);
      b = dice(6, 3);
      stats.wis[x] = MAX(12 + number(0, 1), MAX(a, b));
      quint32 total = stats.str[x] + stats.dex[x] + stats.con[x] + stats.tel[x] + stats.wis[x];
      if (total >= 88)
      {
        // std::cerr <<  "Total = " << total << std::endl;
        // std::cerr <<  "Took " << attempts << " attempts." << std::endl;
        // std::cerr <<  (qreal)attempts / 4 / 60.0 / 60.0 << " hours" << std::endl;
        return 0;
      }
    }
  }
}

void test_random_stats(void)
{
  QMap<qint32, qint32> results;
  for (qint32 i = {}; i < 10000; ++i)
  {
    qint32 result = random_percent_change(33, 6);
    results[result]++;
  }
  // printf("%d\n", result);
  for (const auto &cur : results)
  {
    // std::cerr <<  cur.first << "=" << cur.second << std::endl;
  }
}

QString showObjectAffects(ObjectPtr obj)
{
  QString buffer;
  for (qint32 i = {}; i < obj->num_affects; ++i)
  {
    if (i > 0)
    {
      buffer += ", ";
    }

    if (obj->affected[i].location < 1000)
    {
      buffer += sprinttype(obj->affected[i].location, apply_types);
    }
    else if (!get_skill_name(obj->affected[i].location / 1000).isEmpty())
    {
      buffer += get_skill_name(obj->affected[i].location / 1000);
    }
    else
    {
      buffer += u"Invalid"_s;
    }

    buffer += " by " + QString::number(obj->affected[i].modifier);
  }
  return buffer;
}

QString showObjectVault(ObjectPtr obj)
{
  // std::cerr <<  DC::getInstance()->obj_index[obj->item_number].vnum() << ":";
  QString buffer = QFlagsToStrings(obj->obj_flags.wear_flags);
  // std::cerr <<  buf << ":";

  buffer += sprintbit(obj->obj_flags.size, Object::size_bits);
  // std::cerr <<  buf << ":";

  buffer += sprintbit(obj->obj_flags.extra_flags, Object::extra_bits);
  // std::cerr <<  buf << ":";

  buffer += sprintbit(obj->obj_flags.more_flags, Object::more_obj_bits);
  // std::cerr <<  buf << ":";

  buffer += showObjectAffects(obj);

  // std::cerr <<  " " << qPrintable(obj->short_description()) << " in " << owner << "'s vault." << std::endl;
  return buffer;
}

void showObject(CharacterPtr ch, ObjectPtr obj)
{
  // std::cerr <<  DC::getInstance()->obj_index[obj->item_number].vnum() << ":";
  QString buf;

  QString buffer = QFlagsToStrings(obj->obj_flags.wear_flags);
  // std::cerr <<  buf << ":";

  sprintbit(obj->obj_flags.size, Object::size_bits, buf);
  // std::cerr <<  buf << ":";

  sprintbit(obj->obj_flags.extra_flags, Object::extra_bits, buf);
  // std::cerr <<  buf << ":";

  sprintbit(obj->obj_flags.more_flags, Object::more_obj_bits, buf);
  // std::cerr <<  buf << ":";

  showObjectAffects(obj);

  // std::cerr <<  " " << qPrintable(obj->short_description()) << " in " << qPrintable(ch->name()) << std::endl;
}

void testStrings(void)
{
  test_handle_ansi("");
  test_handle_ansi("$");
  test_handle_ansi("$$");
  test_handle_ansi("$$$");
  test_handle_ansi("$ $$ $$$");
  test_handle_ansi("$$$$$$$$");
  test_handle_ansi("$ ");
  test_handle_ansi("$x");
  test_handle_ansi("$1$2$5$B$b$rttessd$Rddd");

  QString c_arg1, c_arg2, c_input = u"charm sleep "_s;
  QString arg1, remainder = u"charm sleep "_s;
  do
  {
    std::tie(arg1, remainder) = half_chop(remainder);

    half_chop(c_input, c_arg1, c_arg2);
    dc_strncpy(c_input, c_arg2, sizeof(c_input) - 1);

    std::cerr << "[" << arg1.toStdString() << "]"
              << "[" << remainder.toStdString() << "]" << std::endl;
    std::cerr << "[" << c_arg1 << "]"
              << "[" << c_arg2 << "]" << std::endl;
    assert(arg1 == c_arg1);
  } while (!arg1.isEmpty() && c_arg1[0] != '\0');

  std::cerr << sizeof(char_file_u) << " " << sizeof(char_file_u4) << std::endl;
}

qint32 main(qint32 argc, QString *argv)
{
  DC debug(argc, argv);

  // QString namelist = "chief enforcer bob";
  // qDebug() << isexact("enf", namelist) << isprefix("enf", namelist);
  // debug.db().table("shops").column("name", "text").column("name2", "bigint");
  // exit(1);
  // testStrings();

  QString orig_cwd, dclib;
  if (getenv("DCLIB"))
  {
    dclib = QString(getenv("DCLIB"));
    if (!dclib.isEmpty())
    {
      orig_cwd = getcwd(nullptr, 0);
      chdir(qPrintable(dclib));
    }
  }

  DC::getInstance()->logentry(u"Loading the zones"_s, 0, DC::LogChannel::LOG_MISC);
  debug.boot_zones();

  DC::getInstance()->logentry(u"Loading the world."_s, 0, DC::LogChannel::LOG_MISC);

  debug.top_of_world_alloc = 2000;

  debug.boot_world();

  DC::getInstance()->logentry(u"Renumbering the world."_s, 0, DC::LogChannel::LOG_MISC);
  renum_world();

  DC::getInstance()->logentry(u"Generating object indices/loading all objects"_s, 0, DC::LogChannel::LOG_MISC);
  debug.generate_obj_indices(&top_of_objt, debug.obj_index);

  DC::getInstance()->logentry(u"Generating mob indices/loading all mobiles"_s, 0, DC::LogChannel::LOG_MISC);
  debug.generate_mob_indices(&top_of_mobt, debug.mob_index);

  DC::getInstance()->logentry(u"renumbering zone table"_s, 0, DC::LogChannel::LOG_MISC);
  renum_zone_table();

  class Connection *d = new Connection;

  /* Create 1 blank obj to be used when playerfile loads */
  debug.create_blank_item(1);

  debug.load_vaults();

  chdir(qPrintable(orig_cwd));

  // std::cerr << real_mobile(0) << " " << real_mobile(1) << std::endl;

  qint32 vnum = {};
  if (argc >= 3)
  {
    vnum = atoi(argv[2]);
  }

  d = new Connection;
  CharacterPtr ch = new Character(&debug);
  ch->name("Debugimp");
  ch->player = new Player;
  ch->setType(Character::Type::Player);

  ch->desc = d;
  ch->setLevel(110);
  conn->descriptor = 1;
  conn->character = ch;
  conn->output = {};

  auto &character_list = DC::getInstance()->character_list;
  character_list.insert(conn->character);

  conn->character->do_on_login_stuff();

  conn->connected = Connection::states::PLAYING;

  update_max_who();

  do_stand(ch, "");
  conn->process_output();

  char_to_room(ch, 3001);
  conn->process_output();
  // ch->do_toggle({"pager"}, cmd_t::DEFAULT);
  // ch->do_toggle({"ansi"}, cmd_t::DEFAULT);
  // ch->do_toggle({}, cmd_t::DEFAULT);
  //  do_goto(ch, "23");
  // do_score(ch, "");
  // conn->process_output();

  // do_load(ch, "m 23");
  // conn->process_output();
  do_look(ch, "debugimp");
  conn->process_output();

  ch->do_bestow({"debugimp", "load"});
  conn->process_output();

  /*
    qDebug("\n");

    qsizetype size_bits = 8 * sizeof(ch->player->toggles);
    const QString data = reinterpret_cast<const QString>(&ch->player->toggles);
    QBitArray ba;
    if (data)
    {
      ba = QBitArray::fromBits(data, size_bits);
    }

    qDebug() << ch->player->toggles;
    const quint32 *nr = reinterpret_cast<const quint32 *>(ba.bits());
    qDebug() << *nr;
    qDebug() << ba;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid())
    {
      db.close();
      db = QSqlDatabase::addDatabase("QPSQL");
      // db.setHostName("localhost");
      // db.setDatabaseName("dcastle");
      // db.setUserName("dcastle");
      db.open();
    }

    if (!db.isValid())
    {
      qDebug("Database error");
      exit(1);
    }

    QSqlTableModel model{&debug, db};
    model.setEditStrategy(QSqlTableModel::EditStrategy::OnFieldChange);
    model.setTable("player_configurations");
    model.select();
    qint32 row = model.rowCount();
    qDebug() << "Rows: " << row;
    model.insertRows(row, 1);
    qDebug() << QVariant(ba);
    qint32 field_index = model.fieldIndex("testbit");
    bool success = model.setData(model.index(row, field_index), ba.toUInt32(QSysInfo::ByteOrder));
    if (!success)
    {
      qDebug("Failed to setData\n");
      qDebug() << model.lastError();
      exit(1);
    }
    if (!model.submitAll())
    {
      qDebug("Failed to submitAll");
      qDebug() << model.lastError();
      exit(1);
    }

    if (model.rowCount() > 0)
    {
      for (qint32 row = {}; row < model.rowCount(); row++)
      {
        QSqlRecord rec = model.record(row);
        qDebug() << rec.field("testbit");
      }
    }*/

  if (argc > 1 && (QString(argv[1]) == "all" || QString(argv[1]) == "leaderboard"))
  {
    ObjectPtr obj = {};
    QString savepath = dclib + "../save/";
    for (const auto &entry : std::filesystem::directory_iterator(savepath.toStdString()))
    {
      if (entry.is_directory() && entry.path() != "../save/qdata" && entry.path() != "../save/deleted")
      {
        for (const auto &pfile : std::filesystem::directory_iterator(entry.path().c_str()))
        {
          try
          {
            QString path = pfile.path().string().c_str();
            path.remove(0, path.lastIndexOf('/') + 1);
            if (path.isEmpty() == false && path[0] != '.')
            {
              // std::cerr << pfile.path().c_str() << std::endl;
              ch->do_linkload(path.split(' '), cmd_t::DEFAULT);
              conn->process_output();
              do_fsave(ch, qPrintable(path));
              conn->process_output();
            }
            else
            {
              continue;
            }

            if (argv[1] == u"all"_s)
            {
              CharacterPtr ch = conn->character;
              for (qint32 iWear = {}; iWear < MAX_WEAR; iWear++)
              {
                if (ch->equipment[iWear])
                {
                  obj = ch->equipment[iWear];
                  if (obj)
                  {
                    if (vnum > 0 && DC::getInstance()->obj_index[obj->item_number].vnum() == vnum)
                    {
                      showObject(ch, obj);
                    }
                  }
                }
              }

              for (ObjectPtr obj = ch->carrying; obj; obj = obj->next_content)
              {
                if (vnum == 0 || (vnum > 0 && DC::getInstance()->obj_index[obj->item_number].vnum() == vnum))
                {
                  showObject(ch, obj);
                }

                if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
                {
                  for (ObjectPtr container = obj->contains; container; container = container->next_content)
                  {
                    if (vnum > 0 && DC::getInstance()->obj_index[container->item_number].vnum() == vnum)
                    {
                      showObject(ch, container);
                    }
                  }
                }
              }
            }
          }
          catch (...)
          {
          }
        }
      }
    }

    for (const auto &c : DC::getInstance()->character_list)
    {
      c->desc = d;
      do_score(c, "");
      conn->process_output();
      do_vault(c, "list");
      conn->process_output();

      if (c->isImmortalPlayer())
      {
        qWarning(u"WARNING: %1 level: %2"_s.arg(c->name()).arg(qPrintable(c->getLevel())));
      }
    }

    if (argv[1] == u"leaderboard"_s)
    {
      do_leaderboard(ch, "scan");
      conn->process_output();
      do_leaderboard(ch, "");
      conn->process_output();
    }
    /*
        std::multimap<qint32, QString> hp_leaders;
        for (auto& ch : DC::getInstance()->character_list)
        {
          if (ch->isPlayer())
          {
            hp_leaders.insert(std::pair<qint32,QString>(ch->max_hit, qPrintable(ch->name())));
          }
        }

        QQueue<std::pair<qint32,QString>> top_hp_leaders;
        for (auto& l : hp_leaders)
        {
          //// std::cerr <<  l.first << " " << l.second << std::endl;
          top_hp_leaders.push(l);
          if (top_hp_leaders.size() > 5)
          {
            top_hp_leaders.pop();
          }
        }

        quint32 placement = {};
        while (top_hp_leaders.size() > 0)
        {
          // std::cerr <<  top_hp_leaders.front().first << " " << top_hp_leaders.front().second << std::endl;
          leaderboard.setHP(placement++, top_hp_leaders.front().second, top_hp_leaders.front().first);
          top_hp_leaders.pop();
        }
    */

    // leaderboard.check_offline();
    // // std::cerr <<  DC::getInstance()->character_list.size() << std::endl;
    // do_leaderboard(ch, "");
    // conn->process_output();

    VaultPtr vault;

    for (vault = vault_table; vault; vault = vault->next)
    {
      for (vault_items_data *items = vault->items; items; items = items->next)
      {
        ObjectPtr obj = items->obj ? items->obj : get_obj(items->item_vnum);
        if (vnum > 0 && DC::getInstance()->obj_index[obj->item_number].vnum() == vnum)
        {
          ch->send(showObjectVault(obj));
        }
      }
    }
    do_look(ch, "");
    conn->process_output();
    ch->do_force({u"all"_s, u"save"_s});
    conn->process_output();
  }
  else
  {
    try
    {
      ObjectPtr obj;
      auto result = debug.load_char_obj(argv[1]);
      if (!result)
      {
        std::cerr << "Unable to load " << argv[1] << std::endl;
        exit(1);
      }
      else
      {
        conn = result.data();
        conn->character->save();
      }

      CharacterPtr ch = conn->character;
      for (qint32 iWear = {}; iWear < MAX_WEAR; iWear++)
      {
        if (ch->equipment[iWear])
        {
          obj = ch->equipment[iWear];
          if (obj)
          {
            showObject(ch, obj);
          }
        }
      }

      for (ObjectPtr obj = ch->carrying; obj; obj = obj->next_content)
      {
        showObject(ch, obj);

        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)
        {
          for (ObjectPtr container = obj->contains; container; container = container->next_content)
          {
            showObject(ch, container);
          }
        }
      }
    }
    catch (...)
    {
    }
  }

  return 0;
}
//      // std::cerr <<  "Gold: " << conn->character->gold << " Plat: " << conn->character->plat << " XP: " << conn->character->exp << " HP: " << conn->character->raw_hit << " hpmeta: " << conn->character->hpmetas << " Con: " << qint32(conn->character->con) << "," << qint32(conn->character->raw_con) << "," << qint32(conn->character->con_bonus) << std::endl;
//      // std::cerr <<  "Mana: " << conn->character->mana << " MetaMana: " << conn->character->manametas << std::endl;

template <typename T>
class DebugNumber
{
private:
  T number_;
};
