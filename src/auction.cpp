/*

Item Selling System
Written by: Rubicon
December 13, 2008

Objects:
Auction Class

External: (explained more below)

*/
#include "DC/DC.h"
#include "DC/DC.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/interp.h"
#include "DC/inventory.h"

#include <QString>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>

CharacterPtr find_mob_in_room(CharacterPtr ch, qint32 iFriendId);

void AuctionHouse::ShowStats(CharacterPtr ch)
{
  ch->sendln("Vendor Statistics:");
  ch->send(QStringLiteral("Items Posted:     %1\r\n").arg(ItemsPosted));
  ch->send(QStringLiteral("Items For Sale:   %1\r\n").arg(ItemsActive));
  ch->send(QStringLiteral("Items Sold:       %1\r\n").arg(ItemsSold));
  ch->send(QStringLiteral("Items Expired:    %1\r\n").arg(ItemsExpired));
  ch->send(QStringLiteral("Total Revenue:    %1\r\n").arg(Revenue));
  ch->send(QStringLiteral("Tax Collected:    %1\r\n").arg(TaxCollected));
  ch->send(QStringLiteral("Uncollected Gold: %1\r\n").arg(UncollectedGold));
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

  ObjectPtr obj = (ObjectPtr)(DC::getInstance()->obj_index[nr].item);

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

  ObjectPtr obj = (ObjectPtr)(DC::getInstance()->obj_index[nr].item);

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
    ch->send(QStringLiteral("Price must be between %u and %u.\r\n").arg(AUC_MIN_PRICE).arg(AUC_MAX_PRICE));
    return;
  }
  if ((Item_it = Items_For_Sale.find(ticket)) == Items_For_Sale.end())
  {
    ch->send(QStringLiteral("Ticket number %1 doesn't seem to exist.\r\n").arg(ticket));
    return;
  }

  if (Item_it->seller.compare(ch->name()))
  {
    ch->send(QStringLiteral("Ticket number %1 doesn't belong to you.\r\n").arg(ticket));
    return;
  }

  if (new_price > Item_it->price)
  {
    quint32 difference = new_price - Item_it->price;
    quint32 fee = (quint32)((double)difference * 0.025);
    if (ch->getGold() < fee)
    {
      ch->send(QStringLiteral("Increasing the items price by %u costs %u, you don't have enough.\r\n").arg(difference).arg(fee));
      return;
    }
    ch->removeGold(fee);
    ch->send(QStringLiteral("The broker collects %u coins for increasing the price by %u.\r\n").arg(fee).arg(difference));
    ch->save();
  }

  ch->send(QStringLiteral("The new price of ticket %u (%s) is now %u.\r\n").arg(ticket).arg(qPrintable(Item_it->item_name)).arg(new_price));

  for (auto tmp = DC::getInstance()->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
  {
    if (tmp != ch)
    {
      tmp->send(QStringLiteral("%1 has just modified the price of one of %2 items.\r\n").arg(qPrintable(ch->name())).arg((GET_SEX(ch) == SEX_MALE) ? "his" : "her"));
    }
  }

  logentry(QStringLiteral("VEND: %1 modified ticket %2 (%3): old price %4, new price %5.\r\n").arg(qPrintable(ch->name())).arg(Item_it.key()).arg(Item_it->item_name).arg(Item_it->price).arg(new_price), IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
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
        ch->send(QStringLiteral("Your auction of %1 has $2SOLD$R to %2 for %3 coins.\r\n").arg(Item_it->item_name).arg(Item_it->buyer).arg(Item_it->price));
      }
      if (Item_it->state == AUC_EXPIRED)
      {
        has_sold_items = true;
        ch->send(QStringLiteral("Your auction of %1 has $4EXPIRED$R.\r\n").arg(Item_it->item_name));
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

  logentry(QStringLiteral("%1 auction%2 belonging to %3 have been deleted.").arg(tickets_to_delete.size()).arg(plural).arg(name), ANGEL, DC::LogChannel::LOG_GOD);

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
        logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
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
  logentry(QStringLiteral("%1 auction%2 have been converted from %3 to %4.").arg(i).arg(plural).arg(old_name).arg(new_name), ch->getLevel(), DC::LogChannel::LOG_GOD);
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
    ch->send(QStringLiteral("Ticket number %1 doesn't seem to exist.\r\n").arg(ticket));
    return;
  }

  if (!Item_it->buyer.isEmpty() && Item_it->buyer.compare(qPrintable(ch->name())) && Item_it->seller.compare(qPrintable(ch->name())))

  {
    ch->send(QStringLiteral("Ticket number %1 is private.\r\n").arg(ticket));
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
    ch->send(QStringLiteral("There is a problem with ticket %1. Please tell an imm.\r\n").arg(ticket));
    return;
  }

  ObjectPtr obj = ticket_object_load(Item_it, ticket);

  if (!obj)
  {

    logentry(QStringLiteral("Major screw up in auction(identify)! Item %1 belonging to %2 could not be created!").arg(Item_it->item_name).arg(Item_it->seller), IMMORTAL, DC::LogChannel::LOG_BUG);
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
  //  sprintf(out_buf, "%s is an unknown body location.\r\n", buf);
  // ch->send(out_buf);

  qint32 nr = real_object(vnum);

  if (nr < 0)
    return true;

  ObjectPtr obj = (ObjectPtr)(DC::getInstance()->obj_index[nr].item);
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

  ObjectPtr obj = (ObjectPtr)(DC::getInstance()->obj_index[nr].item);
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
  return isSet(((ObjectPtr)(DC::getInstance()->obj_index[nr].item))->obj_flags.more_flags, ITEM_NO_TRADE);
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

  eq_level = ((ObjectPtr)(DC::getInstance()->obj_index[nr].item))->obj_flags.eq_level;

  return (eq_level >= to && eq_level <= from);
}

/*
SEARCH BY NAME
*/
bool AuctionHouse::IsName(QString name, qint32 vnum)
{
  ObjectPtr obj = DC::getInstance()->getObject(vnum);
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
    ch->send(QStringLiteral(" %1").arg(room_it.key()));
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
    ch->send(QStringLiteral("Done. Room %1 added to auction houses.\r\n").arg(room));
    logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s just added room %d to auction houses.", qPrintable(ch->name()), room);
    Save();
    return;
  }
  else
    ch->send(QStringLiteral("Room %1 is already an auction house.\r\n").arg(room));
}

