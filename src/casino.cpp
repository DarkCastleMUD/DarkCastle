
/*

 Real bloody blackjack. Who does two cards then nothing else? Wankers, that's who

 
*/
extern "C"
{
#include <ctype.h>
#include <string.h>
}
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
#include <timeinfo.h>
#include <DC.h>
#include <algorithm>

using namespace std;

extern OBJ_DATA *object_list;
extern CWorld world;
extern struct index_data *obj_index;

void pulse_table_bj(struct table_data *tbl, int recall = 0);
void reset_table(struct table_data *tbl);
void nextturn(struct table_data *tbl);
void bj_dealer_ai(void *arg1, void *arg2, void *arg3);
void add_timer_bj_dealer(struct table_data *tbl);
struct table_data;
void addtimer(struct timer_data *add);
int hand_number(struct player_data *plr);
int hands(struct player_data *plr);
void blackjack_prompt(CHAR_DATA *ch, char *prompt, bool ascii);
bool charExists(CHAR_DATA *ch);
void reel_spin(void *, void *, void *);

/*
   BLACKJACK!
*/

struct player_data
{
  struct player_data *next;
  struct table_data *table;
  CHAR_DATA *ch;
  int hand_data[21];
// theoretical cardmax is lower than 21, but whatever
  int bet;
  bool insurance;
  bool doubled;
  int state;
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
  bool gold;
  int options;
  CHAR_DATA *dealer;
  int hand_data[21]; // dealer
  int handnr;
  int state;
  int won;
  int lost;
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
    Deck->decks = decks;
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
    prev = tmp;
  }
  if (plr->ch && charExists(plr->ch) && !IS_NPC(plr->ch))
  {
     do_save(plr->ch, "", 666);
  }
  if (tbl->cr == plr)
  {
	nextturn(tbl);
/*	tbl->cr = tbl->cr->next;
	if (tbl->cr)
	pulse_table_bj(tbl);
	else
	reset_table(tbl);*/
  }
  if (!tbl->plr) reset_table(tbl);
  dc_free(plr);
}


void nextturn(struct table_data *tbl)
{
      if (!tbl->plr) {
	reset_table(tbl); return; }

      if (tbl->cr->next) {tbl->cr = tbl->cr->next;
	      pulse_table_bj(tbl); }
      else {tbl->cr = NULL; 
	add_timer_bj_dealer(tbl);
	}
}

void send_to_table(char *msg, struct table_data *tbl, struct player_data *plrSilent = NULL)
{
  //  struct player_data *plr;
/*  for (plr = tbl->plr ; plr ; plr = plr->next)
   if (verify(plr) && plrSilent != plr)
     send_to_char(msg,plr->ch);
  */
if (tbl->obj->in_room)
  send_to_room(msg, tbl->obj->in_room, TRUE, plrSilent?plrSilent->ch:0);
}

bool charExists(CHAR_DATA *ch)
{
  auto &character_list = DC::instance().character_list;

  if (character_list.find(ch) != character_list.end()) {
	  return true;
  } else {
	  return false;
  }
}

bool verify(struct player_data *plr)
{
  // make sure player didn't quit, die, or whatever
  //CHAR_DATA *ch;

  auto &character_list = DC::instance().character_list;

  auto result = find_if(character_list.begin(), character_list.end(),[&plr](char_data * const &ch){
	  if (ch == plr->ch) {
		  return true;
	  } else {
	  	  return false;
	  }
  });

  if (result == character_list.end() || (*result)->in_room != plr->table->obj->in_room)
  {
	if (result != character_list.end())
	{
	 char buf[MAX_STRING_LENGTH];
	 sprintf(buf, "%s folds as %s leaves the room.\r\n", GET_NAME(plr->ch), HSSH(plr->ch));
	 send_to_table(buf, plr->table);
	 send_to_char("Your hand is folded as you leave the room.\r\n",plr->ch);
	}
    free_player(plr); 
    return false;
  }// dead or quit
  return true;
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
  if (tDeck->table)
   send_to_table("The dealer shuffles the deck.\r\n",tDeck->table);
 // shuffled
}


char suit(int card)
{
  if (card < 14) return 'h';
  else if (card < 27) return 'd';
  else if (card < 40) return 'c';
  else return 's';
}

char *suitcol(int card)
{
  if (card < 14) return BOLD RED;
  else if (card < 27) return BOLD RED;
  else if (card < 40) return BOLD BLACK;
  return BOLD BLACK;
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
	case 10: return "T";
	case 11: return "J";
	case 12: return "Q";
	case 13: return "K";
	default: return "?";
  }
}

bool canInsurance(struct player_data *plr)
{
  if (val(plr->table->hand_data[0]) == 1 &&
	plr->table->hand_data[2] == 0 &&
	plr->hand_data[2] == 0 &&
	plr->table->state == 1 &&
	!plr->insurance)
    return TRUE;
  return FALSE;
}

bool canSplit(struct player_data *plr)
{ 
  if (plr->hand_data[0] && val(plr->hand_data[0]) == val(plr->hand_data[1]) && !plr->hand_data[2] && plr->table->cr == plr)
   return TRUE;

  return FALSE;
}

struct player_data *createPlayer(CHAR_DATA *ch, struct table_data *tbl, int noadd = 0)
{
  struct player_data *plr;
 #ifdef LEAK_CHECK
  plr = (struct player_data *)calloc(1, sizeof(struct player_data));
 #else
  plr = (struct player_data *)dc_alloc(1, sizeof(struct player_data));
 #endif
 plr->table = tbl;
 plr->ch = ch;
 for (int i = 0; i < 21;i++) plr->hand_data[i] = 0;
 plr->bet = 0;
 plr->insurance = plr->doubled = FALSE;
 plr->state = 0;
 if (!noadd) {
  struct player_data *tmp;
 for (tmp = tbl->plr; tmp; tmp = tmp->next)
   if (tmp->next == NULL) break;

  if (tmp)  tmp->next = plr;
  else tbl->plr = plr;
 plr->next = NULL;
 }
 return plr;
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
		z+=val(plr->hand_data[i]);
		break;
    }
  int v = 0;
  for (i = 0;plr->hand_data[i] != 0;i++)
	if (val(plr->hand_data[i]) == 1)
	{
	v++;
	}
  while (v > 1) { v--; z++; }
  if (v == 1)
  {
    if (z+11 > 21) z++;
    else z+=11;
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
		z+=val(tbl->hand_data[i]);
		break;
    }
  for (i = 0;tbl->hand_data[i] != 0;i++)
	if (val(tbl->hand_data[i]) == 1)
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
  struct table_data *tbl = (struct table_data *) arg3;
  struct player_data *ptmp = NULL;
 for (ptmp = tbl->plr; ptmp; ptmp = ptmp->next)
    if (ptmp == plr) break;
  if (!ptmp) return; // handled elsewhere
  if (!verify(plr) ) return;

  if ((int)arg2 == plr->table->handnr || (int)arg2 == (plr->table->handnr+100) *2)
  {
	  struct timer_data *timer;
 #ifdef LEAK_CHECK
	  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
	  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
	  timer->arg1 = (void*)plr;
	  timer->arg2 = (void*)(((int)arg2 + 100)*2);
	  timer->arg3 = (void*)plr->table;
	  timer->function = check_active;
	  timer->timeleft = 10;
	  addtimer(timer);
	  char buf[MAX_STRING_LENGTH];
  	  sprintf(buf, "The dealer nudges %s.\r\n",GET_NAME(plr->ch));
	  send_to_table(buf, plr->table, plr);
	  send_to_char("The dealer nudges you.\r\n",plr->ch);	  
  }
  if ((int)arg2 == (((plr->table->handnr+100)*2+100)*2))
  {// inactive
	struct table_data *tbl = plr->table;
	CHAR_DATA *ch = plr->ch;
	
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "Security removes a sleepy %s from the table.\r\n",GET_NAME(plr->ch));
	send_to_table(buf,tbl,plr);
	struct player_data *tmp, *tnext;
	for (tmp = tbl->plr; tmp; tmp = tnext)
	{
 	  tnext = tmp->next;
	  if (tmp->ch == ch && tbl->cr != tmp)
		free_player(tmp); // free non-active hands FIRST
	}
	if (tbl->cr && tbl->cr->ch == ch)
		free_player(tbl->cr);
	send_to_char("Security helps you up from the table where you've apparently fallen asleep!\r\n",ch);
  }
}

void addtimer(struct timer_data *add)
{
  struct timer_data *timer;
  for (timer = timer_list;timer;timer = timer->next)
  {
	if (timer->next == NULL) { timer->next = add; return; }
  }
  timer_list = add;
}

void add_timer(struct player_data *plr)
{
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
  timer->arg1 = (void*)plr;
  timer->arg2 = (void*)plr->table->handnr;
  timer->arg3 = (void*)plr->table;
  timer->function = check_active;
//  timer->next = timer_list;
  //timer_list = timer;
  timer->timeleft = 10;
  addtimer(timer);
}

void  bj_dealer_aiz(void *arg1, void *arg2, void *arg3)
{ // hack so I don't have to bother
  struct table_data *tbl = (table_data *)arg1;
  tbl->state = 2;
  bj_dealer_ai(arg1, arg2, arg3);
}

void add_timer_bj_dealer(struct table_data *tbl)
{
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
  timer->arg1 = (void*)tbl;
  timer->arg2 = 0;
  if (tbl->state != 3)
  timer->function = bj_dealer_aiz;
  else
  timer->function = bj_dealer_ai;
  timer->timeleft = 2;
  addtimer(timer);
}

void add_timer_bj_dealer2(struct table_data *tbl, int time = 10)
{
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
  timer->arg1 = (void*)tbl;
  timer->arg2 = (void*)(++tbl->handnr);
  if (tbl->handnr == 0) // not plausible, but possible
    timer->arg2 = (void*)(++tbl->handnr);
  timer->function = bj_dealer_ai;
  timer->timeleft = time;
  addtimer(timer);
}
void bj_finish(void *arg1, void *arg2, void *arg3)
{
  struct table_data *tbl = (table_data*)arg1;
  send_to_room("$B$7The dealer says 'Place your bets!'$R\r\n",tbl->obj->in_room, TRUE);
  tbl->state = 0;
}

void add_new_bets(struct table_data *tbl)
{
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
  timer->arg1 = (void*)tbl;
  timer->function = bj_finish;
  timer->timeleft = 2;
  addtimer(timer);
}

void reset_table(struct table_data *tbl)
{ // called both on error and regular reset
  while (tbl->plr) free_player(tbl->plr);
  tbl->cr = NULL;
  tbl->state = 0;
  for (int i = 0;i < 21;i++) tbl->hand_data[i] = 0;
}

