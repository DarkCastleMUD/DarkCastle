/*

 Real bloody blackjack. Who does two cards then nothing else? Wankers, that's who

*/
extern "C"
{
#include <ctype.h>
#include <string.h>
}
#ifdef LEAK_CHECK
#include <dmalloc.h>
#endif

#include <structs.h>
#include <room.h>
#include <character.h>
#include <obj.h>
#include <utility.h>
#include <terminal.h>
#include <player.h>
#include <levels.h>
#include <mobile.h>
#include <clan.h>
#include <handler.h>
#include <db.h>
#include <interp.h>
#include <connect.h>
#include <spells.h>
#include <race.h>
#include <act.h>
#include <set.h>
#include <returnvals.h>


//
//
//
//


// hit on 16, stand on 17
/*

Z:0 R:17 I:110> 
Omnigod Apocalypse: basically you can double down as in double your bet but you only get 1 more card no matter what

Z:0 R:17 I:110> 
Omnigod Apocalypse: and you have to duoble down before you get any more than 2 cards
Z:0 R:17 I:110> 
Omnigod Apocalypse: split means if you have a pair (any pair)

Z:0 R:17 I:110> 
Wendy: bah

Z:0 R:17 I:110> 
//(socket) Alorean@209.166.95.159 has connected. 

Z:0 R:17 I:110> 
Omnigod Apocalypse: you divide it into 2 hands

Z:0 R:17 I:110> 
Omnigod Apocalypse: and get a 2nd card for each

Z:0 R:17 I:110> 
Wendy: lemme do it in the text editor

Z:0 R:17 I:110> 
Omnigod Apocalypse: b ut you have to double your bet with that too

*/

extern CHAR_DATA *character_list;
extern void pulse_table_bj(struct table_data *tbl, int recall = 0);


struct table_data;

struct player_data
{
  struct player_data *next;
  struct table_data *table;
  CHAR_DATA *ch;
  int hand_data[21];
  int bet;
  bool insurance;
// theoretical cardmax is lower than 21, but whatever
};

struct cDeck
{
  struct table_data *table;
  int *cards;
  int pos;
  int decks;
};

struct table_data 
{
  struct obj_data *obj; // linked to obj
  struct cDeck *deck;
  struct player_data *plr;
  struct player_data *cr; // current
  int options;
  CHAR_DATA *dealer;
  int hand_data[21]; // dealer
  int room;
  int handnr;
  int state;
};


struct cDeck *create_deck(int decks)
{
    struct cDeck *Deck;
    #ifdef LEAK_CHECK
         Deck = (struct cDeck *)calloc(1, sizeof(struct cDeck));
    #else
         Deck = (struct cDeck *)dc_alloc(1, sizeof(struct cDeck));
    #endif
    Deck->cards = new int[decks*52+1];
    Deck->pos = 0;
    int i,z;
    for (i = 0, z = 0;i < decks*52;i++)
    {
	if (++z == 53) z = 1;
	Deck->cards[i] = z;
    }
    return Deck;
}

void freeDeck(struct cDeck *deck)
{
  delete[] deck->cards;
  dc_free(deck);  
}

void switch_cards(struct cDeck *tDeck, int pos1, int pos2)
{
     int b = tDeck->cards[pos1];
     tDeck->cards[pos1] = tDeck->cards[pos2];
     tDeck->cards[pos2] = b;
}

void free_player(struct player_data *plr)
{
  struct player_data *tmp,*prev = NULL;
  struct table_data *tbl = plr->table;
  for (tmp = tbl->plr;tmp;tmp = tmp->next)
  {
    if (tmp == plr)
    {
	if (prev) prev->next = plr->next;
	else	  tbl->plr = plr->next;
    }
  }
  if (tbl->cr == plr)
  {
	tbl->cr = tbl->cr->next;
	pulse_table_bj(tbl);
  }
  dc_free(plr);
}


void nextturn(struct table_data *tbl)
{
      if (tbl->cr->next) tbl->cr = tbl->cr->next;
      else {tbl->cr = NULL; tbl->state = 1; }
      pulse_table_bj(tbl);
}

bool verify(struct player_data *plr)
{
  // make sure player didn't quit, die, or whatever
  CHAR_DATA *ch;
  for (ch = character_list;ch;ch=ch->next)
    if (ch == plr->ch) break;

  if (!ch || ch->in_room != plr->table->room) ;
  {
    if (plr->table->cr == plr)
	nextturn(plr->table);

    free_player(plr); 
    return FALSE; 
  }// dead or quit
  return TRUE;
}

void shuffle_deck(struct cDeck *tDeck)
{ // this would not hold up to a test of true randomization, but then
  // neither would a real dealer shuffling a deck
  struct player_data *plr;
  int pos=0,i,v;
  if (tDeck->pos) // new deck otherwise
  for (plr = tDeck->table->plr;plr;plr = plr->next)
    for (v = 0; plr->hand_data[v]; v++)
    {
      switch_cards(tDeck,pos,--tDeck->pos);
      pos++;
    }
  
  for (i = pos; i < tDeck->decks*52;i++)
    switch_cards(tDeck, i, number(pos, tDeck->decks*52-1));
 tDeck->pos = pos + number(tDeck->decks*52 / 10, tDeck->decks*52/4);
 // shuffled
}


