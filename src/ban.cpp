#include "DC/DC.h"
#include "DC/interp.h"

#include <arpa/inet.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <QFile>
#include <QDebug>
#include <QHostAddress>
#include <QIODevice>
#include <QObject>
#include <QtTypes>

void Bans::load(void)
{
  QFile file(BANNED_FILE);

  if (!file.open(QIODeviceBase::Text))
  {
    qWarning() << "Unable to open" << BANNED_FILE;
    return;
  }

  QTextStream out(&file);
  QString name, site_name;
  Ban::type_t ban_type;
  qint32 i = {};
  qint64 date = {};
  list_.clear();
  while (out >> ban_type >> site_name >> date >> name)
  {
    Ban ban;
    ban.name(name);
    ban.site(site_name);
    auto datetime = QDateTime::fromSecsSinceEpoch(date);
    ban.date(datetime);
    switch (ban_type)
    {
    case Ban::type_t::NOT:
    case Ban::type_t::NEW:
    case Ban::type_t::SELECT:
    case Ban::type_t::ALL:
      ban.type(ban_type);
      break;
    }

    add(ban);
  }
}

void Bans::clear(void)
{
  list_.clear();
}

void Bans::add(Ban ban)
{
  if (!ban.site().isNull())
    list_[ban.site()] = ban;
}

void DC::free_ban_list_from_memory(void)
{
  bans_.clear();
}

Ban::type_t Bans::is_banned(QString site) const
{
  return list_.value(site).type();
}

void Ban::save(QTextStream &out) const
{
  out << ban_types.value(qsizetype(type())) << site() << date().toString() << name();
}

void Bans::save(void) const
{
  QFile file(BANNED_FILE);

  if (!file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
  {
    qWarning() << "Unable to open" << BANNED_FILE;
    return;
  }

  QTextStream out(&file);
  for (auto &entry : list_)
  {
    entry.save(out);
  }
}

command_return_t Character::do_ban(QStringList arguments, cmd_t cmd)
{
  if (arguments.isEmpty())
  {
    if (DC::getInstance()->bans_)
    {
      sendln("No sites are banned.");
      return ReturnValue::eSUCCESS;
    }

    sendln(QStringLiteral("%1  %2  %3  %4").arg("Banned Site Name ", -15).arg("Ban Type", -8).arg("Banned On", -19).arg("Banned By", -16));
    sendln(QStringLiteral("%1  %2  %3  %4").arg("-----------------------", -15).arg("---------------------------------", -8).arg("-------------------", -19).arg("---------------------------------", -16));

    QString buffer;
    for (const auto &ban : DC::getInstance()->bans_.list())
    {
      sendln(QStringLiteral("%1  %2  %3  %4").arg(ban.site(), -15).arg(Ban::ban_types.value(qsizetype(ban.type())), -8).arg(ban.date().toString(), -19).arg(ban.name(), -16));
    }
    return ReturnValue::eSUCCESS;
  }

  auto flag = arguments.value(0).toUpper();
  auto site = arguments.value(1);
  if (flag.isEmpty() || site.isEmpty())
  {
    sendln("Usage: ban {all | select | new} site_name");
    return ReturnValue::eSUCCESS;
  }

  struct sockaddr_in sa{};
  if (inet_pton(AF_INET, qPrintable(site), &(sa.sin_addr)) == 0)
  {
    sendln("Invalid IP address.");
    return ReturnValue::eFAILURE;
  }

  if (flag != "SELECT" && flag != "ALL" && flag != "NEW")
  {
    sendln("Flag must be ALL, SELECT, or NEW.");
    return ReturnValue::eSUCCESS;
  }
  switch (DC::getInstance()->bans_.is_banned(site))
  {
  case Ban::type_t::NOT:
    break;
  case Ban::type_t::NEW:
  case Ban::type_t::SELECT:
  case Ban::type_t::ALL:
    sendln("That site has already been banned -- unban it to change the ban type.");
    return ReturnValue::eSUCCESS;
    break;
  }

  Ban ban;
  ban.site(site);
  ban.name(name());
  ban.date(QDateTime::fromSecsSinceEpoch(time(0)));
  ban.type(flag);
  DC::getInstance()->bans_.add(ban);

  loggod(QStringLiteral("1s has banned %2 for %3 players.").arg(name()).arg(site).arg(Ban::ban_types.value(qsizetype(ban.type()))));
  sendln("Site banned.");
  DC::getInstance()->bans_.save();
  return ReturnValue::eSUCCESS;
}

command_return_t Character::do_unban(QStringList arguments, cmd_t cmd)
{
  auto site = arguments.value(0);
  if (site.isEmpty())
  {
    sendln("A site to unban might help.");
    return ReturnValue::eSUCCESS;
  }

  switch (DC::getInstance()->bans_.is_banned(site))
  {
  case Ban::type_t::NOT:
    sendln("That site is not currently banned.");
    return ReturnValue::eSUCCESS;
    break;
  case Ban::type_t::NEW:
  case Ban::type_t::SELECT:
  case Ban::type_t::ALL:
    break;
  }

  DC::getInstance()->bans_.remove(site);
  sendln("Site unbanned.");
  loggod(QStringLiteral("%1 removed the %2-player ban.").arg(name()).arg(site));
  DC::getInstance()->bans_.save();

  return ReturnValue::eSUCCESS;
}