void check_winner(struct table_data *tbl)
{
  int dealer = hand_strength(tbl);
  struct player_data *plr,*next;
    if (tbl->hand_data[2] == 0)
    {
	char buf[MAX_STRING_LENGTH];
	sprintf(buf, "The dealer has %d!\r\n",hand_strength(tbl));
	send_to_table(buf, tbl);
    }
  for (plr = tbl->plr;plr;plr=next)
  {
   next = plr->next;
   if (!verify(plr)) continue;
    if (plr->insurance && hand_strength(tbl)  == 21 &&
	  tbl->hand_data[2] == 0)
    {// insurance bet paid off
	int pay = plr->doubled ? plr->bet/2 : plr->bet;
	csendf(plr->ch, "Your insurance bet paid %d %s.\r\n",
		pay,plr->table->gold?"gold":"platinum");
	if (plr->table->gold)
	GET_GOLD(plr->ch) += pay;
	else
	GET_PLATINUM(plr->ch) += pay;
    }
    if  (hand_strength(plr) > 21) continue;
    if (dealer == hand_strength(plr))
    {
      char buf[MAX_STRING_LENGTH];
      sprintf(buf,"It's a PUSH!\r\nThe dealer takes your cards and gives you %d %s coins.\r\n",
		plr->bet, plr->table->gold?"gold":"platinum");
      send_to_char(buf,plr->ch);
      sprintf(buf,"The dealer gives %s %d coins.\r\n",GET_NAME(plr->ch),
	      plr->bet);//, plr->table->gold?"gold":"platinum");
//		plr->bet);
      send_to_table(buf,tbl, plr);
	if (plr->table->gold)
      GET_GOLD(plr->ch) += plr->bet;
	else
      GET_PLATINUM(plr->ch) += plr->bet;
      free_player(plr);
	continue;
    }

    if (dealer > 21 || (hand_strength(plr) > dealer && hand_strength(plr) <= 21))
    {
      char buf[MAX_STRING_LENGTH];
	send_to_char("$BYou WIN!$R\r\n",plr->ch);
      sprintf(buf,"The dealer takes your cards and gives you %d %s coins.\r\n",
		plr->bet*2, plr->table->gold?"gold":"platinum");
      send_to_char(buf,plr->ch);
      sprintf(buf,"The dealer gives %s %d %s coins.\r\n",GET_NAME(plr->ch),
		plr->bet*2, plr->table->gold?"gold":"platinum");
      send_to_table(buf,tbl, plr);
	if (plr->table->gold)
      GET_GOLD(plr->ch) += plr->bet*2;
	else
      GET_PLATINUM(plr->ch) += plr->bet*2;
      free_player(plr);
    } else {
	if (verify(plr)) {
      send_to_char("$BYou LOSE your bet!$R\r\n",plr->ch);
      free_player(plr);
     }
    }
  }
  reset_table(tbl);
  add_new_bets(tbl);
}

void bj_dealer_ai(void *arg1, void *arg2, void *arg3)
{
  struct table_data *tbl = (table_data *) arg1;
  int a = (int)arg2;
  char buf[MAX_STRING_LENGTH];
  if (a && tbl->handnr != a) return; // handled earlier
  bool cont = FALSE;

  switch (tbl->state)
  {
    case 2:
	send_to_table("It is now the dealer's turn.\r\n", tbl);
	tbl->state++;
	sprintf(buf, "The dealer flips over his card revealing a %s%s%c%s.\r\n",
		suitcol(tbl->hand_data[1]), valstri(tbl->hand_data[1]), suit(tbl->hand_data[1]),
		NTEXT);
	send_to_table(buf, tbl);
	struct player_data *plr,*pnext;
	for (plr=tbl->plr;plr;plr = pnext)
	{ // check if all players have busted
	   pnext = plr->next;
	  if (hand_strength(plr) < 22) cont = TRUE;
	}
	if (!cont) {check_winner(tbl); return;}
//        add_timer_bj_dealer(tbl);
	// no break;
    case 3:
//	sprintf(buf, "The dealer has %d.\r\n",hand_strength(tbl));
//	send_to_table(buf, tbl);
	tbl->handnr++;
	if (hand_strength(tbl) < 17)
	{ // Hit!
	   int nc = pickCard(tbl->deck),i;
	   for (i = 0;i<21;i++)
		if (tbl->hand_data[i] == 0) break;
	   tbl->hand_data[i] = nc;

	   sprintf(buf, "The dealer takes a new card, revealing a %s%s%c%s! The dealer has %d!\r\n", suitcol(nc), valstri(nc), 
			suit(nc), NTEXT,hand_strength(tbl));
 	   send_to_table(buf, tbl);
 	   add_timer_bj_dealer(tbl);
	} else {
	  check_winner(tbl);
        }
	break;
    case 0:
    case 1: 
// TODO CHECK THIS
//	if (!tbl->plr) { add_timer_bj_dealer2(tbl); return; }
	send_to_room("$B$7The dealer says 'No more bets!'$R\r\n\r\n",tbl->obj->in_room, TRUE);
	tbl->state = 0;
	pulse_table_bj(tbl);
	break;
    default:
	reset_table(tbl);
	return;
  };
  
}

void check_blackjacks(struct table_data *tbl)
{
  char buf[MAX_STRING_LENGTH];
  struct player_data *plr, *next;
  if (hand_strength(tbl) == 21)
  {
    send_to_table("The dealer blackjacked!\r\n",tbl);
    for (plr = tbl->plr; plr; plr = next)
    {
	next = plr->next;
	if (!verify(plr)) continue;
	buf[0] = '\0';
	blackjack_prompt(plr->ch, buf, !IS_SET(plr->ch->pcdata->toggles, PLR_ASCII));
	send_to_char(buf, plr->ch);
	
    }
	check_winner(tbl);
	return;
  }
  for (plr = tbl->plr; plr; plr = next)
  {
	next = plr->next;
	if (!verify(plr)) continue;
	if (hand_strength(plr) == 21 &&
		hand_strength(tbl) != 21)
	{
		sprintf(buf, "%s blackjacks!\r\n",GET_NAME(plr->ch));
		send_to_table(buf,tbl, plr);
		send_to_char("$BYou BLACKJACK!$R\r\n",plr->ch);
		sprintf(buf, "The dealer gives you %d %s coins.\r\n", 
		(int)(plr->bet*2.5), plr->table->gold?"gold":"platinum");

		send_to_char(buf, plr->ch);
		buf[0] = '\0';
		blackjack_prompt(plr->ch, buf, !IS_SET(plr->ch->pcdata->toggles, PLR_ASCII));
		send_to_char(buf, plr->ch);

		if (plr->table->gold)
		GET_GOLD(plr->ch) += (uint32)(plr->bet*2.5);
		else
		GET_PLATINUM(plr->ch) += (uint32)(plr->bet*2.5);
//        	nextturn(plr->table);
		if (tbl->plr == plr && !plr->next)
		{ // all players blackjacked
	char buf[MAX_STRING_LENGTH];
		sprintf(buf, "The dealer flips over his card revealing a %s%s%c%s.\r\n",
		suitcol(tbl->hand_data[1]), valstri(tbl->hand_data[1]), suit(tbl->hand_data[1]),
		NTEXT);
		send_to_table(buf, tbl);
		sprintf(buf, "The dealer has %d!\r\n",hand_strength(tbl));
		send_to_table(buf, tbl);
		free_player(plr);
		reset_table(tbl);
		add_new_bets(tbl);
		return;
		}
		free_player(plr);
		
	}
  }
}

void check_insurance2(void *arg1, void *arg2, void *arg3)
{
  struct table_data *tbl = (table_data *) arg1;

  if (hand_strength(tbl) == 21)
  { // dealer blackjacked
    send_to_table("Dealer blackjacked.\r\n",tbl);
    check_winner(tbl);
  } else {
    tbl->state = 0;
    check_blackjacks(tbl);
    tbl->cr = tbl->plr;
    pulse_table_bj(tbl);
  }
}

void check_insurance(struct table_data *tbl)
{
  if (val(tbl->hand_data[0]) == 1)
  { // ace showing
     tbl->state = 1;
     send_to_table("$B$7The dealer says 'Blackjack insurance is available. Type INSURANCE to buy some.'$R\r\n",tbl);
     struct timer_data *timer;
 #ifdef LEAK_CHECK
     timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
     timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif
     timer->arg1 = (void*)tbl;
     timer->arg2 = 0;
     timer->function = check_insurance2;
     timer->timeleft = 8;
    addtimer(timer);
  } else {
    check_blackjacks(tbl);
    tbl->cr = tbl->plr;
    pulse_table_bj(tbl);   
  }
}
char *hand_thing(struct player_data *plr)
{
 if (hands(plr) <= 1) return "";
 static char buf[MAX_STRING_LENGTH];
 sprintf(buf, " for hand %d", hand_number(plr));
  return &buf[0];
}

