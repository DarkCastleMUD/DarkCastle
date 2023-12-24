#pragma once

class player_data
{
public:
    player_data *next;
    class table_data *table;
    class Character *ch;
    int hand_data[21];
    // theoretical cardmax is lower than 21, but whatever
    int bet;
    bool insurance;
    bool doubled;
    int state;
};

class cDeck
{
public:
    table_data *table;
    int *cards;
    int pos;
    int decks;
};

class table_data
{
public:
    class Object *obj; // linked to obj
    cDeck *deck;
    player_data *plr;
    player_data *cr; // current
    bool gold;
    int options;
    class Character *dealer;
    int hand_data[21]; // dealer
    int handnr;
    int state;
    int won;
    int lost;
};
