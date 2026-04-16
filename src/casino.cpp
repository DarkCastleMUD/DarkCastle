
/*

 Real bloody blackjack. Who does two cards then nothing else? Wankers, that's who


*/
#include "DC/DC.h"

/*
   BLACKJACK!
*/
cDeck *create_deck(qint32 decks)
{
  cDeck *Deck = new cDeck;
  Deck->cards = new qint32[decks * 52 + 1];
  Deck->decks = decks;
  Deck->pos = {};
  qint32 i, z;
  for (i = 0, z = {}; i < decks * 52; i++)
  {
    if (++z == 53)
      z = 1;
    Deck->cards[i] = z;
  }
  return Deck;
}

void freeDeck(cDeck *deck)
{
  deck = {};
}

void switch_cards(cDeck *tDeck, qint32 pos1, qint32 pos2)
{
  qint32 b = tDeck->cards[pos1];
  tDeck->cards[pos1] = tDeck->cards[pos2];
  tDeck->cards[pos2] = b;
}

void free_player(CasinoPlayerPtr plr)
{
  CasinoPlayerPtr tmp, prev = {};
  CasinoTablePtr tbl = plr->table;
  for (tmp = tbl->plr; tmp; tmp = tmp->next)
  {
    if (tmp == plr)
    {
      if (prev)
        prev->next = plr->next;
      else
        tbl->plr = plr->next;
    }
    prev = tmp;
  }
  if (plr->ch && charExists(plr->ch) && plr->ch->isPlayer())
  {
    plr->ch->save(cmd_t::SAVE_SILENTLY);
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
  if (!tbl->plr)
    reset_table(tbl);
  plr = {};
}

void nextturn(CasinoTablePtr tbl)
{
  if (!tbl->plr)
  {
    reset_table(tbl);
    return;
  }

  if (tbl->cr->next)
  {
    tbl->cr = tbl->cr->next;
    pulse_table_bj(tbl);
  }
  else
  {
    tbl->cr = {};
    add_timer_bj_dealer(tbl);
  }
}

void DC::send_to_table(QString msg, CasinoTablePtr tbl, CasinoPlayerPtr plrSilent = {})
{
  //  CasinoPlayerPtr plr;
  /*  for (plr = tbl->plr ; plr ; plr = plr->next)
     if (verify(plr) && plrSilent != plr)
       plr->ch->send(msg);
    */
  if (tbl && tbl->obj && tbl->obj->in_room)
  {
    send_to_room(msg, tbl->obj->in_room, true, plrSilent ? plrSilent->ch : 0);
  }
}

bool charExists(CharacterPtr ch)
{
  const auto &character_list = dc_->character_list;

  if (character_list.find(ch) != character_list.end())
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool DC::verify(CasinoPlayerPtr plr)
{
  // make sure player didn't quit, die, or whatever
  // CharacterPtr ch;

  auto result = std::find_if(character_list.begin(), character_list.end(), [&plr](CharacterPtr const &ch)
                             {
	  if (ch == plr->ch) {
		  return true;
	  } else {
	  	  return false;
	  } });

  if (result == character_list.end() || (*result)->in_room != plr->table->obj->in_room)
  {
    if (result != character_list.end())
    {
      QString buf;
      buf = u"%1 folds as %2 leaves the room.\r\n"_s.arg(plr->ch->name()).arg(HSSH(plr->ch));
      send_to_table(buf, plr->table);
      plr->ch->sendln("Your hand is folded as you leave the room.");
    }
    free_player(plr);
    return false;
  } // dead or quit
  return true;
}

void shuffle_deck(cDeck *tDeck)
{ // this would not hold up to a test of true randomization, but then
  // neither would a real dealer shuffling a deck
  CasinoPlayerPtr plr;
  qint32 pos = 0, i, v;
  if (tDeck->pos) // new deck otherwise
    for (plr = tDeck->table->plr; plr; plr = plr->next)
      for (v = {}; plr->hand_data[v]; v++)
      {
        switch_cards(tDeck, pos, --tDeck->pos);
        pos++;
      }

  for (i = pos; i < tDeck->decks * 52; i++)
    switch_cards(tDeck, i, dc_->number(pos, tDeck->decks * 52 - 1));
  tDeck->pos = pos + dc_->number(tDeck->decks * 52 / 10, tDeck->decks * 52 / 4);
  if (tDeck->table)
    send_to_table("The dealer shuffles the deck.\r\n", tDeck->table);
  // shuffled
}

QChar suit(qint32 card)
{
  if (card < 14)
    return 'h';
  else if (card < 27)
    return 'd';
  else if (card < 40)
    return 'c';
  else
    return 's';
}

QString suitcol(qint32 card)
{
  if (card < 14)
    return u"$B$4"_s;
  else if (card < 27)
    return u"$B$4"_s;
  else if (card < 40)
    return u"$B$0"_s;
  return u"$B$0"_s;
}

qint32 val(qint32 card)
{
  while (card > 13)
    card -= 13;
  return card;
}

const QString valstri(qint32 card)
{
  while (card > 13)
    card -= 13;

  switch (card)
  {
  case 1:
    return "A";
  case 2:
    return "2";
  case 3:
    return "3";
  case 4:
    return "4";
  case 5:
    return "5";
  case 6:
    return "6";
  case 7:
    return "7";
  case 8:
    return "8";
  case 9:
    return "9";
  case 10:
    return "T";
  case 11:
    return "J";
  case 12:
    return "Q";
  case 13:
    return "K";
  default:
    return "?";
  }
}

bool canInsurance(CasinoPlayerPtr plr)
{
  if (val(plr->table->hand_data[0]) == 1 &&
      plr->table->hand_data[2] == 0 &&
      plr->hand_data[2] == 0 &&
      plr->table->state == 1 &&
      !plr->insurance)
    return true;
  return false;
}

bool canSplit(CasinoPlayerPtr plr)
{
  if (plr->hand_data[0] && val(plr->hand_data[0]) == val(plr->hand_data[1]) && !plr->hand_data[2] && plr->table->cr == plr)
    return true;

  return false;
}

CasinoPlayerPtr createPlayer(CharacterPtr ch, CasinoTablePtr tbl, qint32 noadd = 0)
{
  CasinoPlayerPtr plr = new CasinoPlayer;
  plr->table = tbl;
  plr->ch = ch;
  for (qint32 i = {}; i < 21; i++)
  {
    plr->hand_data[i] = {};
  }

  plr->bet = {};
  plr->insurance = plr->doubled = false;
  plr->state = {};
  if (!noadd)
  {
    CasinoPlayerPtr tmp;
    for (tmp = tbl->plr; tmp; tmp = tmp->next)
      if (tmp->next == nullptr)
        break;

    if (tmp)
      tmp->next = plr;
    else
      tbl->plr = plr;
    plr->next = {};
  }
  return plr;
}

qint32 pickCard(cDeck *deck)
{
  if (deck->pos >= deck->decks * 52 - 1)
    shuffle_deck(deck);

  return deck->cards[deck->pos++];
}

void freeHand(CasinoPlayerPtr plr)
{
  qint32 i;
  for (i = {}; i < 21; i++)
    plr->hand_data[i] = {};
}

qint32 hand_strength(CasinoPlayerPtr plr)
{
  qint32 i, z = {};
  for (i = {}; plr->hand_data[i] != 0; i++)
    switch (val(plr->hand_data[i]))
    {
    case 1:
      break; // calculate aces last.
    case 11:
    case 12:
    case 13:
      z += 10;
      break;
    default:
      z += val(plr->hand_data[i]);
      break;
    }
  qint32 v = {};
  for (i = {}; plr->hand_data[i] != 0; i++)
    if (val(plr->hand_data[i]) == 1)
    {
      v++;
    }
  while (v > 1)
  {
    v--;
    z++;
  }
  if (v == 1)
  {
    if (z + 11 > 21)
      z++;
    else
      z += 11;
  }
  return z;
}

qint32 hand_strength(CasinoTablePtr tbl)
{
  qint32 i, z = {};
  for (i = {}; tbl->hand_data[i] != 0; i++)
    switch (val(tbl->hand_data[i]))
    {
    case 1:
      break; // calculate aces last.
    case 11:
    case 12:
    case 13:
      z += 10;
      break;
    default:
      z += val(tbl->hand_data[i]);
      break;
    }
  for (i = {}; tbl->hand_data[i] != 0; i++)
    if (val(tbl->hand_data[i]) == 1)
    {
      if (z + 11 > 21)
        z += 1;
      else
        z += 11;
    }
  return z;
}

void dealcard(CasinoPlayerPtr plr)
{
  // functions calling this should verify that the player can be dealt a card
  qint32 i = {};
  for (; plr->hand_data[i]; i++)
    ;

  plr->hand_data[i] = pickCard(plr->table->deck);
}

void DC::check_active(varg_t arg1, void *arg2, void *arg3)
{
  CasinoPlayerPtr plr = arg1.player;
  CasinoTablePtr tbl = (CasinoTablePtr)arg3;
  CasinoPlayerPtr ptmp = {};
  for (ptmp = tbl->plr; ptmp; ptmp = ptmp->next)
    if (ptmp == plr)
      break;
  if (!ptmp)
    return; // handled elsewhere
  if (!verify(plr))
    return;

  if ((qint64)arg2 == plr->table->handnr || (qint64)arg2 == (plr->table->handnr + 100) * 2)
  {
    TimerPtr timer = TimerPtr(new Timer);
    timer->arg1.player = plr;
    timer->arg2 = (void *)(((qint64)arg2 + 100) * 2);
    timer->arg3 = (void *)plr->table;
    timer->function = check_active;
    timer->timeleft = 10;
    addtimer(timer);
    QString buf;
    dc_sprintf(buf, "The dealer nudges %s.\r\n", qPrintable(plr->ch->name()));
    send_to_table(buf, plr->table, plr);
    plr->ch->sendln("The dealer nudges you.");
  }
  if ((quint64)arg2 == (((plr->table->handnr + 100) * 2 + 100) * 2))
  { // inactive
    CasinoTablePtr tbl = plr->table;
    CharacterPtr ch = plr->ch;

    QString buf;
    dc_sprintf(buf, "Security removes a sleepy %s from the table.\r\n", qPrintable(plr->ch->name()));
    send_to_table(buf, tbl, plr);
    CasinoPlayerPtr tmp, tnext;
    for (tmp = tbl->plr; tmp; tmp = tnext)
    {
      tnext = tmp->next;
      if (tmp->ch == ch && tbl->cr != tmp)
        free_player(tmp); // free non-active hands WEAR_WIELD
    }
    if (tbl->cr && tbl->cr->ch == ch)
      free_player(tbl->cr);
    ch->sendln("Security helps you up from the table where you've apparently fallen asleep!");
  }
}

void addtimer(TimerPtr add)
{
  TimerPtr timer;
  for (timer = timer_list; timer; timer = timer->next)
  {
    if (timer->next == nullptr)
    {
      timer->next = add;
      return;
    }
  }
  timer_list = add;
}

void add_timer(CasinoPlayerPtr plr)
{
  TimerPtr timer = TimerPtr(new Timer);
  timer->arg1.player = plr;
  timer->arg2 = (void *)(qint64)plr->table->handnr;
  timer->arg3 = (void *)plr->table;
  timer->function = check_active;
  //  timer->next = timer_list;
  // timer_list = timer;
  timer->timeleft = 10;
  addtimer(timer);
}

void bj_dealer_aiz(varg_t arg1, void *arg2, void *arg3)
{ // hack so I don't have to bother
  CasinoTablePtr tbl = arg1.table;
  tbl->state = 2;
  bj_dealer_ai(arg1, arg2, arg3);
}

void add_timer_bj_dealer(CasinoTablePtr tbl)
{
  TimerPtr timer = TimerPtr(new Timer);
  timer->arg1.table = tbl;
  timer->arg2 = {};
  if (tbl->state != 3)
    timer->function = bj_dealer_aiz;
  else
    timer->function = bj_dealer_ai;
  timer->timeleft = 2;
  addtimer(timer);
}

void add_timer_bj_dealer2(CasinoTablePtr tbl, qint32 time = 10)
{
  TimerPtr timer = TimerPtr(new Timer);
  timer->arg1.table = tbl;
  timer->arg2 = (void *)(qint64)(++tbl->handnr);
  if (tbl->handnr == 0) // not plausible, but possible
    timer->arg2 = (void *)(qint64)(++tbl->handnr);
  timer->function = bj_dealer_ai;
  timer->timeleft = time;
  addtimer(timer);
}
void bj_finish(varg_t arg1, void *arg2, void *arg3)
{
  CasinoTablePtr tbl = arg1.table;
  send_to_room("$B$7The dealer says 'Place your bets!'$R\r\n", tbl->obj->in_room, true);
  tbl->state = {};
}

void add_new_bets(CasinoTablePtr tbl)
{
  TimerPtr timer = TimerPtr(new Timer);
  timer->arg1.table = tbl;
  timer->function = bj_finish;
  timer->timeleft = 2;
  addtimer(timer);
}

void reset_table(CasinoTablePtr tbl)
{ // called both on error and regular reset
  while (tbl->plr)
    free_player(tbl->plr);
  tbl->cr = {};
  tbl->state = {};
  for (qint32 i = {}; i < 21; i++)
    tbl->hand_data[i] = {};
}

void DC::check_winner(CasinoTablePtr tbl)
{
  qint32 dealer = hand_strength(tbl);
  CasinoPlayerPtr plr, next;
  if (tbl->hand_data[2] == 0)
  {
    QString buf;
    dc_sprintf(buf, "The dealer has %d!\r\n", hand_strength(tbl));
    send_to_table(buf, tbl);
  }
  for (plr = tbl->plr; plr; plr = next)
  {
    next = plr->next;
    if (!verify(plr))
      continue;
    if (plr->insurance && hand_strength(tbl) == 21 &&
        tbl->hand_data[2] == 0)
    { // insurance bet paid off
      qint32 pay = plr->doubled ? plr->bet / 2 : plr->bet;
      plr->ch->send(u"Your insurance bet paid %d %s.\r\n"_s.arg(pay).arg(plr->table->gold ? "gold" : "platinum"));
      if (plr->table->gold)
        plr->ch->addGold(pay);
      else
        GET_PLATINUM(plr->ch) += pay;
    }
    if (hand_strength(plr) > 21)
      continue;
    if (dealer == hand_strength(plr))
    {
      QString buf;
      dc_sprintf(buf, "It's a PUSH!\r\nThe dealer takes your cards and gives you %d %s coins.\r\n",
                 plr->bet, plr->table->gold ? "gold" : "platinum");
      plr->ch->send(buf);
      dc_sprintf(buf, "The dealer gives %s %d coins.\r\n", qPrintable(plr->ch->name()),
                 plr->bet); //, plr->table->gold?"gold":"platinum");
                            //		plr->bet);
      send_to_table(buf, tbl, plr);
      if (plr->table->gold)
        plr->ch->addGold(plr->bet);
      else
        GET_PLATINUM(plr->ch) += plr->bet;
      free_player(plr);
      continue;
    }

    if (dealer > 21 || (hand_strength(plr) > dealer && hand_strength(plr) <= 21))
    {
      QString buf;
      plr->ch->sendln("$BYou WIN!$R");
      dc_sprintf(buf, "The dealer takes your cards and gives you %d %s coins.\r\n",
                 plr->bet * 2, plr->table->gold ? "gold" : "platinum");
      plr->ch->send(buf);
      dc_sprintf(buf, "The dealer gives %s %d %s coins.\r\n", qPrintable(plr->ch->name()),
                 plr->bet * 2, plr->table->gold ? "gold" : "platinum");
      send_to_table(buf, tbl, plr);
      if (plr->table->gold)
        plr->ch->addGold(plr->bet * 2);
      else
        GET_PLATINUM(plr->ch) += plr->bet * 2;
      free_player(plr);
    }
    else
    {
      if (verify(plr))
      {
        plr->ch->sendln("$BYou LOSE your bet!$R");
        free_player(plr);
      }
    }
  }
  reset_table(tbl);
  add_new_bets(tbl);
}

void bj_dealer_ai(varg_t arg1, void *arg2, void *arg3)
{
  CasinoTablePtr tbl = arg1.table;
  qint32 a = (qint64)arg2;
  QString buf;
  if (a && tbl->handnr != a)
    return; // handled earlier
  bool cont = false;

  switch (tbl->state)
  {
  case 2:
    send_to_table("It is now the dealer's turn.\r\n", tbl);
    tbl->state++;
    dc_sprintf(buf, "The dealer flips over his card revealing a %s%s%c%s.\r\n", suitcol(tbl->hand_data[1]), valstri(tbl->hand_data[1]), suit(tbl->hand_data[1]), NTEXT);
    send_to_table(buf, tbl);
    CasinoPlayerPtr plr, pnext;
    for (plr = tbl->plr; plr; plr = pnext)
    { // check if all players have busted
      pnext = plr->next;
      if (hand_strength(plr) < 22)
        cont = true;
    }
    if (!cont)
    {
      check_winner(tbl);
      return;
    }
    //        add_timer_bj_dealer(tbl);
    // no break;
  case 3:
    //	dc_sprintf(buf, "The dealer has %d.\r\n",hand_strength(tbl));
    //	send_to_table(buf, tbl);
    tbl->handnr++;
    if (hand_strength(tbl) < 17)
    { // Hit!
      qint32 nc = pickCard(tbl->deck), i;
      for (i = {}; i < 21; i++)
        if (tbl->hand_data[i] == 0)
          break;
      tbl->hand_data[i] = nc;

      dc_sprintf(buf, "The dealer takes a new card, revealing a %s%s%c%s! The dealer has %d!\r\n", suitcol(nc), valstri(nc),
                 suit(nc), NTEXT, hand_strength(tbl));
      send_to_table(buf, tbl);
      add_timer_bj_dealer(tbl);
    }
    else
    {
      check_winner(tbl);
    }
    break;
  case 0:
  case 1:
    // TODO CHECK THIS
    //	if (!tbl->plr) { add_timer_bj_dealer2(tbl); return; }
    send_to_room("$B$7The dealer says 'No more bets!'$R\r\n\r\n", tbl->obj->in_room, true);
    tbl->state = {};
    pulse_table_bj(tbl);
    break;
  default:
    reset_table(tbl);
    return;
  };
}

void DC::check_blackjacks(CasinoTablePtr tbl)
{
  QString buf;
  CasinoPlayerPtr plr, next;
  if (hand_strength(tbl) == 21)
  {
    send_to_table("The dealer blackjacked!\r\n", tbl);
    for (plr = tbl->plr; plr; plr = next)
    {
      next = plr->next;
      if (!verify(plr))
        continue;

      plr->ch->sendBlackjackPrompt();
    }
    check_winner(tbl);
    return;
  }
  for (plr = tbl->plr; plr; plr = next)
  {
    next = plr->next;
    if (!verify(plr))
      continue;
    if (hand_strength(plr) == 21 &&
        hand_strength(tbl) != 21)
    {
      buf = u"%1 blackjacks!\r\n"_s.arg(qPrintable(plr->ch->name()));
      send_to_table(buf, tbl, plr);
      plr->ch->sendln("$BYou BLACKJACK!$R");
      buf = u"The dealer gives you %1 %2 coins.\r\n"_s.arg((qint32)(plr->bet * 2.5)).arg(plr->table->gold ? "gold" : "platinum");
      plr->ch->send(buf);
      plr->ch->sendBlackjackPrompt();

      if (plr->table->gold)
        plr->ch->addGold((quint32)(plr->bet * 2.5));
      else
        GET_PLATINUM(plr->ch) += (quint32)(plr->bet * 2.5);
      //        	nextturn(plr->table);
      if (tbl->plr == plr && !plr->next)
      { // all players blackjacked
        QString buf;
        dc_sprintf(buf, "The dealer flips over his card revealing a %s%s%c%s.\r\n",
                   suitcol(tbl->hand_data[1]), valstri(tbl->hand_data[1]), suit(tbl->hand_data[1]),
                   NTEXT);
        send_to_table(buf, tbl);
        dc_sprintf(buf, "The dealer has %d!\r\n", hand_strength(tbl));
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

void check_insurance2(varg_t arg1, void *arg2, void *arg3)
{
  CasinoTablePtr tbl = arg1.table;

  if (hand_strength(tbl) == 21)
  { // dealer blackjacked
    send_to_table("Dealer blackjacked.\r\n", tbl);
    check_winner(tbl);
  }
  else
  {
    tbl->state = {};
    check_blackjacks(tbl);
    tbl->cr = tbl->plr;
    pulse_table_bj(tbl);
  }
}

void check_insurance(CasinoTablePtr tbl)
{
  if (val(tbl->hand_data[0]) == 1)
  { // ace showing
    tbl->state = 1;
    send_to_table("$B$7The dealer says 'Blackjack insurance is available. Type INSURANCE to buy some.'$R\r\n", tbl);
    TimerPtr timer = TimerPtr(new Timer);
    timer->arg1.table = tbl;
    timer->arg2 = {};
    timer->function = check_insurance2;
    timer->timeleft = 8;
    addtimer(timer);
  }
  else
  {
    check_blackjacks(tbl);
    tbl->cr = tbl->plr;
    pulse_table_bj(tbl);
  }
}
const QString hand_thing(CasinoPlayerPtr plr)
{
  if (hands(plr) <= 1)
    return "";
  static QString buf;
  dc_sprintf(buf, " for hand %d", hand_number(plr));
  return &buf[0];
}

void DC::pulse_table_bj(CasinoTablePtr tbl, qint32 recall)
{
  /*  if (tbl->state)
    {
     return;
    }*/
  if (tbl->cr && !verify(tbl->cr))
    return; // verify recalls this function with correct data

  if (tbl->cr)
  {
    QString buf;
    dc_sprintf(buf, "$B$7The dealer says 'It's your turn, %s, what would you like to do%s?'$R\r\n",
               qPrintable(tbl->cr->ch->name()), hand_thing(tbl->cr));
    send_to_table(buf, tbl);
    tbl->handnr++;
    add_timer(tbl->cr);
  }
  else if (tbl->plr)
  {
    // new hand
    tbl->handnr++;
    send_to_table("The dealer passes out cards to everyone at the table.\r\n", tbl);
    CasinoPlayerPtr plr, pnext;
    for (plr = tbl->plr; plr; plr = pnext)
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

void create_table(ObjectPtr obj)
{
  CasinoTablePtr table;
  table = new CasinoTable;
  table->obj = obj;
  if (obj->obj_flags.value[2])
    table->gold = false;
  else
    table->gold = true;
  table->deck = create_deck(6);
  table->plr = table->cr = {};
  table->deck->table = table;
  for (qint32 i = {}; i < 21; i++)
    table->hand_data[i] = {};

  table->state = {};
  //  add_timer_bj_dealer2(table);
  obj->table = table;
  shuffle_deck(table->deck);
}

void destroy_table(CasinoTablePtr tbl)
{
  tbl->obj->table = {};
  reset_table(tbl);
  tbl = {};
}

bool playing(CharacterPtr ch, CasinoTablePtr tbl)
{
  CasinoPlayerPtr plr;
  for (plr = tbl->plr; plr; plr = plr->next)
    if (plr->ch == ch)
      return true;

  return false;
}

CasinoPlayerPtr getPlayer(CharacterPtr ch, CasinoTablePtr tbl)
{
  CasinoPlayerPtr plr;
  if (tbl->cr && tbl->cr->ch == ch)
    return tbl->cr; // priority on current hand
  for (plr = tbl->plr; plr; plr = plr->next)
    if (plr->ch == ch)
      return plr;
  return {};
}

qint32 players(CasinoTablePtr tbl)
{
  CasinoPlayerPtr plr;
  qint32 i = {};
  for (plr = tbl->plr; plr; plr = plr->next)
    i++;
  return i;
}

QString tempBuf;
QString lineTwo;
QString lineTop;
qint32 padnext = {};
// Not pretty, but don't feel like redoing the prompt functions, so whatever.

QString show_hand(qint32 hand_data[21], qint32 hide, bool ascii, bool showColor)
{
  static QString buf;
  qint32 i = {};
  buf[0] = '\0';
  dc_sprintf(lineTwo, "%s%*s", lineTwo, (qint32)dc_strlen(tempBuf) + padnext, " ");
  dc_sprintf(lineTop, "%s%*s", lineTop, (qint32)dc_strlen(tempBuf) + padnext, " ");
  if (padnext)
    padnext = {};
  while (hand_data[i] > 0)
  {
    if (!ascii)
    {
      if (i == 1 && hide)
        dc_sprintf(buf, "%s %sDC%s", buf, showColor ? BOLD : "", showColor ? NTEXT : "");
      else
        dc_sprintf(buf, "%s %s%s%c%s", buf, showColor ? suitcol(hand_data[i]) : "", valstri(hand_data[i]), suit(hand_data[i]), showColor ? NTEXT : "");
      i++;
    }
    else
    {
      if (i == 1 && hide)
      {
        dc_sprintf(buf, "%s%s| D |%s", buf, showColor ? BOLD : "", showColor ? NTEXT : "");
        dc_sprintf(lineTwo, "%s%s| C |%s", lineTwo, showColor ? BOLD : "", showColor ? NTEXT : "");
        dc_sprintf(lineTop, "%s%s,---,%s", lineTop, showColor ? BOLD : "", showColor ? NTEXT : "");
      }
      else
      {
        dc_sprintf(buf, "%s%s|%s %s%s%s %s|%s", buf, showColor ? BOLD : "", showColor ? NTEXT : "", showColor ? suitcol(hand_data[i]) : "", valstri(hand_data[i]), showColor ? NTEXT : "", showColor ? BOLD : "", showColor ? NTEXT : "");
        dc_sprintf(lineTwo, "%s%s|%s %s%c%s %s|%s", lineTwo, showColor ? BOLD : "", showColor ? NTEXT : "", showColor ? suitcol(hand_data[i]) : "", suit(hand_data[i]), showColor ? NTEXT : "", showColor ? BOLD : "", showColor ? NTEXT : "");
        dc_sprintf(lineTop, "%s%s,---,%s", lineTop, showColor ? BOLD : "", showColor ? NTEXT : "");
      }
      i++;
    }
  }

  return &buf[0];
}

// qint32 hand_number(CasinoPlayerPtr plr)
qint32 hands(CasinoPlayerPtr plr)
{
  qint32 i = {};
  for (CasinoPlayerPtr ptmp = plr->table->plr; ptmp; ptmp = ptmp->next)
  {
    if (plr->ch == ptmp->ch)
      i++;
  }
  return i;
}

qint32 hand_number(CasinoPlayerPtr plr)
{
  qint32 i = {};
  for (CasinoPlayerPtr ptmp = plr->table->plr; ptmp; ptmp = ptmp->next)
  {
    if (plr->ch == ptmp->ch)
      i++;
    if (plr == ptmp)
      return i;
  }
  return i;
}

QString Character::createBlackjackPrompt(void)
{
  if (!isPlayer())
    return {};

  bool ascii = isSet(player->toggles, Player::PLR_ASCII);
  QString prompt;
  bool showColor = false;
  if (isSet(GET_TOGGLES(this), Player::PLR_ANSI) || isSet(GET_TOGGLES(this), Player::PLR_VT100))
  {
    showColor = true;
  }

  if (in_room < 21902 || in_room > 21905)
    if (in_room != 44)
      return {};
  auto obj = dc_->world[in_room].contents;
  for (; obj; obj = obj->next_content)
  {
    if (obj->table)
      break;
  }

  if (!obj || !obj->table->plr)
    return {};
  // Prompt-time
  qint32 plrsdone = {};
  QString buf;
  buf[0] = '\0';
  lineTwo[0] = '\0';
  lineTop[0] = '\0';
  CasinoPlayerPtr plr, pnext;
  for (plr = obj->table->plr; plr; plr = pnext)
  {
    pnext = plr->next;
    if (!dc_->verify(plr))
      continue;
    if (!plr->hand_data[0])
      continue;
    if (plr->ch == this)
    {
      QString buf2;
      buf2[0] = '\0';
      if (plr->table->cr == plr)
      {
        dc_strcat(buf2, "HIT STAY ");
        if (plr->hand_data[2] == 0)
          dc_strcat(buf2, "DOUBLE ");
      }
      if (canInsurance(plr))
        dc_strcat(buf2, "INSURANCE ");
      if (canSplit(plr))
        dc_strcat(buf2, "SPLIT ");
      if (buf2[0] != '\0')
      {
        prompt += "You can: ";
        prompt += showColor ? BOLD CYAN : "";
        prompt += buf2;
        prompt += showColor ? NTEXT : "";
        prompt += "\r\n";
      }
      if (hands(plr) > 1)
      {
        dc_sprintf(tempBuf, "%s, hand %d: ", qPrintable(plr->ch->name()), hand_number(plr));
        dc_sprintf(buf, "%s%s%s%s, hand %d%s: %s = %d   ", buf, showColor ? BOLD : "", plr == plr->table->cr && showColor ? GREEN : "", qPrintable(plr->ch->name()), hand_number(plr), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor), hand_strength(plr));
        padnext = hand_strength(plr) > 9 ? 8 : 7;
      }
      else
      {
        dc_sprintf(tempBuf, "%s: ", qPrintable(plr->ch->name()));
        dc_sprintf(buf, "%s%s%s%s%s: %s = %d   ", buf, showColor ? BOLD : "", plr == plr->table->cr && showColor ? GREEN : "", qPrintable(plr->ch->name()), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor), hand_strength(plr));
        padnext = hand_strength(plr) > 9 ? 8 : 7;
      }
    }
    //    }
    else
    {
      if (hands(plr) > 1)
      {
        dc_sprintf(tempBuf, "%s, hand %d: ", qPrintable(plr->ch->name()), hand_number(plr));
        dc_sprintf(buf, "%s%s%s, hand %d%s: %s ", buf, plr == plr->table->cr && showColor ? BOLD GREEN : "", qPrintable(plr->ch->name()), hand_number(plr), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor));
        padnext = 1;
      }
      else
      {
        dc_sprintf(tempBuf, "%s: ", qPrintable(plr->ch->name()));
        dc_sprintf(buf, "%s%s%s%s: %s ", buf, plr == plr->table->cr && showColor ? BOLD GREEN : "", qPrintable(plr->ch->name()), showColor ? NTEXT : "", show_hand(plr->hand_data, 0, ascii, showColor));
        padnext = 1;
      }
    }
    if (++plrsdone % 3 == 0)
    {
      if (buf[0] != '\0')
      {
        prompt += "\r\n";
        if (ascii)
        {
          prompt += lineTop;
          prompt += "\r\n";
        }

        prompt += buf;
        prompt += "\r\n";
        if (ascii)
        {
          prompt += lineTwo;
          prompt += "\r\n";
        }

        for (qint32 z = {}; lineTop[z]; z++)
          if (lineTop[z] == ',')
            lineTop[z] = '\'';
        if (ascii)
        {
          prompt += lineTop;
          prompt += "\r\n";
        }
        buf[0] = '\0';
        lineTwo[0] = '\0';
        lineTop[0] = '\0';
        padnext = {};
      }
    }
  }
  if (obj->table->hand_data[0])
  {
    dc_sprintf(tempBuf, "Dealer: ");
    dc_sprintf(buf, "%s%sDealer%s: %s", buf, showColor ? BOLD YELLOW : "", showColor ? NTEXT : "", obj->table->state < 2 ? show_hand(obj->table->hand_data, 1, ascii, showColor) : show_hand(obj->table->hand_data, 0, ascii, showColor));
    dc_sprintf(buf, "%s\r\n", buf);
  }
  // fixPadding(&buf[0]);
  if (buf[0] != '\0')
  {
    prompt += "\r\n";
    if (ascii)
    {
      prompt += lineTop;
      prompt += "\r\n";
    }
    prompt += buf;
    if (ascii)
    {
      prompt += lineTwo;
      prompt += "\r\n";
    }
    for (qint32 z = {}; lineTop[z]; z++)
      if (lineTop[z] == ',')
        lineTop[z] = '\'';
      else if (lineTop[z] == '_')
        lineTop[z] = '-';
    if (ascii)
    {
      prompt += lineTop;
      prompt += "\r\n";
    }
  }
  return prompt;
}

void Character::sendBlackjackPrompt(void)
{
  send(createBlackjackPrompt());
}

qint32 blackjack_table(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg,
                       CharacterPtr invoker)
{
  bool showColor = false;
  if (ch && ch->isPlayer() && (isSet(GET_TOGGLES(ch), Player::PLR_ANSI) || isSet(GET_TOGGLES(ch), Player::PLR_VT100)))
  {
    showColor = true;
  }

  QString arg1;
  arg = one_argument(arg, arg1);
  if (!isCommandTypeCasino(cmd))
  {
    return ReturnValue::eFAILURE;
  }
  if (!ch || ch->isNonPlayer())
    return ReturnValue::eFAILURE; // craziness
  if (obj->in_room <= 0)
    return ReturnValue::eFAILURE;
  if (!obj->table)
    create_table(obj);
  if (ch->isPlayerCantQuit() || ch->isPlayerObjectThief() || ch->isPlayerGoldThief())
  {
    ch->sendln("You cannot play blackjack while you are flagged as naughty.");
    return ReturnValue::eSUCCESS;
  }

  CasinoPlayerPtr plr = getPlayer(ch, obj->table);

  if (cmd == cmd_t::BET) // bet
  {
    if (obj->table->state > 1 || obj->table->cr || obj->table->hand_data[0])
    {
      ch->sendln("There is a hand in progress. No bets are accepted at the moment.");
      return ReturnValue::eSUCCESS;
    }
    if (playing(ch, obj->table))
    {
      ch->sendln("You have already made your bet.");
      return ReturnValue::eSUCCESS;
    }
    if (!is_number(arg1))
    {
      ch->sendln("Bet how much?\r\nSyntax: bet <amount>");
      return ReturnValue::eSUCCESS;
    }
    qint32 amt = dc_atoi(arg1);
    if (obj->table->gold)
    {
      if (amt < 0 || amt > obj->obj_flags.value[1] || amt < obj->obj_flags.value[0])
      {

        ch->send(u"Minimum bet: %d\r\nMaximum bet: %d\r\n"_s.arg(obj->obj_flags.value[0]).arg(obj->obj_flags.value[1]));
        return ReturnValue::eSUCCESS;
      }
    }
    else
    {
      if (amt < 0 || amt > obj->obj_flags.value[3] || amt < obj->obj_flags.value[2])
      {

        ch->send(u"Minimum bet: %d\r\nMaximum bet: %d\r\n"_s.arg(obj->obj_flags.value[2]).arg(obj->obj_flags.value[3]));
        return ReturnValue::eSUCCESS;
      }
    }
    if (obj->table->gold && (quint32)amt > ch->getGold())
    {
      ch->sendln("You cannot afford that.\r\n$B$7The dealer whispers to you, 'You can find an ATM machine in the lobby, buddy.'$R");
      return ReturnValue::eSUCCESS;
    }
    else if (!obj->table->gold && (quint32)amt > GET_PLATINUM(ch))
    {
      ch->sendln("You cannot afford that.");
      return ReturnValue::eSUCCESS;
    }
    if (players(obj->table) > 5)
    {
      ch->sendln("The table is currently full.");
      return ReturnValue::eSUCCESS;
    }
    plr = createPlayer(ch, obj->table);
    plr->bet = amt;
    if (obj->table->gold)
      ch->removeGold(amt);
    else
      GET_PLATINUM(ch) -= amt;
    ch->sendln("The dealer accepts your bet.");
    QString buf;
    dc_sprintf(buf, "%s bets %d.\r\n", qPrintable(ch->name()), amt);
    send_to_table(buf, obj->table, plr);
    if (obj->table->state == 0)
    {
      CharacterPtr tmpch;
      qint32 i = {};
      for (tmpch = dc_->world[ch->in_room].people_; tmpch;
           tmpch = tmpch->next_in_room)
        if (tmpch->isPlayer())
          i++;
      if (i <= players(obj->table))
        add_timer_bj_dealer2(obj->table, 2); // end bets in 1 secs
      else
        add_timer_bj_dealer2(obj->table); // end bets in 10 secs
      obj->table->state = 1;
    }
    else
    {
      CharacterPtr tmpch;
      qint32 i = {};
      for (tmpch = dc_->world[ch->in_room].people_; tmpch;
           tmpch = tmpch->next_in_room)
        if (tmpch->isPlayer())
          i++;
      if (i <= players(obj->table))
      {
        TimerPtr tmr;
        for (tmr = timer_list; tmr; tmr = tmr->next)
          if ((CasinoTablePtr)tmr->arg1.table == obj->table)
            tmr->timeleft = 1;
      }
    }
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::INSURANCE) // insurance
  {
    if (!plr)
    {
      ch->sendln("You are not currently playing.");
      return ReturnValue::eSUCCESS;
    }
    if (!canInsurance(plr))
    {
      ch->sendln("You cannot make an insurance bet at the moment.");
      return ReturnValue::eSUCCESS;
    }
    if (plr->table->gold && ch->getGold() < (quint32)(plr->bet / 2))
    {
      ch->sendln("You cannot afford an insurance bet right now.");
      return ReturnValue::eSUCCESS;
    }
    if (!plr->table->gold && GET_PLATINUM(ch) < (quint32)(plr->bet / 2))
    {
      ch->sendln("You cannot afford an insurance bet right now.");
      return ReturnValue::eSUCCESS;
    }
    plr->table->handnr++;
    plr->insurance = true;
    QString buf;
    if (plr->table->gold)
      ch->removeGold(plr->bet / 2);
    else
      GET_PLATINUM(ch) -= plr->bet / 2;
    dc_sprintf(buf, "%s makes an insurance bet.\r\n", qPrintable(ch->name()));
    send_to_table(buf, plr->table, plr);
    ch->sendln("You make an insurance bet.");
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::DOUBLE) // doubledown
  {
    if (!plr)
    {
      ch->sendln("You are not currently playing.");
      return ReturnValue::eSUCCESS;
    }
    if (plr->table->cr != plr)
    {
      ch->sendln("It is not currently your turn.");
      return ReturnValue::eSUCCESS;
    }
    if ((plr->table->gold && plr->ch->getGold() < (quint32)plr->bet) || (!plr->table->gold && GET_PLATINUM(plr->ch) < (quint32)plr->bet))
    {
      ch->sendln("You cannot afford to double your bet.");
      return ReturnValue::eSUCCESS;
    }
    if (plr->hand_data[2] || plr->doubled)
    {
      ch->sendln("You cannot double right now.");
      return ReturnValue::eSUCCESS;
    }
    if (plr->table->gold)
      plr->ch->removeGold(plr->bet);
    else
      GET_PLATINUM(plr->ch) -= plr->bet;
    plr->bet *= 2;
    plr->doubled = true;
    plr->table->handnr++;
    QString buf;
    dc_sprintf(buf, "%s doubles %s bet.\r\n", qPrintable(ch->name()), HSHR(ch));
    send_to_table(buf, plr->table, plr);
    ch->sendln("You double your bet.");

    plr->hand_data[2] = pickCard(plr->table->deck);
    dc_sprintf(buf, "%s receives a %s%s%c%s.\r\n", qPrintable(ch->name()),
               showColor ? suitcol(plr->hand_data[2]) : "", valstri(plr->hand_data[2]),
               suit(plr->hand_data[2]), showColor ? NTEXT : "");
    send_to_table(buf, plr->table, plr);
    dc_sprintf(buf, "You receive a %s%s%c%s.\r\n", showColor ? suitcol(plr->hand_data[2]) : "",
               valstri(plr->hand_data[2]), suit(plr->hand_data[2]), showColor ? NTEXT : "");
    ch->send(buf);

    if (hand_strength(plr) > 21) // busted
    {
      QString buf;
      dc_sprintf(buf, "%s busted.\r\n", qPrintable(ch->name()));
      send_to_table(buf, plr->table, plr);
      ch->sendln("$BYou BUSTED!$R\r\nThe dealer takes your bet.");
      nextturn(plr->table);

      if (plr->table->plr != plr || plr->next != nullptr) // make dealer show cards..
        free_player(plr);
    }
    else
    {
      nextturn(plr->table);
    }
    //     pulse_table_bj(plr->table);
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::STAY) // stand
  {
    if (!plr)
    {
      ch->sendln("You are not currently playing.");
      return ReturnValue::eSUCCESS;
    }
    if (plr->table->cr != plr)
    {
      ch->sendln("It is not currently your turn.");
      return ReturnValue::eSUCCESS;
    }
    QString buf;
    plr->table->handnr++;
    dc_sprintf(buf, "%s stays.\r\n", qPrintable(ch->name()));
    send_to_table(buf, plr->table);
    nextturn(plr->table);
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::SPLIT) // split
  {
    if (!plr)
    {
      ch->sendln("You are not currently playing.");
      return ReturnValue::eSUCCESS;
    }
    if (!canSplit(plr))
    {
      ch->sendln("You cannot split right now.");
      return ReturnValue::eSUCCESS;
    }
    if ((ch->getGold() < (quint32)plr->bet && plr->table->gold) || (GET_PLATINUM(ch) < (quint32)plr->bet && !plr->table->gold))
    {
      ch->sendln("You cannot afford to split.");
      return ReturnValue::eSUCCESS;
    }
    if (plr->table->gold)
      ch->removeGold(plr->bet);
    else
      GET_PLATINUM(ch) -= plr->bet;
    CasinoPlayerPtr nw = createPlayer(ch, plr->table, 1);
    nw->next = plr->next;
    plr->next = nw;
    nw->bet = plr->bet;
    plr->table->handnr++;
    nw->hand_data[0] = plr->hand_data[1];
    plr->hand_data[1] = pickCard(plr->table->deck);
    nw->hand_data[1] = pickCard(plr->table->deck);
    nw->doubled = plr->doubled;
    ch->sendln("You split your hand.");
    QString buf;
    dc_sprintf(buf, "%s splits %s hand.\r\n", qPrintable(ch->name()), HSHR(ch));
    send_to_table(buf, plr->table, plr);
    pulse_table_bj(plr->table);
    return ReturnValue::eSUCCESS;
  }
  else if (cmd == cmd_t::HIT) // hit
  {
    if (!plr)
    {
      ch->sendln("You are not currently playing.");
      return ReturnValue::eSUCCESS;
    }
    if (plr->table->cr != plr)
    {
      ch->sendln("It is not currently your turn.");
      return ReturnValue::eSUCCESS;
    }
    qint32 i;
    for (i = {}; i < 21; i++)
      if (plr->hand_data[i] == 0)
        break;
    plr->hand_data[i] = pickCard(plr->table->deck);
    QString buf;
    dc_sprintf(buf, "%s hits and receives a %s%s%c%s.\r\n", qPrintable(ch->name()),
               showColor ? suitcol(plr->hand_data[i]) : "", valstri(plr->hand_data[i]),
               suit(plr->hand_data[i]), showColor ? NTEXT : "");
    send_to_table(buf, plr->table, plr);
    dc_sprintf(buf, "You hit and receive a %s%s%c%s.\r\n",
               showColor ? suitcol(plr->hand_data[i]) : "", valstri(plr->hand_data[i]),
               suit(plr->hand_data[i]), showColor ? NTEXT : "");
    ch->send(buf);
    if (hand_strength(plr) > 21) // busted
    {
      QString buf;
      dc_sprintf(buf, "%s busted.\r\n", qPrintable(ch->name()));
      send_to_table(buf, plr->table, plr);
      ch->sendln("$BYou BUSTED!$R\r\nThe dealer takes your bet.");
      nextturn(plr->table);
      if (plr->table->plr != plr || plr->next != nullptr) // make dealer show cards..
        free_player(plr);
    }
    else
    {
      if (plr->doubled)
        nextturn(plr->table);
      else
      {
        pulse_table_bj(plr->table);
      }
    }
    return ReturnValue::eSUCCESS;
  }
  return ReturnValue::eSUCCESS;
}

/* End Blackjack */

/* Texas Hold'em! */
qint32 hand[5][2];
// cycles between using two spaces for temporary data
// as comparisons need to be made

class ttable
{
public:
  cDeck *deck;
  class pot *pots;
  class tplayer *player[6];
  qint32 cards[5]; // cards on the table
  qint32 bet;
  qint32 crPlayer;
  qint32 state;
};
class tplayer
{
public:
  ttable *table;
  qint32 hand[5]; // 0-1 playercards, 2-4 = top table cards
  qint32 chips;
  qint32 pos;
  qint32 options;
  bool nw;
  bool dealer;
};
class pot
{
public:
  pot *next;
  tplayer *player[6];
};

qint32 has_seat(ttable *ttbl)
{
  for (qint32 i = {}; i < 6; i++)
    if (ttbl->player[i] == nullptr)
      return i;
  return -1;
}

bool findcard(qint32 hand[5], qint32 valu, QChar su, qint32 num = 1)
{
  qint32 i;
  for (i = {}; i < 5; i++)
    if (!valu || val(hand[i]) == valu)
      if (!su || suit(hand[i]) == su)
        if (--num <= 0)
          return true;

  return false;
}

qint32 highcard(qint32 hand[5])
{
  qint32 a = hand[0];
  for (qint32 i = 1; i < 5; i++)
    if (val(a) < val(hand[i]) || val(hand[i]) == 1)
      a = val(hand[i]);
  return a;
}

qint32 lowcard(qint32 hand[5])
{ // counts aces for straight purposes.
  qint32 a = hand[0];
  for (qint32 i = 1; i < 5; i++)
    if (val(a) > val(hand[i]))
      a = val(hand[i]);
  return a;
}

qint32 has_flush(qint32 hand[5])
{
  if (findcard(hand, 0, suit(hand[0]), 5))
    return highcard(hand);
  return false;
}

qint32 has_straight(qint32 hand[5])
{
  qint32 i = lowcard(hand);
  if (findcard(hand, i + 1, 0, 1) &&
      findcard(hand, i + 2, 0, 1) &&
      findcard(hand, i + 3, 0, 1) &&
      findcard(hand, i + 4, 0, 1))
    return i + 4;
  if (i == 1)
  {
    if (findcard(hand, 10, 0, 1) &&
        findcard(hand, 11, 0, 1) &&
        findcard(hand, 12, 0, 1) &&
        findcard(hand, 13, 0, 1))
      return 14;
  }
  return false;
}

qint32 has_rsf(qint32 hand[5])
{
  if (hand[0] == 0)
    return false; // shockingly, nope
  if (has_flush(hand) && has_straight(hand) == 14)
    return true; // yegods

  return false;
}

qint32 has_sf(qint32 hand[5])
{
  if (hand[0] == 0)
    return false;
  if (has_flush(hand))
    return has_straight(hand);

  return false;
}

qint32 has_4kind(qint32 hand[5])
{
  if (findcard(hand, val(hand[0]), 0, 4))
    return val(hand[0]);
  if (findcard(hand, val(hand[1]), 0, 4))
    return val(hand[1]);
  // one of the first two cards has to be part of the 4kind
  return false;
}

qint32 has_fhouse(qint32 hand[5])
{
  qint32 card1 = val(hand[0]);
  qint32 i;
  for (i = 1; i < 5; i++)
    if (val(hand[i]) != card1)
      break;
  // no error checking needed, it is not possible to have 5 of a kind
  if ((findcard(hand, card1, 0, 3) &&
       findcard(hand, val(hand[i]), 0, 2)) ||
      (findcard(hand, card1, 0, 2) &&
       findcard(hand, val(hand[i]), 0, 3)))
    return highcard(hand) * 1000 + lowcard(hand); // works
  return false;
}

qint32 has_3kind(qint32 hand[5])
{
  if (findcard(hand, val(hand[0]), 0, 3))
    return val(hand[0]);
  if (findcard(hand, val(hand[1]), 0, 3))
    return val(hand[1]);
  if (findcard(hand, val(hand[2]), 0, 3))
    return val(hand[2]);
  return false;
}

qint32 has_2pair(qint32 hand[5])
{
  qint32 first = {};
  for (qint32 i = {}; i < 5; i++)
    if (val(hand[i]) != val(first) && findcard(hand, val(hand[i]), 0, 2))
    {
      if (first)
        return MAX(val(first), val(hand[i])) * 1000 + MIN(val(first), val(hand[i]));
      else
        first = hand[i];
    }
  return false;
}

qint32 has_pair(qint32 hand[5])
{
  if (findcard(hand, val(hand[0]), 0, 2))
    return val(hand[0]);
  if (findcard(hand, val(hand[1]), 0, 2))
    return val(hand[0]);
  if (findcard(hand, val(hand[2]), 0, 2))
    return val(hand[0]);
  if (findcard(hand, val(hand[3]), 0, 2))
    return val(hand[0]);
  return false;
}

const QString cardname(qint32 card)
{
  switch (card)
  {
  case 1:
    return "Ace";
  case 2:
    return "Two";
  case 3:
    return "Three";
  case 4:
    return "Four";
  case 5:
    return "Five";
  case 6:
    return "Six";
  case 7:
    return "Seven";
  case 8:
    return "Eight";
  case 9:
    return "Nine";
  case 10:
    return "Jack";
  case 12:
    return "Queen";
  case 13:
    return "King";
  }
  return "Invalid";
}

QString hand_name(qint32 hand[5])
{
  QString buf;
  buf[0] = '\0';
  qint32 i = {};
  if (has_rsf(hand))
    return "Royal Straight Flush";
  else if ((i = has_sf(hand)))
  {
    dc_sprintf(buf, "%s-high Straight Flush", cardname(i));
  }
  else if ((i = has_4kind(hand)))
  {
    dc_sprintf(buf, "Four of a kind, %ss", cardname(i));
  }
  else if ((i = has_fhouse(hand)))
  {
    dc_sprintf(buf, "Full House, %s over %ss", cardname(i / 1000), cardname(i - (i / 1000) * 1000));
  }
  else if ((i = has_flush(hand)))
  {
    dc_sprintf(buf, "%s-high Flush", cardname(i));
  }
  else if ((i = has_straight(hand)))
  {
    dc_sprintf(buf, "%s-high Straight", cardname(i));
  }
  else if ((i = has_3kind(hand)))
  {
    dc_sprintf(buf, "Three of a kind, %ss", cardname(i));
  }
  else if ((i = has_2pair(hand)))
  {
    dc_sprintf(buf, "Two Pair, %ss and %ss", cardname(i / 1000), cardname(i - (i / 1000) * 1000));
  }
  else if ((i = has_pair(hand)))
  {
    dc_sprintf(buf, "Pair of %ss", cardname(i));
  }
  else
  {
    dc_sprintf(buf, "%s high", cardname(highcard(hand)));
  }
  return buf;
}

qint32 get_hand(tplayer *tplr, qint32 which)
{
  static qint32 i = {};

  ttable *ttbl = tplr->table;
  TOGGLE_BIT(i, 1); // if it's 1, set to 0, if 0, set to 1. Gooo toggle!
  qint32 z;
  for (z = {}; z < 5; z++)
    hand[z][i] = {};

  qint32 o;
  qint32 temphand[7];
  temphand[0] = tplr->hand[0];
  temphand[1] = tplr->hand[1];
  temphand[2] = ttbl->cards[0];
  temphand[3] = ttbl->cards[1];
  temphand[4] = ttbl->cards[2];
  temphand[5] = ttbl->cards[3];
  temphand[6] = ttbl->cards[4];
  for (z = {}; z < 7; z++)
    if (temphand[z] == 0)
      break;

  // qint32 one = 0, two = 1, three = 2, four = 3, five = 4;
  qint32 bleh[5] = {0, 1, 2, 3, 4}, p;
  for (o = {}; o < which; o++)
  {
    p = 4;
    bleh[p]++;
    while (bleh[p] == (3 + p))
    {
      if (p == 0)
        return -1; // invalid
      bleh[--p]++;
      qint32 zz;
      for (zz = p + 1; zz < 5; zz++)
        bleh[zz] = bleh[zz - 1] + 1;
      //	bleh[p+1] = bleh[p] + 1;
    }
  }
  for (z = {}; z < 5; z++)
    hand[z][i] = temphand[bleh[z]];
  // if (!tplr->hand[0]) return i;
  // if (!ttbl->cards[0]) return i;

  return i;
}

class hand_function
{
public:
  HAND_FUNC *func;
};

const QList<hand_function> funcs = {
    {has_rsf},
    {has_sf},
    {has_4kind},
    {has_fhouse},
    {has_flush},
    {has_straight},
    {has_3kind},
    {has_2pair},
    {has_pair},
    {highcard};

qint32 handcompare(qint32 hand1[5], qint32 hand2[5])
{
  qint32 v = {};
  for (; funcs[v].func != 0; v++)
  {
    qint32 a = (*(funcs[v].func))(hand1), b = (*(funcs[v].func))(hand2);
    if (a > b)
      return 1;
    if (b > a)
      return 2;
    if (a == b)
      return 3; //
  }
  dc_->logentry(u"Error in handcompare."_s, 110, DC::LogChannel::LOG_MORTAL);

  return -1;
}

command_return_t do_testhand(CharacterPtr ch, QString argument, cmd_t cmd)
{
  QString arg;
  one_argument(argument, arg);
  //  qint32 i = dc_atoi(arg);
  //  qint32 z = get_hand(i);
  // QString buf;
  // dc_sprintf(buf, "One: %d Two: %d Three: %d Four: %d Five: %d\r\n",
  //	hand[0][z],hand[1][z],
  //	hand[2][z],hand[3][z],
  //	hand[4][z]);
  // ch->send(buf);
  return ReturnValue::eSUCCESS;
}

qint32 find_highhand(tplayer *tplr)
{
  qint32 handnr = {};
  for (qint32 i = 1; i < 21; i++)
  {
    qint32 v = handcompare(hand[get_hand(tplr, handnr)], hand[get_hand(tplr, i)]);
    if (v == 2)
      handnr = i;
  }
  return handnr;
}

tplayer *createTplayer(ttable *ttbl)
{
  qint32 seat = has_seat(ttbl);
  if (seat < 0)
    return {};
  tplayer *tplr;

  tplr = new tplayer;
  ttbl->player[seat] = tplr;
  tplr->nw = true;
  tplr->table = ttbl;
  for (qint32 i = {}; i < 2; i++)
    tplr->hand[i] = {};
  tplr->chips = {};
  tplr->pos = -1;
  tplr->dealer = false;
  tplr->options = {};
  return tplr;
}

qint32 first_to_act(qint32 state, tplayer *player[6])
{
  qint32 plrs = 0, dlr = {};
  for (qint32 i = {}; i < 6; i++)
  {
    if (player[i])
      plrs++;
    else
      continue;

    if (player[i]->dealer)
      dlr = i;
  }

  if (plrs == 2)
    return dlr; // dealer acts first when there's only 2 players
  qint32 j = 2;
  for (qint32 i = dlr;; i++)
  {
    if (i == 6)
      i = {};
    if (player[i] && --j == 0)
      return i;
  }

  return dlr; // shouldn't happen
}

qint32 find_winner(ttable *ttbl)
{
  qint32 i, win = -1, winhand = {};
  for (i = {}; i < 6; i++)
  {
    if (!ttbl->player[i])
      continue;
    qint32 highhand = find_highhand(ttbl->player[i]);
    if (win == -1)
    {
      win = i;
      winhand = highhand;
    }
    else
    {
      if (handcompare(hand[get_hand(ttbl->player[i], highhand)],
                      hand[get_hand(ttbl->player[win], winhand)]) == 1)
      {
        winhand = highhand;
        win = i;
      }
    }
  }
  return win;
}

void pulse_holdem(class ttbl *tbl)
{
}

/* End Texas Hold'em */

/* Slot Machines! */

class machine_data
{
public:
  ObjectPtr obj;
  CharacterPtr prch;
  CharacterPtr ch;
  uint cost;
  uint lastwin;
  qint32 bet;
  qreal jackpot;
  qint32 linkedto;
  bool busy;
  bool gold;
  bool button;
};

const QStringList reel1 = {
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
    "$B$6 Plum $R"};

const QStringList reel2 = {
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
    "$B$2Mln$R/$5Or$R"};

const QStringList reel3 = {
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
    "$B$5Lemon $R"};

void save_slot_machines()
{
  if (dc_->cf.bport == true)
  {
    return;
  }

  world_file_list_item *curr;
  QString buf;
  QString buf2;

  curr = dc_->obj_file_list;
  while (curr && curr->filename != "21900-21999.obj")
    curr = curr->next;

  if (!curr)
  {
    dc_->logentry(u"Mess up in save_slot_machines, no object file."_s, IMMORTAL, DC::LogChannel::LOG_BUG);
    return;
  }

  LegacyFile lf("objects", curr->filename, "Couldn't open obj save file %1 for save_slot_machines.");
  if (lf.isOpen())
  {
    for (qint32 x = curr->firstnum; x <= curr->lastnum; x++)
    {
      write_object(lf, dc_->obj_index[x].item);
    }
    dc_fprintf(lf.file_handle_, "$~\n");
  }
}

void create_slot(ObjectPtr obj)
{
  auto slot = new machine_data;
  slot->obj = obj;
  slot->ch = {};
  slot->prch = {};
  slot->cost = obj->obj_flags.value[0];
  slot->jackpot = obj->obj_flags.value[1];
  slot->linkedto = obj->obj_flags.value[3];
  slot->lastwin = {};
  slot->bet = 1;
  if (obj->obj_flags.value[2])
    slot->gold = false;
  else
    slot->gold = true;
  slot->busy = false;
  slot->button = false;
  obj->slot = slot;
}

bool verify_slot(machine_data *machine)
{
  if (machine->ch->in_room == machine->obj->in_room)
    return true;

  return false;
}

void update_linked_slots(machine_data *machine)
{
  QString ldesc;

  dc_snprintf(ldesc, MAX_STRING_LENGTH,
              "A slot machine which displays '$R$BJackpot: %d %s!$1' sits here.",
              (qint32)machine->jackpot,
              machine->gold ? "coins" : "plats");

  // Find all the slot machines
  for (qint32 i = 21906; i < 21918; i++)
  {
    ObjectPtr slot_obj = dc_->obj_index[real_object(i)].item;

    // Find all the slot machines linked to the same slot machine as us
    // and update their v1 jackpot, their machine's jackpot (if applicable)
    // and their long description
    if (slot_obj->obj_flags.value[3] == machine->linkedto)
    {
      // leaving the original desc from obj loading alone in the hash table
      //  if(!ishashed(slot_obj->long_description)) slot_obj->long_description={};
      slot_obj->long_description(ldesc);
      slot_obj->obj_flags.value[1] = (qint32)machine->jackpot;
      if (slot_obj->slot)
        slot_obj->slot->jackpot = machine->jackpot;

      // Update instances of the original slot obj
      for (ObjectPtr j = dc_->object_list; j; j = j->next)
      {
        if (j->item_number == real_object(i))
        {
          // if(!ishashed(j->long_description)) j->long_description={};
          j->long_description(ldesc);
          j->obj_flags.value[1] = (qint32)machine->jackpot;
          if (j->slot)
            j->slot->jackpot = machine->jackpot;
        }
      }
    }
  }
}

void slot_timer(machine_data *machine, qint32 stop1, qint32 stop2, qint32 delay)
{
  TimerPtr timer = TimerPtr(new Timer);
  timer->arg1.machine = machine;
  timer->arg2 = (void *)(qint64)stop1;
  timer->arg3 = (void *)(qint64)stop2;
  timer->function = reel_spin;
  timer->timeleft = delay;
  addtimer(timer);
}

void reel_spin(varg_t arg1, void *arg2, void *arg3)
{
  machine_data *machine = arg1.machine;
  qint32 stop1 = (qint64)arg2;
  qint32 stop2 = (qint64)arg3;

  QString buf;

  if (stop1 < 0 && charExists(machine->ch) && verify_slot(machine))
  {
    stop1 = dc_->number(0, 19);
    send_to_room("You hear a loud clunk as the first stopper snaps into place.\r\n", machine->obj->in_room);
    dc_sprintf(buf, "%s    |      |\r\n", reel1[stop1]);
    machine->ch->send(buf);
    slot_timer(machine, stop1, -1, 2);
  }
  else if (stop2 < 0 && charExists(machine->ch) && verify_slot(machine))
  {
    stop2 = dc_->number(0, 19);
    send_to_room("You hear a loud clunk as the second stopper snaps into place.\r\n", machine->obj->in_room);
    dc_sprintf(buf, "%s %s    |\r\n", reel1[stop1], reel2[stop2]);
    machine->ch->send(buf);
    slot_timer(machine, stop1, stop2, 2);
  }
  else if (charExists(machine->ch) && verify_slot(machine))
  {
    qint32 payout = {};
    qint32 stop3 = dc_->number(0, 19);
    send_to_room("You hear a loud clunk as the final stopper snaps into place.\r\n", machine->obj->in_room);
    dc_sprintf(buf, "%s %s %s\r\n", reel1[stop1], reel2[stop2], reel3[stop3]);
    machine->ch->send(buf);

    if (stop1 == 6 && stop2 == 3 && stop3 == 14)
      payout = 200;
    else if ((stop1 == 7 || stop1 == 10 || stop1 == 18) && (stop2 == 6 || stop2 == 15) && stop3 == 14)
      payout = 100;
    else if ((stop1 == 1 || stop1 == 14) && (stop2 == 11 || stop2 == 19) && (stop3 == 7 || stop3 == 17 || stop3 == 14))
      payout = 100;
    else if (stop1 == 7 && (stop2 == 5 || stop2 == 7 || stop2 == 10 || stop2 == 13 || stop2 == 18) && (stop3 == 0 || stop3 == 3 || stop3 == 6 || stop3 == 8 || stop3 == 11 || stop3 == 13 || stop3 == 14 || stop3 == 16 || stop3 == 18))
      payout = 18;
    else if ((stop1 == 2 || stop1 == 4 || stop1 == 11 || stop1 == 13 || stop1 == 15 || stop1 == 17 || stop1 == 19) &&
             (stop2 == 1 || stop2 == 6 || stop2 == 12) && (stop3 == 2 || stop3 == 9 || stop3 == 12 || stop3 == 14))
      payout = 14;
    else if ((stop1 == 0 || stop1 == 5 || stop1 == 8 || stop1 == 12 || stop1 == 16) && (stop2 == 3 || stop2 == 9 || stop2 == 11 || stop2 == 16 || stop2 == 19) && (stop3 == 1 || stop3 == 4 || stop3 == 7 || stop3 == 14 || stop3 == 17))
      payout = 10;
    else if ((stop1 == 3 || stop1 == 9) && (stop2 == 0 || stop2 == 2 || stop2 == 4 || stop2 == 8 || stop2 == 14 || stop2 == 17))
      payout = 5;
    else if (stop1 == 3 || stop1 == 9)
      payout = 2;
    else
    {
      machine->jackpot += (qreal)machine->cost * (qreal)machine->bet * 0.04;
      machine->jackpot = MIN<float>(machine->jackpot, 2000000000.0); // NEVER AGAIN!!! :P
      if (machine->linkedto)
      {
        update_linked_slots(machine);
      }
      else
      {
        (dc_->obj_index[machine->obj->item_number].item)->obj_flags.value[1] = (qint32)machine->jackpot;
        dc_sprintf(buf, "A slot machine which displays '$R$BJackpot: %d %s!$1' sits here.", (qint32)machine->jackpot, machine->gold ? "coins" : "plats");
        // if(!ishashed(machine->obj->long_description)) machine->obj->long_description={};
        machine->obj->long_description(buf);
        (dc_->obj_index[machine->obj->item_number].item)->long_description(buf);
      }
    }

    if (payout == 200 && machine->bet == 5)
    {
      send_to_room("The jackpot lights flash and loud noises come from all around you!\r\n", machine->obj->in_room);
      machine->ch->send(u"$BJACKPOT!!!!!!  You win the jackpot of %d %s!!$R\r\n"_s.arg((qint32)machine->jackpot).arg(machine->gold ? "coins" : "plats"));
      dc_sprintf(buf, "##%s just won the JACKPOT for %d %s!\r\n", qPrintable(machine->ch->name()), (qint32)machine->jackpot, machine->gold ? "coins" : "plats");
      send_info(buf);

      dc_->logf(IMMORTAL, DC::LogChannel::LOG_MORTAL, "Jackpot win! %s won the jackpot of %d %s!",
                qPrintable(machine->ch->name()), (qint32)machine->jackpot, machine->gold ? "coins" : "plats");
      if (machine->gold)
        machine->ch->addGold((qint32)machine->jackpot);
      else
        GET_PLATINUM(machine->ch) += (qint32)machine->jackpot;
      machine->jackpot = machine->cost * 1000;
      if (machine->linkedto)
      {
        update_linked_slots(machine);
      }
      else
      {
        (dc_->obj_index[machine->obj->item_number].item)->obj_flags.value[1] = (qint32)machine->jackpot;
        dc_sprintf(buf, "A slot machine which displays '$R$BJackpot: %d %s!$1' sits here.", (qint32)machine->jackpot, machine->gold ? "coins" : "plats");
        // if(!ishashed(machine->obj->long_description)) machine->obj->long_description={};
        machine->obj->long_description(buf);
        // if(!ishashed(((ObjectPtr )dc_->obj_index[machine->obj->item_number].item)->long_description))
        //    ((Object*)obj_index[machine->obj->item_number].item)->long_description={};
        (dc_->obj_index[machine->obj->item_number].item)->long_description(buf);
      }
    }
    else if (payout)
    {
      send_to_room("Lights flash and noises emanate from the slot machine!\r\n", machine->obj->in_room);
      machine->lastwin = machine->cost * payout * machine->bet;
      dc_sprintf(buf, "$BWinner!!$R  You win %d %s!\r\n", machine->lastwin, machine->gold ? "coins" : "plats");
      if (machine->gold)
        machine->ch->addGold(machine->lastwin);
      else
        GET_PLATINUM(machine->ch) += machine->lastwin;
      machine->ch->send(buf);
      machine->ch->sendln("A tiny panel flips open on the slot machine, revealing red and black buttons.");
      machine->button = true;
      machine->prch = machine->ch;
    }
    machine->busy = false;
  }
  else
  { // something bad happened
    machine->busy = false;
  }

  save_slot_machines();
}

qint32 slot_machine(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr invoker)
{
  QString buf;

  if (cmd != cmd_t::PULL && cmd != cmd_t::BET && cmd != cmd_t::PUSH) // pull or bet or push
    return ReturnValue::eFAILURE;
  if (!ch || ch->isNonPlayer())
    return ReturnValue::eFAILURE;

  if (ch->isPlayerCantQuit() || ch->isPlayerObjectThief() || ch->isPlayerGoldThief())
  {
    ch->sendln("You cannot play the slots while you are flagged as naughty.");
    return ReturnValue::eSUCCESS;
  }

  one_argument(arg, buf);

  if (cmd == cmd_t::PULL && dc_strcmp(buf, "handle"))
  {
    ch->sendln("Try pulling the handle.");
    return ReturnValue::eSUCCESS;
  }

  if (!obj->slot)
    create_slot(obj);

  if (obj->slot->busy)
  {
    ch->sendln("This machine is already in use, try another one.");
    return ReturnValue::eSUCCESS;
  }

  if (cmd == cmd_t::BET)
  {
    if (dc_atoi(buf) >= 1 && dc_atoi(buf) <= 5)
    {
      obj->slot->bet = dc_atoi(buf);
      obj->slot->prch = ch;
      if (obj->slot->button)
      {
        ch->sendln("The panel closes quietly.");
        obj->slot->button = false;
      }
      if (obj->slot->bet == 1)
        ch->sendln("You place only the minimum bet into the slot machine now.");
      else
        ch->send(u"You now start placing %1 times the base amount into the slot machine.\r\n"_s.arg(obj->slot->bet));
      return ReturnValue::eSUCCESS;
    }
    ch->sendln("You can only multiply the bet by 2, 3, 4, or 5, or set it back to 1.");
    return ReturnValue::eSUCCESS;
  }

  if (cmd == cmd_t::PUSH)
  {
    if (obj->slot->button)
    {
      if (obj->slot->prch == ch)
      {
        if (is_abbrev(buf, "red") || is_abbrev(buf, "black"))
        {
          if ((obj->slot->gold && (ch->getGold() < obj->slot->lastwin)) || (!obj->slot->gold && (GET_PLATINUM(ch) < obj->slot->lastwin)))
          {
            ch->sendln("You don't have enough money to try to double your last win.");
          }
          else if (ch->dc_->number(0, 1))
          {
            ch->sendln("$BWinner!!$R The button lights up and the room is filled with whirring noises!");
            if (obj->slot->gold)
              ch->addGold(obj->slot->lastwin);
            else
              GET_PLATINUM(ch) += obj->slot->lastwin;
            obj->slot->button = false;
          }
          else
          {
            ch->sendln("Oh no!! The other button lights up! You lose everything!");
            if (obj->slot->gold)
              ch->removeGold(obj->slot->lastwin);
            else
              GET_PLATINUM(ch) -= obj->slot->lastwin;
            obj->slot->button = false;
          }
        }
        else
          ch->sendln("You must push either the red or black button.");
      }
      else
        ch->sendln("Nothing seems to happen.");
    }
    else
      ch->sendln("You can find nothing to push.");
    return ReturnValue::eSUCCESS;
  }

  if ((obj->slot->gold && (ch->getGold() < (obj->slot->cost * obj->slot->bet))) || (!obj->slot->gold && (GET_PLATINUM(ch) < (obj->slot->cost * obj->slot->bet))))
  {
    ch->sendln("You don't have enough money to start the machine.");
    return ReturnValue::eSUCCESS;
  }

  if (obj->slot->prch != ch)
    obj->slot->bet = 1;
  if (obj->slot->button)
    ch->sendln("The panel closes quietly.");
  obj->slot->button = false;

  if (obj->slot->gold)
    ch->removeGold(obj->slot->cost * obj->slot->bet);
  else
    GET_PLATINUM(ch) -= obj->slot->cost * obj->slot->bet;
  obj->slot->busy = true;
  dc_sprintf(buf, "You place %d %s into the slot and set the reels spinning!\r\n", obj->slot->cost * obj->slot->bet, obj->slot->gold ? "coins" : "plats");
  ch->send(buf);
  act_to_room("$n reaches for the handle and pulls down.", ch, 0, 0, 0);
  ch->sendln("   |      |      |");
  obj->slot->ch = ch;

  slot_timer(obj->slot, -1, -1, 2);

  return ReturnValue::eSUCCESS;
}

/* End Slot Machines */

/* Roulette! */

const QStringList roulette_display = {
    "$2$B0$R", "$4$B1$R", "$0$B2$R", "$4$B3$R", "$0$B4$R", "$4$B5$R", "$0$B6$R", "$4$B7$R", "$0$B8$R",
    "$4$B9$R", "$0$B10$R", "$0$B11$R", "$4$B12$R", "$0$B13$R", "$4$B14$R", "$0$B15$R", "$4$B16$R",
    "$0$B17$R", "$4$B18$R", "$4$B19$R", "$0$B20$R", "$4$B21$R", "$0$B22$R", "$4$B23$R", "$0$B24$R",
    "$4$B25$R", "$0$B26$R", "$4$B27$R", "$0$B28$R", "$0$B29$R", "$4$B30$R", "$0$B31$R", "$4$B32$R",
    "$0$B33$R", "$4$B34$R", "$0$B35$R", "$4$B36$R"};

class roulette_player
{
public:
  CharacterPtr ch;
  quint32 bet_array[48];
};

class wheel_data
{
public:
  ObjectPtr obj;
  roulette_player *plr[6];
  qint32 countdown;
  bool spinning;
};

void create_wheel(ObjectPtr obj)
{
  auto wheel = new wheel_data;
  wheel->obj = obj;
  for (qint32 i = {}; i < 6; i++)
  {
    wheel->plr[i] = new roulette_player;
    wheel->plr[i]->ch = {};
    for (qint32 j = {}; j < 48; j++)
      wheel->plr[i]->bet_array[j] = {};
  }
  wheel->countdown = 11;
  wheel->spinning = false;
  obj->wheel = wheel;
}

void send_wheel_bets(CharacterPtr ch, wheel_data *wheel)
{
  qint32 i, j;
  bool found = false;
  const QStringList bet_name = {
      "$B$0BLACK$R", "$4$BRED$R", "$BEVEN$R", "$BODD$R", "$B1-12$R", "$B13-24$R", "$B25-36$R", "$B1-9$R", "$B10-18$R",
      "$B19-27$R", "$B28-36$R"};

  for (i = {}; i < 6; i++)
  {
    if (ch == wheel->plr[i]->ch)
    {
      for (j = {}; j < 11; j++)
      {
        if (wheel->plr[i]->bet_array[j])
        {
          if (!found)
          {
            ch->send("You have");
            found = true;
          }
          else
            ch->send(" and");
          ch->send(u" a bet of %u on %s"_s.arg(wheel->plr[i]->bet_array[j]).arg(bet_name[j]));
        }
      }
      for (j = 11; j < 48; j++)
      {
        if (wheel->plr[i]->bet_array[j])
        {
          if (!found)
          {
            ch->send("You have");
            found = true;
          }
          else
            ch->send(" and");
          ch->send(u" a bet of %u on %s"_s.arg(wheel->plr[i]->bet_array[j]).arg(roulette_display[j - 11]));
        }
      }
    }
  }
  if (!found)
    ch->send("You have not placed any bets"); // how the hell?
  ch->sendln(".");
}

quint32 check_roulette_wins(roulette_player *plr, qint32 num)
{
  quint32 tmp;
  quint32 winnings = {};

  if (plr->bet_array[0] && (num == 2 || num == 4 || num == 6 || num == 8 || num == 10 ||
                            num == 11 || num == 13 || num == 15 || num == 17 || num == 20 || num == 22 ||
                            num == 24 || num == 26 || num == 28 || num == 29 || num == 31 || num == 33 ||
                            num == 35))
  {
    tmp = 2 * plr->bet_array[0];
    plr->ch->send(u"You WIN %1 coins on your bet of $0$BBLACK$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }

  if (plr->bet_array[1] && !(num == 2 || num == 4 || num == 6 || num == 8 || num == 10 ||
                             num == 11 || num == 13 || num == 15 || num == 17 || num == 20 || num == 22 ||
                             num == 24 || num == 26 || num == 28 || num == 29 || num == 31 || num == 33 ||
                             num == 35 || num == 0))
  {
    tmp = 2 * plr->bet_array[1];
    plr->ch->send(u"You WIN %1 coins on your bet of $4$BRED$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[2] && num % 2 == 0 && num)
  {
    tmp = 2 * plr->bet_array[2];
    plr->ch->send(u"You WIN %1 coins on your bet of $BEVEN$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[3] && num % 2 == 1)
  {
    tmp = 2 * plr->bet_array[3];
    plr->ch->send(u"You WIN %1 coins on your bet of $BODD$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[4] && num > 0 && num < 13)
  {
    tmp = 3 * plr->bet_array[4];
    plr->ch->send(u"You WIN %1 coins on your bet of $B1-12$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[5] && num > 12 && num < 25)
  {
    tmp = 3 * plr->bet_array[5];
    plr->ch->send(u"You WIN %1 coins on your bet of $B13-24$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[6] && num > 24 && num < 37)
  {
    tmp = 3 * plr->bet_array[6];
    plr->ch->send(u"You WIN %1 coins on your bet of $B25-36$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[7] && num > 0 && num < 10)
  {
    tmp = 4 * plr->bet_array[7];
    plr->ch->send(u"You WIN %1 coins on your bet of $B1-9$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[8] && num > 9 && num < 19)
  {
    tmp = 4 * plr->bet_array[8];
    plr->ch->send(u"You WIN %1 coins on your bet of $B10-18$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[9] && num > 18 && num < 28)
  {
    tmp = 4 * plr->bet_array[9];
    plr->ch->send(u"You WIN %1 coins on your bet of $B19-27$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  if (plr->bet_array[10] && num > 27 && num < 37)
  {
    tmp = 4 * plr->bet_array[10];
    plr->ch->send(u"You WIN %1 coins on your bet of $B28-36$R!\r\n"_s.arg(tmp));
    winnings += tmp;
  }
  for (qint32 i = 11; i < 48; i++)
  {
    if (plr->bet_array[i] && num == i - 11)
    {
      tmp = 36 * plr->bet_array[i];
      plr->ch->send(u"You WIN %u coins on your bet of %s!\r\n"_s.arg(tmp).arg(roulette_display[num]));
      winnings += tmp;
    }
  }
  return winnings;
}

void send_roulette_message(wheel_data *wheel)
{
  QString buf;

  const QStringList introbuf = {
      "A silver blur rounds the outside of the wheel as it spins.\r\n",
      "The distinct sound of the metal ball rounding the wheel is heard.\r\n",
  };
  const QStringList middlebuf = {
      "The ball bounces off of the",
      "The ball caroms off of the metal part of the",
      "The ball makes a clacking noise as it hits the",
      "Clacking noises fill the air as the ball hits the",
  };
  const QStringList endbuf = {
      "and immediately bounces out.\r\n",
      "and bounces almost straight up in the air.\r\n",
      "then short-hops a couple of spaces.\r\n",
      "and gets knocked slightly backwards.\r\n",
  };

  if (wheel->countdown >= 2)
    send_to_room(introbuf[number(0, 1)], wheel->obj->in_room);
  else
  {
    dc_sprintf(buf, "%s %s %s", middlebuf[number(0, 3)], roulette_display[number(0, 36)], endbuf[number(0, 3)]);
    send_to_room(buf, wheel->obj->in_room);
  }
}

void wheel_stop(wheel_data *wheel)
{
  qint32 num = dc_->number(0, 36);
  quint32 payout = {};
  QString buf;

  dc_sprintf(buf, "The ball lands on %s!\r\n", roulette_display[num]);
  send_to_room(buf, wheel->obj->in_room);

  for (qint32 i = {}; i < 6; i++)
  {
    if (charExists(wheel->plr[i]->ch))
    {
      if (wheel->plr[i]->ch->in_room == wheel->obj->in_room)
      {
        if ((payout = check_roulette_wins(wheel->plr[i], num)))
        {
          if (payout > 1000000)
            act_to_room("$n is a BIG winner!", wheel->plr[i]->ch, 0, 0, 0);
          else
            act_to_room("$n is a winner!", wheel->plr[i]->ch, 0, 0, 0);
          wheel->plr[i]->ch->addGold(payout);
        }
        else
        {
          send_to_char("The croupier gathers your money.\r\n", wheel->plr[i]->ch);
        }
      }
    }
    for (qint32 j = {}; j < 48; j++)
      wheel->plr[i]->bet_array[j] = {};
    wheel->plr[i]->ch = {};
  }
  wheel->spinning = false;
  wheel->countdown = 11;
}

void pulse_countdown(varg_t arg1, void *arg2, void *arg3);

void roulette_timer(wheel_data *wheel, qint32 spin)
{
  TimerPtr timer = TimerPtr(new Timer);
  timer->arg1.wheel = wheel;
  timer->arg2 = (void *)(qint64)spin;
  timer->function = pulse_countdown;
  timer->timeleft = 4;
  addtimer(timer);
}

void pulse_countdown(varg_t arg1, void *arg2, void *arg3)
{
  wheel_data *wheel = arg1.wheel;
  qint32 spin = (qint64)arg2;
  QString buf;

  if (wheel->countdown <= 0 && !spin)
  {
    wheel->spinning = true;
    send_to_room("The croupier places the ball on the wheel and spins both objects....\r\n", wheel->obj->in_room);
    wheel->countdown = 2;
    roulette_timer(wheel, 1);
  }
  else if (!spin)
  {
    if (!number(0, 3))
    {
      dc_sprintf(buf, "$B$7The croupier says 'The wheel will be spun in about %d seconds!'$R\r\n", wheel->countdown * 2);
      send_to_room(buf, wheel->obj->in_room);
    }
    wheel->countdown -= 1;
    roulette_timer(wheel, 0);
  }
  else if (wheel->countdown < 0)
  {
    wheel_stop(wheel);
  }
  else
  {
    send_roulette_message(wheel);
    wheel->countdown -= 1;
    roulette_timer(wheel, 1);
  }
}

qint32 roulette_table(CharacterPtr ch, ObjectPtr obj, cmd_t cmd, QString arg, CharacterPtr invoker)
{
  QString arg1, arg2, buf;
  quint32 bet = {};
  qint32 i = {};
  bool playing = false;
  arg = one_argument(arg, arg1);
  arg = one_argument(arg, arg2);
  if (cmd != cmd_t::BET)
    return ReturnValue::eFAILURE;
  if (!ch || ch->isNonPlayer())
    return ReturnValue::eFAILURE; // craziness
  if (obj->in_room <= 0)
    return ReturnValue::eFAILURE;
  if (!obj->wheel)
    create_wheel(obj);
  if (ch->isPlayerCantQuit() ||
      ch->isPlayerObjectThief() ||
      ch->isPlayerGoldThief())
  {
    ch->sendln("You cannot play roulette while you are flagged as naughty.");
    return ReturnValue::eSUCCESS;
  }

  if (obj->wheel->spinning)
  {
    ch->sendln("No bets may be placed while the wheel is spinning.");
    return ReturnValue::eSUCCESS;
  }

  for (i = {}; i < 6; i++)
  {
    if (obj->wheel->plr[i]->ch == ch)
    {
      playing = true;
      break;
    }
  }

  if (!playing)
  {
    for (i = {}; i < 7; i++)
    {
      if (i == 6)
      {
        ch->sendln("You cannot muscle your way to the table.");
        return ReturnValue::eSUCCESS;
      }
      if (obj->wheel->plr[i]->ch && charExists(obj->wheel->plr[i]->ch) && obj->wheel->plr[i]->ch->in_room != obj->in_room)
        obj->wheel->plr[i]->ch = {};
      else if (obj->wheel->plr[i]->ch && !charExists(obj->wheel->plr[i]->ch))
        obj->wheel->plr[i]->ch = {};
      if (!obj->wheel->plr[i]->ch)
      {
        obj->wheel->plr[i]->ch = ch;
        break;
      }
    }
  }

  if (cmd == cmd_t::BET) // bet
  {
    if (arg.isEmpty() 2)
    {
      ch->sendln("Syntax: Bet <keyword>/<range (1-12)>/<number>  <amount>");
      return ReturnValue::eSUCCESS;
    }
    if (!is_number(arg2) || dc_atoi(arg2) < 100)
    {
      ch->sendln("You must bet an amount greater than 100 coins.");
      return ReturnValue::eSUCCESS;
    }
    else
      bet = dc_atoi(arg2);
    if (bet > 20000000)
    {
      ch->sendln("The maximum bet is 20 million coins.");
      bet = 20000000;
    }
    if (ch->getGold() < bet)
    {
      ch->sendln("You do not have enough money to place that bet.");
      return ReturnValue::eSUCCESS;
    }
    else
      ch->removeGold(bet);
    if (!str_cmp(arg1, "black"))
    {
      if (obj->wheel->plr[i]->bet_array[0])
      {
        if (obj->wheel->plr[i]->bet_array[0] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B$0BLACK$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[0] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B$0BLACK$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[0] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B$0BLACK$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B$0BLACK$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[0] += bet;
    }
    else if (!str_cmp(arg1, "red"))
    {
      if (obj->wheel->plr[i]->bet_array[1])
      {
        if (obj->wheel->plr[i]->bet_array[1] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B$4RED$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[1] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B$4RED$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[1] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B$4RED$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B$4RED$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[1] += bet;
    }
    else if (!str_cmp(arg1, "even"))
    {
      if (obj->wheel->plr[i]->bet_array[2])
      {
        if (obj->wheel->plr[i]->bet_array[2] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $BEVEN$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[2] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $BEVEN$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[2] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $BEVEN$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $BEVEN$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[2] += bet;
    }
    else if (!str_cmp(arg1, "odd"))
    {
      if (obj->wheel->plr[i]->bet_array[3])
      {
        if (obj->wheel->plr[i]->bet_array[3] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $BODD$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[3] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $BODD$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[3] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $BODD$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $BODD$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[3] += bet;
    }
    else if (!str_cmp(arg1, "1-12"))
    {
      if (obj->wheel->plr[i]->bet_array[4])
      {
        if (obj->wheel->plr[i]->bet_array[4] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B1-12$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[4] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B1-12$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[4] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B1-12$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B1-12$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[4] += bet;
    }
    else if (!str_cmp(arg1, "13-24"))
    {
      if (obj->wheel->plr[i]->bet_array[5])
      {
        if (obj->wheel->plr[i]->bet_array[5] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B13-24$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[5] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B13-24$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[5] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B13-24$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B13-24$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[5] += bet;
    }
    else if (!str_cmp(arg1, "25-36"))
    {
      if (obj->wheel->plr[i]->bet_array[6])
      {
        if (obj->wheel->plr[i]->bet_array[6] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B25-36$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[6] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B25-36$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[6] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B25-36$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B25-36$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[6] += bet;
    }
    else if (!str_cmp(arg1, "1-9"))
    {
      if (obj->wheel->plr[i]->bet_array[7])
      {
        if (obj->wheel->plr[i]->bet_array[7] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B1-9$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[1] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B1-9$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[7] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B1-9$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B1-9$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[7] += bet;
    }
    else if (!str_cmp(arg1, "10-18"))
    {
      if (obj->wheel->plr[i]->bet_array[8])
      {
        if (obj->wheel->plr[i]->bet_array[8] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B10-18$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[8] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B10-18$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[7] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B10-18$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B10-18$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[8] += bet;
    }
    else if (!str_cmp(arg1, "19-27"))
    {
      if (obj->wheel->plr[i]->bet_array[9])
      {
        if (obj->wheel->plr[i]->bet_array[9] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B19-27$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[9] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B19-27$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[9] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B19-27$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B19-27$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[9] += bet;
    }
    else if (!str_cmp(arg1, "28-36"))
    {
      if (obj->wheel->plr[i]->bet_array[10])
      {
        if (obj->wheel->plr[i]->bet_array[10] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on $B28-36$R.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(obj->wheel->plr[i]->bet_array[10] + bet));
          dc_sprintf(buf, "$n adds to $s bet on $B28-36$R for a total of %u coins.", obj->wheel->plr[i]->bet_array[10] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %1 on $B28-36$R.\r\n"_s.arg(bet));
        dc_sprintf(buf, "$n places a bet of %u on $B28-36$R.", bet);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[10] += bet;
    }
    else if (is_number(arg1) && dc_atoi(arg1) >= 0 && dc_atoi(arg1) <= 36)
    {
      qint32 number = dc_atoi(arg1);
      if (obj->wheel->plr[i]->bet_array[number + 11])
      {
        if (obj->wheel->plr[i]->bet_array[number + 11] + bet > 20000000)
        {
          ch->addGold(bet);
          bet = {};
          ch->sendln("That bet would put you over the 20 million coin limit.");
        }
        else
        {
          ch->send(u"You add %u to your bet on %s.  The total bet is now %u coins.\r\n"_s.arg(bet).arg(roulette_display[number]).arg(obj->wheel->plr[i]->bet_array[number + 11] + bet));
          dc_sprintf(buf, "$n adds to $s bet on %s for a total of %u coins.",
                     roulette_display[number], obj->wheel->plr[i]->bet_array[number + 11] + bet);
          act_to_room(buf, ch, 0, 0, 0);
        }
      }
      else
      {
        ch->send(u"You have placed a bet of %u on %s.\r\n"_s.arg(bet).arg(roulette_display[number]));
        dc_sprintf(buf, "$n places a bet of %u on %s.", bet, roulette_display[number]);
        act_to_room(buf, ch, 0, 0, 0);
      }
      obj->wheel->plr[i]->bet_array[number + 11] += bet;
    }
    else
    {
      ch->sendln("Bet what?");
      ch->addGold(bet);
      return ReturnValue::eSUCCESS;
    }
    send_wheel_bets(ch, obj->wheel);
    if (obj->wheel->countdown == 11)
    {
      send_to_room("$BThe croupier says 'The first bet has been placed!'$R\r\n", obj->in_room);
      obj->wheel->countdown = 10;
      roulette_timer(obj->wheel, 0);
    }
  }

  return ReturnValue::eSUCCESS;
}

/* End Roulette */
