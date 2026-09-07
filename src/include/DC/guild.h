#pragma once
/*
 * guild.h
 *
 *  Created on: Feb 21, 2012
 *      Author: jhhudso
 */

class Character;
class class_skill_defines;
enum class cmd_t;

int search_skills(char *arg, class_skill_defines *list_skills);
int search_skills2(int arg, class_skill_defines *list_skills);
int guild(Character *ch, class Object *obj, cmd_t cmd, const char *arg, Character *owner);
