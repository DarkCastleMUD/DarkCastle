/***************************************************************************
 *  file: save.c, Database module.                        Part of DIKUMUD  *
 *  Usage: Saving and loading of characters                                *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 *                                                                         *
 *  Copyright (C) 1992, 1993 Michael Chastain, Michael Quan, Mitchell Tse  *
 *  Rewritten by MERC Industries, based on crash.c by prometheus           *
 *  (Taquin Ho) and abaddon (Jeff Stile).                                  *
 *  You can use our stuff in any way you like whatsoever so long as ths   *
 *  copyright notice remains intact.  If you like it please drop a line    *
 *  to mec\@garnet.berkeley.edu.                                            *
 *                                                                         *
 *  This is free software and you are benefitting.  We hope that you       *
 *  share your changes too.  What goes around, comes around.               *
 ***************************************************************************/
/* $Id: save.cpp,v 1.76 2015/06/15 01:06:10 pirahna Exp $ */

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <fmt/format.h>
#include <memory>

#include "DC/DC.h"

#include "DC/spells.h"
#include "DC/player.h"
#include "DC/db.h"
#include "DC/connect.h"
#include "DC/handler.h"
#include "DC/DC.h"

#ifdef USE_SQL
#include <iostream>
#include <libpq-fe.h>
#include "Backend/Database.h"

extern Database db;
#endif

ObjectPtr obj_store_to_char(CharacterPtr ch, FILE *fpsave, ObjectPtr last_cont);
bool put_obj_in_store(ObjectPtr obj, CharacterPtr ch, FILE *fpsave, qint32 wear_pos);
void restore_weight(ObjectPtr obj);
void store_to_char(char_file_u4 *st, CharacterPtr ch);
QString fread_alias_string(FILE *fpsave);

// return 1 on success
// return 0 on failure
// donno where it would fail off hand though unless we ran out of HD space
// or had a failure.  I'm just not willing to code that much fault protection in
// -pir
void Player::save_char_aliases(FILE *fpsave)
{
  quint32 tmp_size = aliases_.size();
  fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);

  // write the aliases out
  for (const auto [alias, command] : aliases_.asKeyValueRange())
  {
    // note that we save the number of characters in tmp_size
    tmp_size = alias.length();
    fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);
    // but we actually write tmp_size +1 to get the trailing \0
    fwrite(qPrintable(alias), sizeof(QChar), (tmp_size + 1), fpsave);

    tmp_size = command.length();
    fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);
    fwrite(qPrintable(command), sizeof(QChar), (tmp_size + 1), fpsave);
  }
}

// return pointer to aliases or nullptr
aliases_t read_char_aliases(FILE *fpsave)
{
  quint32 total = {};
  fread(&total, sizeof(total), 1, fpsave);

  aliases_t aliases;
  for (auto x = {}; x < total; x++)
  {
    QString keyword = fread_alias_string(fpsave);
    QString command = fread_alias_string(fpsave);
    if (keyword.isEmpty() || command.isEmpty())
    {
      logentry(QStringLiteral("Removing command alias [%1] because it's missing a keyword.").arg(command));
      continue;
    }

    aliases[keyword] = command;
  }

  return aliases;
}

QString fread_alias_string(FILE *fpsave)
{
  quint32 tmp_size = {};
  size_t read_count = fread(&tmp_size, sizeof(tmp_size), 1, fpsave);
  if (read_count != 1)
  {
    logentry(QStringLiteral("fread_alias_string: fread() read %1 bytes instead of %2 at position %3").arg(read_count).arg(sizeof(tmp_size)).arg(ftell(fpsave)));
    return QString();
  }

  if (!tmp_size)
  {
    fseek(fpsave, 1, SEEK_CUR);
    qDebug() << "fread_alias_string: tmp_size=" << tmp_size << " forwarding FILE* 1 byte.";
    return QString();
  }
  else
  {
    QString buffer;
    fread(buffer.get(), sizeof(QChar), (tmp_size + 1), fpsave);
    return QString(buffer.get());
  }
  return QString();
}

void fwrite_var_string(const QString str, FILE *fpsave)
{
  quint16 tmp_size = {};

  if (str)
  {
    tmp_size = strlen(str) + 1; // count the null terminator
  }

  fwrite(&tmp_size, sizeof(tmp_size), 1, fpsave);

  if (str)
  {
    fwrite(str, sizeof(QChar), (tmp_size), fpsave);
  }
}

void fwrite_var_string(QString str, FILE *fpsave)
{
  fwrite_var_string(qPrintable(str), fpsave);
}

QString fread_var_string(FILE *fpsave)
{
  quint16 tmp_size = {};
  QString tmp_str = {};

  size_t records_read = fread(&tmp_size, sizeof(tmp_size), 1, fpsave);
  if (tmp_size > 0 && records_read > 0)
  {
    tmp_str = dc_alloc(tmp_size, sizeof(QChar));
    assert(tmp_str);
    if (tmp_str == nullptr)
    {
      return {};
    }

    records_read = fread(tmp_str, sizeof(QChar), tmp_size, fpsave);
    if (records_read > 0)
    {
      return tmp_str;
    }
    tmp_str = {};
  }

  return {};
}

void Mobile::save(FILE *fpsave)
{
  fwrite(&(nr), sizeof(nr), 1, fpsave);
  fwrite(&(default_pos), sizeof(default_pos), 1, fpsave);
  fwrite(&(attack_type), sizeof(attack_type), 1, fpsave);
  fwrite(&(actflags), sizeof(actflags), 1, fpsave);
  fwrite(&(damnodice), sizeof(damnodice), 1, fpsave);
  fwrite(&(damsizedice), sizeof(damsizedice), 1, fpsave);

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(QChar), 3, fpsave);
}

void Mobile::read(FILE *fpsave)
{
  fread(&(nr), sizeof(nr), 1, fpsave);
  fread(&(default_pos), sizeof(default_pos), 1, fpsave);
  fread(&(attack_type), sizeof(attack_type), 1, fpsave);
  fread(&(actflags), sizeof(actflags), 1, fpsave);
  fread(&(damnodice), sizeof(damnodice), 1, fpsave);
  fread(&(damsizedice), sizeof(damsizedice), 1, fpsave);

  QString typeflag = {};
  fread(&typeflag, sizeof(QChar), 3, fpsave);

  // Add new items in this format
  //  if(!strcmp(typeflag, "XXX"))
  //    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data
}

// TODO - make sure I go back and update the time_data s everywhere when
// we lose link, or logout, etc so that the 'played' variable is correct

