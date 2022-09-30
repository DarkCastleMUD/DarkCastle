/*
 * guild.h
 *
 *  Created on: Feb 21, 2012
 *      Author: jhhudso
 */

#ifndef GUILD_H_
#define GUILD_H_

#include "const.h"

int get_max(struct char_data *ch, int skill);
int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *owner);
bool learn_skill(struct char_data * ch, int skillnum, int amount, int maximum);
class_skill_defines * get_skill_list(struct char_data * ch);
int search_skills(char * arg, class_skill_defines * list_skills);
int search_skills2(int arg, class_skill_defines * list_skills);

#endif /* GUILD_H_ */
