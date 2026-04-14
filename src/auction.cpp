/*

Item Selling System
Written by: Rubicon
December 13, 2008

Objects:
Auction Class

External: (explained more below)

*/
#include "DC/DC.h"

CharacterPtr find_mob_in_room(CharacterPtr ch, qint32 iFriendId);

void AuctionHouse::ShowStats(CharacterPtr ch)
{
  ch->sendln("Vendor Statistics:");
  ch->send(u"Items Posted:     %1\r\n"_s.arg(ItemsPosted));
  ch->send(u"Items For Sale:   %1\r\n"_s.arg(ItemsActive));
  ch->send(u"Items Sold:       %1\r\n"_s.arg(ItemsSold));
  ch->send(u"Items Expired:    %1\r\n"_s.arg(ItemsExpired));
  ch->send(u"Total Revenue:    %1\r\n"_s.arg(Revenue));
  ch->send(u"Tax Collected:    %1\r\n"_s.arg(TaxCollected));
  ch->send(u"Uncollected Gold: %1\r\n"_s.arg(UncollectedGold));
}

/*
IS RACE
Lists all items wearable by the inputted race
*/
bool AuctionHouse::IsRace(qint32 vnum, QString israce)
{
  qint32 nr = real_object(vnum);

  if (nr < 0)
    return false;

  ObjectPtr obj = (dc_->obj_index[nr].item);

  if (!obj)
    return false;

  if (isSet(obj->obj_flags.size, SIZE_ANY))
    return true;

  israce = israce.toLower();

  if (israce == "human")
    return true;

  if (israce == "ogre")
    return isSet(obj->obj_flags.size, SIZE_LARGE);

  if (israce == "troll")
    return isSet(obj->obj_flags.size, SIZE_LARGE);

  if (israce == "elf")
    return (isSet(obj->obj_flags.size, SIZE_MEDIUM) || isSet(obj->obj_flags.size, SIZE_LARGE));

  if (israce == "orc")
    return (isSet(obj->obj_flags.size, SIZE_MEDIUM) || isSet(obj->obj_flags.size, SIZE_LARGE));

  if (israce == "dwarf")
    return (isSet(obj->obj_flags.size, SIZE_MEDIUM) || isSet(obj->obj_flags.size, SIZE_SMALL));

  if (israce == "gnome")
    return (isSet(obj->obj_flags.size, SIZE_MEDIUM) || isSet(obj->obj_flags.size, SIZE_SMALL));

  if (israce == "pixie")
    return isSet(obj->obj_flags.size, SIZE_SMALL);

  if (israce == "hobbit")
    return isSet(obj->obj_flags.size, SIZE_SMALL);

  return false;
}

/*
IS CLASS
Checks to see if the passed in vnum is wearable by the passed in class
*/
bool AuctionHouse::IsClass(qint32 vnum, QString isclass)
{
  qint32 nr = real_object(vnum);
  if (nr < 0)
    return false;

  ObjectPtr obj = (dc_->obj_index[nr].item);

  if (!obj)
    return false;

  if (IS_OBJ_STAT(obj, ITEM_ANY_CLASS))
    return true;

  QMap<QString, quint64> class_lookup = {
      {"warrior", ITEM_WARRIOR},
      {"mage", ITEM_MAGE},
      {"thief", ITEM_THIEF},
      {"cleric", ITEM_CLERIC},
      {"paladin", ITEM_PAL},
      {"anti", ITEM_ANTI},
      {"barbarian", ITEM_BARB},
      {"ranger", ITEM_RANGER},
      {"bard", ITEM_BARD},
      {"druid", ITEM_DRUID},
      {"monk", ITEM_MONK}};

  for (const auto [class_string, item_type] : class_lookup.asKeyValueRange())
  {
    if (class_string.startsWith(isclass.toLower(), Qt::CaseInsensitive))
    {
      return IS_OBJ_STAT(obj, item_type);
    }
  }

  return false;
}

/*
IS SELLER
checks to see if the passed in name is equal to the seller
*/
bool AuctionHouse::IsSeller(QString in_name, QString seller)
{
  return !seller.compare(in_name, Qt::CaseInsensitive);
}

/*
DO MODIFY
modify an existing tickets price
*/
void AuctionHouse::DoModify(CharacterPtr ch, quint32 ticket, quint32 new_price)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  if (new_price < AUC_MIN_PRICE || new_price > AUC_MAX_PRICE)
  {
    ch->send(u"Price must be between %u and %u.\r\n"_s.arg(AUC_MIN_PRICE).arg(AUC_MAX_PRICE));
    return;
  }
  if ((Item_it = Items_For_Sale.find(ticket)) == Items_For_Sale.end())
  {
    ch->send(u"Ticket number %1 doesn't seem to exist.\r\n"_s.arg(ticket));
    return;
  }

  if (Item_it->seller.compare(ch->name()))
  {
    ch->send(u"Ticket number %1 doesn't belong to you.\r\n"_s.arg(ticket));
    return;
  }

  if (new_price > Item_it->price)
  {
    quint32 difference = new_price - Item_it->price;
    quint32 fee = (quint32)((double)difference * 0.025);
    if (ch->getGold() < fee)
    {
      ch->send(u"Increasing the items price by %u costs %u, you don't have enough.\r\n"_s.arg(difference).arg(fee));
      return;
    }
    ch->removeGold(fee);
    ch->send(u"The broker collects %u coins for increasing the price by %u.\r\n"_s.arg(fee).arg(difference));
    ch->save();
  }

  ch->send(u"The new price of ticket %u (%s) is now %u.\r\n"_s.arg(ticket).arg(qPrintable(Item_it->item_name)).arg(new_price));

  for (auto &vch : dc_->world[ch->in_room].people_)
  {
    if (vch != ch)
    {
      vch->send(u"%1 has just modified the price of one of %2 items.\r\n"_s.arg(qPrintable(ch->name())).arg((GET_SEX(ch) == SEX_MALE) ? "his" : "her"));
    }
  }

  dc_->logentry(u"VEND: %1 modified ticket %2 (%3): old price %4, new price %5.\r\n"_s.arg(qPrintable(ch->name())).arg(Item_it.key()).arg(Item_it->item_name).arg(Item_it->price).arg(new_price), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
  Item_it->price = new_price;
  Save();
}
/*
check for sold items
*/
void AuctionHouse::CheckForSoldItems(CharacterPtr ch)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  bool has_sold_items = false;
  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if (!Item_it->seller.compare(qPrintable(ch->name())))
    {
      if (Item_it->state == AUC_SOLD)
      {
        has_sold_items = true;
        ch->send(u"Your auction of %1 has $2SOLD$R to %2 for %3 coins.\r\n"_s.arg(Item_it->item_name).arg(Item_it->buyer).arg(Item_it->price));
      }
      if (Item_it->state == AUC_EXPIRED)
      {
        has_sold_items = true;
        ch->send(u"Your auction of %1 has $4EXPIRED$R.\r\n"_s.arg(Item_it->item_name));
      }
    }
  }
  if (has_sold_items)
    ch->sendln("Please stop by your local Consignment House.");
}