void fwrite_string_tilde(FILE *fpsave)
{
  QString buf;
  strcpy(buf, "Bugfixbugfixbugfixbugfixbugfixbugfix~");
  fwrite(&buf, 37, 1, fpsave);
}
void Player::save(FILE *fpsave, time_data tmpage)
{
  fwrite(qPrintable(password_), sizeof(QChar), PASSWORD_LEN + 1, fpsave);
  save_char_aliases(fpsave);

  fwrite_string_tilde(fpsave);
  fwrite(&(rdeaths), sizeof(rdeaths), 1, fpsave);
  fwrite(&(pdeaths), sizeof(pdeaths), 1, fpsave);
  fwrite(&(pkills), sizeof(pkills), 1, fpsave);
  fwrite(&(pklvl), sizeof(pklvl), 1, fpsave);
  // we save tmpage cause it was calculated when all eq was off
  fwrite(&(tmpage), sizeof(time_data), 1, fpsave);
  fwrite(&(bad_pw_tries), sizeof(bad_pw_tries), 1, fpsave);
  fwrite(&(practices), sizeof(practices), 1, fpsave);
  fwrite(&(bank), sizeof(bank), 1, fpsave);
  fwrite(&(toggles), sizeof(toggles), 1, fpsave);
  fwrite(&(punish), sizeof(punish), 1, fpsave);
  fwrite_var_string(last_site, fpsave);
  fwrite_var_string(poofin, fpsave);
  fwrite_var_string(poofout, fpsave);
  fwrite_var_string(getPrompt(), fpsave);
  fwrite_var_string("NewSaveType", fpsave);

  // Quest bitvector one
  if (quest_bv1)
  {
    fwrite("QS1", sizeof(QChar), 3, fpsave);
    fwrite(&(quest_bv1), sizeof(quest_bv1), 1, fpsave);
  }

  // Saving throw mods
  fwrite("SVM", sizeof(QChar), 3, fpsave);
  fwrite(&(saves_mods), sizeof(saves_mods[0]), SAVE_TYPE_MAX + 1, fpsave); // Write the whole array

  // Specializations
  fwrite("SPC", sizeof(QChar), 3, fpsave);
  fwrite(&(specializations), sizeof(specializations), 1, fpsave);

  // Stat metas
  if (statmetas)
  {
    fwrite("STM", sizeof(QChar), 3, fpsave);
    fwrite(&(statmetas), sizeof(statmetas), 1, fpsave);
  }

  // Ki metas
  if (kimetas)
  {
    fwrite("KIM", sizeof(QChar), 3, fpsave);
    fwrite(&(kimetas), sizeof(kimetas), 1, fpsave);
  }
  // autojoinin'
  if (!joining.empty())
  {
    fwrite("JIN", sizeof(QChar), 3, fpsave);
    fwrite_var_string(getJoining(), fpsave);
  }

  fwrite("QST", sizeof(QChar), 3, fpsave);
  fwrite(&(quest_points), sizeof(quest_points), 1, fpsave);
  for (qint32 j = {}; j < QUEST_MAX_CANCEL; j++)
    fwrite(&(quest_cancel[j]), sizeof(quest_cancel[j]), 1, fpsave);
  for (qint32 j = {}; j <= QUEST_TOTAL / ASIZE; j++)
    fwrite(&(quest_complete[j]), sizeof(quest_complete[j]), 1, fpsave);
  if (buildLowVnum)
  {
    fwrite("BLO", sizeof(QChar), 3, fpsave);
    fwrite(&(buildLowVnum), sizeof(buildLowVnum), 1, fpsave);
  }
  if (buildHighVnum)
  {
    fwrite("BHI", sizeof(QChar), 3, fpsave);
    fwrite(&(buildHighVnum), sizeof(buildHighVnum), 1, fpsave);
  }
  if (buildMLowVnum)
  {
    fwrite("BMO", sizeof(QChar), 3, fpsave);
    fwrite(&(buildMLowVnum), sizeof(buildMLowVnum), 1, fpsave);
  }
  if (buildMHighVnum)
  {
    fwrite("BMI", sizeof(QChar), 3, fpsave);
    fwrite(&(buildMHighVnum), sizeof(buildMHighVnum), 1, fpsave);
  }
  if (buildOLowVnum)
  {
    fwrite("BOO", sizeof(QChar), 3, fpsave);
    fwrite(&(buildOLowVnum), sizeof(buildOLowVnum), 1, fpsave);
  }
  if (buildOHighVnum)
  {
    fwrite("BOI", sizeof(QChar), 3, fpsave);
    fwrite(&(buildOHighVnum), sizeof(buildOHighVnum), 1, fpsave);
  }
  if (profession)
  {
    fwrite("PRO", sizeof(QChar), 3, fpsave);
    fwrite(&(profession), sizeof(profession), 1, fpsave);
  }
  if (wizinvis)
  {
    fwrite("WIZ", sizeof(QChar), 3, fpsave);
    fwrite(&(wizinvis), sizeof(wizinvis), 1, fpsave);
  }
  if (config != nullptr)
  {
    for (auto setting = config->constBegin(); setting != config->constEnd(); ++setting)
    {
      if (setting.key() == "color.good" ||
          setting.key() == "color.bad" ||
          setting.key() == "tell.history.timestamp" ||
          setting.key() == "gossip.history.timestamp" ||
          setting.key() == "locale" ||
          setting.key() == "mode" ||
          setting.key() == "timezone" ||
          setting.key() == "fighting.showdps" ||
          setting.key() == "dateformat")
      {
        fwrite("OPT", sizeof(QChar), 3, fpsave);
        fwrite_var_string(setting.key(), fpsave);
        fwrite_var_string(setting.value(), fpsave);
      }
    }
  }
  if (ignoring.empty() == false)
  {
    for (const auto &name : ignoring)
    {
      if (name.second.ignore)
      {
        fwrite("IGN", sizeof(QChar), 3, fpsave);
        fwrite_var_string(name.first.c_str(), fpsave);
        fwrite(&name.second.ignored_count, sizeof(name.second.ignored_count), 1, fpsave);
      }
    }
  }

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(QChar), 3, fpsave);
}

qsizetype fread_to_tilde(FILE *fpsave, QString filename)
{
  qsizetype characters_read = {};
  QString buffer;
  QChar a = {};

  while (characters_read++ < 160)
  {
    if (feof(fpsave))
    {
      qDebug(QStringLiteral("fread_to_tilde: unexpected EOF in %1").arg(qPrintable(filename)));
      return characters_read;
    }

    if (ferror(fpsave))
    {
      qDebug(QStringLiteral("fread_to_tilde: unexpected error in %1").arg(qPrintable(filename)));
      return characters_read;
    }

    long before_fread_offset = ftell(fpsave);
    size_t read_count = fread(&a, 1, 1, fpsave);
    long after_fread_offset = ftell(fpsave);
    if (read_count != 1)
    {
      qDebug(QStringLiteral("fread_to_tilde: fread returned %1 at position %2 now at position %3 in %4").arg(read_count).arg(before_fread_offset).arg(after_fread_offset).arg(qPrintable(filename)));
    }

    buffer += a;
    if (a == '~')
    {
      break;
    }
  }

  if (characters_read >= 160)
  {
    qDebug(QStringLiteral("fread_to_tilde: >= 160 buffer: [%1] in %2").arg(buffer).arg(qPrintable(filename)));
  }

  return characters_read;
}

