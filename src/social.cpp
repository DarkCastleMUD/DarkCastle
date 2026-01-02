// social.C
// Description:  Anything to do with socials

#include <cstring>
#include <cstdlib> // qsort()

#include "DC/fileinfo.h" // SOCIAL_FILE
#include "DC/structs.h"  // MAX_INPUT_LENGTH
#include "DC/room.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/mobile.h"
#include "DC/connect.h"
#include "DC/player.h"
#include "DC/social.h"
#include "DC/handler.h"
#include "DC/act.h"
#include "DC/db.h"
#include "DC/interp.h" // len_cmp
#include "DC/returnvals.h"

command_return_t Character::check_social(QString pcomm)
{
  auto arguments = pcomm.split(' ');
  auto social = arguments.value(0);
  auto target = arguments.value(1);
  auto action = dc_->socials_.find(social);
  if (!action)
    return eFAILURE;

  if (isPlayer() && isSet(player->punish, PUNISH_NOEMOTE))
  {
    sendln("You are anti-social!");
    return eSUCCESS;
  }

  switch (GET_POS(this))
  {
  case position_t::DEAD:
    sendln("Lie still; you are DEAD.");
    return eSUCCESS;

  case position_t::STUNNED:
    sendln("You are too stunned to do that.");
    return eSUCCESS;

  case position_t::SLEEPING:
    sendln("In your dreams, or what?");
    return eSUCCESS;
  }

  if (isSet(dc_->world[in_room].room_flags, QUIET))
  {
    sendln("SHHHHHH!! Can't you see people are trying to read?");
    return eSUCCESS;
  }

  if (target.isEmpty())
  {
    act(action->char_no_arg_, this, 0, 0, TO_CHAR, 0);
    act(action->others_no_arg_, this, 0, 0, TO_ROOM, (action->hide_) ? INVIS_NULL : 0);
    return SOCIAL_true_WITH_NOISE;
  }

  if (auto vict = get_char_room_vis(target); !vict)
  {
    act(action->not_found_, this, 0, 0, TO_CHAR, 0);
  }
  else if (vict == this)
  {
    act(action->char_auto_, this, 0, 0, TO_CHAR, 0);
    act(action->others_auto_, this, 0, 0, TO_ROOM, (action->hide_) ? INVIS_NULL : 0);
  }
  else if (GET_POS(vict) < action->min_victim_position_)
  {
    act("$N is not in a proper position for that.", this, 0, vict, TO_CHAR, 0);
  }
  else
  {
    act(action->char_found_, this, 0, vict, TO_CHAR, 0);
    act(action->others_found_, this, 0, vict, TO_ROOM, NOTVICT | ((action->hide_) ? INVIS_NULL : 0));
    act(action->vict_found_, this, 0, vict, TO_VICT, (action->hide_) ? INVIS_NULL : 0);
  }

  return SOCIAL_true_WITH_NOISE;
}

QString fread_social_string(QTextStream &in)
{
  auto buffer = in.readLine();
  if (buffer.startsWith('#'))
    return {};
  return buffer;
}

int do_social(Character *ch, char *argument, cmd_t cmd)
{
  for (const auto &social : ch->getDC()->socials_.list())
    ch->send(QStringLiteral("%1").arg(social, 18));

  ch->sendln();
  ch->sendln(QStringLiteral("Current Socials: %1").arg(QString::number(ch->getDC()->socials_.list().length())));
  return eSUCCESS;
}

template <typename T>
auto &operator>>(auto &in, QList<T> &container)
{
  T val;
  while (!in.atEnd())
  {
    in >> val;
    container.push_back(val);
  }
  return in;
}

auto &operator>>(auto &in, Social &social)
{
  social = {};
  in >> social.name_;
  if (social.name_.startsWith('#'))
    return in;

  in >> social.hide_;

  int buffer;
  in >> buffer << Qt::ws;
  social.min_victim_position_ = static_cast<decltype(Social::min_victim_position_)>(buffer);

  social.char_no_arg_ = fread_social_string(in);
  social.others_no_arg_ = fread_social_string(in);

  social.char_found_ = fread_social_string(in);
  // if no char_found, then the social is done, and the ones below won't be there
  if (social.char_found_.isEmpty())
    return in;
  social.others_found_ = fread_social_string(in);
  social.vict_found_ = fread_social_string(in);
  social.not_found_ = fread_social_string(in);
  social.char_auto_ = fread_social_string(in);
  social.others_auto_ = fread_social_string(in);
  return in;
}

QDebug operator<<(QDebug debug, const Social &social)
{
  QDebugStateSaver saver(debug);
  debug.nospace() << "Social{"
                  << ".name_=" << social.name_

                  << ",.char_auto_=" << social.char_auto_
                  << ",.others_auto_=" << social.others_auto_

                  << ",.char_found=_" << social.char_found_
                  << ",.others_found=_" << social.others_found_
                  << ",.vict_found=_" << social.vict_found_
                  << ",.not_found=_" << social.not_found_

                  << ",.char_no_arg_=" << social.char_no_arg_
                  << ",.others_no_arg_=" << social.others_no_arg_

                  << ",.hide_=" << social.hide_
                  << ",.min_victim_position_" << static_cast<uint_fast8_t>(social.min_victim_position_)
                  << "}";
  return debug;
}

Socials::Socials(void)
{
  QFile social_file(SOCIAL_FILE);
  if (!social_file.open(QIODeviceBase::ReadOnly))
  {
    logbug(QStringLiteral("error reading %1").arg(SOCIAL_FILE));
    return;
  }

  QTextStream in(&social_file);
  in >> socials_;

  for (const auto &social : socials_)
  {
    abbreviated_socials_[social.name_] = social;
    for (qsizetype position = 1; position < social.name_.length(); position++)
    {
      auto keyword = social.name_;
      keyword.truncate(social.name_.length() - position);
      if (!abbreviated_socials_.contains(keyword))
      {
        abbreviated_socials_[keyword] = social;
      }
    }
  }
}

auto Socials::find(QString arg) -> std::expected<Social, search_error>
{
  if (abbreviated_socials_.contains(arg))
    return abbreviated_socials_[arg];

  return std::unexpected(search_error::not_found);
}

QStringList Socials::list(void)
{
  QStringList list;
  for (const auto &social : socials_)
    list.push_back(social.name_);
  return list;
}
