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
#include <db.h> // exp_table
#include <interp.h>
#include <connect.h>
#include <spells.h>
#include <race.h>
#include <act.h>
#include <set.h>
#include <returnvals.h>

// hit on 16, stand on 17
// push = "tie"
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

struct table_data 
{
 int plrs;
};

struct cDeck
{
  struct table_data *tbl;
  int *cards;
  int pos;
  int decks;
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
	if (++z == 14) z = 1;
	Deck->cards[i] = z;
    }
    return Deck;
}

void freeDeck(struct cDeck *deck)
{
  delete[] deck->cards;
  dc_free(deck);  
}

void shuffle_deck(struct cDeck *tDeck)
{ // this would not hold up to a test of true randomization, but then
  // neither would a real dealer shuffling a deck
   

  // TODO: put cards on table in the beginning of deck, and move up pos
  int i;
  for (i = 0; i < tDeck->decks*52;i++)
  {
     int a = number(0, tDeck->decks*52-1);
     int b = tDeck->cards[a];
     tDeck->cards[a] = tDeck->cards[i];
     tDeck->cards[i] = b;
  }
 // shuffled
}

int pickCard(struct cDeck *deck)
{
  return deck->cards[deck->pos++];
}