/*
Handle deletes/zaps
*/
void AuctionHouse::HandleDelete(QString name)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  QQueue<quint32> tickets_to_delete;
  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if (!Item_it->seller.compare(name))
    {
      Item_it->state = AUC_DELETED;
      tickets_to_delete.push_back(Item_it.key());
    }
  }

  QString plural{" "};
  if (tickets_to_delete.size() > 1)
  {
    plural = "s ";
  }

  dc_->logentry(u"%1 auction%2 belonging to %3 have been deleted."_s.arg(tickets_to_delete.size()).arg(plural).arg(name), ANGEL, DC::LogChannel::LOG_GOD);

  while (!tickets_to_delete.isEmpty())
  {
    Items_For_Sale.remove(tickets_to_delete.front());

    std::stringstream obj_filename;
    obj_filename << "../lib/auctions/" << tickets_to_delete.front() << ".auction_obj";
    struct stat sbuf;
    if (stat(obj_filename.str().c_str(), &sbuf) == 0)
    {
      if (unlink(obj_filename.str().c_str()) == -1)
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
      }
    }

    tickets_to_delete.pop_front();
  }
  Save();
}

/*
Handle Renames
*/
void AuctionHouse::HandleRename(CharacterPtr ch, QString old_name, QString new_name)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  quint32 i = {};

  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if (!Item_it->seller.compare(old_name))
    {
      i++;
      Item_it->seller = new_name;
    }
  }

  QString plural{" "};
  if (i > 1)
  {
    plural = "s ";
  }
  dc_->logentry(u"%1 auction%2 have been converted from %3 to %4."_s.arg(i).arg(plural).arg(old_name).arg(new_name), ch->getLevel(), DC::LogChannel::LOG_GOD);
  Save();
}

/*
Is the player selling the max # of items already?
*/
bool AuctionHouse::CanSellMore(CharacterPtr ch)
{
  return true;
}

/*
Is item type ok to sell?
*/
bool AuctionHouse::IsOkToSell(ObjectPtr obj)
{
  if (isSet(obj->obj_flags.more_flags, ITEM_24H_SAVE))
  {
    return false;
  }

  switch (obj->obj_flags.type_flag)
  {
  case ITEM_LIGHT:
  case ITEM_SCROLL:
  case ITEM_WAND:
  case ITEM_STAFF:
  case ITEM_WEAPON:
  case ITEM_FIREWEAPON:
  case ITEM_MISSILE:
  case ITEM_TREASURE:
  case ITEM_ARMOR:
  case ITEM_POTION:
  case ITEM_CONTAINER:
  case ITEM_DRINKCON:
  case ITEM_INSTRUMENT:
  case ITEM_LOCKPICK:
  case ITEM_BOAT:
  case ITEM_KEYRING:
    return true;
    break;
  default:
    return false;
    break;
  }

  return false;
}

/*
Identify an item.
*/
void AuctionHouse::Identify(CharacterPtr ch, quint32 ticket)
{
  qint32 spell_identify(quint8 level, CharacterPtr ch, CharacterPtr victim, ObjectPtr obj, qint32 skill);
  auto Item_it = Items_For_Sale.find(ticket);
  if (Item_it == Items_For_Sale.end())
  {
    ch->send(u"Ticket number %1 doesn't seem to exist.\r\n"_s.arg(ticket));
    return;
  }

  if (!Item_it->buyer.isEmpty() && Item_it->buyer.compare(qPrintable(ch->name())) && Item_it->seller.compare(qPrintable(ch->name())))

  {
    ch->send(u"Ticket number %1 is private.\r\n"_s.arg(ticket));
    return;
  }

  if (ch->getGold() < 6000)
  {
    ch->sendln("The broker charges 6000 $B$5gold$R to identify items.");
    return;
  }

  qint32 nr = real_object(Item_it->vitem);
  if (nr < 0)
  {
    ch->send(u"There is a problem with ticket %1. Please tell an imm.\r\n"_s.arg(ticket));
    return;
  }

  ObjectPtr obj = dc_->ticket_object_load(Item_it, ticket);

  if (!obj)
  {

    dc_->logentry(u"Major screw up in auction(identify)! Item %1 belonging to %2 could not be created!"_s.arg(Item_it->item_name).arg(Item_it->seller), IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  identify(ch, obj);
}

/*
Is item of type slot?
*/
bool AuctionHouse::IsSlot(QString slot, qint32 vnum)
{
  qint32 keyword;
  QString buf = slot;

  static const QStringList keywords =
      {
          "finger", // 0
          "neck",   // 1
          "body",   // 2
          "head",   // 3
          "legs",   // 4
          "feet",   // 5
          "hands",  // 6
          "arms",   // 7
          "about",  // 8
          "waist",  // 9
          "wrist",  // 10
          "face",   // 11
          "wield",  // 12
          "shield", // 13
          "hold",   // 14
          "ear",    // 15
          "light",  // 16
          "\n"};

  keyword = search_list(buf, keywords);
  if (keyword == -1)
    return false;

  //   QString out_buf;
  //  dc_sprintf(out_buf, "%s is an unknown body location.\r\n", buf);
  // ch->send(out_buf);

  qint32 nr = real_object(vnum);

  if (nr < 0)
    return true;

  ObjectPtr obj = (dc_->obj_index[nr].item);
  switch (keyword)
  {
  case 0:
    return CAN_WEAR(obj, FINGER);
    break;
  case 1:
    return CAN_WEAR(obj, NECK);
    break;
  case 2:
    return CAN_WEAR(obj, BODY);
    break;
  case 3:
    return CAN_WEAR(obj, HEAD);
    break;
  case 4:
    return CAN_WEAR(obj, LEGS);
    break;
  case 5:
    return CAN_WEAR(obj, FEET);
    break;
  case 6:
    return CAN_WEAR(obj, HANDS);
    break;
  case 7:
    return CAN_WEAR(obj, ARMS);
    break;
  case 8:
    return CAN_WEAR(obj, ABOUT);
    break;
  case 9:
    return CAN_WEAR(obj, WAISTE);
    break;
  case 10:
    return CAN_WEAR(obj, WRIST);
    break;
  case 11:
    return CAN_WEAR(obj, FACE);
    break;
  case 12:
    return CAN_WEAR(obj, WIELD);
    break;
  case 13:
    return CAN_WEAR(obj, SHIELD);
    break;
  case 14:
    return CAN_WEAR(obj, HOLD);
    break;
  case 15:
    return CAN_WEAR(obj, EAR);
    break;
  case 16:
    return CAN_WEAR(obj, LIGHT_SOURCE);
    break;
  default:
    return false;
    break;
  }
  return false;
}

/*
Is the item wearable by the player?
*/
bool AuctionHouse::IsWearable(CharacterPtr ch, qint32 vnum)
{
  qint32 class_restricted(CharacterPtr ch, ObjectPtr obj);
  qint32 size_restricted(CharacterPtr ch, ObjectPtr obj);
  qint32 nr = real_object(vnum);

  if (nr < 0)
    return true;

  ObjectPtr obj = (dc_->obj_index[nr].item);
  return !(class_restricted(ch, obj) || size_restricted(ch, obj) || (obj->obj_flags.eq_level > ch->getLevel()));
}

/*
Is the player already selling the unique item?
*/
bool AuctionHouse::IsExist(QString name, qint32 vnum)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if ((Item_it->vitem == vnum) && !Item_it->seller.compare(name))
      return true;
  }
  return false;
}