/*
REMOVE ROOM
*/
void AuctionHouse::RemoveRoom(CharacterPtr ch, qint32 room)
{
  if (1 == auction_rooms.remove(room))
  {
    ch->send(QStringLiteral("Done. Room %1 has been removed from auction houses.\r\n").arg(room));
    logf(ch->getLevel(), DC::LogChannel::LOG_GOD, "%s just removed room %d from auction houses.", qPrintable(ch->name()), room);
    Save();
    return;
  }
  else
    ch->send(QStringLiteral("Room %1 doesn't appear to be an auction house.\r\n").arg(room));
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

  FILE *the_file;
  quint32 num_rooms, num_items, ticket, i, state;
  qint32 room;
  QString nl;
  QString buf;
  AuctionTicket InTicket;
  the_file = fopen(qPrintable(file_name), "r");

  if (!the_file)
  {
    QString buf;
    sprintf(buf, "Unable to open the save file \"%s\" for Auction files!!", qPrintable(file_name));
    logentry(buf, 0, DC::LogChannel::LOG_MISC);
    return;
  }

  fscanf(the_file, "%u\n", &num_rooms);

  for (i = {}; i < num_rooms; i++)
  {
    fscanf(the_file, "%d\n", &room);
    auction_rooms[room] = 1;
  }

  fscanf(the_file, "%u\n", &num_items);
  for (i = {}; i < num_items; i++)
  {
    fscanf(the_file, "%u\n", &ticket);
    fscanf(the_file, "%d\n", &InTicket.vitem);
    fgets(buf, MAX_STRING_LENGTH, the_file);
    nl = strrchr(buf, '\n');
    if (nl)
      *nl = '\0'; // fgets grabs newline too, removing it here
    InTicket.item_name = buf;
    fgets(buf, MAX_STRING_LENGTH, the_file);
    nl = strrchr(buf, '\n');
    if (nl)
      *nl = '\0'; // fgets grabs newline too, removing it here
    InTicket.seller = buf;
    fgets(buf, MAX_STRING_LENGTH, the_file);
    nl = strrchr(buf, '\n');
    if (nl)
      *nl = '\0'; // fgets grabs newline too, removing it here
    InTicket.buyer = buf;
    fscanf(the_file, "%u\n", &state);
    InTicket.state = (AuctionStates)state;
    fscanf(the_file, "%u\n", &InTicket.end_time);
    fscanf(the_file, "%u\n", &InTicket.price);
    InTicket.obj = {};

    Items_For_Sale[ticket] = InTicket;
  } // LOOP

  if (feof(the_file)) // this means the stat info was lost somehow
    ParseStats();
  else
  {
    fscanf(the_file, "%u\n", &ItemsPosted);
    fscanf(the_file, "%u\n", &ItemsExpired);
    fscanf(the_file, "%u\n", &ItemsSold);
    fscanf(the_file, "%u\n", &TaxCollected);
    fscanf(the_file, "%u\n", &Revenue);
    fscanf(the_file, "%u\n", &ItemsActive);
    fscanf(the_file, "%u\n", &UncollectedGold);
  }
  fclose(the_file);
}

