#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <set>
#include <vector>
#include <unordered_map>
#include <netinet/in.h>

#include <QMap>

#include "Trace.h"

using special_function = int (*)(class Character *, class Object *, int, const char *, class Character *);

typedef uint64_t zone_t;
typedef uint64_t room_t;
typedef uint64_t gold_t;
typedef uint64_t vnum_t;
typedef uint8_t level_t;

typedef std::map<vnum_t, special_function> special_function_list_t;

typedef std::set<class Character *> character_list_t;
typedef std::set<class Object *> obj_list_t;
typedef std::set<int> client_descriptor_list_t;
typedef std::set<int> server_descriptor_list_t;
typedef std::set<Character *>::iterator character_list_i;
typedef std::set<int>::iterator client_descriptor_list_i;
typedef std::set<int>::iterator server_descriptor_list_i;

typedef std::vector<in_port_t>::iterator port_list_i;
typedef std::vector<in_port_t> port_list_t;

typedef std::unordered_map<class Character *, Trace> death_list_t;
typedef std::unordered_map<class Character *, Trace> free_list_t;

// class Zone;
typedef QMap<zone_t, class Zone> zones_t;
typedef QMap<QString, bool> joining_t;
typedef QList<QString> hints_t;

#endif