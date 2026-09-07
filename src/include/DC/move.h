#pragma once
#include "DC/common.h"

class Character;
enum class cmd_t;

int attempt_move(Character *ch, cmd_t cmd, int is_retreat = 0);
int ambush(Character *ch);