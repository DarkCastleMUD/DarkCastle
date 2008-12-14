/*

Brand new auction file!

*/

extern "C"
{
#include <ctype.h>
#include <string.h>
}
#include <room.h>
#include <obj.h>
#include <player.h> // MAX_*
#include <vault.h>
#include <connect.h> // CON_WRITE_BOARD
#include <terminal.h> // BOLD
#include <fileinfo.h> // for the board files
#include <levels.h> // levels..
#include <clan.h>
#include <character.h> 
#include <utility.h> // FALSE
#include <memory.h>
#include <act.h>
#include <db.h>
#include <returnvals.h>
#include <string>
#include <map>
#include <queue>

using namespace std;

#define auction_duration 300 //1209600
#define THALOS_AUCTION_HOUSE 5200

int do_auction(struct char_data *ch, char *argument, int cmd, bool is_special = false);
extern struct index_data *obj_index;
extern CWorld world;
extern struct descriptor_data *descriptor_list;

enum ListOptions
{
  LIST_ALL = 0,
  LIST_MINE
};

enum AuctionStates
{
  AUC_FOR_SALE = 0,
  AUC_EXPIRED
};

struct AuctionTicket
{
  int vitem;
  string item_name;
  unsigned int price;
  string seller;
  AuctionStates state;
  unsigned int end_time;
};

class AuctionHouse
{
public:
  AuctionHouse(string in_file);
  AuctionHouse();
  ~AuctionHouse();
  void CollectExpired(CHAR_DATA *ch, unsigned int ticket = 0);
  void AddItem(CHAR_DATA *ch, OBJ_DATA *obj, unsigned int price);
  void CancelItem(CHAR_DATA *ch, unsigned int ticket);
  void BuyItem(CHAR_DATA *ch, unsigned int ticket);
  void ListItems(CHAR_DATA *ch, ListOptions option);  
  void CheckExpire();
  void Save() {}//;
  void Load() {}//;  
private:
  unsigned int cur_index;
  string file_name;
  map<unsigned int, AuctionTicket> Items_For_Sale;
};

AuctionHouse::~AuctionHouse()
{}

AuctionHouse::AuctionHouse()
{
  cur_index = 0;
}

AuctionHouse::AuctionHouse(string in_file)
{
  cur_index = 0;
  file_name = in_file;
}

/*
COLLECT EXPIRED
*/
void AuctionHouse::CollectExpired(CHAR_DATA *ch, unsigned int ticket)
{
  map<unsigned int, AuctionTicket>::iterator Item_it;
  if(ticket > 0)
  {
     Item_it = Items_For_Sale.find(ticket);
     if(Item_it != Items_For_Sale.end())
     {
       if(Item_it->second.seller.compare(GET_SHORT(ch)))
       {
         csendf(ch, "Ticket %d doesn't seem to belong to you.\n\r", ticket);
         return;
       }
       if(Item_it->second.state != AUC_EXPIRED)
       {
         csendf(ch, "Ticket %d doesn't seem to be expired.\n\r", ticket);
         return;
       }
     }
     CancelItem(ch, ticket);
     return;
  }
  queue<unsigned int> tickets_to_cancel;
  for(Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if(Item_it->second.state == AUC_EXPIRED && !Item_it->second.seller.compare(GET_SHORT(ch)))
      tickets_to_cancel.push(Item_it->first);    
  }
  while(!tickets_to_cancel.empty())
  {  
      CancelItem(ch, tickets_to_cancel.front());
      tickets_to_cancel.pop();
  }
  return;
}


/*
CHECK EXPIRE
*/
void AuctionHouse::CheckExpire()
{
  unsigned int cur_time = time(0);
  CHAR_DATA *ch;
  map<unsigned int, AuctionTicket>::iterator Item_it;
  for(Item_it = Items_For_Sale.begin(); Item_it != Items_For_Sale.end(); Item_it++)
  {
    if(Item_it->second.state == AUC_FOR_SALE && cur_time >= Item_it->second.end_time)
    {
      if((ch = get_active_pc(Item_it->second.seller.c_str()))) 
        csendf(ch, "Your auction of %s has expired.\n\r", Item_it->second.item_name.c_str());
      Item_it->second.state = AUC_EXPIRED;
    }
  }
}