void pulse_table_bj(struct table_data *tbl, int recall )
{
/*  if (tbl->state)
  {
	return;
  }*/
  if (tbl->cr && !verify(tbl->cr)) return; // verify recalls this function with correct data

  if (tbl->cr)
  {  
    char buf[MAX_STRING_LENGTH];
    sprintf(buf, "$B$7The dealer says 'It's your turn, %s, what would you like to do%s?'$R\r\n",
		GET_NAME(tbl->cr->ch),hand_thing(tbl->cr));
    send_to_table(buf, tbl);
    tbl->handnr++;
    add_timer(tbl->cr);
  } else if (tbl->plr) {
  // new hand
    tbl->handnr++;
    send_to_table("The dealer passes out cards to everyone at the table.\r\n",tbl);
    struct player_data *plr,*pnext;
    for (plr = tbl->plr;plr; plr = pnext)
    {
	pnext = plr->next;
	if (verify(plr))
	{
	  plr->hand_data[0] = pickCard(tbl->deck);
	  plr->hand_data[1] = pickCard(tbl->deck);
	  plr->state = 1;
	}
    }
    tbl->hand_data[0] = pickCard(tbl->deck);
    tbl->hand_data[1] = pickCard(tbl->deck);
    check_insurance(tbl);
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
  if (obj->obj_flags.value[2]) table->gold = FALSE;
  else table->gold = TRUE;
  table->deck = create_deck(6);
  shuffle_deck(table->deck);
  table->plr = table->cr = NULL;
  table->deck->table = table;
  for (int i = 0; i < 21; i++) table->hand_data[i] = 0;

  table->state = 0;
//  add_timer_bj_dealer2(table);
  obj->table = table;
}

void destroy_table(struct table_data *tbl)
{
  tbl->obj->table = NULL;
  reset_table(tbl);
  dc_free(tbl);
}

bool playing(CHAR_DATA *ch, struct table_data *tbl)
{
  struct player_data *plr;
  for (plr = tbl->plr;plr;plr = plr->next)
    if (plr->ch == ch) return TRUE;

  return FALSE;
}

struct player_data *getPlayer(CHAR_DATA *ch, struct table_data *tbl)
{
  struct player_data *plr;
  if (tbl->cr && tbl->cr->ch == ch) return tbl->cr; // priority on current hand
  for (plr = tbl->plr;plr;plr = plr->next)
    if (plr->ch == ch) return plr;
 return NULL;
}

int players(struct table_data *tbl)
{
  struct player_data *plr;
  int i = 0;
  for (plr = tbl->plr; plr; plr= plr->next)
   i++;
  return i;    
}

char tempBuf[MAX_STRING_LENGTH];
char lineTwo[MAX_STRING_LENGTH];
char lineTop[MAX_STRING_LENGTH];
int padnext = 0;
// Not pretty, but don't feel like redoing the prompt functions, so whatever.


char *show_hand(int hand_data[21], int hide, bool ascii)
{
  static char buf[MAX_STRING_LENGTH];
  int i = 0;
  buf[0] = '\0';
  sprintf(lineTwo, "%s%*s", lineTwo, strlen(tempBuf)+padnext, " ");
  sprintf(lineTop, "%s%*s", lineTop, strlen(tempBuf)+padnext, " ");
  if (padnext)
	padnext = 0;
  while (hand_data[i] > 0)
  {
     if(!ascii) {
	if (i == 1 && hide)
	sprintf(buf,"%s %sDC%s", buf, BOLD, NTEXT);
	else
	sprintf(buf,"%s %s%s%c%s", buf, suitcol(hand_data[i]), valstri(hand_data[i]), suit(hand_data[i]),NTEXT);
	i++;
      } else {
	if (i == 1 && hide)
	{
	  sprintf(buf,"%s%s| D |%s", buf, BOLD, NTEXT);
	  sprintf(lineTwo, "%s%s| C |%s",lineTwo, BOLD,NTEXT);
	  sprintf(lineTop, "%s%s,---,%s",lineTop, BOLD, NTEXT);
	}
	else {
	sprintf(buf,"%s%s|%s %s%s%s %s|%s", buf, BOLD, NTEXT, suitcol(hand_data[i]), valstri(hand_data[i]),NTEXT,BOLD, NTEXT);
	sprintf(lineTwo, "%s%s|%s %s%c%s %s|%s", lineTwo,BOLD, NTEXT,suitcol(hand_data[i]),suit(hand_data[i]),NTEXT,BOLD, NTEXT);
	  sprintf(lineTop, "%s%s,---,%s",lineTop, BOLD, NTEXT);
	}
	i++;

      }
  }

  return &buf[0];
}

//int hand_number(struct player_data *plr)
int hands(struct player_data *plr)
{
  int i = 0;
  for (struct player_data *ptmp = plr->table->plr; ptmp;ptmp = ptmp->next)
  {
   if (plr->ch == ptmp->ch) i++;
  }
  return i;
}

int hand_number(struct player_data *plr)
{
  int i = 0;
  for (struct player_data *ptmp = plr->table->plr; ptmp;ptmp = ptmp->next)
  {
   if (plr->ch == ptmp->ch) i++;
   if (plr == ptmp) return i;
  }
  return i;
}

void blackjack_prompt(CHAR_DATA *ch, char *prompt, bool ascii)
{
  if (ch->in_room < 21902 || ch->in_room > 21905)
    if (ch->in_room != 44)
 	return;
  struct obj_data *obj;
  for (obj = world[ch->in_room].contents;obj;obj = obj->next_content)
	if (obj->table) break;
  if (!obj || !obj->table->plr) return;
 // Prompt-time
  int plrsdone = 0;
  char buf[MAX_STRING_LENGTH];
  buf[0] = '\0';
  lineTwo[0] = '\0';
  lineTop[0] = '\0';
  struct player_data *plr,*pnext;
  for (plr = obj->table->plr; plr; plr = pnext)
  {
	pnext = plr->next;
	if (!verify(plr)) continue;
	if (!plr->hand_data[0]) continue;
	if (plr->ch == ch) {
  	  char buf2[MAX_STRING_LENGTH];
	  buf2[0] = '\0';
          if (plr->table->cr == plr)
	  {
	    strcat(buf2, "HIT STAY ");
	    if (plr->hand_data[2] == 0)
		strcat(buf2, "DOUBLE ");
	  }
	  if (canInsurance(plr))
		strcat(buf2, "INSURANCE ");
	  if (canSplit(plr))
		strcat(buf2, "SPLIT ");
	  if (buf2[0] != '\0')
	  {
	    strcat(prompt, "You can: ");
	    strcat(prompt, BOLD CYAN);
	    strcat(prompt, buf2);
	    strcat(prompt, NTEXT);
	    strcat(prompt, "\r\n");
	  }
	if (hands(plr) > 1)
	{
	sprintf(tempBuf, "%s, hand %d: ", GET_NAME(plr->ch), hand_number(plr));
       sprintf(buf,"%s%s%s%s, hand %d%s: %s = %d   ",buf, BOLD,  plr==plr->table->cr?GREEN:"",GET_NAME(plr->ch), hand_number(plr), NTEXT, show_hand(plr->hand_data, 0,ascii), hand_strength(plr));
	padnext = hand_strength(plr) > 9 ? 8:7;
	}
	else {
	sprintf(tempBuf, "%s: ", GET_NAME(plr->ch));
       sprintf(buf,"%s%s%s%s%s: %s = %d   ",buf, BOLD, plr==plr->table->cr?GREEN:"",GET_NAME(plr->ch), NTEXT, show_hand(plr->hand_data, 0,ascii), hand_strength(plr));
	padnext = hand_strength(plr) > 9 ? 8:7;
        }
	}
//    }
   else {
   if (hands(plr) > 1) {
  sprintf(tempBuf, "%s, hand %d: ", GET_NAME(plr->ch), hand_number(plr));
    sprintf(buf,"%s%s%s, hand %d%s: %s ",buf, plr==plr->table->cr?BOLD GREEN:"",GET_NAME(plr->ch),hand_number(plr),
	NTEXT, show_hand(plr->hand_data, 0,ascii));
      padnext = 1;
    }
   else {
  sprintf(tempBuf, "%s: ", GET_NAME(plr->ch));
    
    sprintf(buf,"%s%s%s%s: %s ",buf,plr==plr->table->cr?BOLD GREEN:"", GET_NAME(plr->ch),
	NTEXT,show_hand(plr->hand_data, 0,ascii));
      padnext = 1;
  }
   }
    if (++plrsdone % 3 == 0)
    {
  if (buf[0]!= '\0'){
  strcat(prompt,"\r\n");
  if (ascii) {
  strcat(prompt,lineTop);
  strcat(prompt,"\r\n"); }
  strcat(prompt,buf);
  strcat(prompt,"\r\n");
  if (ascii) {
  strcat(prompt,lineTwo); 
  strcat(prompt,"\r\n"); }

  for (int z = 0; lineTop[z];z++)
	if (lineTop[z] == ',') lineTop[z] = '\'';
  if (ascii) {
  strcat(prompt,lineTop);
  strcat(prompt,"\r\n"); }
  buf[0] = '\0';
  lineTwo[0] = '\0';
  lineTop[0] = '\0';
  padnext = 0;
  }  
	
    }
  }
  if (obj->table->hand_data[0]) {
  sprintf(tempBuf, "Dealer: ");
  sprintf(buf, "%s%sDealer%s: %s",buf,BOLD YELLOW, NTEXT,obj->table->state < 2?
	show_hand(obj->table->hand_data, 1,ascii):show_hand(obj->table->hand_data, 0,ascii));
  sprintf(buf, "%s\r\n",buf); }
  //fixPadding(&buf[0]);
  if (buf[0]!= '\0'){
  strcat(prompt,"\r\n");
  if (ascii) {
  strcat(prompt,lineTop);
  strcat(prompt,"\r\n"); }
  strcat(prompt,buf);
  if (ascii) {
  strcat(prompt,lineTwo);
  strcat(prompt,"\r\n"); }
  for (int z = 0; lineTop[z];z++)
	if (lineTop[z] == ',') lineTop[z] = '\'';
	else if (lineTop[z] == '_') lineTop[z] = '-';
  if (ascii) {
  strcat(prompt,lineTop);
  strcat(prompt,"\r\n");
  }
  }  
}

int blackjack_table(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg,
		CHAR_DATA *invoker) {
	char arg1[MAX_INPUT_LENGTH];
	arg = one_argument(arg, arg1);
	if (cmd < 189 || cmd > 194) {
		return eFAILURE;
	}
	if (!ch || IS_NPC(ch))
		return eFAILURE; // craziness
	if (obj->in_room <= 0)
		return eFAILURE;
	if (!obj->table)
		create_table(obj);
	if (IS_AFFECTED(ch, AFF_CANTQUIT) || affected_by_spell(ch, FUCK_PTHIEF)
			|| affected_by_spell(ch, FUCK_GTHIEF)) {
		send_to_char(
				"You cannot play blackjack while you are flagged as naughty.\r\n",
				ch);
		return eSUCCESS;
	}

	struct player_data *plr = getPlayer(ch, obj->table);

	if (cmd == 189) // bet
			{
		if (obj->table->state > 1 || obj->table->cr
				|| obj->table->hand_data[0]) {
			send_to_char(
					"There is a hand in progress. No bets are accepted at the moment.\r\n",
					ch);
			return eSUCCESS;
		}
		if (playing(ch, obj->table)) {
			send_to_char("You have already made your bet.\r\n", ch);
			return eSUCCESS;
		}
		if (!is_number(arg1)) {
			send_to_char("Bet how much?\r\nSyntax: bet <amount>\r\n", ch);
			return eSUCCESS;
		}
		int amt = atoi(arg1);
		if (obj->table->gold) {
			if (amt < 0 || amt > obj->obj_flags.value[1]
					|| amt < obj->obj_flags.value[0]) {

				csendf(ch, "Minimum bet: %d\r\nMaximum bet: %d\r\n",
						obj->obj_flags.value[0], obj->obj_flags.value[1]);
				return eSUCCESS;
			}
		} else {
			if (amt < 0 || amt > obj->obj_flags.value[3]
					|| amt < obj->obj_flags.value[2]) {

				csendf(ch, "Minimum bet: %d\r\nMaximum bet: %d\r\n",
						obj->obj_flags.value[2], obj->obj_flags.value[3]);
				return eSUCCESS;
			}
		}
		if (obj->table->gold && (uint32) amt > GET_GOLD(ch)) {
			send_to_char(
					"You cannot afford that.\r\n$B$7The dealer whispers to you, 'You can find an ATM machine in the lobby, buddy.'$R\r\n",
					ch);
			return eSUCCESS;
		} else if (!obj->table->gold && (uint32) amt > GET_PLATINUM(ch)) {
			send_to_char("You cannot afford that.\r\n", ch);
			return eSUCCESS;
		}
		if (players(obj->table) > 5) {
			send_to_char("The table is currently full.\r\n", ch);
			return eSUCCESS;
		}
		plr = createPlayer(ch, obj->table);
		plr->bet = amt;
		if (obj->table->gold)
			GET_GOLD(ch) -= amt;
		else
			GET_PLATINUM(ch) -= amt;
		send_to_char("The dealer accepts your bet.\r\n", ch);
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "%s bets %d.\r\n", GET_NAME(ch), amt);
		send_to_table(buf, obj->table, plr);
		if (obj->table->state == 0) {
			CHAR_DATA *tmpch;
			int i = 0;
			for (tmpch = world[ch->in_room].people; tmpch;
					tmpch = tmpch->next_in_room)
				if (!IS_NPC(tmpch))
					i++;
			if (i <= players(obj->table))
				add_timer_bj_dealer2(obj->table, 2); // end bets in 1 secs
			else
				add_timer_bj_dealer2(obj->table); // end bets in 10 secs
			obj->table->state = 1;
		} else {
			CHAR_DATA *tmpch;
			int i = 0;
			for (tmpch = world[ch->in_room].people; tmpch;
					tmpch = tmpch->next_in_room)
				if (!IS_NPC(tmpch))
					i++;
			if (i <= players(obj->table)) {
				struct timer_data *tmr;
				for (tmr = timer_list; tmr; tmr = tmr->next)
					if ((struct table_data*) tmr->arg1 == obj->table)
						tmr->timeleft = 1;
			}
		}
		return eSUCCESS;
	} else if (cmd == 190) //insurance
			{
		if (!plr) {
			send_to_char("You are not currently playing.\r\n", ch);
			return eSUCCESS;
		}
		if (!canInsurance(plr)) {
			send_to_char("You cannot make an insurance bet at the moment.\r\n",
					ch);
			return eSUCCESS;
		}
		if (plr->table->gold && GET_GOLD(ch) < (uint32) (plr->bet / 2)) {
			send_to_char("You cannot afford an insurance bet right now.\r\n",
					ch);
			return eSUCCESS;
		}
		if (!plr->table->gold && GET_PLATINUM(ch) < (uint32) (plr->bet / 2)) {
			send_to_char("You cannot afford an insurance bet right now.\r\n",
					ch);
			return eSUCCESS;
		}
		plr->table->handnr++;
		plr->insurance = TRUE;
		char buf[MAX_STRING_LENGTH];
		if (plr->table->gold)
			GET_GOLD(ch) -= plr->bet / 2;
		else
			GET_PLATINUM(ch) -= plr->bet / 2;
		sprintf(buf, "%s makes an insurance bet.\r\n", GET_NAME(ch));
		send_to_table(buf, plr->table, plr);
		send_to_char("You make an insurance bet.\r\n", ch);
		return eSUCCESS;
	} else if (cmd == 191) // doubledown
			{
		if (!plr) {
			send_to_char("You are not currently playing.\r\n", ch);
			return eSUCCESS;
		}
		if (plr->table->cr != plr) {
			send_to_char("It is not currently your turn.\r\n", ch);
			return eSUCCESS;
		}
		if ((plr->table->gold && GET_GOLD(plr->ch) < (uint32) plr->bet)
				|| (!plr->table->gold
						&& GET_PLATINUM(plr->ch) < (uint32) plr->bet)) {
			send_to_char("You cannot afford to double your bet.\r\n", ch);
			return eSUCCESS;
		}
		if (plr->hand_data[2] || plr->doubled) {
			send_to_char("You cannot double right now.\r\n", ch);
			return eSUCCESS;
		}
		if (plr->table->gold)
			GET_GOLD(plr->ch) -= plr->bet;
		else
			GET_PLATINUM(plr->ch) -= plr->bet;
		plr->bet *= 2;
		plr->doubled = TRUE;
		plr->table->handnr++;
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "%s doubles %s bet.\r\n", GET_NAME(ch), HSHR(ch));
		send_to_table(buf, plr->table, plr);
		send_to_char("You double your bet.\r\n", ch);

		plr->hand_data[2] = pickCard(plr->table->deck);
		sprintf(buf, "%s receives a %s%s%c%s.\r\n", GET_NAME(ch),
				suitcol(plr->hand_data[2]), valstri(plr->hand_data[2]),
				suit(plr->hand_data[2]), NTEXT);
		send_to_table(buf, plr->table, plr);
		sprintf(buf, "You receive a %s%s%c%s.\r\n", suitcol(plr->hand_data[2]),
				valstri(plr->hand_data[2]), suit(plr->hand_data[2]), NTEXT);
		send_to_char(buf, ch);

		if (hand_strength(plr) > 21) // busted
				{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "%s busted.\r\n", GET_NAME(ch));
			send_to_table(buf, plr->table, plr);
			send_to_char("$BYou BUSTED!$R\r\nThe dealer takes your bet.\r\n",
					ch);
			nextturn(plr->table);
			if (plr->table->plr != plr || plr->next == NULL) // make dealer show cards..
				free_player(plr);
		} else {
			nextturn(plr->table);
		}