/*
SAVE
*/
void AuctionHouse::Save()
{
  FILE *the_file;
  QMap<quint32, AuctionTicket>::iterator Item_it;
  QMap<qint32, qint32>::iterator room_it;

  if (DC::getInstance()->cf.bport)
  {
    logentry(QStringLiteral("Unable to save auction files because this is the testport!"), ANGEL, DC::LogChannel::LOG_MISC);
    return;
  }
  QString temp_file_name = file_name + ".temp";
  the_file = fopen(qPrintable(temp_file_name), "w");

  if (!the_file)
  {
    QString buf;
    sprintf(buf, "Unable to open/create the save file \"%s\" for Auction files!!", qPrintable(file_name));
    logentry(buf, ANGEL, DC::LogChannel::LOG_BUG);
    return;
  }

  qfprintf(the_file, "%lld\n", auction_rooms.size());
  for (room_it = auction_rooms.begin(); room_it != auction_rooms.end(); room_it++)
  {
    qfprintf(the_file, "%d\n", room_it.key());
  }

  qfprintf(the_file, "%llu\n", Items_For_Sale.size());
  for (Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    qfprintf(the_file, "%u\n", Item_it.key());
    qfprintf(the_file, "%d\n", Item_it->vitem);
    qfprintf(the_file, "%s\n", qPrintable(Item_it->item_name));
    qfprintf(the_file, "%s\n", qPrintable(Item_it->seller));
    qfprintf(the_file, "%s\n", qPrintable(Item_it->buyer));
    qfprintf(the_file, "%u\n", Item_it->state);
    qfprintf(the_file, "%u\n", Item_it->end_time);
    qfprintf(the_file, "%u\n", Item_it->price);

    if (Item_it->obj)
    {
      std::stringstream obj_filename;
      obj_filename << "../lib/auctions/" << Item_it.key() << ".auction_obj";

      std::ofstream auction_obj_file;
      auction_obj_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
      errno = {};
      try
      {
        struct stat statinfo;
        if (stat("../lib/auctions/", &statinfo) != 0)
        {
          if (mkdir("../lib/auctions/", S_IRWXU) != 0)
          {
            perror("mkdir");
            throw(0);
          }
        }
        auction_obj_file.open(obj_filename.str().c_str());
        auction_obj_file << Item_it->obj << std::flush;
        auction_obj_file.close();
      }
      catch (...)
      {
        perror("AuctionHouse::Save()");
        logf(IMMORTAL, DC::LogChannel::LOG_BUG, "AuctionHouse::Save(): Ticket %d", Item_it.key());
      }
    }
  }
  qfprintf(the_file, "%u\n", ItemsPosted);
  qfprintf(the_file, "%u\n", ItemsExpired);
  qfprintf(the_file, "%u\n", ItemsSold);
  qfprintf(the_file, "%u\n", TaxCollected);
  qfprintf(the_file, "%u\n", Revenue);
  qfprintf(the_file, "%u\n", ItemsActive);
  qfprintf(the_file, "%u\n", UncollectedGold);

  fclose(the_file);
  if (rename(qPrintable(temp_file_name), qPrintable(file_name)) != 0)
  {
    perror("AuctionHouse::save() rename");
    logf(IMMORTAL, DC::LogChannel::LOG_BUG, "AuctionHouse::Save() rename: %s", strerror(errno));
  }
}

/*
Destructor
*/
AuctionHouse::~AuctionHouse()
{
}

/*
Empty constructor
*/
AuctionHouse::AuctionHouse()
{
  /*
   In the current implementation this must NEVER be called
   */
  ItemsPosted = {};
  ItemsActive = {};
  ItemsExpired = {};
  ItemsSold = {};
  TaxCollected = {};
  UncollectedGold = {};
  Revenue = {};
  cur_index = {};
  assert(0);
}

