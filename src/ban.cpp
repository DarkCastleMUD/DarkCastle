#include "DC/DC.h"

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

ReturnValue Character::do_ban(QStringList arguments, cmd_t cmd)
{
  if (arguments.isEmpty())
  {
    if (dc_->bans_)
    {
      sendln(u"No sites are banned."_s);
      return ReturnValue::eSUCCESS;
    }

    sendln(u"%1  %2  %3  %4"_s.arg("Banned Site Name ", -15).arg("Ban Type", -8).arg("Banned On", -19).arg("Banned By", -16));
    sendln(u"%1  %2  %3  %4"_s.arg("-----------------------", -15).arg("---------------------------------", -8).arg("-------------------", -19).arg("---------------------------------", -16));

    QString buffer;
    for (const auto &ban : dc_->bans_.list())
    {
      sendln(u"%1  %2  %3  %4"_s.arg(ban.site(), -15).arg(Ban::ban_types.value(qsizetype(ban.type())), -8).arg(ban.date().toString(), -19).arg(ban.name(), -16));
    }
    return ReturnValue::eSUCCESS;
  }

  auto flag = arguments.value(0).toUpper();
  auto site = arguments.value(1);
  if (flag.isEmpty() || site.isEmpty())
  {
    sendln(u"Usage: ban {all | select | new} site_name"_s);
    return ReturnValue::eSUCCESS;
  }

  struct sockaddr_in sa{};
  if (inet_pton(AF_INET, qPrintable(site), &(sa.sin_addr)) == 0)
  {
    sendln(u"Invalid IP address."_s);
    return ReturnValue::eFAILURE;
  }

  if (flag != "SELECT" && flag != "ALL" && flag != "NEW")
  {
    sendln(u"Flag must be ALL, SELECT, or NEW."_s);
    return ReturnValue::eSUCCESS;
  }
  switch (dc_->bans_.is_banned(site))
  {
  case Ban::type_t::NOT:
    break;
  case Ban::type_t::NEW:
  case Ban::type_t::SELECT:
  case Ban::type_t::ALL:
    sendln(u"That site has already been banned -- unban it to change the ban type."_s);
    return ReturnValue::eSUCCESS;
    break;
  }

  Ban ban;
  ban.site(site);
  ban.name(name());
  ban.date(QDateTime::fromSecsSinceEpoch(time(0)));
  ban.type(flag);
  dc_->bans_.add(ban);

  loggod(u"1s has banned %2 for %3 players."_s.arg(name()).arg(site).arg(Ban::ban_types.value(qsizetype(ban.type()))));
  sendln(u"Site banned."_s);
  dc_->bans_.save();
  return ReturnValue::eSUCCESS;
}

ReturnValue Character::do_unban(QStringList arguments, cmd_t cmd)
{
  auto site = arguments.value(0);
  if (site.isEmpty())
  {
    sendln(u"A site to unban might help."_s);
    return ReturnValue::eSUCCESS;
  }

  switch (dc_->bans_.is_banned(site))
  {
  case Ban::type_t::NOT:
    sendln(u"That site is not currently banned."_s);
    return ReturnValue::eSUCCESS;
    break;
  case Ban::type_t::NEW:
  case Ban::type_t::SELECT:
  case Ban::type_t::ALL:
    break;
  }

  dc_->bans_.remove(site);
  sendln(u"Site unbanned."_s);
  loggod(u"%1 removed the %2-player ban."_s.arg(name()).arg(site));
  dc_->bans_.save();

  return ReturnValue::eSUCCESS;
}