//     pulse_table_bj(plr->table);
		return eSUCCESS;
	} else if (cmd == 192) // stand
			{
		if (!plr) {
			send_to_char("You are not currently playing.\r\n", ch);
			return eSUCCESS;
		}
		if (plr->table->cr != plr) {
			send_to_char("It is not currently your turn.\r\n", ch);
			return eSUCCESS;
		}
		char buf[MAX_STRING_LENGTH];
		plr->table->handnr++;
		sprintf(buf, "%s stays.\r\n", GET_NAME(ch));
		send_to_table(buf, plr->table);
		nextturn(plr->table);
		return eSUCCESS;
	} else if (cmd == 193) // split
			{
		if (!plr) {
			send_to_char("You are not currently playing.\r\n", ch);
			return eSUCCESS;
		}
		if (!canSplit(plr)) {
			send_to_char("You cannot split right now.\r\n", ch);
			return eSUCCESS;
		}
		if ((GET_GOLD(ch) < (uint32) plr->bet && plr->table->gold)
				|| (GET_PLATINUM(ch) < (uint32) plr->bet && !plr->table->gold)) {
			send_to_char("You cannot afford to split.\r\n", ch);
			return eSUCCESS;
		}
		if (plr->table->gold)
			GET_GOLD(ch) -= plr->bet;
		else
			GET_PLATINUM(ch) -= plr->bet;
		struct player_data *nw = createPlayer(ch, plr->table, 1);
		nw->next = plr->next;
		plr->next = nw;
		nw->bet = plr->bet;
		plr->table->handnr++;
		nw->hand_data[0] = plr->hand_data[1];
		plr->hand_data[1] = pickCard(plr->table->deck);
		nw->hand_data[1] = pickCard(plr->table->deck);
		nw->doubled = plr->doubled;
		send_to_char("You split your hand.\r\n", ch);
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "%s splits %s hand.\r\n", GET_NAME(ch), HSHR(ch));
		send_to_table(buf, plr->table, plr);
		pulse_table_bj(plr->table);
		return eSUCCESS;
	} else if (cmd == 194) // hit
			{
		if (!plr) {
			send_to_char("You are not currently playing.\r\n", ch);
			return eSUCCESS;
		}
		if (plr->table->cr != plr) {
			send_to_char("It is not currently your turn.\r\n", ch);
			return eSUCCESS;
		}
		int i;
		for (i = 0; i < 21; i++)
			if (plr->hand_data[i] == 0)
				break;
		plr->hand_data[i] = pickCard(plr->table->deck);
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "%s hits and receives a %s%s%c%s.\r\n", GET_NAME(ch),
				suitcol(plr->hand_data[i]), valstri(plr->hand_data[i]),
				suit(plr->hand_data[i]), NTEXT);
		send_to_table(buf, plr->table, plr);
		sprintf(buf, "You hit and receive a %s%s%c%s.\r\n",
				suitcol(plr->hand_data[i]), valstri(plr->hand_data[i]),
				suit(plr->hand_data[i]), NTEXT);
		send_to_char(buf, ch);
		if (hand_strength(plr) > 21) // busted
				{
			char buf[MAX_STRING_LENGTH];
			sprintf(buf, "%s busted.\r\n", GET_NAME(ch));
			send_to_table(buf, plr->table, plr);
			send_to_char("$BYou BUSTED!$R\r\nThe dealer takes your bet.\r\n",
					ch);
			nextturn(plr->table);
			if (plr->table->plr != plr || plr->next != NULL) // make dealer show cards..
				free_player(plr);
		} else {
			if (plr->doubled)
				nextturn(plr->table);
			else {
				pulse_table_bj(plr->table);
			}
		}
		return eSUCCESS;
	}
	return eSUCCESS;
}

/* End Blackjack */

/* Texas Hold'em! */
struct tplayer;
struct ttable;

int hand[5][2]; 
 // cycles between using two spaces for temporary data
 // as comparisons need to be made


struct pot
{
  struct pot *next;
  struct tplayer *player[6]; 
};

struct tplayer
{
  struct ttable *table;
  int hand[5]; // 0-1 playercards, 2-4 = top table cards
  int chips;
  int pos;
  int options;
  bool nw;
  bool dealer;
};

struct ttable
{
  struct cDeck *deck;
  struct pot *pots;
  struct tplayer *player[6];
  int cards[5]; // cards on the table
  int bet;
  int crPlayer;
  int state;
};

int has_seat(struct ttable *ttbl)
{
  for (int i = 0; i < 6; i++)
     if (ttbl->player[i] == NULL) return i;
  return -1;
}

bool findcard(int hand[5], int valu, char su, int num = 1)
{
  int i;
  for (i = 0; i<5;i++)
    if (!valu || val(hand[i]) == valu)
      if (!su || suit(hand[i]) == su)
	if (--num <= 0) return TRUE;

  return FALSE;
}

int highcard(int hand[5])
{
  int a = hand[0];
  for (int i = 1; i < 5; i++)
    if (val(a) < val(hand[i]) || val(hand[i]) == 1)
	a = val(hand[i]);
  return a;
}

int lowcard(int hand[5])
{ // counts aces for straight purposes.
  int a = hand[0];
  for (int i = 1; i < 5; i++)
    if (val(a) > val(hand[i]))
	a = val(hand[i]);
  return a;
}

int has_flush(int hand[5])
{
  if (findcard(hand, 0, suit(hand[0]), 5))
   return highcard(hand);
  return FALSE;
}

int has_straight(int hand[5])
{
  int i = lowcard(hand);
  if (findcard(hand, i+1, 0, 1) &&
	findcard(hand, i+2, 0, 1) &&
	findcard(hand, i+3, 0, 1) &&
	findcard(hand, i+4, 0, 1))
    return i+4;
 if (i == 1) {
   if (findcard(hand, 10, 0, 1) &&
	findcard(hand, 11, 0, 1) &&
	findcard(hand, 12, 0, 1) &&
	findcard(hand, 13, 0, 1))
         return 14;
 }
 return FALSE;
    
}

int has_rsf(int hand[5])
{
  if (hand[0] == 0) return FALSE; // shockingly, nope
  if (has_flush(hand) && has_straight(hand) == 14)
   return TRUE; // yegods

  return FALSE;  
}

int has_sf(int hand[5])
{
  if (hand[0] == 0) return FALSE;
  if (has_flush(hand))
   return has_straight(hand);

  return FALSE;  
}

int has_4kind(int hand[5])
{
  if (findcard(hand, val(hand[0]), 0, 4))
    return val(hand[0]);
  if (findcard(hand,val(hand[1]), 0, 4))
    return val(hand[1]);
   // one of the first two cards has to be part of the 4kind
 return FALSE;
}

int has_fhouse(int hand[5])
{
  int card1 = val(hand[0]);
  int i;
  for (i = 1; i < 5; i++) 
   if (val(hand[i]) != card1)
     break;
  // no error checking needed, it is not possible to have 5 of a kind
  if ((findcard(hand, card1, 0, 3) &&
	findcard(hand, val(hand[i]), 0, 2)) ||
	(findcard(hand, card1, 0, 2) &&
	findcard(hand, val(hand[i]), 0, 3)))
  	return highcard(hand) * 1000 + lowcard(hand);// works
  return FALSE;
}