/*
Is the item no_trade?
*/
bool AuctionHouse::IsNoTrade(qint32 vnum)
{
  qint32 nr = real_object(vnum);
  if (nr < 0)
    return false;
  return isSet(((dc_->obj_index[nr].item))->obj_flags.more_flags, ITEM_NO_TRADE);
}

/*
SEARCY BY LEVEL
*/
bool AuctionHouse::IsLevel(quint32 to, quint32 from, qint32 vnum)
{
  qint32 nr;
  if (to > from)
  {
    std::swap(to, from); // @suppress("Invalid arguments")
  }

  quint32 eq_level;
  if (from == 0)
    from = 999;

  if ((nr = real_object(vnum)) < 0)
    return false;

  eq_level = ((dc_->obj_index[nr].item))->obj_flags.eq_level;

  return (eq_level >= to && eq_level <= from);
}

/*
SEARCH BY NAME
*/
bool AuctionHouse::IsName(QString name, qint32 vnum)
{
  ObjectPtr obj = dc_->getObject(vnum);
  if (!obj)
  {
    return false;
  }

  return isexact(name, obj->name());
}

/*
IS AUCTION HOUSE?
*/
bool AuctionHouse::IsAuctionHouse(qint32 room)
{
  if (auction_rooms.end() == auction_rooms.find(room))
    return false;
  else
    return true;
}

/*
LIST ROOMS
*/
void AuctionHouse::ListRooms(CharacterPtr ch)
{
  QMap<qint32, qint32>::iterator room_it;
  ch->send("Auction Rooms:");

  for (room_it = auction_rooms.begin(); room_it != auction_rooms.end(); room_it++)
  {
    ch->send(u" %1"_s.arg(room_it.key()));
  }

  ch->sendln();
}

/*
ADD ROOM
*/
void AuctionHouse::AddRoom(CharacterPtr ch, qint32 room)
{
  if (auction_rooms.end() == auction_rooms.find(room))
  {
    auction_rooms[room] = 1;
    ch->send(u"Done. Room %1 added to auction houses.\r\n"_s.arg(room));
    dc_->logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s just added room %d to auction houses.", qPrintable(ch->name()), room);
    Save();
    return;
  }
  else
    ch->send(u"Room %1 is already an auction house.\r\n"_s.arg(room));
}

/*
REMOVE ROOM
*/
void AuctionHouse::RemoveRoom(CharacterPtr ch, qint32 room)
{
  if (1 == auction_rooms.remove(room))
  {
    ch->send(u"Done. Room %1 has been removed from auction houses.\r\n"_s.arg(room));
    dc_->logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s just removed room %d from auction houses.", qPrintable(ch->name()), room);
    Save();
    return;
  }
  else
    ch->send(u"Room %1 doesn't appear to be an auction house.\r\n"_s.arg(room));
}

void AuctionHouse::ParseStats()
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  ItemsPosted = Items_For_Sale.size();

  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    quint32 fee = Item_it->price * 0.025;
    if (fee > 500000)
      fee = 500000;

    TaxCollected += fee;
    if (Item_it->state == AUC_SOLD)
    {
      Revenue += Item_it->price;
      UncollectedGold += Item_it->price;
      ItemsSold++;
    }

    if (Item_it->state == AUC_EXPIRED)
      ItemsExpired++;

    if (Item_it->state == AUC_FOR_SALE)
      ItemsActive++;
  }
  Save();
}

/*
LOAD
*/
void AuctionHouse::Load()
{
  qint32 room;
  QString nl;
  QString buf;
  AuctionTicket InTicket;

  QFile ah_file(filename_);
  if (!ah_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
  {
    QString buf;
    dc_sprintf(buf, "Unable to open the save file \"%s\" for Auction files!!", qPrintable(filename_));
    dc_->logentry(buf, 0, DC::LogChannel::LOG_MISC);
    return;
  }
  QTextStream in(&ah_file);

  room_t num_rooms;
  in >> num_rooms;

  quint32 num_items;
  in >> num_items;

  for (room_t i = {}; i < num_rooms; i++)
  {
    in >> room;
    auction_rooms[room] = 1;
  }

  in >> num_items;
  for (room_t i = {}; i < num_items; i++)
  {
    quint32 ticket;
    in >> ticket;
    in >> InTicket.vitem;
    in >> InTicket.item_name;
    in >> InTicket.seller;
    in >> InTicket.buyer;
    in >> InTicket.state;
    in >> InTicket.end_time;
    in >> InTicket.price;
    InTicket.obj = {};

    Items_For_Sale[ticket] = InTicket;
  } // LOOP

  if (in.atEnd()) // this means the stat info was lost somehow
    ParseStats();
  else
  {
    in >> ItemsPosted;
    in >> ItemsExpired;
    in >> ItemsSold;
    in >> TaxCollected;
    in >> Revenue;
    in >> ItemsActive;
    in >> UncollectedGold;
  }
}

/*
SAVE
*/
void AuctionHouse::Save()
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  QMap<qint32, qint32>::iterator room_it;

  if (dc_->cf.bport)
  {
    dc_->logentry(u"Unable to save auction files because this is the testport!"_s, ANGEL, DC::LogChannel::LOG_MISC);
    return;
  }
  QSaveFile ah_file(filename_);

  if (!ah_file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
  {
    QString buf;
    dc_sprintf(buf, "Unable to open/create the save file \"%s\" for Auction files!!", qPrintable(filename_));
    dc_->logentry(buf, ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }
  QTextStream out(&ah_file);

  out << auction_rooms.size();
  for (room_it = auction_rooms.begin(); room_it != auction_rooms.end(); room_it++)
  {
    out << room_it.key();
  }

  out << Items_For_Sale.size();
  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    out << Item_it.key();
    out << Item_it->vitem;
    out << Item_it->item_name;
    out << Item_it->seller;
    out << Item_it->buyer;
    out << Item_it->state;
    out << Item_it->end_time;
    out << Item_it->price;

    if (Item_it->obj)
    {
      auto obj_filename = u"../lib/auctions/%1.auction_obj"_s.arg(Item_it.key());
      QSaveFile auction_obj_file(obj_filename);
      if (!QDir(u"../lib/auctions/"_s).exists())
      {
        if (!QDir(u"."_s).mkdir(u"../lib/auctions/"_s))
        {
          qFatal("Unable to create ../lib/auctions/");
          return;
        }
      }
      if (auction_obj_file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
      {
        QTextStream out(&auction_obj_file);
        out << Item_it->obj;
      }
    }
  }

  out << ItemsPosted;
  out << ItemsExpired;
  out << ItemsSold;
  out << TaxCollected;
  out << Revenue;
  out << ItemsActive;
  out << UncollectedGold;
}

AuctionHouse::AuctionHouse(QString filename, DCPtr dc)
    : filename_(filename), dc_(dc)
{
}

/*
CANCEL ALL
*/
void AuctionHouse::CancelAll(CharacterPtr ch)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  QQueue<quint32> tickets_to_cancel;
  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if (!Item_it->seller.compare(qPrintable(ch->name())))
      tickets_to_cancel.push_back(Item_it.key());
  }
  if (tickets_to_cancel.isEmpty())
  {
    ch->sendln("You have no tickets to cancel!");
    return;
  }
  while (!tickets_to_cancel.isEmpty())
  {
    RemoveTicket(ch, tickets_to_cancel.front());
    tickets_to_cancel.pop_front();
  }
}

