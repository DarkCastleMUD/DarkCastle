/*
 * guild.h
 *
 *  Created on: Feb 21, 2012
 *      Author: jhhudso
 */

#ifndef GUILD_H_
#define GUILD_H_

#include "const.h"

int get_max(Character *ch, int skill);
int guild(Character *ch, class Object *obj, int cmd, char *arg, Character *owner);
bool learn_skill(Character *ch, int skillnum, int amount, int maximum);
int search_skills(char *arg, class_skill_defines *list_skills);
int search_skills2(int arg, class_skill_defines *list_skills);

#endif /* GUILD_H_ */