/*
BUY ITEM
*/
void AuctionHouse::BuyItem(CHAR_DATA *ch, unsigned int ticket)
{ 
  map<unsigned int, AuctionTicket>::iterator Item_it;
  OBJ_DATA *obj;
  CHAR_DATA *vict;
  struct vault_data *vault;
  unsigned int fee;
  char buf[MAX_STRING_LENGTH];

  Item_it = Items_For_Sale.find(ticket);
  if(Item_it == Items_For_Sale.end())
  {
    csendf(ch, "Ticket number %d doesn't seem to exist.\n\r", ticket);
    return;
  }
  if(GET_GOLD(ch) < Item_it->second.price)
  {
    csendf(ch, "Ticket number %d costs %d coins, you can't afford that!\n\r", 
                ticket, Item_it->second.price);
    return;
  }
  

  fee = (unsigned int)((double)Item_it->second.price * 0.025);

  if (!(vault = has_vault(Item_it->second.seller.c_str()))) 
  { 
    sprintf(buf, "Major screw up in auction! Couldn't find %s's vault to deposit %d coins!", 
                    Item_it->second.seller.c_str(), Item_it->second.price-fee);
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }
   
  GET_GOLD(ch) -= Item_it->second.price;
  vault->gold += (Item_it->second.price - fee);

  if((vict = get_active_pc(Item_it->second.seller.c_str()))) 
    csendf(vict, "Your auction of %s has been purchased.\n\r", Item_it->second.item_name.c_str());

  sprintf(buf, "Added %d gold from sale of %s.", 
      Item_it->second.price - fee, Item_it->second.item_name.c_str());
  vault_log(buf, Item_it->second.seller.c_str());
  int rnum = real_object(Item_it->second.vitem);

  if(rnum < 0)
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Major screw up in auction(buy)! Item %s[VNum %d] belonging to %s could not be created!", 
                    Item_it->second.item_name.c_str(), Item_it->second.vitem, Item_it->second.seller.c_str());
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }  
  obj = clone_object(rnum);

  if(!obj)
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Major screw up in auction(buy)! Item %s[RNum %d] belonging to %s could not be created!", 
                    Item_it->second.item_name.c_str(), rnum, Item_it->second.seller.c_str());
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }

  csendf(ch, "You have purchased %s.\n\r", obj->short_description);

  if(1 != Items_For_Sale.erase(ticket))
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Major screw up in auction(buy)! Ticket %d belonging to %s could not be removed!", 
                    ticket, Item_it->second.seller.c_str());
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }
  obj_to_char(obj, ch); 
}

/*
CANCEL ITEM
*/
void AuctionHouse::CancelItem(CHAR_DATA *ch, unsigned int ticket)
{
  map<unsigned int, AuctionTicket>::iterator Item_it;
  OBJ_DATA *obj;

  Item_it = Items_For_Sale.find(ticket);
  if(Item_it == Items_For_Sale.end())
  {
    csendf(ch, "Ticket number %d doesn't seem to exist.\n\r", ticket);
    return;
  }
  if(Item_it->second.seller.compare(GET_SHORT(ch)))
  {
    csendf(ch, "Ticket number %d doesn't belong to you.\n\r", ticket);
    return;
  }

  int rnum = real_object(Item_it->second.vitem);

  if(rnum < 0)
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Major screw up in auction(cancel)! Item %s[VNum %d] belonging to %s could not be created!", 
                    Item_it->second.item_name.c_str(), Item_it->second.vitem, Item_it->second.seller.c_str());
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }

  obj = clone_object(rnum);
   
  if(!obj)
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Major screw up in auction(cancel)! Item %s[RNum %d] belonging to %s could not be created!", 
                    Item_it->second.item_name.c_str(), rnum, Item_it->second.seller.c_str());
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }

  csendf(ch, "You are given your %s.\n\r", obj->short_description);

  if(1 != Items_For_Sale.erase(ticket))
  {
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "Major screw up in auction(cancel)! Ticket %d belonging to %s could not be removed!", 
                    ticket, Item_it->second.seller.c_str());
    log(buf, IMMORTAL, LOG_BUG);
    return;
  }
  obj_to_char(obj, ch); 
}