void send_to_table(char *msg, struct table_data *tbl, struct player_data *plrSilent = NULL)
{
  struct player_data *plr;
  for (plr = tbl->plr ; plr ; plr = plr->next)
   if (verify(plr) && plrSilent != plr)
     send_to_char(msg,plr->ch);
  
}

char suit(int card)
{
  if (card < 14) return 'h';
  else if (card < 27) return 'd';
  else if (card < 40) return 'c';
  else return 's';
}

int val(int card)
{
  while (card > 13) card-=13;
  return card;
}

char *valstri(int card)
{
  while (card > 13) card-=13;

  switch (card)
  {
	case 1: return "A";
	case 2: return "2";
	case 3: return "3";
	case 4: return "4";
	case 5: return "5";
	case 6: return "6";
	case 7: return "7";
	case 8: return "8";
	case 9: return "9";
	case 10: return "10";
	case 11: return "J";
	case 12: return "Q";
	case 13: return "K";
	default: return "??";
  }
}

bool canInsurance(struct player_data *plr)
{
  if (val(plr->table->hand_data[0]) == 1 &&
	plr->table->hand_data[2] == 0 &&
	plr->hand_data[2] == 0)
    return TRUE;
  return FALSE;
}

bool splittable(struct player_data *plr)
{ // expand if we only want aces splittable
  if (val(plr->hand_data[0]) == val(plr->hand_data[1]) && !plr->hand_data[2])
   return TRUE;

  return FALSE;
}

int pickCard(struct cDeck *deck)
{
  if (deck->pos >= deck->decks*52-1)
   shuffle_deck(deck);

  return deck->cards[deck->pos++];
}

void freeHand(struct player_data *plr)
{
  int i;
  for (i = 0;i < 21;i++)
    plr->hand_data[i] = 0;
}

int hand_strength(struct player_data *plr)
{
  int i,z=0;
  for (i = 0;plr->hand_data[i] != 0;i++)
    switch (val(plr->hand_data[i]))
    {
	case 1: break; // calculate aces last. 
	case 11:
	case 12:
	case 13:
		z+=10;
		break;
	default:
		z+=plr->hand_data[i];
		break;
    }
  for (i = 0;plr->hand_data[i] != 0;i++)
	if (plr->hand_data[i] == 1)
	{
	 if (z + 11 > 21) z += 1;
	 else z += 11;
	}
  return z;
}

int hand_strength(struct table_data *tbl)
{
  int i,z=0;
  for (i = 0;tbl->hand_data[i] != 0;i++)
    switch (val(tbl->hand_data[i]))
    {
	case 1: break; // calculate aces last. 
	case 11:
	case 12:
	case 13:
		z+=10;
		break;
	default:
		z+=tbl->hand_data[i];
		break;
    }
  for (i = 0;tbl->hand_data[i] != 0;i++)
	if (tbl->hand_data[i] == 1)
	{
	 if (z + 11 > 21) z += 1;
	 else z += 11;
	}
  return z;
}

void dealcard(struct player_data *plr)
{
  // functions calling this should verify that the player can be dealt a card
  int i=0;
  for (; plr->hand_data[i];i++);

  plr->hand_data[i] = pickCard(plr->table->deck);
}

void check_active(void *arg1, void *arg2, void *arg3)
{
  struct player_data *plr = (struct player_data *) arg1;
  if ((int)arg2 == plr->table->handnr)
  {// inactive
    if (verify(plr))
    {
	struct table_data *tbl = plr->table;
	CHAR_DATA *ch = plr->ch;
	
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "Security removes a sleepy %s from the table.\r\n",GET_NAME(plr->ch));
	free_player(plr);
	send_to_table(buf,tbl);
	send_to_char("Security helps you up from the table where you've apparently fallen asleep!\r\n",ch);
    }
  }
}

void add_timer(struct player_data *plr)
{
  extern struct timer_data *timer_list;
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
  timer->arg1 = (void*)plr;
  timer->arg2 = (void*)plr->table->handnr;
  timer->function = check_active;
  timer->next = timer_list;
  timer_list = timer;
  timer->timeleft = 10;
}
void bj_dealer_ai(void *arg1, void *arg2, void *arg3);

void add_timer_bj_dealer(struct table_data *tbl)
{
  extern struct timer_data *timer_list;
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
  timer->arg1 = (void*)tbl;
  timer->function = bj_dealer_ai;
  timer->next = timer_list;
  timer_list = timer;
  timer->timeleft = 1;
}

