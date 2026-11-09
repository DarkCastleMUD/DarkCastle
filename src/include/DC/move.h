#pragma once
#include "DC/common.h"

class Character;
enum class cmd_t;

int attempt_move(CharacterPtr ch, cmd_t cmd, int is_retreat = 0);
int ambush(CharacterPtr ch);