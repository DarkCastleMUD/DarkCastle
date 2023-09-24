/***************************************************************************
| comm.h: New comm stuff to make the rewrite of comm.C work properly
*/
#ifndef __COMM_H__
#define __COMM_H__

#include <string>

using namespace std;
typedef int socket_t;

#define SEND_TO_Q(messg, desc) write_to_output((messg), desc)
#ifndef WIN32
#define CLOSE_SOCKET(sock) close(sock)
#else
#define CLOSE_SOCKET(sock) closesocket(sock)
#endif

#define SMALL_BUFSIZE 1024
#define LARGE_BUFSIZE (24 * 2048)
#define GARBAGE_SPACE 32
#define NUM_RESERVED_DESCS 8
#define HOST_LENGTH 30

enum pulse_type
{
	TIMER,
	MOBILE,
	OBJECT,
	VIOLENCE,
	BARD,
	TENSEC,
	WEATHER,
	TIME,
	REGEN,
	SHORT
};

struct pulse_info
{
	pulse_type pulse;
	uint64_t duration;
	char name[];
};

#define BAN_NOT 0
#define BAN_NEW 1
#define BAN_SELECT 2
#define BAN_ALL 3

#include "character.h"

// void     write_to_output(const char *txt, class Connection *d);
void write_to_output(string txt, class Connection *d);
void scramble_text(char *txt);
QString scramble_text(QString txt);
void warn_if_duplicate_ip(Character *ch);
void record_msg(string messg, Character *ch);
int write_hotboot_file(char **argv);
void send_info(QString messg);
void send_info(string messg);
void send_info(const char *messg);
bool is_multi(Character *ch);
void new_string_add(class Connection *d, char *str);
void telnet_ga(Connection *d);
void telnet_sga(Connection *d);
void telnet_echo_off(class Connection *d);
void telnet_echo_on(class Connection *d);
string remove_non_color_codes(string input);

#endif