/*
LIST ITEMS
*/
void AuctionHouse::ListItems(CHAR_DATA *ch, ListOptions options)
{
  map<unsigned int, AuctionTicket>::iterator Item_it;
  int i;
  string output_buf;
  char buf[MAX_STRING_LENGTH];
  
  for(i = 0, Item_it = Items_For_Sale.begin(); (i < 50) && (Item_it != Items_For_Sale.end()); Item_it++)
  {
    if((options == LIST_MINE && !Item_it->second.seller.compare(GET_SHORT(ch)))
       || (options == LIST_ALL && Item_it->second.state == AUC_FOR_SALE)
      )
    {
      i++;
      sprintf(buf, "\n\r%05d)  Seller: %-18s Item: %-30s\n\r%s Price: %-12d\n\r\n\r", 
               Item_it->first, Item_it->second.seller.c_str(), Item_it->second.item_name.c_str(), 
               (Item_it->second.state == AUC_EXPIRED)?"$4EXPIRED$R":"       ", Item_it->second.price);
      output_buf += buf;
    }
  }

  if(i == 0)
    send_to_char("There is nothing for sale!\n\r", ch);

  if(i >= 50)
   send_to_char("Maximum number of results reached.\n\r", ch);

  page_string(ch->desc, output_buf.c_str(), 1);
}

/*
ADD ITEM
*/
void AuctionHouse::AddItem(CHAR_DATA *ch, OBJ_DATA *obj, unsigned int price)
{
  if(!obj)
  {
    send_to_char("You don't seem to have that item.\n\r", ch);
    return;
  }
  
  if(price > 2000000000)
  {
    send_to_char("Price must be between 0 and 2000000000.\n\r", ch);
    return;
  }
  
  AuctionTicket NewTicket;
  NewTicket.vitem = obj_index[obj->item_number].virt;
  NewTicket.price = price;
  NewTicket.state = AUC_FOR_SALE;
  NewTicket.end_time = time(0) + auction_duration;
  NewTicket.seller = GET_SHORT(ch);
  NewTicket.item_name = obj->short_description;

  unsigned int full_checker = cur_index;
  while(Items_For_Sale.end() != Items_For_Sale.find(++cur_index))
  {
    if(cur_index > 99999)
      cur_index = 1;
    if(full_checker == cur_index)
    {
      send_to_char("The auctioneer is selling too many items already!\n\r", ch);
      return;
    }
  }
  Items_For_Sale[cur_index] = NewTicket;
  
  csendf(ch, "You are now selling %s for %d coins.\n\r", obj->short_description, price);

/*
  char auc_buf[MAX_STRING_LENGTH];
  snprintf(auc_buf, MAX_STRING_LENGTH, "%s is now selling %s for %d coins.",
                         GET_SHORT(ch), obj->short_description, price);


  act(auc_buf, ch, 0, 0, TO_CHAR, 0);

  bool silence = FALSE;
  struct descriptor_data *i;
  OBJ_DATA *tmp_obj;
  for(i = descriptor_list; i; i = i->next)
    if(i->character != ch && !i->connected &&
        (IS_SET(i->character->misc, CHANNEL_AUCTION)) &&
             !is_ignoring(i->character, ch)) 
    {
       for(tmp_obj = world[i->character->in_room].contents; tmp_obj; tmp_obj = tmp_obj->next_content)
         if(obj_index[tmp_obj->item_number].virt == SILENCE_OBJ_NUMBER) 
         {
            silence = TRUE;
            break;
         }
         if(!silence) act(auc_buf, ch, 0, i->character, TO_VICT, 0);
    }
*/

  extract_obj(obj);
  return;

}