int has_3kind(int hand[5])
{
  if (findcard(hand, val(hand[0]), 0, 3))
     return val(hand[0]);
  if (findcard(hand, val(hand[1]), 0, 3))
     return val(hand[1]);
  if (findcard(hand, val(hand[2]), 0, 3))
     return val(hand[2]);
  return FALSE;
}

int has_2pair(int hand[5])
{
  int first = 0;
  for (int i = 0; i < 5; i++)
    if (val(hand[i]) != val(first) && findcard(hand, val(hand[i]), 0, 2))
	{
		if (first) return MAX(val(first), val(hand[i]))*1000+MIN(val(first), val(hand[i]));
		else first = hand[i];
	}
 return FALSE;
}

int has_pair(int hand[5])
{
  if (findcard(hand, val(hand[0]), 0, 2))
    return val(hand[0]);
	if (findcard(hand, val(hand[1]), 0, 2))
    return val(hand[0]);
	if (findcard(hand, val(hand[2]), 0, 2))
    return val(hand[0]);
	if (findcard(hand, val(hand[3]), 0, 2))
    return val(hand[0]);
  return FALSE;
}

char *cardname(int card)
{
  switch (card)
  {
    case 1: return "Ace";
    case 2: return "Two";
    case 3: return "Three";
    case 4: return "Four"; 
    case 5: return "Five";
    case 6: return "Six";
    case 7: return "Seven";
    case 8: return "Eight";
    case 9:  return "Nine";
    case 10: return "Jack";
    case 12: return "Queen";
    case 13: return "King";
  }
  return "Invalid";
}


char *hand_name(int hand[5])
{
  char buf[MAX_STRING_LENGTH];
  buf[0] = '\0';
  int i = 0;
  if (has_rsf(hand))
    return "Royal Straight Flush";
  else if ((i = has_sf(hand)))
  {
     sprintf(buf, "%s-high Straight Flush",cardname(i));
  } else if ((i = has_4kind(hand))) {
     sprintf(buf, "Four of a kind, %ss",cardname(i));
  } else if ((i = has_fhouse(hand))) {
     sprintf(buf, "Full House, %s over %ss",cardname(i/1000), cardname(i-(i/1000)*1000));
  } else if ((i = has_flush(hand))) {
     sprintf(buf, "%s-high Flush",cardname(i));
  } else if ((i = has_straight(hand))) {
     sprintf(buf, "%s-high Straight",cardname(i));
  } else if ((i = has_3kind(hand))) {
     sprintf(buf, "Three of a kind, %ss",cardname(i));
  } else if ((i = has_2pair(hand))) {
     sprintf(buf, "Two Pair, %ss and %ss",cardname(i/1000), cardname(i-(i/1000)*1000));
  } else if ((i = has_pair(hand))) {
     sprintf(buf, "Pair of %ss",cardname(i));
  } else {
    sprintf(buf, "%s high", cardname(highcard(hand)));
  }
  return str_dup(buf);
}


int get_hand(struct tplayer *tplr, int which)
{
 static int i = 0;
 
 struct ttable *ttbl = tplr->table;
 TOGGLE_BIT(i,1); // if it's 1, set to 0, if 0, set to 1. Gooo toggle!
 int z;
 for (z = 0; z < 5; z++)
    hand[z][i] = 0;

 int o;
 int temphand[7];
 temphand[0] = tplr->hand[0];
 temphand[1] = tplr->hand[1];
 temphand[2] = ttbl->cards[0];
 temphand[3] = ttbl->cards[1];
 temphand[4] = ttbl->cards[2];
 temphand[5] = ttbl->cards[3];
 temphand[6] = ttbl->cards[4];
 for (z = 0; z < 7; z++)
    if (temphand[z] == 0) break;

// int one = 0, two = 1, three = 2, four = 3, five = 4;
 int bleh[5] = {0,1,2,3,4},p;
 for (o = 0; o < which; o++)
 {
    p = 4;
    bleh[p]++;
    while (bleh[p] == (3+p))
    {
	if (p == 0) return -1; // invalid
	bleh[--p]++;
	int zz;
	for (zz = p+1; zz < 5;zz++)
	  bleh[zz] = bleh[zz-1]+1;
//	bleh[p+1] = bleh[p] + 1;
    }
 }
  for (z = 0; z < 5; z++)
    hand[z][i] = temphand[bleh[z]];
// if (!tplr->hand[0]) return i;
// if (!ttbl->cards[0]) return i;

 return i;
}


typedef int HAND_FUNC   (int hand[5]);
struct hand_function
{
     HAND_FUNC *func;
};

const struct hand_function funcs[] = {
  { has_rsf },
  { has_sf },
  { has_4kind},
  { has_fhouse},
  { has_flush },
  { has_straight},
  { has_3kind},
  { has_2pair},
  { has_pair},
  { highcard},
  { 0}
};

int handcompare(int hand1[5], int hand2[5])
{
   int v = 0;
   for (; funcs[v].func != 0;v++)
   {
      int a = (*(funcs[v].func))(hand1), b = (*(funcs[v].func))(hand2);
      if (a > b) return 1;
      if (b > a) return 2;
      if (a == b) return 3; //
   }   
    log("Error in handcompare.", 110, LOG_MORTAL);

  return -1;
}

int do_testhand(CHAR_DATA *ch, char *argument, int cmd)
{
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);
  //  int i = atoi(arg);
//  int z = get_hand(i);
  //char buf[MAX_STRING_LENGTH];
 // sprintf(buf, "One: %d Two: %d Three: %d Four: %d Five: %d\r\n",
//	hand[0][z],hand[1][z],
//	hand[2][z],hand[3][z],
//	hand[4][z]);
  //send_to_char(buf, ch);
  return eSUCCESS;
}

int find_highhand(struct tplayer *tplr)
{
  int handnr=0;
  for (int i = 1; i < 21; i++)
  {
    int v = handcompare(hand[get_hand(tplr, handnr)], hand[get_hand(tplr, i)]);
    if (v == 2) handnr = i;
  }
  return handnr;
}


struct tplayer *createTplayer(struct ttable *ttbl)
{
  int seat = has_seat(ttbl);
  if (seat < 0) return NULL; 
  struct tplayer *tplr;
  
#ifdef LEAK_CHECK
  tplr = (struct tplayer*) calloc(1, sizeof(struct tplayer));
#else
  tplr = (struct tplayer*) dc_alloc(1, sizeof(struct tplayer));
#endif
  ttbl->player[seat] = tplr;
  tplr->nw = TRUE;
  tplr->table=ttbl;
  for (int i = 0; i < 2; i++) tplr->hand[i] = 0;
  tplr->chips = 0;
  tplr->pos = -1;
  tplr->dealer = FALSE;
  tplr->options = 0;
  return tplr;
}

int first_to_act(int state, struct tplayer *player[6])
{
   int plrs = 0,  dlr = 0;
   for (int i = 0; i < 6; i++)
   {
     if (player[i]) plrs++;
     else continue;

     if (player[i]->dealer) dlr = i;
   }

  if (plrs == 2)
   return dlr; // dealer acts first when there's only 2 players   
  int j = 2;
  for (int i = dlr; ; i++)
  {
	if (i==6) i = 0;
        if (player[i]&& --j == 0) return i;
  }

  return dlr; // shouldn't happen  
}

int find_winner(struct ttable *ttbl)
{
  int i, win = -1, winhand = 0;
  for (i = 0; i < 6; i++)
  {
	if (!ttbl->player[i]) continue;
	int highhand = find_highhand(ttbl->player[i]);
	if (win == -1) {win = i; winhand = highhand;}
        else { 
		if (handcompare(hand[get_hand(ttbl->player[i], highhand)],
				hand[get_hand(ttbl->player[win], winhand)]) == 1)
			{ winhand = highhand; win = i; }
	}
  }
  return win;
}

void pulse_holdem(struct ttbl *tbl)
{
  
}

/* End Texas Hold'em */

/* Slot Machines! */

struct machine_data
{
   OBJ_DATA *obj;
   CHAR_DATA *prch;
   CHAR_DATA *ch;
   uint cost;
   uint lastwin;
   int bet;
   float jackpot;
   int linkedto;
   bool busy;
   bool gold;
   bool button;
};

char *reel1[] = {
"$5Orange$R",
"$B$2 Melon$R",
"$B$6 Plum $R",
"$B$4Cherry$R",
"$B$6 Plum $R",
"$5Orange$R",
"$B   7  $R", 
"$B$3Bl$R/$B$0BAR$R",
"$5Orange$R",
"$B$4Cherry$R",
"$B$0  BAR $R",
"$B$6 Plum $R",
"$5Orange$R",
"$B$6 Plum $R", 
"$B$2 Melon$R",
"$B$6 Plum $R",
"$5Orange$R",
"$B$6 Plum $R",
"$B$0  BAR $R",
"$B$6 Plum $R"
};

char *reel2[] = {
"$B$4Cherry$R",
"$B$6 Plum $R",
"$B$4Cherry$R",
"$B7$R/$5Orng$R",
"$B$4Cherry$R",
"$B$3 Bell $R", 
"$B$6Pl$R/$B$0BAR$R",
"$B$3 Bell $R",
"$B$4Cherry$R",
"$5Orange$R",
"$B$3 Bell $R",
"$B$2Mln$R/$5Or$R",
"$B$6 Plum $R",
"$B$3 Bell $R",
"$B$4Cherry$R",
"$B$0 BAR  $R",
"$5Orange$R",
"$B$4Cherry$R",
"$B$3 Bell $R",
"$B$2Mln$R/$5Or$R"
};

char *reel3[] = {
"$B$3 Bell $R", 
"$5Orange$R", 
"$B$6 Plum $R", 
"$B$3 Bell $R", 
"$5Orange$R", 
"$B$5Lemon $R", 
"$B$3 Bell $R",
"$B$2Mln$R/$5Or$R",
"$B$3 Bell $R", 
"$B$6 Plum $R", 
"$B$5Lemon $R", 
"$B$3 Bell $R", 
"$B$6 Plum $R", 
"$B$3 Bell $R", 
"$B 7$R/$B$0BAR$R", 
"$B$5Lemon $R", 
"$B$3 Bell $R", 
"$B$2Mln$R/$5Or$R", 
"$B$3 Bell $R", 
"$B$5Lemon $R"
};

void save_slot_machines()
{
  FILE *f = (FILE *)NULL;
  world_file_list_item * curr;
  char buf[180];
  char buf2[180];

  extern world_file_list_item * obj_file_list;

  curr = obj_file_list;
  while(curr && strcmp(curr->filename, "21900-21999.obj"))
    curr = curr->next;

  if(!curr) {
    log("Mess up in save_slot_machines, no object file.", IMMORTAL, LOG_BUG);
    return;
  }

  sprintf(buf, "objects/%s", curr->filename);
  sprintf(buf2, "cp %s %s.last", buf, buf);
  system(buf2);

  if((f = dc_fopen(buf, "w")) == NULL) {
    fprintf(stderr,"Couldn't open obj save file %s for save_slot_machines.\n\r",
            curr->filename);
    return;
  }

  for(int x = curr->firstnum; x <= curr->lastnum; x++) 
     write_object((obj_data *)obj_index[x].item, f);

  // end file
  fprintf(f, "$~\n");

  dc_fclose(f);
}