/*
COLLECT EXPIRED
*/
void AuctionHouse::CollectTickets(CharacterPtr ch, quint32 ticket)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  if (ticket > 0)
  {
    Item_it = Items_For_Sale.find(ticket);
    if (Item_it != Items_For_Sale.end())
    {
      if (Item_it->seller.compare(qPrintable(ch->name())))
      {
        ch->send(u"Ticket %1 doesn't seem to belong to you.\r\n"_s.arg(ticket));
        return;
      }
      if (Item_it->state != AUC_EXPIRED && Item_it->state != AUC_SOLD)
      {
        ch->send(u"Ticket %1 isn't collectible!.\r\n"_s.arg(ticket));
        return;
      }
    }
    RemoveTicket(ch, ticket);
    return;
  }

  QQueue<quint32> tickets_to_remove;
  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if ((Item_it->state == AUC_EXPIRED || Item_it->state == AUC_SOLD) && !Item_it->seller.compare(qPrintable(ch->name())))
      tickets_to_remove.push_back(Item_it.key());
  }
  if (tickets_to_remove.isEmpty())
  {
    ch->sendln("You have nothing to collect!");
    return;
  }
  while (!tickets_to_remove.isEmpty())
  {
    RemoveTicket(ch, tickets_to_remove.front());
    tickets_to_remove.pop_front();
  }
}

/*
CHECK EXPIRE
*/
void AuctionHouse::CheckExpire()
{
  quint32 cur_time = time(0);
  CharacterPtr ch;
  bool something_expired = false;
  QMap<quint32, AuctionTicket>::iterator Item_it;

  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if (Item_it->state == AUC_FOR_SALE && cur_time >= Item_it->end_time)
    {
      ItemsActive -= 1;
      ItemsExpired += 1;
      if ((ch = get_active_pc(qPrintable(Item_it->seller))))
        ch->send(u"Your auction of %s has expired.\r\n"_s.arg(qPrintable(Item_it->item_name)));
      Item_it->state = AUC_EXPIRED;
      something_expired = true;
    }
  }

  if (something_expired)
    Save();
}

/*
BUY ITEM
*/
void AuctionHouse::BuyItem(CharacterPtr ch, quint32 ticket)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  ObjectPtr obj;
  CharacterPtr vict;
  qint32 i = {};

  Item_it = Items_For_Sale.find(ticket);
  if (Item_it == Items_For_Sale.end())
  {
    ch->send(u"Ticket number %1 doesn't seem to exist.\r\n"_s.arg(ticket));
    return;
  }

  if (!Item_it->buyer.isEmpty() && Item_it->buyer.compare(qPrintable(ch->name())))
  {
    ch->send(u"Ticket number %1 is private.\r\n"_s.arg(ticket));
    return;
  }

  if (Item_it->state != AUC_FOR_SALE)
  {
    ch->send(u"Ticket number %1 has already been sold\r\n."_s.arg(ticket));
    return;
  }

  if (!Item_it->seller.compare(qPrintable(ch->name())))
  {
    ch->sendln("That's your own item you're selling, dumbass!");
    return;
  }

  if (ch->getGold() < Item_it->price)
  {
    ch->send(u"Ticket number %d costs %d coins, you can't afford that!\r\n"_s.arg(ticket).arg(Item_it->price));
    return;
  }

  qint32 rnum = real_object(Item_it->vitem);

  if (rnum < 0)
  {
    QString buf;
    dc_sprintf(buf, "Major screw up in auction(buy)! Item %s[VNum %d] belonging to %s could not be created!",
               qPrintable(Item_it->item_name), Item_it->vitem, qPrintable(Item_it->seller));
    dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  obj = dc_->ticket_object_load(Item_it, ticket);

  if (!obj)
  {
    QString buf;
    dc_sprintf(buf, "Major screw up in auction(buy)! Item %s[RNum %d] belonging to %s could not be created!",
               qPrintable(Item_it->item_name), rnum, qPrintable(Item_it->seller));
    dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && search_char_for_item(ch, obj->item_number, false))
  {
    ch->sendln("Why would you want another one of those?");
    return;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_NO_TRADE))
  {
    ObjectPtr no_trade_obj;
    qint32 nr = real_object(27909);

    no_trade_obj = search_char_for_item(ch, nr, false);

    if (!no_trade_obj)
    { // 27909 == wingding right now (notrade transfer token)
      if (nr > 0)
        ch->send(u"You need to have \"%s\"_s to buy a NO_TRADE item.\r\n"_s.arg(dc_->obj_index[nr].item->short_description()));
      return;
    }
    else
    {
      ch->send(u"You give your %1 to the broker.\r\n"_s.arg(no_trade_obj->short_description()));
      extract_obj(no_trade_obj);
    }
  }

  std::stringstream obj_filename;
  obj_filename << "../lib/auctions/" << ticket << ".auction_obj";
  struct stat sbuf;
  if (stat(obj_filename.str().c_str(), &sbuf) == 0)
  {
    if (unlink(obj_filename.str().c_str()) == -1)
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
      return;
    }
  }

  Item_it->obj = {};
  ItemsSold += 1;
  ItemsActive -= 1;
  Revenue += Item_it->price;
  UncollectedGold += Item_it->price;
  ch->removeGold(Item_it->price);

  if ((vict = get_active_pc(qPrintable(Item_it->seller))))
    vict->send(u"%s just purchased your ticket of %s for %u coins.\r\n"_s.arg(qPrintable(ch->name())).arg(qPrintable(Item_it->item_name)).arg(Item_it->price));

  ch->send(u"You have purchased %s for %u coins.\r\n"_s.arg(qPrintable(obj->short_description())).arg(Item_it->price));

  for (auto &vch : dc_->world[ch->in_room].people_)
    if (vch != ch)
      vch->send(u"%s just purchased %s's %s\r\n"_s.arg(qPrintable(ch->name())).arg(qPrintable(Item_it->seller)).arg(qPrintable(obj->short_description())));

  Item_it->state = AUC_SOLD;
  Item_it->buyer = qPrintable(ch->name());

  Save();
  QString log_buf = {};
  dc_sprintf(log_buf, "VEND: %s bought %s's %s[%d] for %u coins.\r\n", qPrintable(ch->name()), qPrintable(Item_it->seller), qPrintable(Item_it->item_name), Item_it->vitem, Item_it->price);
  dc_->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
  obj_to_char(obj, ch);
  ch->save();

  if (dc_->cf.bport == false)
  {
    errno = {};
    QFile auction_file(WEB_AUCTION_FILE);
    if (!auction_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "%s: %s", qPrintable(WEB_AUCTION_FILE), strerror(errno));
      return;
    }
    QTextStream in(&auction_file);

    QString buffer = in.readAll();
    auction_file.close();

    if (!auction_file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "%s: %s", qPrintable(WEB_AUCTION_FILE), strerror(errno));
      return;
    }
    QTextStream out(&auction_file);
    out << u"%1 purchased %2's %3~\n"_s.arg(ch->name()).arg(Item_it->seller).arg(obj->short_description());
    out << buffer;
  }
  else
  {
    dc_->logentry(u"bport mode: Not saving auction file to web dir."_s, 0, DC::LogChannel::LOG_MISC);
  }
}