/*
Constructor with file name
*/
AuctionHouse::AuctionHouse(QString in_file)
{
  ItemsPosted = {};
  ItemsActive = {};
  ItemsExpired = {};
  ItemsSold = {};
  TaxCollected = {};
  UncollectedGold = {};
  Revenue = {};
  cur_index = {};
  file_name = in_file;
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
        ch->send(QStringLiteral("Ticket %1 doesn't seem to belong to you.\r\n").arg(ticket));
        return;
      }
      if (Item_it->state != AUC_EXPIRED && Item_it->state != AUC_SOLD)
      {
        ch->send(QStringLiteral("Ticket %1 isn't collectible!.\r\n").arg(ticket));
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
        ch->send(QStringLiteral("Your auction of %s has expired.\r\n").arg(qPrintable(Item_it->item_name)));
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
  FILE *fl;
  QString buf;
  qint32 i = {};

  Item_it = Items_For_Sale.find(ticket);
  if (Item_it == Items_For_Sale.end())
  {
    ch->send(QStringLiteral("Ticket number %1 doesn't seem to exist.\r\n").arg(ticket));
    return;
  }

  if (!Item_it->buyer.isEmpty() && Item_it->buyer.compare(qPrintable(ch->name())))
  {
    ch->send(QStringLiteral("Ticket number %1 is private.\r\n").arg(ticket));
    return;
  }

  if (Item_it->state != AUC_FOR_SALE)
  {
    ch->send(QStringLiteral("Ticket number %1 has already been sold\r\n.").arg(ticket));
    return;
  }

  if (!Item_it->seller.compare(qPrintable(ch->name())))
  {
    ch->sendln("That's your own item you're selling, dumbass!");
    return;
  }

  if (ch->getGold() < Item_it->price)
  {
    ch->send(QStringLiteral("Ticket number %d costs %d coins, you can't afford that!\r\n").arg(ticket).arg(Item_it->price));
    return;
  }

  qint32 rnum = real_object(Item_it->vitem);

  if (rnum < 0)
  {
    QString buf;
    sprintf(buf, "Major screw up in auction(buy)! Item %s[VNum %d] belonging to %s could not be created!",
            qPrintable(Item_it->item_name), Item_it->vitem, qPrintable(Item_it->seller));
    logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  obj = ticket_object_load(Item_it, ticket);

  if (!obj)
  {
    QString buf;
    sprintf(buf, "Major screw up in auction(buy)! Item %s[RNum %d] belonging to %s could not be created!",
            qPrintable(Item_it->item_name), rnum, qPrintable(Item_it->seller));
    logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
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
        ch->send(QStringLiteral("You need to have \"%s\" to buy a NO_TRADE item.\r\n").arg(qPrintable(((ObjectPtr)(DC::getInstance()->obj_index[nr].item))->short_description())));
      return;
    }
    else
    {
      ch->send(QStringLiteral("You give your %1 to the broker.\r\n").arg(no_trade_obj->short_description()));
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
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
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
    vict->send(QStringLiteral("%s just purchased your ticket of %s for %u coins.\r\n").arg(qPrintable(ch->name())).arg(qPrintable(Item_it->item_name)).arg(Item_it->price));

  ch->send(QStringLiteral("You have purchased %s for %u coins.\r\n").arg(qPrintable(obj->short_description())).arg(Item_it->price));

  CharacterPtr tmp;
  for (tmp = DC::getInstance()->world[ch->in_room].people; tmp; tmp = tmp->next_in_room)
    if (tmp != ch)
      tmp->send(QStringLiteral("%s just purchased %s's %s\r\n").arg(qPrintable(ch->name())).arg(qPrintable(Item_it->seller)).arg(qPrintable(obj->short_description())));

  Item_it->state = AUC_SOLD;
  Item_it->buyer = qPrintable(ch->name());

  Save();
  QString log_buf = {};
  sprintf(log_buf, "VEND: %s bought %s's %s[%d] for %u coins.\r\n", qPrintable(ch->name()), qPrintable(Item_it->seller), qPrintable(Item_it->item_name), Item_it->vitem, Item_it->price);
  logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
  obj_to_char(obj, ch);
  ch->save();

  if (DC::getInstance()->cf.bport == false)
  {
    errno = {};
    if (!(fl = fopen(WEB_AUCTION_FILE, "r")))
    {
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "%s: %s", WEB_AUCTION_FILE, strerror(errno));
      return;
    }

    while (!feof(fl) && i <= 9)
    {
      buf[i] = fread_string(fl, 0);
      i++;
    }

    fclose(fl);

    errno = {};
    if (!(fl = fopen(WEB_AUCTION_FILE, "w")))
    {
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "%s: %s", WEB_AUCTION_FILE, strerror(errno));
      return;
    }

    qfprintf(fl, "%s purchased %s's %s~\n", qPrintable(ch->name()), qPrintable(Item_it->seller), qPrintable(obj->short_description()));

    for (qint32 j = {}; j < i; j++)
      qfprintf(fl, "%s~\n", buf[j]);

    fclose(fl);
  }
  else
  {
    logentry(QStringLiteral("bport mode: Not saving auction file to web dir."), 0, DC::LogChannel::LOG_MISC);
  }
}