void create_slot(OBJ_DATA *obj)
{
   struct machine_data *slot;
 #ifdef LEAK_CHECK
   slot = (struct machine_data *)calloc(1, sizeof(struct machine_data));
 #else
   slot = (struct machine_data *)dc_alloc(1, sizeof(struct machine_data));
 #endif

   slot->obj = obj;
   slot->ch = NULL;
   slot->prch = NULL;
   slot->cost = obj->obj_flags.value[0];
   slot->jackpot = obj->obj_flags.value[1];
   slot->linkedto = obj->obj_flags.value[3];
   slot->lastwin = 0;
   slot->bet = 1;
   if(obj->obj_flags.value[2]) slot->gold = FALSE;
   else slot->gold = TRUE;
   slot->busy = FALSE;
   slot->button = FALSE;
   obj->slot = slot;
}

bool verify_slot(struct machine_data *machine)
{
   if(machine->ch->in_room == machine->obj->in_room)
      return TRUE;

   return FALSE;
}

void update_linked_slots(struct machine_data *machine)
{
   char ldesc[MAX_STRING_LENGTH];

   snprintf(ldesc, MAX_STRING_LENGTH,
	   "A slot machine which displays '$R$BJackpot: %d %s!$1' sits here.",
	   (int)machine->jackpot, 
	   machine->gold ? "coins": "plats");

   // Find all the slot machines
   for(int i=21906; i<21918; i++) {
     OBJ_DATA *slot_obj = (OBJ_DATA *)obj_index[real_object(i)].item;

     // Find all the slot machines linked to the same slot machine as us
     // and update their v1 jackpot, their machine's jackpot (if applicable)
     // and their long description
      if(slot_obj->obj_flags.value[3] == machine->linkedto) {
         //leaving the original desc from obj loading alone in the hash table
       //  if(!ishashed(slot_obj->description)) dc_free(slot_obj->description);
	 slot_obj->description = str_dup(ldesc);
         slot_obj->obj_flags.value[1] = (int)machine->jackpot;
	 if (slot_obj->slot)
	   slot_obj->slot->jackpot = machine->jackpot;

	 // Update instances of the original slot obj
         for(OBJ_DATA *j=object_list; j; j = j->next) {
	   if(j->item_number == real_object(i)) {
             //if(!ishashed(j->description)) dc_free(j->description);
	     j->description = str_dup(ldesc);
	     j->obj_flags.value[1] = (int)machine->jackpot;
	     if (j->slot)
	       j->slot->jackpot = machine->jackpot;
	   }
	 }
      }
   }
}


void slot_timer(struct machine_data *machine, int stop1, int stop2, int delay)
{
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif

   timer->arg1 = (void*)machine;
   timer->arg2 = (void*)stop1;
   timer->arg3 = (void*)stop2;
   timer->function = reel_spin;
   timer->timeleft = delay;
   addtimer(timer);
}

void reel_spin(void *arg1, void *arg2, void *arg3)
{
   struct machine_data *machine = (struct machine_data *) arg1;
   int stop1 = (int)arg2;
   int stop2 = (int)arg3;

   char buf[MAX_STRING_LENGTH];

   if(stop1 < 0 && charExists(machine->ch) && verify_slot(machine)) {
      stop1 = number(0, 19);
      send_to_room("You hear a loud clunk as the first stopper snaps into place.\n\r", machine->obj->in_room);
      sprintf(buf, "%s    |      |\n\r", reel1[stop1]);
      send_to_char(buf, machine->ch);
      slot_timer(machine, stop1, -1, 2);
   } else if(stop2 < 0 && charExists(machine->ch) && verify_slot(machine)) {
      stop2 = number(0, 19);
      send_to_room("You hear a loud clunk as the second stopper snaps into place.\n\r", machine->obj->in_room);
      sprintf(buf, "%s %s    |\n\r", reel1[stop1], reel2[stop2]);
      send_to_char(buf, machine->ch);
      slot_timer(machine, stop1, stop2, 2);
   } else if(charExists(machine->ch) && verify_slot(machine)) {
      int payout = 0;
      int stop3 = number(0, 19);
      send_to_room("You hear a loud clunk as the final stopper snaps into place.\n\r", machine->obj->in_room);
      sprintf(buf, "%s %s %s\n\r", reel1[stop1], reel2[stop2], reel3[stop3]);
      send_to_char(buf, machine->ch);

      if(stop1 == 6 && stop2 == 3 && stop3 == 14) payout = 200;
      else if((stop1 == 7 || stop1 == 10 || stop1 == 18) && (stop2 == 6 || stop2 == 15) && stop3 == 14) payout = 100;
      else if((stop1 == 1 || stop1 == 14) && (stop2 == 11 || stop2 == 19) && (stop3 == 7 || stop3 == 17 || stop3 == 14)) payout = 100;
      else if(stop1 == 7 && (stop2 == 5 || stop2 == 7 || stop2 == 10 || stop2 == 13 || stop2 == 18) && (stop3 == 0 || 
            stop3 == 3 || stop3 == 6 || stop3 == 8 || stop3 == 11 || stop3 == 13 || stop3 == 14 || stop3 == 16 || stop3 == 18)) payout = 18;
      else if((stop1 == 2 || stop1 == 4 || stop1 == 11 || stop1 == 13 || stop1 == 15 || stop1 == 17 || stop1 == 19) && 
            (stop2 == 1 || stop2 == 6 || stop2 == 12) && (stop3 == 2 || stop3 == 9 || stop3 == 12 || stop3 == 14)) payout = 14;
      else if((stop1 == 0 || stop1 == 5 || stop1 == 8 || stop1 == 12 || stop1 == 16) && (stop2 == 3 || stop2 == 9 || 
            stop2 == 11 || stop2 == 16 || stop2 == 19) && (stop3 == 1 || stop3 == 4 || stop3 == 7 || stop3 == 14 || stop3 == 17)) payout = 10;
      else if((stop1 == 3 || stop1 == 9) && (stop2 == 0 || stop2 == 2 || stop2 == 4 || stop2 == 8 || stop2 == 14 || stop2 == 17)) payout = 5;
      else if(stop1 == 3 || stop1 == 9) payout = 2;
      else {
         machine->jackpot += (float)machine->cost * (float)machine->bet * 0.04;
         machine->jackpot = MIN(machine->jackpot, 2000000000); // NEVER AGAIN!!! :P
         if(machine->linkedto) {
            update_linked_slots(machine);
         } else {
            ((OBJ_DATA *)obj_index[machine->obj->item_number].item)->obj_flags.value[1] = (int)machine->jackpot;
            sprintf(buf, "A slot machine which displays '$R$BJackpot: %d %s!$1' sits here.", (int)machine->jackpot, machine->gold?"coins":"plats");
            //if(!ishashed(machine->obj->description)) dc_free(machine->obj->description);
            machine->obj->description = str_dup(buf);
            if(!ishashed(((OBJ_DATA*)obj_index[machine->obj->item_number].item)->description))
               dc_free(((OBJ_DATA *)obj_index[machine->obj->item_number].item)->description);
            ((OBJ_DATA *)obj_index[machine->obj->item_number].item)->description = str_dup(buf);
         }
      }
      if(payout == 200 && machine->bet == 5) {
         send_to_room("The jackpot lights flash and loud noises come from all around you!\n\r", machine->obj->in_room);
         csendf(machine->ch, "$BJACKPOT!!!!!!  You win the jackpot of %d %s!!$R\n\r", (int)machine->jackpot, machine->gold?"coins":"plats");
	 sprintf(buf, "##%s just won the JACKPOT for %d %s!\r\n", GET_NAME(machine->ch), (int)machine->jackpot, machine->gold?"coins":"plats");
 	 extern void send_info(char *messg);
	 send_info(buf);

         logf( IMMORTAL, LOG_MORTAL, "Jackpot win! %s won the jackpot of %d %s!",
			GET_NAME(machine->ch), (int)machine->jackpot, machine->gold?"coins":"plats");
         if(machine->gold)
            GET_GOLD(machine->ch) += (int)machine->jackpot;
         else GET_PLATINUM(machine->ch) += (int)machine->jackpot;
         machine->jackpot = machine->cost * 1000;
         if(machine->linkedto) {
            update_linked_slots(machine);
         } else {
            ((OBJ_DATA *)obj_index[machine->obj->item_number].item)->obj_flags.value[1] = (int)machine->jackpot;
            sprintf(buf, "A slot machine which displays '$R$BJackpot: %d %s!$1' sits here.", (int)machine->jackpot, machine->gold?"coins":"plats");
            //if(!ishashed(machine->obj->description)) dc_free(machine->obj->description);
            machine->obj->description = str_dup(buf);
            //if(!ishashed(((OBJ_DATA *)obj_index[machine->obj->item_number].item)->description))
            //   dc_free(((OBJ_DATA*)obj_index[machine->obj->item_number].item)->description);
            ((OBJ_DATA *)obj_index[machine->obj->item_number].item)->description = str_dup(buf);
         }
         save_slot_machines();
      } else if(payout) {
         send_to_room("Lights flash and noises emanate from the slot machine!\n\r", machine->obj->in_room);
         machine->lastwin = machine->cost * payout * machine->bet;
         sprintf(buf, "$BWinner!!$R  You win %d %s!\n\r", machine->lastwin, machine->gold?"coins":"plats");
         if(machine->gold)
            GET_GOLD(machine->ch) += machine->lastwin;
         else GET_PLATINUM(machine->ch) += machine->lastwin;
         send_to_char(buf, machine->ch);
         send_to_char("A tiny panel flips open on the slot machine, revealing red and black buttons.\n\r", machine->ch);
         machine->button = TRUE;
         machine->prch = machine->ch;
      }
      machine->busy = FALSE;
   } else { //something bad happened
      machine->busy = FALSE;
   }
}

