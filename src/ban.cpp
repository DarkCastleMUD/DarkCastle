#include <arpa/inet.h>
#include <cstring>
#include <fmt/format.h>
#include <fmt/chrono.h>

#include "DC/player.h"
#include "DC/levels.h"
#include "DC/structs.h"
#include "DC/character.h"
#include "DC/utility.h"
#include "DC/comm.h"
#include "DC/interp.h"
#include "DC/handler.h"
#include "DC/db.h"
#include "DC/returnvals.h"
#include "DC/spells.h"

struct ban_list_element *ban_list = nullptr;

const char *ban_types[] = {
    "no",
    "new",
    "select",
    "all",
    "ERROR"};

void load_banned(void)
{
  FILE *fl;
  int i, date;
  char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
  char name[100 + 1];
  struct ban_list_element *next_node;

  ban_list = 0;

  if (!(fl = fopen("banned", "r")))
  {
    perror("Unable to open banfile 'banned'");
    return;
  }
  while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4)
  {
    CREATE(next_node, struct ban_list_element, 1);
    strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);
    next_node->site[BANNED_SITE_LENGTH] = '\0';
    strncpy(next_node->name, name, 100);
    next_node->name[99] = '\0';
    next_node->date = date;

    for (i = BAN_NOT; i <= BAN_ALL; i++)
      if (!strcmp(ban_type, ban_types[i]))
        next_node->type = i;

    next_node->next = ban_list;
    ban_list = next_node;
  }

  fclose(fl);
}

void DC::free_ban_list_from_memory(void)
{
  ban_list_element *next = nullptr;

  for (; ban_list; ban_list = next)
  {
    next = ban_list->next;
    dc_free(ban_list);
  }
}

int isbanned(QHostAddress address)
{
  QString hostname = address.toString();

  if (hostname.isEmpty())
  {
    return 0;
  }

  hostname = hostname.trimmed().toLower();

  int i = 0;
  for (struct ban_list_element *banned_node = ban_list; banned_node; banned_node = banned_node->next)
  {
    if (hostname == banned_node->site) /* if hostname is a substring */
    {
      i = MAX(i, banned_node->type);
    }
  }

  return i;
}

void _write_one_node(FILE *fp, struct ban_list_element *node)
{
  if (node)
  {
    _write_one_node(fp, node->next);
    fprintf(fp, "%s %s %d %s\n", ban_types[node->type], node->site, (int32_t)node->date, node->name);
  }
}

void write_ban_list(void)
{
  FILE *fl;

  if (!(fl = fopen("banned", "w")))
  {
    perror("write_ban_list");
    return;
  }
  _write_one_node(fl, ban_list); /* recursively write from end to start */
  fclose(fl);
  return;
}

int do_ban(Character *ch, char *argument, int cmd)
{
  char flag[MAX_INPUT_LENGTH], format[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH], *nextchar;
  int i;
  char buf[MAX_STRING_LENGTH];
  struct ban_list_element *ban_node;
  std::string buffer;

  *buf = '\0';

  if (!*argument)
  {
    if (!ban_list)
    {
      ch->sendln("No sites are banned.");
      return eSUCCESS;
    }
    strcpy(format, "%-15.15s  %-8.8s  %-19s  %-16.16s\r\n");
    sprintf(buf, format,
            "Banned Site Name",
            "Ban Type",
            "Banned On",
            "Banned By");
    ch->send(buf);
    sprintf(buf, format,
            "-----------------------",
            "---------------------------------",
            "-------------------",
            "---------------------------------");
    ch->send(buf);

    for (ban_node = ban_list; ban_node; ban_node = ban_node->next)
    {
      if (ban_node->date)
      {
        buffer = fmt::format("{:%Y-%m-%d %H:%M:%S}", *std::localtime(&(ban_node->date)));
      }
      else
      {
        buffer = "Unknown";
      }

      csendf(ch, format, ban_node->site, ban_types[ban_node->type], buffer.c_str(), ban_node->name);
    }
    return eSUCCESS;
  }
  half_chop(argument, flag, site);
  if (!*site || !*flag)
  {
    ch->sendln("Usage: ban {all | select | new} site_name");
    return eSUCCESS;
  }

  struct sockaddr_in sa;
  if (inet_pton(AF_INET, site, &(sa.sin_addr)) == 0)
  {
    ch->sendln("Invalid IP address.");
    return eFAILURE;
  }

  if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all") || !str_cmp(flag, "new")))
  {
    ch->sendln("Flag must be ALL, SELECT, or NEW.");
    return eSUCCESS;
  }
  for (ban_node = ban_list; ban_node; ban_node = ban_node->next)
  {
    if (!str_cmp(ban_node->site, site))
    {
      ch->sendln("That site has already been banned -- unban it to change the ban type.");
      return eSUCCESS;
    }
  }

  CREATE(ban_node, struct ban_list_element, 1);
  strncpy(ban_node->site, site, BANNED_SITE_LENGTH);
  for (nextchar = ban_node->site; *nextchar; nextchar++)
    *nextchar = LOWER(*nextchar);
  ban_node->site[BANNED_SITE_LENGTH] = '\0';
  strncpy(ban_node->name, GET_NAME(ch), 100);
  ban_node->name[99] = '\0';
  ban_node->date = time(0);

  for (i = BAN_NEW; i <= BAN_ALL; i++)
    if (!str_cmp(flag, ban_types[i]))
      ban_node->type = i;

  ban_node->next = ban_list;
  ban_list = ban_node;

  sprintf(buf, "%s has banned %s for %s players.", GET_NAME(ch), site,
          ban_types[ban_node->type]);
  logentry(buf, POWER, DC::LogChannel::LOG_GOD);
  ch->sendln("Site banned.");
  write_ban_list();
  return eSUCCESS;
}

int do_unban(Character *ch, char *argument, int cmd)
{
  char site[MAX_INPUT_LENGTH + 1];
  struct ban_list_element *ban_node, *temp;
  int found = 0;
  char buf[MAX_STRING_LENGTH];

  one_argument(argument, site);
  if (!*site)
  {
    ch->sendln("A site to unban might help.");
    return eSUCCESS;
  }
  ban_node = ban_list;
  while (ban_node && !found)
  {
    if (!str_cmp(ban_node->site, site))
      found = 1;
    else
      ban_node = ban_node->next;
  }

  if (!found)
  {
    ch->sendln("That site is not currently banned.");
    return eSUCCESS;
  }
  REMOVE_FROM_LIST(ban_node, ban_list, next);
  ch->sendln("Site unbanned.");
  sprintf(buf, "%s removed the %s-player ban on %s.",
          GET_NAME(ch), ban_types[ban_node->type], ban_node->site);
  logentry(buf, POWER, DC::LogChannel::LOG_GOD);

  dc_free(ban_node);
  write_ban_list();
  return eSUCCESS;
}