bool Player::read(FILE *fpsave, CharacterPtr ch, QString filename)
{
  if (!ch)
  {
    return false;
  }

  QString typeflag = {};
  golem = {};
  quest_points = {};
  for (qint32 j = {}; j < QUEST_MAX_CANCEL; j++)
    quest_cancel[j] = {};
  for (qint32 j = {}; j <= QUEST_TOTAL / ASIZE; j++)
    quest_complete[j] = {};

  fread(pwd, sizeof(QChar), PASSWORD_LEN + 1, fpsave);
  aliases_ = read_char_aliases(fpsave);
  if (ch->has_skill(NEW_SAVE))
  {
    if (fread_to_tilde(fpsave, filename) >= 160)
    {
      logbug(QStringLiteral("read_Player: Error reading %1. fread_to_tilde >= 160. Aborting.").arg(ch->name()));
      return false;
    }
  }
  fread(&(rdeaths), sizeof(rdeaths), 1, fpsave);
  fread(&(pdeaths), sizeof(pdeaths), 1, fpsave);
  fread(&(pkills), sizeof(pkills), 1, fpsave);
  fread(&(pklvl), sizeof(pklvl), 1, fpsave);
  fread(&(time), sizeof(time_data), 1, fpsave);
  fread(&(bad_pw_tries), sizeof(bad_pw_tries), 1, fpsave);
  fread(&(practices), sizeof(practices), 1, fpsave);
  fread(&(bank), sizeof(bank), 1, fpsave);
  fread(&(toggles), sizeof(toggles), 1, fpsave);
  fread(&(punish), sizeof(punish), 1, fpsave);
  last_site = fread_var_string(fpsave);
  poofin = fread_var_string(fpsave);
  poofout = fread_var_string(fpsave);
  setPrompt(fread_var_string(fpsave));

  QString tmp = fread_var_string(fpsave);
  if (!tmp || str_cmp(tmp, "NewSaveType"))
  {
    tmp = fread_var_string(fpsave);
    tmp = fread_var_string(fpsave);
  }
  skillchange = {};

  fread(&typeflag, sizeof(QChar), 3, fpsave);

  if (!strcmp("QS1", typeflag))
  {
    fread(&quest_bv1, sizeof(quest_bv1), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }

  if (!strcmp("SVM", typeflag))
  {
    fread(&(saves_mods), sizeof(saves_mods[0]), SAVE_TYPE_MAX + 1, fpsave); // read the whole array
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }

  if (!strcmp("SPC", typeflag))
  {
    fread(&(specializations), sizeof(specializations), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }

  if (!strcmp("STM", typeflag))
  {
    fread(&statmetas, sizeof(statmetas), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }

  if (!strcmp("KIM", typeflag))
  {
    fread(&kimetas, sizeof(kimetas), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  joining = {};

  if (!strcmp("JIN", typeflag))
  {
    QString buffer = fread_var_string(fpsave);
    setJoining(buffer);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("QST", typeflag))
  {
    fread(&(quest_points), sizeof(quest_points), 1, fpsave);
    for (qint32 j = {}; j < QUEST_MAX_CANCEL; j++)
      fread(&(quest_cancel[j]), sizeof(quest_cancel[j]), 1, fpsave);
    for (qint32 j = {}; j <= QUEST_TOTAL / ASIZE; j++)
      fread(&(quest_complete[j]), sizeof(quest_complete[j]), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("BLO", typeflag))
  {
    fread(&buildLowVnum, sizeof(buildLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("BHI", typeflag))
  {
    fread(&buildHighVnum, sizeof(buildHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("BMO", typeflag))
  {
    fread(&buildMLowVnum, sizeof(buildMLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("BMI", typeflag))
  {
    fread(&buildMHighVnum, sizeof(buildMHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("BOO", typeflag))
  {
    fread(&buildOLowVnum, sizeof(buildOLowVnum), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("BOI", typeflag))
  {
    fread(&buildOHighVnum, sizeof(buildOHighVnum), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("PRO", typeflag))
  {
    fread(&profession, sizeof(profession), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("WIZ", typeflag))
  {
    fread(&wizinvis, sizeof(wizinvis), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  while (!strcmp("OPT", typeflag))
  {
    if (config == nullptr)
    {
      config = new PlayerConfig();
    }

    QString key = fread_var_string(fpsave);
    QString value = fread_var_string(fpsave);
    if (QRegularExpression("^(color.(good|bad)|(tell|gossip).history.timestamp|locale|mode|fighting.showdps|timezone)$").match(key).hasMatch())
    {
      config->insert(key, value);
    }

    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  while (!strcmp("IGN", typeflag))
  {
    QString var_string = fread_var_string(fpsave);
    if (var_string != nullptr)
    {
      QString name = var_string;
      if (name.empty() == false)
      {
        ignore_entry ie = {true, 0};
        fread(&ie.ignored_count, sizeof(ie.ignored_count), 1, fpsave);
        ignoring[name] = ie;
      }
    }

    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }

  skillchange = {};
  // Add new items in this format
  //  if(!strcmp(typeflag, "XXX"))
  //    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data
  return true;
}

bool Character::save_pc_or_mob_data(FILE *fpsave, time_data tmpage)
{
  if (isNonPlayer())
  {
    mobdata->save(fpsave);
    return true;
  }
  else if (isPlayer())
  {
    player->save(fpsave, tmpage);
    return true;
  }

  logbug(QStringLiteral("save_pc_or_mob_data: %1 not an NPC and not a player.").arg(name()));
  return false;
}

bool read_pc_or_mob_data(CharacterPtr ch, FILE *fpsave, QString filename)
{
  if (ch->isNonPlayer())
  {
    ch->player = {};
    auto ch->mobdata = new Mobile;
    ch->mobdata->read(fpsave);
  }
  else
  {
    ch->mobdata = {};
    ch->player = new Player;
    ch->setType(Character::Type::Player);
    if (!ch->player->read(fpsave, ch, filename))
    {
      return false;
    }
  }
  return true;
}

// return 1 on success
// return 0 on failure
qint32 store_worn_eq(CharacterPtr ch, FILE *fpsave)
{
  qint32 wear_pos = -1;
  qint32 iWear = {};

  for (iWear = {}; iWear < MAX_WEAR; iWear++)
  {
    wear_pos = iWear;
    if (ch->equipment[iWear])
    {
      if (!obj_to_store(ch->equipment[iWear], ch, fpsave, wear_pos))
        return 0;
    }
  }
  return 1;
}

qint32 Character::char_to_store_variable_data(FILE *fpsave)
{
  fwrite_var_string(this->name(), fpsave);
  fwrite_var_string(this->short_desc, fpsave);
  fwrite_var_string(this->long_desc, fpsave);
  fwrite_var_string(this->description, fpsave);
  fwrite_var_string(this->title, fpsave);

  if (!has_skill(NEW_SAVE)) // New save.
    learn_skill(NEW_SAVE, 1, 100);

  for (const auto &skill : this->skills)
  {
    fwrite("SKL", sizeof(QChar), 3, fpsave);
    fwrite(&(skill.first), sizeof(skill.first), 1, fpsave);
    fwrite(&(skill.second.learned), sizeof(skill.second.learned), 1, fpsave);
    fwrite(&(skill.second.unused), sizeof(skill.second.unused[0]), 5, fpsave);
  }
  fwrite("END", sizeof(QChar), 3, fpsave);

  affected_type *af;
  qint16 aff_count = {}; // do not change from qint16

  for (af = this->affected; af; af = af->next)
    aff_count++;

  if (aff_count)
  {
    fwrite("AFS", sizeof(QChar), 3, fpsave);
    fwrite(&aff_count, sizeof(aff_count), 1, fpsave);
    for (af = this->affected; af; af = af->next)
    {
      fwrite(&(af->type), sizeof(af->type), 1, fpsave);
      fwrite(&(af->duration), sizeof(af->duration), 1, fpsave);
      fwrite(&(af->modifier), sizeof(af->modifier), 1, fpsave);
      fwrite(&(af->location), sizeof(af->location), 1, fpsave);
      fwrite(&(af->bitvector), sizeof(af->bitvector), 1, fpsave);
    }
  }

  tempvariable *mpv;
  for (mpv = this->tempVariable; mpv; mpv = mpv->next)
  {
    if (!mpv->save)
      continue;
    fwrite("MPV", sizeof(QChar), 3, fpsave);
    fwrite_var_string(mpv->name, fpsave);
    fwrite_var_string(mpv->data, fpsave);
  }

  fwrite("GLD", sizeof(QChar), 3, fpsave);
  fwrite(&this->gold_, sizeof(this->gold_), 1, fpsave);

  // Any future additions to this save file will need to be placed LAST here with a 3 letter code
  // and appropriate strcmp statement in the read_mob_data object

  fwrite("STP", sizeof(QChar), 3, fpsave);

  return 1;
}

void read_skill(CharacterPtr ch, FILE *fpsave)
{
  char_skill_data curr = {};

  if (fread(&(curr.skillnum), sizeof(curr.skillnum), 1, fpsave) != 1)
  {
    logentry(QStringLiteral("Unable to read a skill from player file for %1.").arg(qPrintable(ch->name())), IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (fread(&(curr.learned), sizeof(curr.learned), 1, fpsave) != 1)
  {
    logentry(QStringLiteral("Unable to read a skill from player file for %1.").arg(qPrintable(ch->name())), IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (fread(&(curr.unused), sizeof(curr.unused[0]), 5, fpsave) != 5)
  {
    logentry(QStringLiteral("Unable to read a skill from player file for %1.").arg(qPrintable(ch->name())), IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  //  The above line takes care of these four.  They are here for future use
  //  fread(&(curr.unused[1]), sizeof(curr.unused[1]), 1, fpsave);
  //  fread(&(curr.unused[2]), sizeof(curr.unused[2]), 1, fpsave);
  //  fread(&(curr.unused[3]), sizeof(curr.unused[3]), 1, fpsave);
  //  fread(&(curr.unused[4]), sizeof(curr.unused[4]), 1, fpsave);
  ch->skills[curr.skillnum] = curr;
}

qint32 Character::store_to_char_variable_data(FILE *fpsave)
{
  QString typeflag;

  name(fread_var_string(fpsave));
  short_description(fread_var_string(fpsave));
  long_description(fread_var_string(fpsave));
  description(fread_var_string(fpsave));
  title = fread_var_string(fpsave);

  typeflag[3] = '\0';
  fread(&typeflag, sizeof(QChar), 3, fpsave);

  while (strcmp(typeflag, "END"))
  {
    read_skill(this, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }

  fread(&typeflag, sizeof(QChar), 3, fpsave);

  if (!strncmp(typeflag, "AFS", 3)) // affects
  {
    qint16 aff_count; // do not change form qint16
    fread(&aff_count, sizeof(aff_count), 1, fpsave);
    this->affected = {};
    for (qint16 i = {}; i < aff_count; i++)
    {
      affected_type *af = new (std::nothrow) affected_type;
      af->duration_type = {};
      af->next = this->affected;
      this->affected = af;

      fread(&(af->type), sizeof(af->type), 1, fpsave);
      fread(&(af->duration), sizeof(af->duration), 1, fpsave);
      fread(&(af->modifier), sizeof(af->modifier), 1, fpsave);
      fread(&(af->location), sizeof(af->location), 1, fpsave);
      fread(&(af->bitvector), sizeof(af->bitvector), 1, fpsave);

      affect_modify(this, af->location, af->modifier, af->bitvector, true); // re-affect the character
    }
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }

  while (!strcmp(typeflag, "MPV"))
  { // MobProgVars6
    tempvariable *mpv;
    auto mpv = new tempvariable;
    mpv->name = fread_var_string(fpsave);
    mpv->data = fread_var_string(fpsave);
    mpv->save = 1;
    mpv->next = this->tempVariable;
    this->tempVariable = mpv;
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp(typeflag, "GLD"))
  {
    fread(&(this->gold_), sizeof(this->gold_), 1, fpsave);
    fread(&typeflag, sizeof(QChar), 3, fpsave);
  }
  // Add new items in this format
  //  if(!strcmp(typeflag, "XXX"))
  //    do_something

  // Any future additions to this read file will need to be placed LAST

  // at this point, typeflag should = "STP", and we're done reading mob data

  return 1;
}

#ifdef USE_SQL
void save_char_obj_db(CharacterPtr ch)
{
  if (ch == 0)
    return;

  if (ch->isNonPlayer() || ch->getLevel() < 2)
    return;

  // so weapons stop falling off
  SETBIT(this->affected_by, AFF_IGNORE_WEAPON_WEIGHT);

  char_file_u4 uchar;
  time_data tmpage;
  memset(&uchar, 0, sizeof(uchar));
  memset(&tmpage, 0, sizeof(tmpage));

  char_to_store(&uchar, tmpage);

  // if they're in a safe room, save them there.
  // if they're a god, send 'em home
  // otherwise save them in tavern
  if (isSet(DC::getInstance()->world[this->in_room].room_flags, SAFE))
    uchar.load_room = DC::getInstance()->world[this->in_room].number;
  else
    uchar.load_room = real_room(ch->hometown);

  timeval start, finish;

  gettimeofday(&start, nullptr);
  db.save(ch, &uchar);
  gettimeofday(&finish, nullptr);

  qint32 msec = finish.tv_sec * 1000 + finish.tv_usec / 1000;
  msec -= start.tv_sec * 1000 + start.tv_usec / 1000;
  ch->send(QStringLiteral("Save took %1ms\r\n").arg(msec));

  /*
  if((fwrite(&uchar, sizeof(uchar), 1, fpsave))               &&
     (char_to_store_variable_data(ch, fpsave))                &&
     (ch->save_pc_or_mob_data(fpsave, tmpage))                &&
     (obj_to_store (this->carrying, ch, fpsave, -1))            &&
     (store_worn_eq(ch, fpsave))
    )
  {
    if(fpsave != nullptr)
      fclose(fpsave);
    sprintf(log_buf, "mv -f %s %s", strsave, name);
    system(log_buf);
  }
  else
  {
    if(fpsave != nullptr)
      fclose(fpsave);
    sprintf(log_buf, "Save_char_obj: %s", strsave);
    ch->send("WARNING: file problem. You did not save!");
    perror(log_buf);
    logentry(log_buf, ANGEL, DC::LogChannel::LOG_BUG);
  }

  REMBIT(this->affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  Vault *vault;
  if ((vault = has_vault(qPrintable(ch->name()))))
    save_vault(vault->owner);
  */
}
#endif

// save a character and inventory.
// maybe modify it to save mobs for quest purposes too
void Character::save_char_obj(void)
{
  char_file_u4 uchar = {};
  time_data tmpage;
  FILE *fpsave = {};
  QString strsave = {0};
  QString name = {0};

  memset(&tmpage, 0, sizeof(tmpage));

  if (this->isNonPlayer() || getLevel() < 1)
  {
    return;
  }

  if (name().isEmpty())
  {
    name("Unknown");
  }

  // TODO - figure out a way for mob's to save...maybe <mastername>.pet ?
  if (DC::getInstance()->cf.bport)
  {
    sprintf(name, "%s/%c/%s", BSAVE_DIR, qPrintable(name())[0], qPrintable(name()));
  }
  else
  {
    sprintf(name, "%s/%c/%s", SAVE_DIR, qPrintable(name())[0], qPrintable(name()));
  }

  sprintf(strsave, "%s.back", name);

  if (!(fpsave = fopen(strsave, "wb")))
  {
    sendln("Warning!  Did not save.  Could not open file.  Contact a god, do not logoff.");
    QString log_buf = {};
    sprintf(log_buf, "Could not open file in save_char_obj. '%s'", strsave);
    perror(log_buf);
    logentry(log_buf, ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  SETBIT(affected_by, AFF_IGNORE_WEAPON_WEIGHT); // so weapons stop falling off

  char_to_store(&uchar, tmpage);

  // if they're in a safe room, save them there.
  // if they're a god, send 'em home
  // otherwise save them in tavern

  if (in_room < 1)
  {
    uchar.load_room = START_ROOM;
  }
  else
  {
    if (isSet(DC::getInstance()->world[in_room].room_flags, SAFE))
      uchar.load_room = DC::getInstance()->world[in_room].number;
    else
      uchar.load_room = real_room(hometown);
  }

  if ((fwrite(&uchar, sizeof(uchar), 1, fpsave)) &&
      (char_to_store_variable_data(fpsave)) &&
      (save_pc_or_mob_data(fpsave, tmpage)) &&
      (obj_to_store(carrying, this, fpsave, -1)) &&
      (store_worn_eq(this, fpsave)))
  {
    if (fpsave != nullptr)
      fclose(fpsave);

    QString log_buf = {};
    sprintf(log_buf, "mv -f %s %s", strsave, name);
    system(log_buf);
  }
  else
  {
    if (fpsave != nullptr)
      fclose(fpsave);
    QString log_buf = {};
    sprintf(log_buf, "Save_char_obj: %s", strsave);
    send("WARNING: file problem. You did not save!");
    perror(log_buf);
    logentry(log_buf, ANGEL, DC::LogChannel::LOG_BUG);
  }

  REMBIT(affected_by, AFF_IGNORE_WEAPON_WEIGHT);
  Vault *vault;
  if ((vault = has_vault(name())))
    save_vault(vault->owner);
}

// just error crap to avoid using "goto" like we were
void load_char_obj_error(FILE *fpsave, QString strsave)
{
  QString log_buf = QStringLiteral("Load_char_obj: %1").arg(strsave);
  perror(qPrintable(log_buf));
  logentry(log_buf, ANGEL, DC::LogChannel::LOG_BUG);
  if (fpsave != nullptr)
    fclose(fpsave);
}

// Load a character and inventory into a new_new ch ure.
load_status_t DC::load_char_obj(ConnectionPtr conn, QString name)
{
  if (!conn || name.isEmpty())
    return load_status_t::bad_input;

  FILE *fpsave = {};
  QString strsave;
  char_file_u4 uchar;
  ObjectPtr last_cont = {};
  CharacterPtr ch;

  name.toLower();
  name[0] = name[0].toUpper();

  ch = new Character(this);

  if (conn->character)
  {
    free_char(conn->character, Trace("load_char_obj"));
  }

  conn->character = ch;
  clear_char(ch);
  ch->desc = d;

  if (DC::getInstance()->cf.bport)
  {
    strsave = QStringLiteral("%1/%2/%3").arg(BSAVE_DIR).arg(name[0]).arg(name);
  }
  else
  {
    strsave = QStringLiteral("%1/%2/%3").arg(SAVE_DIR).arg(name[0]).arg(name);
  }

  //   stat mystats;
  //  stat(strsave, &mystats);
  //  TODO - Eventually, i'm going to just slurp in the whole file
  //  then parse the memory instead of reading each item from file seperately
  //  Should be much faster and save our HD from turning itself to mush -pir

  if ((fpsave = fopen(qPrintable(strsave), "rb")) == nullptr)
    return load_status_t::missing;

  if (fread(&uchar, sizeof(uchar), 1, fpsave) == 0)
  {
    load_char_obj_error(fpsave, strsave);
    return load_status_t::error;
  }

  reset_char(ch);

  store_to_char(&uchar, ch);
  ch->store_to_char_variable_data(fpsave);
  if (!read_pc_or_mob_data(ch, fpsave, strsave))
  {
    return load_status_t::error;
  }

  if (ch->isPlayer() && ch->player->time.logon < 1117527906)
  {
    do_clearaff(ch, "");
    ch->affected_by[0] = ch->affected_by[1] = {};
  }

  // stored names only matter for mobs
  if (!ch->isNonPlayer())
  {
    ch->name(name);
  }

  while (!feof(fpsave))
  {
    last_cont = obj_store_to_char(ch, fpsave, last_cont);
  }

  if (fpsave != nullptr)
    fclose(fpsave);
  return load_status_t::success;
}

// read data from file for an item.
ObjectPtr obj_store_to_char(CharacterPtr ch, FILE *fpsave, ObjectPtr last_cont)
{
  ObjectPtr obj;
  //  extra_descr_data *new_new_descr;
  //  extra_descr_data *ed, *next_ed;

  qint32 j;
  qint32 nr;
  quint16 length; // do not change this type
  qint32 wear_pos;
  QString mod_type;
  QString buf;

  // read in the standard file data
  obj_file_elem object;
  fread(&object, sizeof(object), 1, fpsave);

  if (feof(fpsave))
    return {};

  // if it's a current object, clone it and continue
  // if it's not, then we need to remove it from the pfile so clone obj 1

  if ((nr = real_object(object.item_number)) > -1)
    obj = clone_object(nr);
  else
    obj = clone_object(1);

  obj->obj_flags.timer = object.timer;
  wear_pos = object.wear_pos;

  // begin sequence find any modifications to the item the person has
  // what happens, is the mods are written in a particular order to the pfile
  // so I only need to go through this once instead of looping through for each
  // one each time.  If we later decide to want to add something else, we just
  // put it at the end of the sequence and all is good.  We keep reading until
  // we hit a STP flag.  If we aren't on STP by the end of the sequence, then
  // something very bad has happened. -pir
  mod_type[3] = {};
  fread(&mod_type, sizeof(QChar), 3, fpsave);

  if (!strcmp("EQL", mod_type))
  {
    fread(&obj->obj_flags.eq_level, sizeof(obj->obj_flags.eq_level), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("VA0", mod_type))
  {
    fread(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("VA1", mod_type))
  {
    fread(&obj->obj_flags.value[1], sizeof(obj->obj_flags.value[1]), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("VA2", mod_type))
  {
    fread(&obj->obj_flags.value[2], sizeof(obj->obj_flags.value[2]), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("VA3", mod_type))
  {
    fread(&obj->obj_flags.value[3], sizeof(obj->obj_flags.value[3]), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("EXF", mod_type))
  {
    fread(&obj->obj_flags.extra_flags, sizeof(obj->obj_flags.extra_flags), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("MOF", mod_type))
  {
    fread(&obj->obj_flags.more_flags, sizeof(obj->obj_flags.more_flags), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("TYF", mod_type))
  {
    fread(&obj->obj_flags.type_flag, sizeof(obj->obj_flags.type_flag), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("WEA", mod_type))
  {
    fread(&obj->obj_flags.wear_flags, sizeof(obj->obj_flags.wear_flags), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("SZE", mod_type))
  {
    fread(&obj->obj_flags.size, sizeof(obj->obj_flags.size), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("WEI", mod_type))
  {
    fread(&obj->obj_flags.weight, sizeof(obj->obj_flags.weight), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("AFF", mod_type))
  {
    fread(&obj->num_affects, sizeof(obj->num_affects), 1, fpsave);

    obj->affected = {};
    for (j = {}; j < obj->num_affects; j++)
    {
      fread(&obj->affected[j].location, sizeof(obj->affected[j].location), 1, fpsave);
      fread(&obj->affected[j].modifier, sizeof(obj->affected[j].modifier), 1, fpsave);
    }

    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("RPR", mod_type))
  {
    auto a = obj->affected;
    a[i].location = APPLY_DAMAGED;
    fread(&a[i].modifier, sizeof(a[i].modifier), 1, fpsave);
    obj->affected = a;
    obj->num_affects++;
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("NAM", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(QChar), length, fpsave);
    buf[length] = '\0';
    obj->name(buf);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("DES", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(QChar), length, fpsave);
    buf[length] = '\0';
    obj->long_description = QStringLiteral(buf);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("SDE", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(QChar), length, fpsave);
    buf[length] = '\0';
    qPrintable(obj->short_description()) = QStringLiteral(buf);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("ADE", mod_type))
  {
    fread(&length, sizeof(length), 1, fpsave);
    fread(&buf, sizeof(QChar), length, fpsave);
    buf[length] = '\0';
    obj->ActionDescription(buf);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("COS", mod_type))
  {
    fread(&obj->obj_flags.cost, sizeof(obj->obj_flags.cost), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("SAV", mod_type))
  {
    fread(&obj->save_expiration, sizeof(quint32), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }
  if (!strcmp("SEL", mod_type))
  {
    fread(&obj->no_sell_expiration, sizeof(quint32), 1, fpsave);
    fread(&mod_type, sizeof(QChar), 3, fpsave);
  }

  // TODO - put extra desc support here
  // NEW READS GO HERE

  if (nr == -1)
  {
    extract_obj(obj);
    return last_cont;
  }
  // Handle worn EQ
  if ((wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos]) && CAN_WEAR(obj, Character::wear_to_item_wear[wear_pos]))
  {
    ch->equip_char(obj, wear_pos, 1);
    return obj;
  }
  else if ((wear_pos > -1) && (wear_pos < MAX_WEAR) && (!ch->equipment[wear_pos + 1]) && CAN_WEAR(obj, Character::wear_to_item_wear[wear_pos + 1]))
  {
    ch->equip_char(obj, wear_pos + 1, 1);
    return obj;
  }
  else if (object.container_depth == 1 && last_cont)
  {
    // put the eq in a container
    // this code does not currently support containers in containers
    if (ARE_CONTAINERS(last_cont))
    {
      obj_to_obj(obj, last_cont);
      // we don't add weight to the character for containers that are worn
      if (!last_cont->equipped_by && DC::getInstance()->obj_index[last_cont->item_number].vnum() != 536)
        IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
    }
    else
    {
      obj_to_char(obj, ch); // just in case
      return last_cont;
    }
  }
  // screw it, just put it in their inventory
  else
  {
    obj_to_char(obj, ch);
    if (wear_pos > -1 && wear_pos < MAX_WEAR)
    {
      SETBIT(ch->affected_by, AFF_ITEM_REMOVE);
    }
    return obj;
  }

  return last_cont;
}

bool obj_to_store(ObjectPtr obj, CharacterPtr ch, FILE *fpsave, qint32 wear_pos)
{
  // ObjectPtr tmp;

  if (obj == nullptr)
    return true;

  // recurse down next item in list
  if (!obj_to_store(obj->next_content, ch, fpsave, -1))
    return false;

  // store myself
  if (!put_obj_in_store(obj, ch, fpsave, wear_pos))
    return false;

  // store anything IN myself.  That way they get put back in on read
  if (!obj_to_store(obj->contains, ch, fpsave, -1))
    return false;

  return true;
}

// return true on success
// return false on error
// write one object to file
bool put_obj_in_store(ObjectPtr obj, CharacterPtr ch, FILE *fpsave, qint32 wear_pos)
{
  obj_file_elem object;
  ObjectPtr standard_obj = {};
  quint16 length = {}; // do not change this type

  memset(&object, 0, sizeof(object));

  if (GET_ITEM_TYPE(obj) == ITEM_NOTE)
    return true;

  if (isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE))
    return true;

  if (isSet(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  {
    // First time we try to save this object we set the
    // expiration to 24 hours from this point
    if (obj->save_expiration == 0)
    {
      obj->save_expiration = time(nullptr) + (60 * 60 * 24);
    }
    else if (time(nullptr) > obj->save_expiration)
    {
      // If the object's window for saving has expired then
      // we don't save it as-if it had ITEM_NOSAVE
      return true;
    }
  }

  if (obj->item_number < 0)
    return true;

  // Set up items saved for all items
  object.version = CURRENT_OBJ_VERSION;
  object.item_number = DC::getInstance()->obj_index[obj->item_number].vnum();
  object.timer = obj->obj_flags.timer;
  object.wear_pos = wear_pos;
  if (obj->in_obj) // I'm in a container
    object.container_depth = 1;
  else
    object.container_depth = {};

  // write basic item format to file
  if (!(fwrite(&object, sizeof(object), 1, fpsave)))
    return false;

  // get a pointer to the standard version of this item
  standard_obj = ((ObjectPtr)DC::getInstance()->obj_index[obj->item_number].item);

  // Begin checking if this item has been modified in any way from the standard
  // If it has, we need to save that particular modification to the file
  // THESE MUST REMAIN IN PROPER ORDER
  // IF YOU HAVE ANYMORE TO ADD, ADD THEM BEFORE THE "STP" FLAG AT END
  /*  if(obj->obj_flags.eq_level    != standard_obj->obj_flags.eq_level)
    {
      fwrite("EQL", sizeof(QChar), 3, fpsave);
      fwrite(&obj->obj_flags.eq_level, sizeof(obj->obj_flags.eq_level), 1, fpsave);
    }
    if(obj->obj_flags.value[0]    != standard_obj->obj_flags.value[0])
    {
      fwrite("VA0", sizeof(QChar), 3, fpsave);
      fwrite(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
    }*/

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.value[0] != standard_obj->obj_flags.value[0])
  {
    fwrite("VA0", sizeof(QChar), 3, fpsave);
    fwrite(&obj->obj_flags.value[0], sizeof(obj->obj_flags.value[0]), 1, fpsave);
  }

  if ((obj->obj_flags.type_flag == ITEM_CONTAINER || obj->obj_flags.type_flag == ITEM_DRINKCON || isSet(obj->obj_flags.more_flags, ITEM_CUSTOM)) && obj->obj_flags.value[1] != standard_obj->obj_flags.value[1])
  {
    fwrite("VA1", sizeof(QChar), 3, fpsave);
    fwrite(&obj->obj_flags.value[1], sizeof(obj->obj_flags.value[1]), 1, fpsave);
  }

  if ((obj->obj_flags.type_flag == ITEM_DRINKCON || obj->obj_flags.type_flag == ITEM_STAFF || obj->obj_flags.type_flag == ITEM_WAND || isSet(obj->obj_flags.more_flags, ITEM_CUSTOM)) && obj->obj_flags.value[2] != standard_obj->obj_flags.value[2])
  {
    fwrite("VA2", sizeof(QChar), 3, fpsave);
    fwrite(&obj->obj_flags.value[2], sizeof(obj->obj_flags.value[2]), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.value[3] != standard_obj->obj_flags.value[3])
  {
    fwrite("VA3", sizeof(QChar), 3, fpsave);
    fwrite(&obj->obj_flags.value[3], sizeof(obj->obj_flags.value[3]), 1, fpsave);
  }

  if (obj->obj_flags.extra_flags != standard_obj->obj_flags.extra_flags)
  {
    fwrite("EXF", sizeof(QChar), 3, fpsave);
    fwrite(&obj->obj_flags.extra_flags, sizeof(obj->obj_flags.extra_flags), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM) && obj->obj_flags.more_flags != standard_obj->obj_flags.more_flags)
  {
    fwrite("MOF", sizeof(QChar), 3, fpsave);
    fwrite(&obj->obj_flags.more_flags, sizeof(obj->obj_flags.more_flags), 1, fpsave);
  }

  /*
    if(obj->obj_flags.more_flags != standard_obj->obj_flags.more_flags)
    {
      fwrite("MOF", sizeof(QChar), 3, fpsave);
      fwrite(&obj->obj_flags.more_flags, sizeof(obj->obj_flags.more_flags), 1, fpsave);
    }
    if(obj->obj_flags.type_flag != standard_obj->obj_flags.type_flag)
    {
      fwrite("TYF", sizeof(QChar), 3, fpsave);
      fwrite(&obj->obj_flags.type_flag, sizeof(obj->obj_flags.type_flag), 1, fpsave);
    }
    if(obj->obj_flags.wear_flags != standard_obj->obj_flags.wear_flags)
    {
      fwrite("WEA", sizeof(QChar), 3, fpsave);
      fwrite(&obj->obj_flags.wear_flags, sizeof(obj->obj_flags.wear_flags), 1, fpsave);
    }
    if(obj->obj_flags.size != standard_obj->obj_flags.size)
    {
      fwrite("SZE", sizeof(QChar), 3, fpsave);
      fwrite(&obj->obj_flags.size, sizeof(obj->obj_flags.size), 1, fpsave);
    }

    if(obj->obj_flags.weight != standard_obj->obj_flags.weight)
      {
        fwrite("WEI", sizeof(QChar), 3, fpsave);
        fwrite(&obj->obj_flags.weight, sizeof(obj->obj_flags.weight), 1, fpsave);
      }


    tmp_weight = obj->obj_flags.weight;
    if(GET_ITEM_TYPE(obj) == ITEM_CONTAINER && (loop_obj = obj->contains)
    && DC::getInstance()->obj_index[obj->item->number].vnum() != 536)
      for (; loop_obj; loop_obj = loop_obj->next_content)
        tmp_weight -= GET_OBJ_WEIGHT(loop_obj);
    if(tmp_weight      != standard_obj->obj_flags.weight)
    {
      fwrite("WEI", sizeof(QChar), 3, fpsave);
      fwrite(&tmp_weight, sizeof(tmp_weight), 1, fpsave);
    }
    change = (obj->num_affects != standard_obj->num_affects);
    // since they aren't always in the same order (builder might have swapped them in an
    // rsave or something) we have to search through for each one to see if they are there,
    // just in a different spot
    for (iAffect = {}; (iAffect < obj->num_affects) && !change; iAffect++)
    {
      // set it to changed, and if we find it, set it back to unchanged, then continue prior loop
      change = 1;
      for(iAff2 = {}; (iAff2 < obj->num_affects) && change; iAff2++)
        if( (obj->affected[iAffect].location == standard_obj->affected[iAff2].location) ||
            (obj->affected[iAffect].modifier == standard_obj->affected[iAff2].modifier))
          change = {};
    }
    */
  // Custom objects get all of their affects copied
  if (isSet(obj->obj_flags.more_flags, ITEM_CUSTOM))
  {
    fwrite("AFF", sizeof(QChar), 3, fpsave);
    fwrite(&obj->num_affects, sizeof(obj->num_affects), 1, fpsave);
    for (qint32 iAffect = {}; iAffect < obj->num_affects; iAffect++)
    {
      fwrite(&obj->affected[iAffect].location, sizeof(obj->affected[iAffect].location), 1, fpsave);
      fwrite(&obj->affected[iAffect].modifier, sizeof(obj->affected[iAffect].modifier), 1, fpsave);
    }
  }
  else
  { // non-custom objects only get the damaged affect copied by way of RPR
    qint32 i;
    for (i = {}; i < obj->num_affects; i++)
    {
      if (obj->affected[i].location == APPLY_DAMAGED)
      {
        fwrite("RPR", sizeof(QChar), 3, fpsave);
        fwrite(&obj->affected[i].modifier, sizeof(obj->affected[i].modifier), 1, fpsave);
        break; // Fixed!
      }
    }
  }

  if (!obj->name().isEmpty() && obj->name() != standard_obj->name())
  {
    fwrite("NAM", sizeof(QChar), 3, fpsave);
    length = strlen(qPrintable(obj->name()));
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(qPrintable(obj->name()), sizeof(QChar), length, fpsave);
  }
  if (obj->long_description && strcmp(obj->long_description, standard_obj->long_description))
  {
    fwrite("DES", sizeof(QChar), 3, fpsave);
    length = strlen(obj->long_description);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(obj->long_description, sizeof(QChar), length, fpsave);
  }
  if (obj->short_description && strcmp(qPrintable(obj->short_description()), standard_obj->short_description))
  {
    fwrite("SDE", sizeof(QChar), 3, fpsave);
    length = strlen(obj->short_description);
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(qPrintable(obj->short_description()), sizeof(QChar), length, fpsave);
  }
  if (!obj->ActionDescription().isEmpty() && obj->ActionDescription() != standard_obj->ActionDescription())
  {
    fwrite("ADE", sizeof(QChar), 3, fpsave);
    length = obj->ActionDescription().length();
    fwrite(&length, sizeof(length), 1, fpsave);
    fwrite(qPrintable(obj->ActionDescription()), sizeof(QChar), length, fpsave);
  }

  if (obj->obj_flags.cost != standard_obj->obj_flags.cost)
  {
    fwrite("COS", sizeof(QChar), 3, fpsave);
    fwrite(&obj->obj_flags.cost, sizeof(obj->obj_flags.cost), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  {
    fwrite("SAV", sizeof(QChar), 3, fpsave);
    fwrite(&obj->save_expiration, sizeof(quint32), 1, fpsave);
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_24H_NO_SELL))
  {
    fwrite("SEL", sizeof(QChar), 3, fpsave);
    fwrite(&obj->no_sell_expiration, sizeof(quint32), 1, fpsave);
  }

  // extra descs are a little strange...it's a pointer to a list of them
  // I don't really want to handle this right now, so I'm going to just ignore them now
  // TODO - figure out a way to save extra descs later.  I'll just make them impossible
  // to restring for now

  // THIS IS WHERE YOU SHOULD PUT ANY ADDITIONS TO THE OBJ PFILE THAT NEED TO BE SAVED
  // A CORRESPONDING ENTRY SHOULD BE MADE IN THE READ FUNCTION
  // MAKE SURE YOUR FLAG ISN'T ALREADY USED

  // Stop flag.  This means we are done with this object on the read
  fwrite("STP", sizeof(QChar), 3, fpsave);

  return true;
}

/*
 * Restore container weights after a save.
 */
void restore_weight(ObjectPtr obj)
{
  ObjectPtr tmp;

  if (obj == nullptr)
    return;
  if (DC::getInstance()->obj_index[obj->item_number].vnum() == 536)
    return;
  restore_weight(obj->contains);
  restore_weight(obj->next_content);
  for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
    GET_OBJ_WEIGHT(tmp) += GET_OBJ_WEIGHT(obj);
}

void donothin() {}
// Read shared data from pfile
void store_to_char(char_file_u4 *st, CharacterPtr ch)
{
  qint32 i;

  ch->clan = st->clan;

  GET_SEX(ch) = st->sex;
  GET_CLASS(ch) = st->c_class;
  ch->race = st->race;
  ch->setLevel(st->level);

  ch->hometown = st->hometown;
  if (ch->getLevel() < 11)
    ch->hometown = START_ROOM;

  GET_STR(ch) = GET_RAW_STR(ch) = st->raw_str;
  GET_INT(ch) = GET_RAW_INT(ch) = st->raw_intel;
  GET_WIS(ch) = GET_RAW_WIS(ch) = st->raw_wis;
  GET_DEX(ch) = GET_RAW_DEX(ch) = st->raw_dex;
  GET_CON(ch) = GET_RAW_CON(ch) = st->raw_con;

  ch->weight = st->weight;
  ch->height = st->height;
  ch->setGold(st->gold);
  ch->plat = st->plat;
  ch->exp = st->exp;
  ch->immune = st->immune;
  ch->resist = st->resist;
  ch->suscept = st->suscept;

  ch->setHP(st->hit);
  GET_RAW_HIT(ch) = st->raw_hit;
  GET_MANA(ch) = st->mana;
  GET_RAW_MANA(ch) = st->raw_mana;

  // since move and ki don't get "redone" with stat bonuses we need to set the max here
  ch->setMove(st->move);
  ch->max_move = GET_RAW_MOVE(ch) = st->raw_move;
  GET_KI(ch) = st->ki;
  GET_RAW_KI(ch) = st->raw_ki;

  ch->alignment = st->alignment;
  ch->misc = st->misc;

  GET_HP_METAS(ch) = st->hpmetas;
  GET_MANA_METAS(ch) = st->manametas;
  GET_MOVE_METAS(ch) = st->movemetas;
  GET_AC_METAS(ch) = st->acmetas;
  GET_AGE_METAS(ch) = st->agemetas;

  ch->armor = st->armor;
  ch->hitroll = st->hitroll;
  ch->damroll = st->damroll;
  donothin();

  ch->affected_by[0] = st->afected_by;
  ch->affected_by[1] = st->afected_by2;

  /*    i = {};
      while(st->afected_by[i] != -1) {
         ch->affected_by[i] = st->afected_by[i];
         i++;
      }
      st->afected_by[i] = -1;
  */
  for (i = {}; i <= 2; i++)
    GET_COND(ch, i) = st->conditions[i];

  // it's ok assigning the in_room directly since do_on_login_stuff() will
  // make the actual call to "char_to_room" using this data later
  ch->in_room = real_room(st->load_room);

  if (ch->in_room == DC::NOWHERE)
  {
    if (ch->isImmortalPlayer())
      ch->in_room = real_room(17);
    else
      ch->in_room = real_room(START_ROOM);
  }
}

// copy vital data from a players character-ure to the file ure
// return 'age' of character unmodified
void Character::char_to_store(char_file_u4 *st, time_data &tmpage)
{
  qint32 i;
  qint32 x;
  affected_type *af;
  ObjectPtr ch_eq[MAX_WEAR];

  // Remove all the eq and store it in temp storage
  for (i = {}; i < MAX_WEAR; i++)
  {
    if (equipment[i])
      ch_eq[i] = unequip_char(i, true);
    else
      ch_eq[i] = {};
  }

  // Unaffect everything a character can be affected by spell-wise
  for (af = affected; af; af = af->next)
  {
    affect_modify(this, af->location, af->modifier, af->bitvector, false);
  }

  st->sex = GET_SEX(this);
  st->c_class = GET_CLASS(this);
  st->race = this->race;
  st->level = getLevel();

  st->raw_str = GET_RAW_STR(this);
  st->raw_intel = GET_RAW_INT(this);
  st->raw_wis = GET_RAW_WIS(this);
  st->raw_dex = GET_RAW_DEX(this);
  st->raw_con = GET_RAW_CON(this);

  st->mana = GET_MANA(this);
  st->raw_mana = GET_RAW_MANA(this);
  st->hit = getHP();
  st->raw_hit = GET_RAW_HIT(this);
  st->move = GET_MOVE(this);
  st->raw_move = GET_RAW_MOVE(this);
  st->ki = GET_KI(this);
  st->raw_ki = GET_RAW_KI(this);

  st->weight = GET_WEIGHT(this);
  st->height = GET_HEIGHT(this);
  for (i = {}; i < 3; i++)
    st->conditions[i] = GET_COND(this, i);

  st->hometown = hometown;

  //  gets set outside
  //  st->load_room = DC::getInstance()->world[in_room].number;

  //  st->gold      = getGold();
  st->gold = {}; // Moved
  st->plat = GET_PLATINUM(this);
  st->exp = this->exp;
  st->immune = immune;
  st->resist = resist;
  st->suscept = suscept;
  st->alignment = alignment;
  st->misc = misc;

  st->hpmetas = GET_HP_METAS(this);
  st->manametas = GET_MANA_METAS(this);
  st->movemetas = GET_MOVE_METAS(this);
  st->clan = clan;

  // make sure rest of unused are set to 0
  for (x = {}; x < 3; x++)
    st->extra_ints[x] = {};

  if (this->isNonPlayer())
  {
    st->armor = armor;
    st->hitroll = hitroll;
    st->damroll = damroll;
    st->afected_by = affected_by[0];
    st->afected_by2 = affected_by[1];
    //  x=0;
    //  while(afected_by[x] != -1) {
    //     st->afected_by[x] = affected_by[x];
    //     x++;
    //  }
    //  st->afected_by[x] = -1;
  }
  else
  {
    switch (GET_CLASS(this))
    {
    case CLASS_MAGE:
      st->armor = 150;
      break;
    case CLASS_DRUID:
      st->armor = 140;
      break;
    case CLASS_CLERIC:
      st->armor = 130;
      break;
    case CLASS_ANTI_PAL:
      st->armor = 120;
      break;
    case CLASS_THIEF:
      st->armor = 110;
      break;
    case CLASS_BARD:
      st->armor = 100;
      break;
    case CLASS_BARBARIAN:
      st->armor = 80;
      break;
    case CLASS_RANGER:
      st->armor = 60;
      break;
    case CLASS_PALADIN:
      st->armor = 40;
      break;
    case CLASS_WARRIOR:
      st->armor = 20;
      break;
    case CLASS_MONK:
      st->armor = {};
      break;
    default:
      st->armor = 100;
      break;
    }
    st->hitroll = {};
    st->damroll = {};
    st->afected_by = {};
    st->afected_by2 = {};
    st->acmetas = GET_AC_METAS(this);
    st->agemetas = GET_AGE_METAS(this);
    tmpage = player->time;
  }

  // re-affect the character with spells
  for (af = affected; af; af = af->next)
  {
    affect_modify(this, af->location, af->modifier, af->bitvector, true);
  }

  // re-equip the character with his eq
  for (i = {}; i < MAX_WEAR; i++)
  {
    if (ch_eq[i])
      equip_char(ch_eq[i], i, 1);
  }
}