int slot_machine(CHAR_DATA *ch, OBJ_DATA *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
   char buf[MAX_STRING_LENGTH];

   if(cmd != 186 && cmd != 189 && cmd != 185) //pull or bet or push
      return eFAILURE;
   if(!ch || IS_NPC(ch)) return eFAILURE;

   if (IS_AFFECTED(ch, AFF_CANTQUIT) || affected_by_spell(ch, FUCK_PTHIEF) || affected_by_spell(ch, FUCK_GTHIEF))
   {
      send_to_char("You cannot play the slots while you are flagged as naughty.\r\n",ch);
      return eSUCCESS;
   }

   one_argument(arg, buf);

   if(cmd == 186 && strcmp(buf, "handle")) {
      send_to_char("Try pulling the handle.\n\r", ch);
      return eSUCCESS;
   }

   if(!obj->slot) create_slot(obj);

   if(obj->slot->busy) {
      send_to_char("This machine is already in use, try another one.\n\r", ch);
      return eSUCCESS;
   }

   if(cmd == 189) {
      if(atoi(buf) >= 1 && atoi(buf) <= 5) {
         obj->slot->bet = atoi(buf);
         obj->slot->prch = ch;
         if(obj->slot->button) {
            send_to_char("The panel closes quietly.\n\r", ch);
            obj->slot->button = FALSE;
         }
         if(obj->slot->bet == 1)
            send_to_char("You place only the minimum bet into the slot machine now.\n\r", ch);
         else
            csendf(ch, "You now start placing %d times the base amount into the slot machine.\n\r", obj->slot->bet);
         return eSUCCESS;
      }
      send_to_char("You can only multiply the bet by 2, 3, 4, or 5, or set it back to 1.\n\r", ch);
      return eSUCCESS;
   }

   if(cmd == 185) {
      if(obj->slot->button) {
         if(obj->slot->prch == ch) {
            if(is_abbrev(buf, "red") || is_abbrev(buf, "black")) {
	      if((obj->slot->gold && (GET_GOLD(ch) < obj->slot->lastwin)) || (!obj->slot->gold && (GET_PLATINUM(ch) < obj->slot->lastwin))) {
                  send_to_char("You don't have enough money to try to double your last win.\n\r", ch);
               } else if(number(0,1)) {
                  send_to_char("$BWinner!!$R The button lights up and the room is filled with whirring noises!\n\r", ch);
                  if(obj->slot->gold) GET_GOLD(ch) += obj->slot->lastwin;
                  else GET_PLATINUM(ch) += obj->slot->lastwin;
                  obj->slot->button = FALSE;
               } else {
                  send_to_char("Oh no!! The other button lights up! You lose everything!\n\r", ch);
                  if(obj->slot->gold) GET_GOLD(ch) -= obj->slot->lastwin;
                  else GET_PLATINUM(ch) -= obj->slot->lastwin;
                  obj->slot->button = FALSE;
               }
            } else send_to_char("You must push either the red or black button.\n\r", ch);
         } else send_to_char("Nothing seems to happen.\n\r", ch);            
      } else send_to_char("You can find nothing to push.\n\r", ch);
      return eSUCCESS;
   }

   if((obj->slot->gold && (GET_GOLD(ch) < (obj->slot->cost * obj->slot->bet))) || (!obj->slot->gold && (GET_PLATINUM(ch) < (obj->slot->cost * obj->slot->bet)))) {
      send_to_char("You don't have enough money to start the machine.\n\r", ch);
      return eSUCCESS;
   }

   if(obj->slot->prch != ch) obj->slot->bet = 1;
   if(obj->slot->button) send_to_char("The panel closes quietly.\n\r", ch);
   obj->slot->button = FALSE;

   if(obj->slot->gold)
      GET_GOLD(ch) -= obj->slot->cost * obj->slot->bet;
   else GET_PLATINUM(ch) -= obj->slot->cost * obj->slot->bet;
   obj->slot->busy = TRUE;
   sprintf(buf, "You place %d %s into the slot and set the reels spinning!\n\r", obj->slot->cost * obj->slot->bet, obj->slot->gold?"coins":"plats");
   send_to_char(buf, ch);
   act("$n reaches for the handle and pulls down.", ch, 0, 0, TO_ROOM, 0);
   send_to_char("   |      |      |\n\r", ch);
   obj->slot->ch = ch;

   slot_timer(obj->slot, -1, -1, 2);

   return eSUCCESS;
}

/* End Slot Machines */

/* Roulette! */

char *roulette_display[] = {
   "$2$B0$R", "$4$B1$R", "$0$B2$R", "$4$B3$R", "$0$B4$R", "$4$B5$R", "$0$B6$R", "$4$B7$R", "$0$B8$R",
   "$4$B9$R", "$0$B10$R", "$0$B11$R", "$4$B12$R", "$0$B13$R", "$4$B14$R", "$0$B15$R", "$4$B16$R",
   "$0$B17$R", "$4$B18$R", "$4$B19$R", "$0$B20$R", "$4$B21$R", "$0$B22$R", "$4$B23$R", "$0$B24$R",
   "$4$B25$R", "$0$B26$R", "$4$B27$R", "$0$B28$R", "$0$B29$R", "$4$B30$R", "$0$B31$R", "$4$B32$R",
   "$0$B33$R", "$4$B34$R", "$0$B35$R", "$4$B36$R"
};

struct roulette_player
{
   CHAR_DATA *ch;
   uint32 bet_array[48];
};

struct wheel_data
{
   OBJ_DATA *obj;
   struct roulette_player *plr[6];
   int countdown;
   bool spinning;
};

void create_wheel(OBJ_DATA *obj)
{
   struct wheel_data *wheel;
 #ifdef LEAK_CHECK
   wheel = (struct wheel_data *)calloc(1, sizeof(struct wheel_data));
 #else
   wheel = (struct wheel_data *)dc_alloc(1, sizeof(struct wheel_data));
 #endif
   wheel->obj = obj;
   for(int i=0;i<6;i++) {
 #ifdef LEAK_CHECK
   wheel->plr[i] = (struct roulette_player *)calloc(1, sizeof(struct roulette_player));
 #else
   wheel->plr[i] = (struct roulette_player *)dc_alloc(1, sizeof(struct roulette_player));
 #endif
      wheel->plr[i]->ch = NULL;
      for(int j=0;j<48;j++)
         wheel->plr[i]->bet_array[j] = 0;
   }
   wheel->countdown = 11;
   wheel->spinning = FALSE;
   obj->wheel = wheel;
}

void send_wheel_bets(CHAR_DATA *ch, struct wheel_data *wheel)
{
   int i,j;
   bool found = FALSE;
   char *bet_name[] = {
      "$B$0BLACK$R", "$4$BRED$R", "$BEVEN$R", "$BODD$R", "$B1-12$R", "$B13-24$R", "$B25-36$R", "$B1-9$R", "$B10-18$R", 
      "$B19-27$R", "$B28-36$R"
   };

   for(i=0;i<6;i++) {
      if(ch == wheel->plr[i]->ch) {
         for(j=0;j<11;j++) {
            if(wheel->plr[i]->bet_array[j]) {
               if(!found) {send_to_char("You have", ch); found = TRUE;}
               else send_to_char(" and", ch);
               csendf(ch, " a bet of %u on %s", wheel->plr[i]->bet_array[j], bet_name[j]);
            }
         }
         for(j=11;j<48;j++) {
            if(wheel->plr[i]->bet_array[j]) {
               if(!found) {send_to_char("You have", ch); found = TRUE;}
               else send_to_char(" and", ch);
               csendf(ch, " a bet of %u on %s", wheel->plr[i]->bet_array[j], roulette_display[j-11]);
            }
         }
      }
   }
   if(!found) send_to_char("You have not placed any bets", ch); //how the hell?
   send_to_char(".\n\r", ch);
}