ObjectPtr ticket_object_load(QMap<quint32, AuctionTicket>::iterator Item_it, qint32 ticket)
{
  // If obj is nullptr then either we haven't loaded this object yet or it's not custom
  if (Item_it->obj == nullptr)
  {
    std::stringstream obj_filename;
    obj_filename << "../lib/auctions/" << ticket << ".auction_obj";

    std::ifstream auction_obj_file;
    auction_obj_file.open(obj_filename.str().c_str());
    if (auction_obj_file.is_open())
    {
      auction_obj_file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

      try
      {
        Item_it->obj = new Object;
        auction_obj_file >> Item_it->obj;
        auction_obj_file.close();
      }
      catch (std::ifstream::failure &e)
      {
        if ((auction_obj_file.rdstate() & std::ios_base::eofbit) == std::ios_base::eofbit)
        {
          logf(IMMORTAL, DC::LogChannel::LOG_BUG, "ticket_object_load(): could not load obj file for ticket %d due to std::ios_base::eofbit", ticket);
        }
        else if ((auction_obj_file.rdstate() & std::ios_base::badbit) == std::ios_base::badbit)
        {
          logf(IMMORTAL, DC::LogChannel::LOG_BUG, "ticket_object_load(): could not load obj file for ticket %d due to std::ios_base::badbit", ticket);
        }
        else if ((auction_obj_file.rdstate() & std::ios_base::failbit) == std::ios_base::failbit)
        {
          logf(IMMORTAL, DC::LogChannel::LOG_BUG, "ticket_object_load(): could not load obj file for ticket %d due to std::ios_base::failbit", ticket);
        }
        else
        {
          logf(IMMORTAL, DC::LogChannel::LOG_BUG, "ticket_object_load(): could not load obj file for ticket %d due to reasons unknown", ticket);
        }
        Item_it->obj = {};
      }
      catch (...)
      {
        logf(IMMORTAL, DC::LogChannel::LOG_BUG, "ticket_object_load(): unknown error");
        Item_it->obj = {};
      }
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
    ch->send(QStringLiteral("Ticket number %1 doesn't seem to exist.\r\n").arg(ticket));
    return;
  }
  if (Item_it->seller.compare(qPrintable(ch->name())))
  {
    ch->send(QStringLiteral("Ticket number %1 doesn't belong to you.\r\n").arg(ticket));
    return;
  }

  switch (Item_it->state)
  {
  case AUC_SOLD:
  {
    quint32 fee = (quint32)((double)Item_it->price * 0.025);
    if (fee > 500000)
      fee = 500000;
    ch->sendln(QStringLiteral("The Broker hands you %u $B$5gold$R coins from your sale of %s.\r\nHe pockets %u $B$5gold$R as a broker's fee.").arg((Item_it->price - fee)).arg(Item_it->item_name).arg(fee));
    TaxCollected += fee;
    Revenue -= fee;
    UncollectedGold -= Item_it->price;
    ch->addGold(Item_it->price - fee);
    QString log_buf = {};
    sprintf(log_buf, "VEND: %s just collected %u coins from their sale of %s (ticket %u).\r\n",
            qPrintable(ch->name()), Item_it->price, qPrintable(Item_it->item_name), ticket);
    logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
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
      sprintf(buf, "Major screw up in auction(cancel)! Item %s[VNum %d] belonging to %s could not be created!",
              qPrintable(Item_it->item_name), Item_it->vitem, qPrintable(Item_it->seller));
      logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }

    if (isSet(((ObjectPtr)(DC::getInstance()->obj_index[rnum].item))->obj_flags.more_flags, ITEM_UNIQUE) && search_char_for_item(ch, rnum, false))
    {
      ch->sendln("Why would you want another one of those?");
      return;
    }

    obj = ticket_object_load(Item_it, ticket);
    if (!obj)
    {
      QString buf;
      sprintf(buf, "Major screw up in auction(RemoveTicket)! Item %s[RNum %d] belonging to %s could not be created!",
              qPrintable(Item_it->item_name), rnum, qPrintable(Item_it->seller));
      logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
      return;
    }

    ch->send(QStringLiteral("The Consignment Broker retrieves %1 and returns it to you.\r\n").arg(obj->short_description()));
    QString log_buf = {};
    sprintf(log_buf, "VEND: %s cancelled or collected ticket # %u (%s) that was for sale for %u coins.\r\n",
            qPrintable(ch->name()), ticket, qPrintable(Item_it->item_name), Item_it->price);
    logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);
    obj_to_char(obj, ch);
  }
  break;
  case AUC_DELETED:
  {
    QString buf;
    sprintf(buf, "%s just tried to cheat and collect ticket %u which didn't get erased properly!", qPrintable(ch->name()), ticket);
    logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    Items_For_Sale.remove(ticket);

    std::stringstream obj_filename;
    obj_filename << "../lib/auctions/" << ticket << ".auction_obj";
    struct stat sbuf;
    if (stat(obj_filename.str().c_str(), &sbuf) == 0)
    {
      if (unlink(obj_filename.str().c_str()) == -1)
      {
        logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
      }
    }

    return;
  }
  break;
  default:
    logentry(QStringLiteral("Default case reached in Removeticket, contact a coder!"), IMMORTAL, DC::LogChannel::LOG_BUG);
    break;
  }

  Item_it->state = AUC_DELETED; // just a safety precaution
  ch->save();

  if (1 != Items_For_Sale.remove(ticket))
  {
    QString buf;
    sprintf(buf, "Major screw up in auction(cancel)! Ticket %d belonging to %s could not be removed!",
            ticket, qPrintable(Item_it->seller));
    logentry(buf, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  std::stringstream obj_filename;
  obj_filename << "../lib/auctions/" << ticket << ".auction_obj";
  struct stat sbuf;
  if (stat(obj_filename.str().c_str(), &sbuf) == 0)
  {
    if (unlink(obj_filename.str().c_str()) == -1)
    {
      logf(IMMORTAL, DC::LogChannel::LOG_BUG, "unlink %s: %s", obj_filename.str().c_str(), strerror(errno));
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
  QString buf = {0};

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
      std::stringstream ss;
      ss.imbue(std::locale("en_US"));
      ss << Item_it->price;
      sprintf(buf, "\r\n%05d) $7$B%-12s$R $5%-10s$R %s %s %s%-30s\r\n",
              Item_it.key(),
              (options == LIST_MINE) ? qPrintable(Item_it->buyer) : qPrintable(Item_it->seller),
              ss.str().c_str(),
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
      ch->send(QStringLiteral("\r\nThere are no %s items currently posted.\r\n").arg(qPrintable(name)));
    else if (options == LIST_BY_SELLER)
      ch->sendln(QStringLiteral("\r\n\"%s\" doesn't seem to be selling any public items.\r\n\r\nTo view private items, use \"vend list private\".\r\n").arg(qPrintable(name));
    else if (options == LIST_BY_CLASS)
      ch->send(QStringLiteral("\r\nThere are no \"%s\" wearable public items for sale.\r\n").arg(qPrintable(name)));
    else if (options == LIST_MINE)
      ch->sendln("\r\nYou do not have any tickets.");
    else if (options == LIST_BY_RACE)
      ch->send(QStringLiteral("\r\nThere is nothing for sale that would fit a \"%s\".\r\n").arg(qPrintable(name)));
    else
      ch->sendln("\r\nThere is nothing for sale!");
  }

  if (options == LIST_MINE)
  {
    Vault vault = DC::getInstance()->vaults_.has_vault(ch->name());
    if (vault)
    {
      qint32 max_items = vault.size_ / 100;
      ch->send(QStringLiteral("\r\nYou are using %d of your %d available tickets.\r\n").arg(i).arg(max_items));
    }
  }

  if (i > 0) // only display this if there was at least 1 item listed
  {
    qint32 nr = real_object(27909);
    if (nr >= 0)
    {
      sprintf(buf, "\r\n'$4N$R' indicates an item is NO_TRADE and requires %s to purchase.\r\n", qPrintable(((ObjectPtr)(DC::getInstance()->obj_index[nr].item))->short_description()));
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
    ch->send(QStringLiteral("%s needs to be emptied first.\r\n").arg(GET_OBJ_SHORT(obj)));
    return;
  }

  if (price > AUC_MAX_PRICE)
  {
    ch->send(QStringLiteral("Price must be between %u and %u.\r\n").arg(AUC_MIN_PRICE).arg(AUC_MAX_PRICE));
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

  if (isSet(obj->obj_flags.more_flags, ITEM_UNIQUE) && IsExist(ch->name(), DC::getInstance()->obj_index[obj->item_number].vnum()))
  {
    ch->send(QStringLiteral("You're selling %1 already and it's unique!\r\n").arg(obj->short_description()));
    return;
  }

  if (strcmp(qPrintable(obj->short_description()), qPrintable(((ObjectPtr)(DC::getInstance()->obj_index[obj->item_number].item))->short_description())))
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
  NewTicket.vitem = DC::getInstance()->obj_index[obj->item_number].vnum();
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
    ch->sendln(QStringLiteral("You are now selling %1 for %2 coins.").arg(obj->short_description()).arg(price));
  else
  {
    ch->sendln(QStringLiteral("You are now selling %1 to %2 for %3 coins.").arg(obj->short_description()).arg(buyer).arg(price));
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
      Broker->do_auction(QStringLiteral("%1 is selling \"$R%2$R$6$B\" for %3 gold.").arg(qPrintable(ch->shortdesc_or_name())).arg(obj->short_description()).arg(price).split(' '));
    }
    else
    {
      ch->sendln("The Consignment Broker couldn't auction. Contact an imm.");
      logentry(QStringLiteral("CharacterPtr Broker was nullptr in AuctionHouse::AddItem([%1], [%2], [%3], [%4])").arg(qPrintable(ch->name())).arg(GET_OBJ_SHORT(obj)).arg(price).arg(buyer));
    }
  }

  QString log_buf = {};
  if (NewTicket.buyer.isEmpty())
  {
    sprintf(log_buf, "VEND: %s just listed %s for sale for %u coins.\r\n", qPrintable(ch->name()), qPrintable(obj->short_description()), price);
  }
  else
  {
    sprintf(log_buf, "VEND: %s just listed %s for sale for %u coins for %s.\r\n", qPrintable(ch->name()), qPrintable(obj->short_description()), price, qPrintable(NewTicket.buyer));
  }
  logentry(log_buf, IMPLEMENTER, DC::LogChannel::LOG_OBJECTS);

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

  if (!DC::getInstance()->TheAuctionHouse.IsAuctionHouse(ch->in_room) && ch->getLevel() < 104)
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

  if (!*buf)
  {
    ch->sendln("Syntax: vend <buy | sell | list | cancel | modify | collect | search | identify>");
    if (ch->getLevel() >= 104)
      ch->sendln("Also: <addroom | removeroom | listrooms | stats>");
    return ReturnValue::eSUCCESS;
  }

  /*MODIFY*/
  if (!strcmp(buf, "modify"))
  {
    quint32 ticket;
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Modify what ticket?\r\nSyntax: vend modify <ticket> <new_price>");
      return ReturnValue::eSUCCESS;
    }
    ticket = atoi(buf);
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("What price do you want it?\r\nSyntax: vend modify <ticket> <new_price>");
      return ReturnValue::eSUCCESS;
    }
    DC::getInstance()->TheAuctionHouse.DoModify(ch, ticket, atoi(buf));

    return ReturnValue::eSUCCESS;
  }

  /*SEARCH*/
  if (!strcmp(buf, "search"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Search by what?\r\nSyntax: vend search <name | level | slot | seller | race | class>");
      return ReturnValue::eSUCCESS;
    }
    if (!strcmp(buf, "name"))
    {
      argument = one_argument(argument, buf);
      if (!*buf)
      {
        ch->sendln("What name do you want to search for?\r\nSyntax: vend search name <keyword>");
        return ReturnValue::eSUCCESS;
      }
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_BY_NAME, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (!strcmp(buf, "slot"))
    {
      argument = one_argument(argument, buf);
      if (!*buf)
      {
        send_to_char("What slot do you want to search for?\r\n"
                     "finger, neck, body, head, legs, feet, hands, arms, shield,\r\n"
                     "about, waist, wrist, wield, hold, throw, light, face, ear\r\n"
                     "\r\nSyntax: vend search slot <keyword>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_BY_SLOT, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (!strcmp(buf, "race"))
    {
      argument = one_argument(argument, buf);
      if (!*buf)
      {
        send_to_char("What race do you want to search for?\r\n"
                     "Human, Elf, Dwarf, Hobbit, Pixie, Gnome, Orc, Troll\r\n"
                     "\r\nSyntax: vend search race <race>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_BY_RACE, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (!strcmp(buf, "class"))
    {
      argument = one_argument(argument, buf);
      if (!*buf)
      {
        send_to_char("What class do you want to search for?\r\n"
                     "\r\nSyntax: vend search class <class_name>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      if (strlen(buf) < 4)
      {
        ch->sendln("Class name needs to be at least 4 letters to search!");
        return ReturnValue::eSUCCESS;
      }
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_BY_CLASS, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (!strcmp(buf, "seller"))
    {
      argument = one_argument(argument, buf);
      if (!*buf)
      {
        send_to_char("What person are you looking for?\r\n"
                     "\r\nSyntax: vend search seller <name>\r\n",
                     ch);
        return ReturnValue::eSUCCESS;
      }
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_BY_SELLER, buf, 0, 0);
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    if (!strcmp(buf, "level"))
    {
      quint32 level;
      argument = one_argument(argument, buf);
      if (!*buf)
      {
        ch->sendln("What level?\r\nSyntax: vend search level <min_level> [max_level]");
        return ReturnValue::eSUCCESS;
      }
      level = atoi(buf);
      argument = one_argument(argument, buf);
      if (!*buf)
        DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_BY_LEVEL, "", level, 0);
      else
        DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_BY_LEVEL, "", level, atoi(buf));
      ch->add_command_lag(cmd, DC::PULSE_VIOLENCE);

      return ReturnValue::eSUCCESS;
    }

    ch->sendln("Search by what?\r\nSyntax: vend search <name | level | slot | seller | race | class>");
    return ReturnValue::eSUCCESS;
  }

  /*COLLECT*/
  if (!strcmp(buf, "collect"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Collect what?\r\nSyntax: vend collect <all | ticket#>");
      return ReturnValue::eSUCCESS;
    }
    if (!strcmp(buf, "all"))
    {
      DC::getInstance()->TheAuctionHouse.CollectTickets(ch);
      return ReturnValue::eSUCCESS;
    }
    if (atoi(buf) > 0)
    {
      DC::getInstance()->TheAuctionHouse.CollectTickets(ch, atoi(buf));
      return ReturnValue::eSUCCESS;
    }

    ch->sendln("Syntax: vend collect <all | ticket#>");
    return ReturnValue::eSUCCESS;
  }

  /*BUY*/
  if (!strcmp(buf, "buy"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Buy what?\r\nSyntax: vend buy <ticket #>");
      return ReturnValue::eSUCCESS;
    }
    DC::getInstance()->TheAuctionHouse.BuyItem(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*CANCEL*/
  if (!strcmp(buf, "cancel"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Cancel what?\r\nSyntax: vend cancel <all | ticket#>");
      return ReturnValue::eSUCCESS;
    }
    if (!strcmp(buf, "all")) // stupid cancel all didn't fit my design, but the boss wanted it
    {
      DC::getInstance()->TheAuctionHouse.CancelAll(ch);
      return ReturnValue::eSUCCESS;
    }
    DC::getInstance()->TheAuctionHouse.RemoveTicket(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*LIST*/
  if (!strcmp(buf, "list"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("List what?\r\nSyntax: vend list <all | mine | private | recent>");
      return ReturnValue::eSUCCESS;
    }

    if (!strcmp(buf, "all"))
    {
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_ALL, "", 0, 0);
    }
    else if (!strcmp(buf, "mine"))
    {
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_MINE, "", 0, 0);
    }
    else if (!strcmp(buf, "private"))
    {
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_PRIVATE, "", 0, 0);
    }
    else if (!strcmp(buf, "recent"))
    {
      DC::getInstance()->TheAuctionHouse.ListItems(ch, LIST_RECENT, "", 0, 0);
    }
    else
    {
      ch->sendln("List what?\r\nSyntax: vend list <all | mine | private>");
    }
    return ReturnValue::eSUCCESS;
  }

  /*SELL*/
  if (!strcmp(buf, "sell"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
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
    if (!*buf)
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
    DC::getInstance()->TheAuctionHouse.AddItem(ch, obj, price, buf);
    return ReturnValue::eSUCCESS;
  }

  /*IDENTIFY*/
  if (!strcmp(buf, "identify"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Identify what?\r\nSyntax: vend identify <ticket>");
      return ReturnValue::eSUCCESS;
    }
    DC::getInstance()->TheAuctionHouse.Identify(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*SHOW STATS*/
  if (ch->getLevel() >= 104 && !strcmp(buf, "stats"))
  {
    DC::getInstance()->TheAuctionHouse.ShowStats(ch);
    return ReturnValue::eSUCCESS;
  }

  /*ADD ROOM*/
  if (ch->getLevel() >= 104 && !strcmp(buf, "addroom"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Add what room?\r\nSyntax: vend addroom <vnum>");
      return ReturnValue::eSUCCESS;
    }
    DC::getInstance()->TheAuctionHouse.AddRoom(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*REMOVE ROOM*/
  if (ch->getLevel() >= 104 && !strcmp(buf, "removeroom"))
  {
    argument = one_argument(argument, buf);
    if (!*buf)
    {
      ch->sendln("Remove what room?\r\nSyntax: vend removeroom <vnum>");
      return ReturnValue::eSUCCESS;
    }
    DC::getInstance()->TheAuctionHouse.RemoveRoom(ch, atoi(buf));
    return ReturnValue::eSUCCESS;
  }

  /*LIST ROOMS*/
  if (ch->getLevel() >= 104 && !strcmp(buf, "listrooms"))
  {
    DC::getInstance()->TheAuctionHouse.ListRooms(ch);
    return ReturnValue::eSUCCESS;
  }

  ch->sendln("Do what?\r\nSyntax: vend <buy | sell | list | cancel | modify | collect | search | identify>");
  if (ch->getLevel() >= 104)
    ch->sendln("Also: <addroom | removeroom | listroom | stats>");
  return ReturnValue::eSUCCESS;
}
