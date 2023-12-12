/*
 * guild.h
 *
 *  Created on: Feb 21, 2012
 *      Author: jhhudso
 */

#ifndef GUILD_H_
#define GUILD_H_

#include "const.h"

int search_skills(char *arg, class_skill_defines *list_skills);
int search_skills2(int arg, class_skill_defines *list_skills);
int guild(Character *ch, class Object *obj, int cmd, const char *arg, Character *owner);

#endif /* GUILD_H_ */