AuctionHouse TheAuctionHouse("lib/auctionhouse");

void auction_expire()
{
  TheAuctionHouse.CheckExpire();
  return;
}


int do_vend(CHAR_DATA *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj;
  unsigned int price;
  unsigned int fee;

  if(ch->in_room != real_room(THALOS_AUCTION_HOUSE))
  {
    send_to_char("You must be in an auction house to do this!\n\r", ch);
    return eFAILURE;
  }

  argument = one_argument(argument, buf);

  if(!*buf)
  {
    send_to_char("Syntax goes here\n\r", ch);
    return eSUCCESS;       

  }

  if(!strcmp(buf, "collect"))
  {
    argument = one_argument(argument, buf);
    if(!*buf)
    {
      send_to_char("Collect what?\n\r", ch);
      return eSUCCESS;
    }
    if(!strcmp(buf, "all"))
    {
      TheAuctionHouse.CollectExpired(ch);
      return eSUCCESS;
    }
    if(atoi(buf) > 0)
    {
      TheAuctionHouse.CollectExpired(ch, atoi(buf));
      return eSUCCESS;
    }
    else
      send_to_char("Collect \"all\" or ticket number.\n\r", ch);
    return eSUCCESS;
  }
 
  if(!strcmp(buf, "buy"))
  {
    argument = one_argument(argument, buf);
    if(!*buf)
    {
      send_to_char("Buy what?\n\r", ch);
      return eSUCCESS;
    }
    TheAuctionHouse.BuyItem(ch, atoi(buf));
    return eSUCCESS;
  }

  if(!strcmp(buf, "cancel"))
  {
    argument = one_argument(argument, buf);
    if(!*buf)
    {
      send_to_char("Cancel what?\n\r", ch);
      return eSUCCESS;
    }
    TheAuctionHouse.CancelItem(ch, atoi(buf));
    return eSUCCESS;
  }
 
  if(!strcmp(buf, "list"))
  {
    argument = one_argument(argument, buf);
    if(!*buf)
    {
      TheAuctionHouse.ListItems(ch, LIST_ALL);
    }
    else if (!strcmp(buf, "mine"))
    {
      TheAuctionHouse.ListItems(ch, LIST_MINE);
    }
    else
    {
      send_to_char("List what?\n\r", ch);
    }
    return eSUCCESS;
  }

  if(!strcmp(buf, "sell"))
  {
    argument = one_argument(argument, buf);
    if(!*buf)
    {
      send_to_char("Sell what?\n\r", ch);
      return eSUCCESS;
    }  
    obj = get_obj_in_list_vis(ch, buf, ch->carrying);
    if(!obj)
    {
      send_to_char("You don't seem to have that item.\n\r", ch);
      return eSUCCESS;
    }
    argument = one_argument(argument, buf);
    if(!*buf)
    {
       send_to_char("How much do you want to sell it for?\n\r", ch);
       return eSUCCESS;
    }
    price = atoi(buf);
    if(!price)
    {
      send_to_char("You don't want to just give it away for free!\n\r", ch);
      return eSUCCESS;
    }
    fee = (int)((double)price * 0.025); //2.5% fee to list, then 2.5% fee during sale
    if(GET_GOLD(ch) < fee)
    {
      csendf(ch, "You don't have enough gold to pay the %d coin auction fee.\n\r", fee);
      return eSUCCESS;
    }
    GET_GOLD(ch) -= fee;

    csendf(ch, "Done! You pay the %d coin tax to auction the item.\n\r", fee);

    TheAuctionHouse.AddItem(ch, obj, price);
    return eSUCCESS;    
  }

  return eSUCCESS;
}