void add_timer_bj_dealer2(struct table_data *tbl)
{
  extern struct timer_data *timer_list;
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
  timer->arg1 = (void*)tbl;
  timer->function = bj_dealer_ai;
  timer->next = timer_list;
  timer_list = timer;
  timer->timeleft = 10;
}

void reset_table(struct table_data *tbl)
{ // some error, reset table
  send_to_table("Error. Table being reset.\r\n",tbl);
  while (tbl->plr) free_player(tbl->plr);
  tbl->cr = NULL;
  tbl->state = 0;
}

void check_winner(struct table_data *tbl)
{
  int dealer = hand_strength(tbl);
  struct player_data *plr,*next;
  for (plr = tbl->plr;plr;plr=next)
  {
   next = plr->next;
    if (dealer > 21 || (hand_strength(plr) > dealer && hand_strength(plr) <= 21))
    {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf,"A dealer takes your cards and gives you %d coins.\r\n",
		plr->bet*2);
      send_to_char(buf,plr->ch);
      GET_GOLD(plr->ch) += plr->bet*2;
      free_player(plr);
    } else {
      send_to_char("You lose your bet.\r\n",plr->ch);
      free_player(plr);
    }
  }

}

void bj_dealer_ai(void *arg1, void *arg2, void *arg3)
{
  struct table_data *tbl = (table_data *) arg1;
  switch (tbl->state)
  {
    case 1:
	send_to_table("It is now the dealer's turn.\r\n", tbl);
	tbl->state++;
    case 2:
	char buf[MAX_STRING_LENGTH];
//	sprintf(buf, "A dealer has %d.\r\n",hand_strength(tbl));
//	send_to_table(buf, tbl);
	if (hand_strength(tbl) < 17)
	{ // Hit!
	   int nc = pickCard(tbl->deck),i;
	   for (i = 0;i<21;i++)
		if (tbl->hand_data[i] == 0) break;
	   tbl->hand_data[i] = nc;

	   sprintf(buf, "A dealer takes a new card, revealing a %s%c! A dealer has %d!", valstri(nc), 
			suit(nc), hand_strength(tbl));
 	   send_to_table(buf, tbl);
 	   add_timer_bj_dealer(tbl);
	} else {
	  check_winner(tbl);
	  reset_table(tbl);
	  tbl->state = 3;
	  send_to_room("$B$7A dealer says 'Taking new bets!'$R\r\n",tbl->obj->in_room);
	  add_timer_bj_dealer2(tbl);
        }
	break;
    case 3: 
	// no bets were made
	if (!tbl->plr) { add_timer_bj_dealer2(tbl); return; }
	send_to_room("$B$7A dealer says 'No more bets!'$R\r\n",tbl->obj->in_room);
	tbl->state = 0;
	pulse_table_bj(tbl);
	return; // Taking new bets. 
    default:
	reset_table(tbl);
	return;
  };
  
}

void pulse_table_bj(struct table_data *tbl, int recall )
{
  if (tbl->state)
  {
	return;    
  }
  if (tbl->cr && !verify(tbl->cr)) return; // verify recalls this function with correct data

  if (tbl->cr)
  {  
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "A dealer turns to %s.\r\n$B$7A dealer says 'It's your turn, %s, what would you like to do?'\r\n",
		GET_NAME(tbl->cr->ch), GET_NAME(tbl->cr->ch));
    send_to_table(buf, tbl, tbl->cr);
    sprintf(buf, "A dealer turns to you.\r\n$B$7A dealer says 'It's your turn, %s, what would you like to do?'\r\n",
	 GET_NAME(tbl->cr->ch));
    send_to_char(buf, tbl->cr->ch);
    tbl->handnr++;
    add_timer(tbl->cr);
  } else {
  // new hand
    tbl->handnr++;
    send_to_table("A dealer passes out cards to everyone at the table.\r\n",tbl);
    struct player_data *plr;
    for (plr = tbl->plr;plr; plr = plr->next)
	if (verify(plr))
	{
	  plr->hand_data[0] = pickCard(tbl->deck);
	  plr->hand_data[1] = pickCard(tbl->deck);
	}
    tbl->cr = tbl->plr;
    tbl->hand_data[0] = pickCard(tbl->deck);
    tbl->hand_data[1] = pickCard(tbl->deck);
    pulse_table_bj(tbl,1); // first persons turn
  }
}

void create_table(struct obj_data *obj)
{
  struct table_data *table;
 #ifdef LEAK_CHECK
  table = (struct table_data *)calloc(1, sizeof(struct table_data));
 #else
  table = (struct table_data *)dc_alloc(1, sizeof(struct table_data));
 #endif
  table->obj = obj;
  table->deck = create_deck(6);
  table->plr = table->cr = NULL;
  for (int i = 0; i < 21; i++) table->hand_data[i] = 0;
  table->state = 3;
//  add_timer_dealer_bj2(table);
  obj->table = table;
}

int blackjack_table(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
                   CHAR_DATA *invoker)
{
  if (!obj->table)
  {
	create_table(obj);
  }
}

// 29100+
