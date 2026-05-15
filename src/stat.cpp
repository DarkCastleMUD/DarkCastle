#include "DC/DC.h"

/******************* Area start **************************************/

bool CompareAreaXPStats(AreaStats first, AreaStats second)
{
  return first.xps > second.xps;
}

bool CompareAreaGoldStats(AreaStats first, AreaStats second)
{
  return first.gold > second.gold;
}

void AreaData::SortAreaData(CharacterPtr ch, SortState state)
{
  QList<AreaStats> lAreaStats;
  AreaStats aStats;
  qint32 i = {};

  for (const auto &areastat : areaStats)
  {
    switch (state)
    {
    case SORT_XP:
      if (areastat.xps)
        lAreaStats.push_back(areastat);
      break;
    case SORT_GOLD:
      if (areastat.gold)
        lAreaStats.push_back(areastat);
      break;
    case SORT_MOB:
      lAreaStats.push_back(areastat);
      break;
    }
  }

  switch (state)
  {
  case SORT_XP:
    std::sort(lAreaStats.begin(), lAreaStats.end(), CompareAreaXPStats);
    for (const auto &lit : lAreaStats)
      ch->sendln(u"%1)%1 $5%2$R xps"_s.arg(++i).arg(dc_->zones.value(lit.area).name()).arg(lit.xps));
    break;
  case SORT_GOLD:
    std::sort(lAreaStats.begin(), lAreaStats.end(), CompareAreaGoldStats);
    for (const auto &lit : lAreaStats)
      ch->sendln(u"%1)%1 $5%2$R gold"_s.arg(++i).arg(dc_->zones.value(lit.area).name()).arg(lit.gold));
    break;
  case SORT_MOB:
    break;
  }
}

void AreaData::DisplaySingleArea(CharacterPtr ch, zone_t area)
{
  if (dc_->zones.contains(area) == false)
  {
    ch->send(u"Area number is outside the limits\r\n"_s);
    return;
  }

  ch->sendln(u"%1)%2 -- $5%3$R xps -- $5%4$R gold"_s.arg(area).arg(dc_->zones.value(area).name(), 30).arg(areaStats[area].xps, 12).arg(areaStats[area].gold, 12));
  ch->sendln(u"%1 %2"_s.arg("Mob Name", -30).arg("Killed", -5));

  for (auto mobs = areaStats[area].mobKills.begin(); mobs != areaStats[area].mobKills.end(); mobs++)
  {
    auto tmpchar = get_mob_vnum(mobs->name);
    if (!tmpchar)
    {
      ch->sendln(u"Shit a mob is missing from game!!!"_s);
      continue;
    }
    ch->sendln(u"%1 %2 %3"_s.arg(get_mob_vnum(mobs->name)->short_description(), -30).arg(mobs->howmany, -5).arg(mobs->howmany < 2 ? "time" : "times"));
  }
}

void AreaData::DisplayAreaData(CharacterPtr ch)
{
  for (auto [zone_key, zone] : dc_->zones.asKeyValueRange())
    if (areaStats[zone_key].xps)
      ch->sendln(u"%1)%2|$5%3$R xps|$5%4$R gold|"_.arg(zone_key).arg(zone.name()).arg(areaStats[zone_key].xps).arg(areaStats[zone_key].gold));
}

void AreaData::GetAreaData(zone_t zone, qint32 mob, qint64 xps, qint64 gold)
{
  MobKills mobObj;

  areaStats[zone].area = zone;
  if (areaStats.end() == areaStats.find(zone))
  {
    areaStats[zone].xps = xps;
    areaStats[zone].gold = gold;
  }
  else
  {
    areaStats[zone].xps += xps;
    areaStats[zone].gold += gold;
  }

  for (auto mobs = areaStats[zone].mobKills.begin(); mobs != areaStats[zone].mobKills.end(); mobs++)
  {
    if (mobs->name == mob)
    {
      mobs->howmany++;
      return;
    }
  }
  mobObj.name = mob;
  mobObj.howmany = 1;
  areaStats[zone].mobKills.push_back(mobObj);
}

AreaData areaData;

ReturnValues do_areastats(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;

  argument = one_argument(argument, buf);
  if (buf.isEmpty())
  {
    ch->sendln(u"$BUsage:$R "_s);
    ch->sendln(u"$Bareastats$R "_s);
    ch->sendln(u"$Bareastats area #$R"_s);
    ch->sendln(u"$Bareastats topxps$R"_s);
    ch->sendln(u"$Bareastats topgold$R"_s);
    return ReturnValue::eSUCCESS;
  }
  if (buf == u"area"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln(u"You need to specify a zone number."_s);
      return ReturnValue::eSUCCESS;
    }
    areaData.DisplaySingleArea(ch, dc_atoi(buf));
    return ReturnValue::eSUCCESS;
  }
  if (buf == u"all"_s)
  {
    areaData.DisplayAreaData(ch);
    return ReturnValue::eSUCCESS;
  }
  if (buf == u"topxps"_s)
  {
    areaData.SortAreaData(ch, SORT_XP);
    return ReturnValue::eSUCCESS;
  }
  if (buf == u"topgold"_s)
  {
    areaData.SortAreaData(ch, SORT_GOLD);
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eSUCCESS;
}

void getAreaData(quint32 zone, qint32 mob, quint32 xps, quint32 gold)
{
  areaData.GetAreaData(zone, mob, xps, gold);
}
