#pragma once
#include "DC/common.h"
class Character;
int attempt_move(Character *ch, cmd_t cmd, int is_retreat = 0);
int ambush(Character *ch);