uint32 check_roulette_wins(struct roulette_player *plr, int num)
{
   uint32 tmp;
   uint32 winnings = 0;

   if(plr->bet_array[0] && (num == 2 || num == 4 || num == 6 || num == 8 || num == 10 ||
         num == 11 || num == 13 || num == 15 || num == 17 || num == 20 || num == 22 ||
         num == 24 || num == 26 || num == 28 || num == 29 || num == 31 || num == 33 ||
         num == 35)) {
      tmp = 2 * plr->bet_array[0];
      csendf(plr->ch, "You WIN %u coins on your bet of $0$BBLACK$R!\n\r", tmp);
      winnings += tmp;
   }

   if(plr->bet_array[1] && !(num == 2 || num == 4 || num == 6 || num == 8 || num == 10 ||
         num == 11 || num == 13 || num == 15 || num == 17 || num == 20 || num == 22 ||
         num == 24 || num == 26 || num == 28 || num == 29 || num == 31 || num == 33 ||
         num == 35 || num == 0)) {
      tmp = 2 * plr->bet_array[1];
      csendf(plr->ch, "You WIN %u coins on your bet of $4$BRED$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[2] && num%2 == 0 && num) {
      tmp = 2 * plr->bet_array[2];
      csendf(plr->ch, "You WIN %u coins on your bet of $BEVEN$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[3] && num%2 == 1) {
      tmp = 2 * plr->bet_array[3];
      csendf(plr->ch, "You WIN %u coins on your bet of $BODD$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[4] && num > 0 && num < 13) {
      tmp = 3 * plr->bet_array[4];
      csendf(plr->ch, "You WIN %u coins on your bet of $B1-12$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[5] && num > 12 && num < 25) {
      tmp = 3 * plr->bet_array[5];
      csendf(plr->ch, "You WIN %u coins on your bet of $B13-24$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[6] && num > 24 && num < 37) {
      tmp = 3 * plr->bet_array[6];
      csendf(plr->ch, "You WIN %u coins on your bet of $B25-36$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[7] && num > 0 && num < 10) {
      tmp = 4 * plr->bet_array[7];
      csendf(plr->ch, "You WIN %u coins on your bet of $B1-9$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[8] && num > 9 && num < 19) {
      tmp = 4 * plr->bet_array[8];
      csendf(plr->ch, "You WIN %u coins on your bet of $B10-18$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[9] && num > 18 && num < 28) {
      tmp = 4 * plr->bet_array[9];
      csendf(plr->ch, "You WIN %u coins on your bet of $B19-27$R!\n\r", tmp);
      winnings += tmp;
   }
   if(plr->bet_array[10] && num > 27 && num < 37) {
      tmp = 4 * plr->bet_array[10];
      csendf(plr->ch, "You WIN %u coins on your bet of $B28-36$R!\n\r", tmp);
      winnings += tmp;
   }
   for(int i=11;i<48;i++) {
      if(plr->bet_array[i] && num == i-11) {
         tmp = 36 * plr->bet_array[i];
         csendf(plr->ch, "You WIN %u coins on your bet of %s!\n\r", tmp, roulette_display[num]);
         winnings += tmp;
      }
   }
   return winnings;
}

void send_roulette_message(struct wheel_data *wheel)
{
   char buf[MAX_STRING_LENGTH];

   char *introbuf[] = {
      "A silver blur rounds the outside of the wheel as it spins.\n\r",
      "The distinct sound of the metal ball rounding the wheel is heard.\n\r",
   };
   char *middlebuf[] = {
      "The ball bounces off of the",
      "The ball caroms off of the metal part of the",
      "The ball makes a clacking noise as it hits the",
      "Clacking noises fill the air as the ball hits the",
   };
   char *endbuf[] = {
      "and immediately bounces out.\n\r",
      "and bounces almost straight up in the air.\n\r",
      "then short-hops a couple of spaces.\n\r",
      "and gets knocked slightly backwards.\n\r",
   };

   if(wheel->countdown >= 2)
      send_to_room(introbuf[number(0,1)], wheel->obj->in_room);
   else {
      sprintf(buf, "%s %s %s", middlebuf[number(0,3)], roulette_display[number(0,36)], endbuf[number(0,3)]);
      send_to_room(buf, wheel->obj->in_room);
   }
}

void wheel_stop(struct wheel_data *wheel)
{
   int num = number(0,36);
   uint32 payout = 0;
   char buf[MAX_STRING_LENGTH];

   sprintf(buf, "The ball lands on %s!\n\r", roulette_display[num]);
   send_to_room(buf, wheel->obj->in_room);

   for(int i=0;i<6;i++) {
      if(charExists(wheel->plr[i]->ch)) {
         if(wheel->plr[i]->ch->in_room == wheel->obj->in_room) {
            if((payout = check_roulette_wins(wheel->plr[i], num))) {
               if(payout > 1000000)
                  act("$n is a BIG winner!", wheel->plr[i]->ch, 0, 0, TO_ROOM, 0);
               else
                  act("$n is a winner!", wheel->plr[i]->ch, 0, 0, TO_ROOM, 0);
               GET_GOLD(wheel->plr[i]->ch) += payout;
            } else {
               send_to_char("The croupier gathers your money.\n\r", wheel->plr[i]->ch);
            }
         }
      }
      for(int j=0;j<48;j++)
         wheel->plr[i]->bet_array[j] = 0;
      wheel->plr[i]->ch = NULL;
   }
   wheel->spinning = FALSE;
   wheel->countdown = 11;
}

void pulse_countdown(void *arg1, void *arg2, void *arg3);

void roulette_timer(struct wheel_data *wheel, int spin)
{
  struct timer_data *timer;
 #ifdef LEAK_CHECK
  timer = (struct timer_data *)calloc(1, sizeof(struct timer_data));
 #else
  timer = (struct timer_data *)dc_alloc(1, sizeof(struct timer_data));
 #endif

   timer->arg1 = (void*)wheel;
   timer->arg2 = (void*)spin;
   timer->function = pulse_countdown;
   timer->timeleft = 4;
   addtimer(timer);
}

void pulse_countdown(void *arg1, void *arg2, void *arg3)
{
   struct wheel_data *wheel = (struct wheel_data *) arg1;
   int spin = (int) arg2;
   char buf[MAX_STRING_LENGTH];

   if(wheel->countdown <= 0 && !spin) {
      wheel->spinning = TRUE;
      send_to_room("The croupier places the ball on the wheel and spins both objects....\n\r", wheel->obj->in_room);
      wheel->countdown = 2;
      roulette_timer(wheel, 1);
   } else if(!spin) {
      if(!number(0,3)) {
         sprintf(buf, "$B$7The croupier says 'The wheel will be spun in about %d seconds!'$R\n\r", wheel->countdown * 2);
         send_to_room(buf, wheel->obj->in_room);
      }
      wheel->countdown -= 1;
      roulette_timer(wheel, 0);
   } else if(wheel->countdown < 0) {
      wheel_stop(wheel);
   } else {
      send_roulette_message(wheel);
      wheel->countdown -= 1;
      roulette_timer(wheel, 1);
   }
}

int roulette_table(CHAR_DATA *ch, struct obj_data *obj, int cmd, char *arg, CHAR_DATA *invoker)
{
   char arg1[MAX_INPUT_LENGTH], arg2[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
   uint32 bet=0;
   int i=0;
   bool playing = FALSE;
   arg = one_argument(arg, arg1);
   arg = one_argument(arg, arg2);
   if (cmd != 189) return eFAILURE;
   if (!ch || IS_NPC(ch)) return eFAILURE; // craziness
   if (obj->in_room <= 0) return eFAILURE;
   if (!obj->wheel)
	create_wheel(obj);
   if (IS_AFFECTED(ch, AFF_CANTQUIT) ||
        affected_by_spell(ch, FUCK_PTHIEF) ||
        affected_by_spell(ch, FUCK_GTHIEF))
   {
        send_to_char("You cannot play roulette while you are flagged as naughty.\r\n",ch);
	return eSUCCESS;
   }

   if(obj->wheel->spinning) {
      send_to_char("No bets may be placed while the wheel is spinning.\n\r", ch);
      return eSUCCESS;
   }

   for(i=0;i<6;i++) {
      if(obj->wheel->plr[i]->ch == ch) {
         playing = TRUE;
         break;
      }
   }

   if(!playing) {
      for(i=0;i<7;i++) {
         if(i==6) {
            send_to_char("You cannot muscle your way to the table.\n\r", ch);
            return eSUCCESS;
         }
         if(obj->wheel->plr[i]->ch && charExists(obj->wheel->plr[i]->ch) && obj->wheel->plr[i]->ch->in_room != obj->in_room)
            obj->wheel->plr[i]->ch = NULL;
         else if(obj->wheel->plr[i]->ch && !charExists(obj->wheel->plr[i]->ch))
            obj->wheel->plr[i]->ch = NULL;
         if(!obj->wheel->plr[i]->ch) {
            obj->wheel->plr[i]->ch = ch;
            break;
         }
      }
   }   

   if(cmd == 189) // bet
   {
      if(!*arg2) {
         send_to_char("Syntax: Bet <keyword>/<range (1-12)>/<number>  <amount>\n\r", ch);
         return eSUCCESS;
      }
      if(!is_number(arg2) || atoi(arg2) < 100) {
         send_to_char("You must bet an amount greater than 100 coins.\n\r", ch);
         return eSUCCESS;
      } else bet = atoi(arg2);
      if(bet > 20000000) {
         send_to_char("The maximum bet is 20 million coins.\n\r", ch);
         bet = 20000000;
      }
      if(GET_GOLD(ch) < bet) {
         send_to_char("You do not have enough money to place that bet.\n\r", ch);
         return eSUCCESS;
      } else GET_GOLD(ch) -= bet;
      if(!str_cmp(arg1, "black")) {
         if(obj->wheel->plr[i]->bet_array[0]) {
            if(obj->wheel->plr[i]->bet_array[0] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B$0BLACK$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[0] + bet);
               sprintf(buf, "$n adds to $s bet on $B$0BLACK$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[0] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B$0BLACK$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B$0BLACK$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[0] += bet;
      } else if(!str_cmp(arg1, "red")) {
         if(obj->wheel->plr[i]->bet_array[1]) {
            if(obj->wheel->plr[i]->bet_array[1] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B$4RED$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[1] + bet);
               sprintf(buf, "$n adds to $s bet on $B$4RED$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[1] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B$4RED$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B$4RED$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[1] += bet;
      } else if(!str_cmp(arg1, "even")) {
         if(obj->wheel->plr[i]->bet_array[2]) {
            if(obj->wheel->plr[i]->bet_array[2] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $BEVEN$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[2] + bet);
               sprintf(buf, "$n adds to $s bet on $BEVEN$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[2] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $BEVEN$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $BEVEN$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[2] += bet;
      } else if(!str_cmp(arg1, "odd")) {
         if(obj->wheel->plr[i]->bet_array[3]) {
            if(obj->wheel->plr[i]->bet_array[3] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $BODD$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[3] + bet);
               sprintf(buf, "$n adds to $s bet on $BODD$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[3] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $BODD$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $BODD$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[3] += bet;
      } else if(!str_cmp(arg1, "1-12")) {
         if(obj->wheel->plr[i]->bet_array[4]) {
            if(obj->wheel->plr[i]->bet_array[4] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B1-12$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[4] + bet);
               sprintf(buf, "$n adds to $s bet on $B1-12$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[4] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B1-12$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B1-12$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[4] += bet;
      } else if(!str_cmp(arg1, "13-24")) {
         if(obj->wheel->plr[i]->bet_array[5]) {
            if(obj->wheel->plr[i]->bet_array[5] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B13-24$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[5] + bet);
               sprintf(buf, "$n adds to $s bet on $B13-24$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[5] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B13-24$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B13-24$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[5] += bet;
      } else if(!str_cmp(arg1, "25-36")) {
         if(obj->wheel->plr[i]->bet_array[6]) {
            if(obj->wheel->plr[i]->bet_array[6] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B25-36$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[6] + bet);
               sprintf(buf, "$n adds to $s bet on $B25-36$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[6] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B25-36$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B25-36$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[6] += bet;
      } else if(!str_cmp(arg1, "1-9")) {
         if(obj->wheel->plr[i]->bet_array[7]) {
            if(obj->wheel->plr[i]->bet_array[7] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B1-9$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[1] + bet);
               sprintf(buf, "$n adds to $s bet on $B1-9$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[7] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B1-9$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B1-9$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[7] += bet;
      } else if(!str_cmp(arg1, "10-18")) {
         if(obj->wheel->plr[i]->bet_array[8]) {
            if(obj->wheel->plr[i]->bet_array[8] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B10-18$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[8] + bet);
               sprintf(buf, "$n adds to $s bet on $B10-18$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[7] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B10-18$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B10-18$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[8] += bet;
      } else if(!str_cmp(arg1, "19-27")) {
         if(obj->wheel->plr[i]->bet_array[9]) {
            if(obj->wheel->plr[i]->bet_array[9] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B19-27$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[9] + bet);
               sprintf(buf, "$n adds to $s bet on $B19-27$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[9] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B19-27$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B19-27$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[9] += bet;
      } else if(!str_cmp(arg1, "28-36")) {
         if(obj->wheel->plr[i]->bet_array[10]) {
            if(obj->wheel->plr[i]->bet_array[10] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on $B28-36$R.  The total bet is now %u coins.\n\r", bet, obj->wheel->plr[i]->bet_array[10] + bet);
               sprintf(buf, "$n adds to $s bet on $B28-36$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[10] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on $B28-36$R.\n\r", bet);
            sprintf(buf, "$n places a bet of %u on $B28-36$R.", bet);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[10] += bet;
      } else if(is_number(arg1) && atoi(arg1) >= 0 && atoi(arg1) <= 36) {
         int number = atoi(arg1);
         if(obj->wheel->plr[i]->bet_array[number+11]) {
            if(obj->wheel->plr[i]->bet_array[number+11] + bet > 20000000) {
               GET_GOLD(ch) += bet;
               bet = 0;
               send_to_char("That bet would put you over the 20 million coin limit.\n\r", ch);
            } else {
               csendf(ch, "You add %u to your bet on %s.  The total bet is now %u coins.\n\r", bet, 
                  roulette_display[number], obj->wheel->plr[i]->bet_array[number+11] + bet);
               sprintf(buf, "$n adds to $s bet on %s for a total of %u coins.", 
                  roulette_display[number], obj->wheel->plr[i]->bet_array[number+11] + bet);
               act(buf, ch, 0, 0, TO_ROOM, 0);
            }
         }
         else {
            csendf(ch, "You have placed a bet of %u on %s.\n\r", bet, roulette_display[number]);
            sprintf(buf, "$n places a bet of %u on %s.", bet, roulette_display[number]);
            act(buf, ch, 0, 0, TO_ROOM, 0);
         }
         obj->wheel->plr[i]->bet_array[number+11] += bet;
      } else {
         send_to_char("Bet what?\n\r", ch);
	 GET_GOLD(ch) += bet;
         return eSUCCESS;
      }
      send_wheel_bets(ch, obj->wheel);
      if(obj->wheel->countdown == 11) {
         send_to_room("$BThe croupier says 'The first bet has been placed!'$R\n\r", obj->in_room);
         obj->wheel->countdown = 10;
         roulette_timer(obj->wheel, 0);
      }
   }

   return eSUCCESS;
}

/* End Roulette */