ObjectPtr DC::ticket_object_load(QMap<quint32, AuctionTicket>::iterator Item_it, qint32 ticket)
{
  // If obj is nullptr then either we haven't loaded this object yet or it's not custom
  if (Item_it->obj == nullptr)
  {

    QString obj_filename = u"../lib/auctions/"_s + QString::number(ticket) + u".auction_obj"_s;
    QFile auction_obj_file(obj_filename);

    if (auction_obj_file.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly))
    {
      QTextStream in(&auction_obj_file);
      Item_it->obj = ObjectPtr(new Object(this));
      in >> Item_it->obj;
    }
  }

  ObjectPtr obj;
  qint32 rnum = real_object(Item_it->vitem);
  // If load was successful use it as a copy reference for a clone
  if (Item_it->obj)
  {
    ObjectPtr reference_obj = Item_it->obj;

    obj = clone_object(rnum);
    copySaveData(obj, reference_obj);

    if (verify_item(&obj))
    {
      copySaveData(obj, reference_obj);
    }
  }
  else
  {
    obj = clone_object(rnum);
  }

  return obj;
}

/*
CANCEL ITEM
*/
void AuctionHouse::RemoveTicket(CharacterPtr ch, quint32 ticket)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  ObjectPtr obj;
  bool expired = false;

  Item_it = Items_For_Sale.find(ticket);
  if (Item_it == Items_For_Sale.end())
  {
    ch->send(u"Ticket number %1 doesn't seem to exist.\r\n"_s.arg(ticket));
    return;
  }
  if (Item_it->seller.compare(qPrintable(ch->name())))
  {
    ch->send(u"Ticket number %1 doesn't belong to you.\r\n"_s.arg(ticket));
    return;
  }

  switch (Item_it->state)
  {
  case AUC_SOLD:
  {
    quint32 fee = (quint32)((double)Item_it->price * 0.025);
    if (fee > 500000)
      fee = 500000;
    ch->sendln(u"The Broker hands you %u $B$5gold$R coins from your sale of %s.\r\nHe pockets %u $B$5gold$R as a broker's fee."_s.arg((Item_it->price - fee)).arg(Item_it->item_name).arg(fee));
    TaxCollected += fee;
    Revenue -= fee;
    UncollectedGold -= Item_it->price;
    ch->addGold(Item_it->price - fee);
    QString log_buf = {};
    dc_sprintf(log_buf, "VEND: %s just collected %u coins from their sale of %s (ticket %u).\r\n",
               qPrintable(ch->name()), Item_it->price, qPrintable(Item_it->item_name), ticket);
    dc_->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
  }
  break;
  case AUC_EXPIRED: // intentional fallthrough
    expired = true;
    /* no break */
  case AUC_FOR_SALE:
  {
    qint32 rnum = real_object(Item_it->vitem);
    if (!expired)
      ItemsActive -= 1; // this is removed during expiration check
    if (rnum < 0)
    {
      QString buf;
      dc_sprintf(buf, "Major screw up in auction(cancel)! Item %s[VNum %d] belonging to %s could not be created!",
                 qPrintable(Item_it->item_name), Item_it->vitem, qPrintable(Item_it->seller));
      dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }

    if (isSet(((dc_->obj_index[rnum].item))->obj_flags.more_flags, ITEM_UNIQUE) && search_char_for_item(ch, rnum, false))
    {
      ch->sendln("Why would you want another one of those?");
      return;
    }

    obj = dc_->ticket_object_load(Item_it, ticket);
    if (!obj)
    {
      QString buf;
      dc_sprintf(buf, "Major screw up in auction(RemoveTicket)! Item %s[RNum %d] belonging to %s could not be created!",
                 qPrintable(Item_it->item_name), rnum, qPrintable(Item_it->seller));
      dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }

    ch->send(u"The Consignment Broker retrieves %1 and returns it to you.\r\n"_s.arg(obj->short_description()));
    QString log_buf = {};
    dc_sprintf(log_buf, "VEND: %s cancelled or collected ticket # %u (%s) that was for sale for %u coins.\r\n",
               qPrintable(ch->name()), ticket, qPrintable(Item_it->item_name), Item_it->price);
    dc_->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
    obj_to_char(obj, ch);
  }
  break;
  case AUC_DELETED:
  {
    QString buf;
    dc_sprintf(buf, "%s just tried to cheat and collect ticket %u which didn't get erased properly!", qPrintable(ch->name()), ticket);
    dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    Items_For_Sale.remove(ticket);

    std::stringstream obj_filename;
    obj_filename << "../lib/auctions/" << ticket << ".auction_obj";
    struct stat sbuf;
    if (stat(obj_filename.str().c_str(), &sbuf) == 0)
    {
      if (unlink(obj_filename.str().c_str()) == -1)
      {
        dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
      }
    }

    return;
  }
  break;
  default:
    dc_->logentry(u"Default case reached in Removeticket, contact a coder!"_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    break;
  }

  Item_it->state = AUC_DELETED; // just a safety precaution
  ch->save();

  if (1 != Items_For_Sale.remove(ticket))
  {
    QString buf;
    dc_sprintf(buf, "Major screw up in auction(cancel)! Ticket %d belonging to %s could not be removed!",
               ticket, qPrintable(Item_it->seller));
    dc_->logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  std::stringstream obj_filename;
  obj_filename << "../lib/auctions/" << ticket << ".auction_obj";
  struct stat sbuf;
  if (stat(obj_filename.str().c_str(), &sbuf) == 0)
  {
    if (unlink(obj_filename.str().c_str()) == -1)
    {
      dc_->logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
    }
  }

  Save();
}

/*
LIST ITEMS
*/
void AuctionHouse::ListItems(CharacterPtr ch, ListOptions options, QString name, quint32 to, quint32 from)
{
  QMap<quint32, AuctionTicket>::iterator Item_it;
  QQueue<QString> recent;
  qint32 i;
  QString output_buf;
  QString state_output;
  QString buf;

  if (options == LIST_MINE)
    ch->sendln("Ticket-Buyer--------Price------Status--T--Item---------------------------");
  else
    ch->sendln("Ticket-Seller-------Price------Status--T--Item---------------------------");

  for (i = 0, Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if (
        (options == LIST_MINE && !Item_it->seller.compare(qPrintable(ch->name()))) || (options == LIST_ALL) || (options == LIST_RECENT) || (options == LIST_PRIVATE && !Item_it->buyer.compare(qPrintable(ch->name()))) || (options == LIST_BY_NAME && IsName(name, Item_it->vitem)) || (options == LIST_BY_LEVEL && IsLevel(to, from, Item_it->vitem)) || (options == LIST_BY_SLOT && IsSlot(name, Item_it->vitem)) || (options == LIST_BY_SELLER && IsSeller(name, Item_it->seller)) || (options == LIST_BY_CLASS && IsClass(Item_it->vitem, name)) || (options == LIST_BY_RACE && IsRace(Item_it->vitem, name)))
    {
      // don't show it if its expired or sold and its not the searchers item
      if ((Item_it->state == AUC_EXPIRED || Item_it->state == AUC_SOLD) && options != LIST_MINE && ch->getLevel() < OVERSEER)
        continue;

      /*
      Things guaranteed at this point:
      Auction is being actively sold. (or the searcher is viewing their expired/sold items)
      Search parameters are met.
      */

      if (!Item_it->buyer.isEmpty() && ch->getLevel() < OVERSEER) // if its a private auction
      {
        if (Item_it->buyer.compare(qPrintable(ch->name()))      // if the buyer isn't the searcher
            && Item_it->seller.compare(qPrintable(ch->name()))) // and isn't the seller
          continue;
      }

      switch (Item_it->state)
      {
      case AUC_SOLD:
        state_output = "$0$BSOLD$R   ";
        break;
      case AUC_FOR_SALE:
        if (Item_it->buyer.isEmpty())
          state_output = "$2PUBLIC$R ";
        else
          state_output = "$2PRIVATE$R";
        break;
      case AUC_EXPIRED:
        state_output = "$4EXPIRED$R";
        break;
      default:
        state_output = "$7ERROR$R  ";
        break;
      }
      i++;

      dc_sprintf(buf, "\r\n%05d) $7$B%-12s$R $5%-10s$R %s %s %s%-30s\r\n",
                 Item_it.key(),
                 (options == LIST_MINE) ? qPrintable(Item_it->buyer) : qPrintable(Item_it->seller),
                 qPrintable(QString::number(Item_it->price)),
                 qPrintable(state_output), IsNoTrade(Item_it->vitem) ? "$4N$R" : " ",
                 IsWearable(ch, Item_it->vitem) ? " " : "$4*$R", qPrintable(Item_it->item_name));
      if (options == LIST_RECENT)
      {
        recent.push_back(buf);
        if (recent.size() > 50)
          recent.pop_front();
      }
      else
      {
        output_buf += buf;
      }
    }
  }

  if (options == LIST_RECENT)
  {
    while (recent.size() > 0)
    {
      output_buf += recent.front();
      recent.pop_front();
    }
  }

  if (i == 0)
  {
    if (options == LIST_BY_SLOT)
      ch->send(u"\r\nThere are no %s items currently posted.\r\n"_s.arg(qPrintable(name)));
    else if (options == LIST_BY_SELLER)
      ch->sendln(u"\r\n\"%s\" doesn't seem to be selling any public items.\r\n\r\nTo view private items, use \"vend list private\".\r\n"_s).arg(name);
    else if (options == LIST_BY_CLASS)
      ch->send(u"\r\nThere are no \"%s\" wearable public items for sale.\r\n"_s.arg(qPrintable(name)));
    else if (options == LIST_MINE)
      ch->sendln("\r\nYou do not have any tickets.");
    else if (options == LIST_BY_RACE)
      ch->send(u"\r\nThere is nothing for sale that would fit a \"%s\".\r\n"_s.arg(qPrintable(name)));
    else
      ch->sendln("\r\nThere is nothing for sale!");
  }

  if (options == LIST_MINE)
  {
    auto vault = dc_->vaults_.has_vault(ch->name());
    if (vault)
    {
      qint32 max_items = vault->size_ / 100;
      ch->send(u"\r\nYou are using %d of your %d available tickets.\r\n"_s.arg(i).arg(max_items));
    }
  }

  if (i > 0) // only display this if there was at least 1 item listed
  {
    qint32 nr = real_object(27909);
    if (nr >= 0)
    {
      dc_sprintf(buf, "\r\n'$4N$R' indicates an item is NO_TRADE and requires %s to purchase.\r\n", qPrintable(((dc_->obj_index[nr].item))->short_description()));
      output_buf += buf;
    }
    output_buf += "'$4*$R' indicates you are unable to use this item.\r\n";
  }

  page_string(ch->desc, qPrintable(output_buf), 1);
}

/*
ADD ITEM
*/
void AuctionHouse::AddItem(CharacterPtr ch, ObjectPtr obj, quint32 price, QString buyer)
{
  buyer = buyer.toLower();
  buyer[0] = buyer[0].toUpper();

  if (!obj)
  {
    ch->sendln("You don't seem to have that item.");
    return;
  }

  if (!IsOkToSell(obj))
  {
    ch->sendln("You can't sell that type of item here!");
    return;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_NOSAVE))
  {
    ch->sendln("You can't sell that item!");
    return;
  }

  if (isSet(obj->obj_flags.extra_flags, ITEM_SPECIAL))
  {
    ch->sendln("You can't sell godload.");
    return;
  }

  if (ARE_CONTAINERS(obj) && obj->contains)
  { // non-empty containers
    ch->send(u"%s needs to be emptied first.\r\n"_s.arg(GET_OBJ_SHORT(obj)));
    return;
  }

  if (price > AUC_MAX_PRICE)
  {
    ch->send(u"Price must be between %u and %u.\r\n"_s.arg(AUC_MIN_PRICE).arg(AUC_MAX_PRICE));
    return;
  }

  quint32 full_checker = cur_index;
  while (Items_For_Sale.end() != Items_For_Sale.find(++cur_index))
  {
    if (cur_index > 99999)
      cur_index = 1;
    if (full_checker == cur_index)
    {
      ch->sendln("The auctioneer is selling too many items already!");
      return;
    }
  }

  if (buyer == ch->name())
  {
    ch->sendln("Why would you want to privately sell something to yourself?");
    return;
  }

  // Players no longer need to specify Advertise to advertise an auction
  if (buyer == "Advertise")
  {
    buyer.clear();
  }

  bool advertise{};
  if (price >= 1000000)
  {
    advertise = true;
  }

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && IsExist(ch->name(), dc_->obj_index[obj->item_number].vnum()))
  {
    ch->send(u"You're selling %1 already and it's unique!\r\n"_s.arg(obj->short_description()));
    return;
  }

  if (obj->short_description() != dc_->obj_index[obj->item_number].item->short_description())
  {
    ch->sendln("The Consignment broker informs you that he does not handle items that have been restrung.");
    return;
  }

  if (eq_current_damage(obj) > 0)
  {
    ch->sendln("The Consignment Broker curtly informs you that all items sold must be in $B$2Excellent Condition$R.");
    return;
  }

  if ((obj->obj_flags.type_flag == ITEM_WAND || obj->obj_flags.type_flag == ITEM_STAFF) && obj->obj_flags.value[1] != obj->obj_flags.value[2])
  {
    ch->sendln("The Consignment Broker curtly informs you that it needs to have full charges.");
    return;
  }

  // Private sales cost money now
  if (!buyer.isEmpty())
  {
    if (ch->getGold() < 500000)
    {
      ch->sendln("You do not have the 500,000 $B$5gold$R required to sell an item privately.");
      return;
    }

    ch->removeGold(500000);
    ch->sendln("The Consignment Broker takes 500,000 $B$5gold$R from you as a cost for selling something privately.");
  }

  ItemsPosted += 1;
  ItemsActive += 1;

  AuctionTicket NewTicket;
  NewTicket.vitem = dc_->obj_index[obj->item_number].vnum();
  NewTicket.price = price;
  NewTicket.state = AUC_FOR_SALE;
  NewTicket.end_time = time(0) + auction_duration;
  NewTicket.seller = qPrintable(ch->name());
  NewTicket.item_name = qPrintable(obj->short_description());

  if (!buyer.isEmpty())
  {
    NewTicket.buyer = buyer;
  }

  if (fullSave(obj))
  {
    NewTicket.obj = obj;
  }
  else
  {
    NewTicket.obj = {};
  }

  Items_For_Sale[cur_index] = NewTicket;
  Save();

  if (buyer.isEmpty())
    ch->sendln(u"You are now selling %1 for %2 coins."_s.arg(obj->short_description()).arg(price));
  else
  {
    ch->sendln(u"You are now selling %1 to %2 for %3 coins."_s.arg(obj->short_description()).arg(buyer).arg(price));
  }

  if (advertise == true && NewTicket.buyer.isEmpty())
  {
    QString auc_buf;
    auto Broker = find_mob_in_room(ch, 5258);
    if (!Broker)
    {
      Broker = get_mob_vnum(5258);
    }

    if (Broker)
    {
      Broker->do_auction(u"%1 is selling \"_s$R%2$R$6$B\" for %3 gold."_s.arg(ch->shortdesc_or_name()).arg(obj->short_description()).arg(price).split(' '));
    }
    else
    {
      ch->sendln("The Consignment Broker couldn't auction. Contact an imm.");
      dc_->logentry(u"CharacterPtr Broker was nullptr in AuctionHouse::AddItem([%1], [%2], [%3], [%4])"_s.arg(ch->name()).arg(GET_OBJ_SHORT(obj)).arg(price).arg(buyer));
    }
  }

  QString log_buf = {};
  if (NewTicket.buyer.isEmpty())
  {
    dc_sprintf(log_buf, "VEND: %s just listed %s for sale for %u coins.\r\n", qPrintable(ch->name()), qPrintable(obj->short_description()), price);
  }
  else
  {
    dc_sprintf(log_buf, "VEND: %s just listed %s for sale for %u coins for %s.\r\n", qPrintable(ch->name()), qPrintable(obj->short_description()), price, qPrintable(NewTicket.buyer));
  }
  dc_->logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);

  // If this is a custom item we need it to continue existing otherwise we remove the clone
  if (fullSave(obj))
  {
    obj_from_char(obj);
  }
  else
  {
    extract_obj(obj);
  }

  ch->save();
}

