/*
 * guild.h
 *
 *  Created on: Feb 21, 2012
 *      Author: jhhudso
 */

#ifndef GUILD_H_
#define GUILD_H_

int get_max(CHAR_DATA *ch, int skill);
int guild(struct char_data *ch, struct obj_data *obj, int cmd, char *arg, struct char_data *owner);
int learn_skill(char_data * ch, int skill, int amount, int maximum);

#endif /* GUILD_H_ */
