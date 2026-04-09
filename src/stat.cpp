#include "DC/DC.h"
#include "DC/handler.h"
#include "DC/returnvals.h"
#include "DC/interp.h"

/******************* Area start **************************************/
enum SortState
{
  SORT_XP,
  SORT_GOLD,
  SORT_MOB
};

class AreaData
{
public:
  void GetAreaData(zone_t zone_nr, qint32 mob, qint64 xps, qint64 gold);
  void DisplaySingleArea(CharacterPtr ch, zone_t area);
  void DisplayAreaData(CharacterPtr ch);
  void SortAreaData(CharacterPtr ch, SortState state);

private:
  area_stats_t areaStats;
};

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
      ch->sendln(QStringLiteral("%1)%1 $5%2$R xps").arg(++i).arg(DC::getInstance()->zones.value(lit.area).name()).arg(lit.xps));
    break;
  case SORT_GOLD:
    std::sort(lAreaStats.begin(), lAreaStats.end(), CompareAreaGoldStats);
    for (const auto &lit : lAreaStats)
      ch->sendln(QStringLiteral("%1)%1 $5%2$R gold").arg(++i).arg(DC::getInstance()->zones.value(lit.area).name()).arg(lit.gold));
    break;
  case SORT_MOB:
    break;
  }
}

void AreaData::DisplaySingleArea(CharacterPtr ch, zone_t area)
{
  if (DC::getInstance()->zones.contains(area) == false)
  {
    ch->send("Area number is outside the limits\r\n");
    return;
  }

  ch->sendln(QStringLiteral("%1)%2 -- $5%3$R xps -- $5%4$R gold").arg(area).arg(DC::getInstance()->zones.value(area).name(), 30).arg(areaStats[area].xps, 12).arg(areaStats[area].gold, 12));
  ch->sendln(QStringLiteral("%1 %2").arg("Mob Name", -30).arg("Killed", -5));

  for (auto mobs = areaStats[area].mobKills.begin(); mobs != areaStats[area].mobKills.end(); mobs++)
  {
    auto tmpchar = get_mob_vnum(mobs->name);
    if (!tmpchar)
    {
      ch->sendln("Shit a mob is missing from game!!!");
      continue;
    }
    ch->sendln(QStringLiteral("%1 %2 %3").arg(get_mob_vnum(mobs->name)->short_description(), -30).arg(mobs->howmany, -5).arg(mobs->howmany < 2 ? "time" : "times"));
  }
}

void AreaData::DisplayAreaData(CharacterPtr ch)
{
  for (auto [zone_key, zone] : DC::getInstance()->zones.asKeyValueRange())
    if (areaStats[zone_key].xps)
      ch->sendln(QStringLiteral("%1)%2|$5%3$R xps|$5%4$R gold|").arg(zone_key).arg(zone.name()).arg(areaStats[zone_key].xps).arg(areaStats[zone_key].gold));
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

qint32 do_areastats(CharacterPtr ch, QString argument, cmd_t cmd)
{
  char buf[MAX_STRING_LENGTH];

  argument = one_argument(argument, buf);
  if (!*buf)
  {
    ch->sendln("$BUsage:$R ");
    ch->sendln("$Bareastats$R ");
    ch->sendln("$Bareastats area #$R");
    ch->sendln("$Bareastats topxps$R");
    ch->sendln("$Bareastats topgold$R");
    return ReturnValue::eSUCCESS;
  }
  if (!strcmp(buf, "area"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("You need to specify a zone number.");
      return ReturnValue::eSUCCESS;
    }
    areaData.DisplaySingleArea(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }
  if (!strcmp(buf, "all"))
  {
    areaData.DisplayAreaData(ch);
    return ReturnValue::eSUCCESS;
  }
  if (!strcmp(buf, "topxps"))
  {
    areaData.SortAreaData(ch, SORT_XP);
    return ReturnValue::eSUCCESS;
  }
  if (!strcmp(buf, "topgold"))
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