command_return_t do_vend(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString buf;
  ObjectPtr obj;
  quint32 price;

  if (!ch->dc_->TheAuctionHouse.IsAuctionHouse(ch->in_room) && ch->getLevel() < 104)
  {
    ch->sendln("You must be in an auction house to do this!");
    return ReturnValue::eFAILURE;
  }

  if (ch->isPlayerObjectThief() || (ch->isPlayerGoldThief()))
  {
    ch->sendln("You're too busy running from the law!");
    return ReturnValue::eFAILURE;
  }

  argument = one_argument(argument, buf);

  if (buf.isEmpty())
  {
    ch->sendln("Syntax: vend <buy | sell | list | cancel | modify | collect | search | identify>");
    if (ch->getLevel() >= 104)
      ch->sendln("Also: <addroom | removeroom | listrooms | stats>");
    return ReturnValue::eSUCCESS;
  }

  /*MODIFY*/
  if (buf == u"modify"_s)
  {
    quint32 ticket;
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Modify what ticket?\r\nSyntax: vend modify <ticket> <new_price>");
      return ReturnValue::eSUCCESS;
    }
    ticket = atoi(buf);
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("What price do you want it?\r\nSyntax: vend modify <ticket> <new_price>");
      return ReturnValue::eSUCCESS;
    }
    dc_->TheAuctionHouse.DoModify(ch, ticket, atoi(buf));

    return ReturnValue::eSUCCESS;
  }

  /*SEARCH*/
  if (buf == u"search"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Search by what?\r\nSyntax: vend search <name | level | slot | seller | race | class>");
      return ReturnValue::eSUCCESS;
    }
    if (buf == u"name"_s)
    {
      argument = one_argument(argument, buf);
      if (buf.isEmpty())
      {
        ch->sendln("What name do you want to search for?\r\nSyntax: vend search name <keyword>");
        return ReturnValue::eSUCCESS;
      }
      dc_->TheAuctionHouse.ListItems(ch, LIST_BY_NAME, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (buf == u"slot"_s)
    {
      argument = one_argument(argument, buf);
      if (buf.isEmpty())
      {
        send_to_char("What slot do you want to search for?\r\n"
                     "finger, neck, body, head, legs, feet, hands, arms, shield,\r\n"
                     "about, waist, wrist, wield, hold, throw, light, face, ear\r\n"
                     "\r\nSyntax: vend search slot <keyword>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      dc_->TheAuctionHouse.ListItems(ch, LIST_BY_SLOT, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (buf == u"race"_s)
    {
      argument = one_argument(argument, buf);
      if (buf.isEmpty())
      {
        send_to_char("What race do you want to search for?\r\n"
                     "Human, Elf, Dwarf, Hobbit, Pixie, Gnome, Orc, Troll\r\n"
                     "\r\nSyntax: vend search race <race>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      dc_->TheAuctionHouse.ListItems(ch, LIST_BY_RACE, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (buf == u"class"_s)
    {
      argument = one_argument(argument, buf);
      if (buf.isEmpty())
      {
        send_to_char("What class do you want to search for?\r\n"
                     "\r\nSyntax: vend search class <class_name>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      if (dc_strlen(buf) < 4)
      {
        ch->sendln("Class name needs to be at least 4 letters to search!");
        return ReturnValue::eSUCCESS;
      }
      dc_->TheAuctionHouse.ListItems(ch, LIST_BY_CLASS, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (buf == u"seller"_s)
    {
      argument = one_argument(argument, buf);
      if (buf.isEmpty())
      {
        send_to_char("What person are you looking for?\r\n"
                     "\r\nSyntax: vend search seller <name>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      dc_->TheAuctionHouse.ListItems(ch, LIST_BY_SELLER, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (buf == u"level"_s)
    {
      quint32 level;
      argument = one_argument(argument, buf);
      if (buf.isEmpty())
      {
        ch->sendln("What level?\r\nSyntax: vend search level <min_level> [max_level]");
        return ReturnValue::eSUCCESS;
      }
      level = atoi(buf);
      argument = one_argument(argument, buf);
      if (buf.isEmpty())
        dc_->TheAuctionHouse.ListItems(ch, LIST_BY_LEVEL, "", level, 0);
      else
        dc_->TheAuctionHouse.ListItems(ch, LIST_BY_LEVEL, "", level, atoi(buf));
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    ch->sendln("Search by what?\r\nSyntax: vend search <name | level | slot | seller | race | class>");
    return ReturnValue::eSUCCESS;
  }

  /*COLLECT*/
  if (buf == u"collect"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Collect what?\r\nSyntax: vend collect <all | ticket#>");
      return ReturnValue::eSUCCESS;
    }
    if (buf == u"all"_s)
    {
      dc_->TheAuctionHouse.CollectTickets(ch);
      return ReturnValue::eSUCCESS;
    }
    if (atoi(buf) > 0)
    {
      dc_->TheAuctionHouse.CollectTickets(ch, atoi(buf));
      return ReturnValue::eSUCCESS;
    }

    ch->sendln("Syntax: vend collect <all | ticket#>");
    return ReturnValue::eSUCCESS;
  }

  /*BUY*/
  if (buf == u"buy"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Buy what?\r\nSyntax: vend buy <ticket #>");
      return ReturnValue::eSUCCESS;
    }
    dc_->TheAuctionHouse.BuyItem(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*CANCEL*/
  if (buf == u"cancel"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Cancel what?\r\nSyntax: vend cancel <all | ticket#>");
      return ReturnValue::eSUCCESS;
    }
    if (buf == u"all"_s) // stupid cancel all didn't fit my design, but the boss wanted it
    {
      dc_->TheAuctionHouse.CancelAll(ch);
      return ReturnValue::eSUCCESS;
    }
    dc_->TheAuctionHouse.RemoveTicket(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*LIST*/
  if (buf == u"list"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("List what?\r\nSyntax: vend list <all | mine | private | recent>");
      return ReturnValue::eSUCCESS;
    }

    if (buf == u"all"_s)
    {
      dc_->TheAuctionHouse.ListItems(ch, LIST_ALL, "", 0, 0);
    }
    else if (buf == u"mine"_s)
    {
      dc_->TheAuctionHouse.ListItems(ch, LIST_MINE, "", 0, 0);
    }
    else if (buf == u"private"_s)
    {
      dc_->TheAuctionHouse.ListItems(ch, LIST_PRIVATE, "", 0, 0);
    }
    else if (buf == u"recent"_s)
    {
      dc_->TheAuctionHouse.ListItems(ch, LIST_RECENT, "", 0, 0);
    }
    else
    {
      ch->sendln("List what?\r\nSyntax: vend list <all | mine | private>");
    }
    return ReturnValue::eSUCCESS;
  }

  /*SELL*/
  if (buf == u"sell"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Sell what?\r\nSyntax: vend sell <item> <price> [person]");
      return ReturnValue::eSUCCESS;
    }
    obj = get_obj_in_list_vis(ch, buf, ch->carrying);
    if (!obj)
    {
      ch->sendln("You don't seem to have that item.\r\nSyntax: vend sell <item> <price> [person]");
      return ReturnValue::eSUCCESS;
    }
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("How much do you want to sell it for?\r\nSyntax: vend sell <item> <price> [person]");
      return ReturnValue::eSUCCESS;
    }
    price = atoi(buf);
    if (price < 1000)
    {
      ch->sendln("Minimum sell price is 1000 coins!");
      return ReturnValue::eSUCCESS;
    }

    argument = one_argument(argument, buf); // private name
    dc_->TheAuctionHouse.AddItem(ch, obj, price, buf);
    return ReturnValue::eSUCCESS;
  }

  /*IDENTIFY*/
  if (buf == u"identify"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Identify what?\r\nSyntax: vend identify <ticket>");
      return ReturnValue::eSUCCESS;
    }
    dc_->TheAuctionHouse.Identify(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*SHOW STATS*/
  if (ch->getLevel() >= 104 && buf == u"stats"_s)
  {
    dc_->TheAuctionHouse.ShowStats(ch);
    return ReturnValue::eSUCCESS;
  }

  /*ADD ROOM*/
  if (ch->getLevel() >= 104 && buf == u"addroom"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Add what room?\r\nSyntax: vend addroom <vnum>");
      return ReturnValue::eSUCCESS;
    }
    dc_->TheAuctionHouse.AddRoom(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*REMOVE ROOM*/
  if (ch->getLevel() >= 104 && buf == u"removeroom"_s)
  {
    argument = one_argument(argument, buf);
    if (buf.isEmpty())
    {
      ch->sendln("Remove what room?\r\nSyntax: vend removeroom <vnum>");
      return ReturnValue::eSUCCESS;
    }
    dc_->TheAuctionHouse.RemoveRoom(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*LIST ROOMS*/
  if (ch->getLevel() >= 104 && buf == u"listrooms"_s)
  {
    dc_->TheAuctionHouse.ListRooms(ch);
    return ReturnValue::eSUCCESS;
  }

  ch->sendln("Do what?\r\nSyntax: vend <buy | sell | list | cancel | modify | collect | search | identify>");
  if (ch->getLevel() >= 104)
    ch->sendln("Also: <addroom | removeroom | listroom | stats>");
  return ReturnValue::eSUCCESS;
}
