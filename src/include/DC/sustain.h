#pragma once

#include <QList>

#include "DC/types.h"

constexpr int SUSTAIN_RESERVE_MULTIPLIER = 2;
constexpr int SUSTAIN_DELAY_TICKS = 5;

int sustain_interval(int skill);
QList<int> sustained_spells(CharacterPtr ch);
QList<int> sustained_effects(CharacterPtr ch);
int sustain_reserve(CharacterPtr ch);
void sustain_delay(CharacterPtr ch);
void sustain_update(void);
