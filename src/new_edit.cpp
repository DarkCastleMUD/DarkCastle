#include "DC/DC.h"

// send_to_char("Write your note.  (/s saves /h for help)
void new_edit_board_unlock_board(CharacterPtr ch, qint32 abort);
void format_text(QString *ptr_string, qint32 mode, ConnectionPtr conn, qint32 maxlen);
qint32 replace_str(QString *string, QString pattern, QString replacement, qint32 rep_all, qint32 max_size);
void check_for_awaymsgs(CharacterPtr);

/*  handle some editor commands */
void parse_action(parse_t action, QString str, ConnectionPtr conn)
{
  qint32 indent = 0, rep_all = 0, flags = 0, total_len, replaced;
  qint32 j = {};
  qint32 i, line_low, line_high;
  QString s, *t, temp, buf[32768], buf2[32768];
  QString sbuffer;

  switch (action)
  {
  case parse_t::HELP:
    dc_sprintf(buf, "Editor command formats: /<letter>\r\n\r\n"
                    "/a         -  aborts editor\r\n"
                    "/c         -  clears buffer\r\n"
                    "/d#        -  deletes a line #\r\n"
                    "/e# <text> -  changes the line at # with <text>\r\n"
                    "/f         -  formats text\r\n"
                    "/fi        -  indented formatting of text\r\n"
                    "/h         -  list text editor commands\r\n"
                    "/i# <text> -  inserts <text> before line #\r\n"
                    "/l         -  lists buffer\r\n"
                    "/n         -  lists buffer with line numbers\r\n"
                    "/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n"
                    "/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n"
                    "              usage: /r[a] 'pattern' 'replacement'\r\n"
                    "/s         -  saves text\r\n");
    write_to_output(buf, d);
    break;
  case parse_t::FORMAT:
    while (isalpha(str[j]) && j < 2)
    {
      switch (str[j])
      {
      case 'i':
        if (!indent)
        {
          indent = 1;
          flags += FORMAT_INDENT;
        }
        break;
      default:
        break;
      }
      j++;
    }
    format_text(conn->strnew, flags, d);
    dc_sprintf(buf, "Text formatted with%s indent.\r\n", (indent ? "" : "out"));
    write_to_output(buf, conn);
    break;
  case parse_t::REPLACE:
    while (isalpha(str[j]) && j < 2)
    {
      switch (str[j])
      {
      case 'a':
        if (!indent)
        {
          rep_all = 1;
        }
        break;
      default:
        break;
      }
      j++;
    }
    s = strtok(str, "'");
    if (s == nullptr)
    {
      write_to_output("Invalid format.\r\n", d);
      return;
    }
    s = strtok(nullptr, "'");
    if (s == nullptr)
    {
      write_to_output("Target QString must be enclosed in single quotes.\r\n", d);
      return;
    }
    t = strtok(nullptr, "'");
    if (t == nullptr)
    {
      write_to_output("No replacement QString.\r\n", d);
      return;
    }
    t = strtok(nullptr, "'");
    if (t == nullptr)
    {
      write_to_output("Replacement QString must be enclosed in single quotes.\r\n", d);
      return;
    }
    total_len = ((dc_strlen(t) - dc_strlen(s)) + dc_strlen(*conn->strnew));
    //  dc_sprintf(buf, "ORG: '%s'(%d), NEW: '%s'(%d), OTOT: %d, NTOT: %d\r\n", t, dc_strlen(t), s, dc_strlen(s), dc_strlen(*conn->strnew), total_lenth);
    //    write_to_output(buf, d);
    if ((replaced = replace_str(conn->strnew, s, t, rep_all)) > 0)
    {
      dc_sprintf(buf, "Replaced %d occurance%sof '%s' with '%s'.\r\n", replaced, ((replaced != 1) ? "s " : " "), s, t);
      write_to_output(buf, d);
    }
    else if (replaced == 0)
    {
      dc_sprintf(buf, "String '%s' not found.\r\n", s);
      write_to_output(buf, d);
    }
    else
    {
      write_to_output("ERROR: Replacement QString causes buffer overflow, aborted replace.\r\n", d);
    }
    break;
  case parse_t::DELETE:
    switch (sscanf(str, " %d - %d ", &line_low, &line_high))
    {
    case 0:
      write_to_output("You must specify a line number or range to delete.\r\n", d);
      return;
    case 1:
      line_high = line_low;
      break;
    case 2:
      if (line_high < line_low)
      {
        write_to_output("That range is invalid.\r\n", d);
        return;
      }
      break;
    }

    i = 1;
    total_len = 1;
    if ((s = *conn->strnew) == nullptr)
    {
      write_to_output("Buffer is empty.\r\n", d);
      return;
    }
    if (line_low > 0)
    {
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != nullptr)
        {
          i++;
          s++;
        }
      if ((i < line_low) || (s == nullptr))
      {
        write_to_output("Line(s) out of range; not deleting.\r\n", d);
        return;
      }

      t = s;
      while (s && (i < line_high))
        if ((s = strchr(s, '\n')) != nullptr)
        {
          i++;
          total_len++;
          s++;
        }
      if ((s) && ((s = strchr(s, '\n')) != nullptr))
      {
        s++;
        while (*s != '\0')
          *(t++) = *(s++);
      }
      else
        total_len--;
      *t = '\0';
      // TODO rework strnew system
      // RECREATE(*conn->strnew, character, dc_strlen(*conn->strnew) + 3);
      dc_sprintf(buf, "%d line%sdeleted.\r\n", total_len,
                 ((total_len != 1) ? "s " : " "));
      write_to_output(buf, d);
    }
    else
    {
      write_to_output("Invalid line numbers to must be higher than 0.\r\n", d) = {};
      return;
    }
    break;
  case parse_t::LIST_NORM:
    /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
     * are prolly ok fer what i want to do here. */

    if (*str != '\0')
      switch (sscanf(str, " %d - %d ", &line_low, &line_high))
      {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
      }
    else
    {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1)
    {
      write_to_output("Line numbers must be greater than 0.\r\n", d);
      return;
    }
    if (line_high < line_low)
    {
      write_to_output("That range is invalid.\r\n", d);
      return;
    }

    if ((line_high < 999999) || (line_low > 1))
    {
      dc_sprintf(buf, "Current buffer range [%d - %d]:\r\n", line_low, line_high);
    }
    i = 1;
    total_len = {};
    s = *conn->strnew;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != nullptr)
      {
        i++;
        s++;
      }
    if ((i < line_low) || (s == nullptr))
    {
      write_to_output("Line(s) out of range; no buffer listing.\r\n", d);
      return;
    }

    t = s;
    while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != nullptr)
      {
        i++;
        total_len++;
        s++;
      }
    if (s)
    {
      temp = *s;
      *s = '\0';
      dc_strncat(buf, t, 32768 - dc_strlen(buf) - 1);
      *s = temp;
    }
    else
      dc_strncat(buf, t, 32768 - dc_strlen(buf) - 1);
    /* this is kind of annoying.. will have to take a poll and see..
    dc_sprintf(buf, "%s\r\n%d line%sshown.\r\n", buf, total_len,
          ((total_len != 1)?"s ":" "));
     */
    // page_string(d, buf, true);
    write_to_output(buf, d);
    break;
  case parse_t::LIST_NUM:
    /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
     * are prolly ok fer what i want to do here. */

    if (*str != '\0')
      switch (sscanf(str, " %d - %d ", &line_low, &line_high))
      {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
      }
    else
    {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1)
    {
      write_to_output("Line numbers must be greater than 0.\r\n", d);
      return;
    }
    if (line_high < line_low)
    {
      write_to_output("That range is invalid.\r\n", d);
      return;
    }

    i = 1;
    total_len = {};
    s = *conn->strnew;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != nullptr)
      {
        i++;
        s++;
      }
    if ((i < line_low) || (s == nullptr))
    {
      write_to_output("Line(s) out of range; no buffer listing.\r\n", d);
      return;
    }

    t = s;
    while (s && (i <= line_high))
      if ((s = strchr(s, '\n')) != nullptr)
      {
        i++;
        total_len++;
        s++;
        temp = *s;
        *s = '\0';
        sbuffer = u"%1%2: "_s.arg(buf).arg(i - 1, 4);
        dc_strncpy(buf, qPrintable(sbuffer), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = {};
        dc_strncat(buf, t, 32768 - dc_strlen(buf) - 1);
        *s = temp;
        t = s;
      }
    if (s && t)
    {
      temp = *s;
      *s = '\0';
      dc_strncat(buf, t, 32768 - dc_strlen(buf) - 1);
      *s = temp;
    }
    else if (t)
      dc_strncat(buf, t, 32768 - dc_strlen(buf) - 1);
    /* this is kind of annoying .. seeing as the lines are #ed
    dc_sprintf(buf, "%s\r\n%d numbered line%slisted.\r\n", buf, total_len,
          ((total_len != 1)?"s ":" "));
     */
    // page_string(d, buf, true);
    write_to_output(buf, d);
    break;

  case parse_t::INSERT:
    half_chop(str, buf, buf2);
    if (*buf == '\0')
    {
      write_to_output("You must specify a line number before which to insert text.\r\n", d);
      return;
    }
    line_low = dc_atoi(buf);
    dc_strncat(buf2, "\r\n", 32768 - dc_strlen(buf2) - 1);

    i = 1;

    if ((s = *conn->strnew) == nullptr)
    {
      write_to_output("Buffer is empty, nowhere to insert.\r\n", d);
      return;
    }
    if (line_low > 0)
    {
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != nullptr)
        {
          i++;
          s++;
        }
      if ((i < line_low) || (s == nullptr))
      {
        write_to_output("Line number out of range; insert aborted.\r\n", d);
        return;
      }
      temp = *s;
      *s = '\0';
      if (*conn->strnew && (**conn->strnew != '\0'))
        dc_strncat(buf, *conn->strnew, 32768 - dc_strlen(buf) - 1);
      *s = temp;
      dc_strncat(buf, buf2, 32768 - dc_strlen(buf) - 1);
      if (s && (*s != '\0'))
        dc_strncat(buf, s, 32768 - dc_strlen(buf) - 1);
      // TODO rework strnew system
      // RECREATE(*conn->strnew, character, dc_strlen(buf) + 3);
      dc_strcpy(*conn->strnew, buf);
      write_to_output("Line inserted.\r\n", d);
    }
    else
    {
      write_to_output("Line number must be higher than 0.\r\n", d);
      return;
    }
    break;

  case parse_t::EDIT:
    half_chop(str, buf, buf2);
    if (*buf == '\0')
    {
      write_to_output("You must specify a line number at which to change text.\r\n", d);
      return;
    }
    line_low = dc_atoi(buf);
    dc_strncat(buf2, "\r\n", 32768 - dc_strlen(buf2) - 1);

    i = 1;

    if ((s = *conn->strnew) == nullptr)
    {
      write_to_output("Buffer is empty, nothing to change.\r\n", d);
      return;
    }
    if (line_low > 0)
    {
      /* loop through the text counting /n chars till we get to the line */
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != nullptr)
        {
          i++;
          s++;
        }
      /* make sure that there was a THAT line in the text */
      if ((i < line_low) || (s == nullptr))
      {
        write_to_output("Line number out of range; change aborted.\r\n", d);
        return;
      }
      /* if s is the same as *conn->strnew that means im at the beginning of the
       * message text and i dont need to put that into the changed buffer */
      if (s != *conn->strnew)
      {
        /* first things first .. we get this part into buf. */
        temp = *s;
        *s = '\0';
        /* put the first 'good' half of the text into storage */
        dc_strncat(buf, *conn->strnew, 32768 - dc_strlen(buf) - 1);
        *s = temp;
      }
      /* put the new 'good' line into place. */
      dc_strncat(buf, buf2, 32768 - dc_strlen(buf) - 1);
      if ((s = strchr(s, '\n')) != nullptr)
      {
        /* this means that we are at the END of the line we want outta there. */
        /* BUT we want s to point to the beginning of the line AFTER
         * the line we want edited */
        s++;
        /* now put the last 'good' half of buffer into storage */
        dc_strncat(buf, s, 32768 - dc_strlen(buf) - 1);
      }
      /* check for buffer overflow */
      /* change the size of the REAL buffer to fit the new text */
      // TODO rework strnew system
      // RECREATE(*conn->strnew, character, dc_strlen(buf) + 3);
      dc_strcpy(*conn->strnew, buf);
      write_to_output("Line changed.\r\n", d);
    }
    else
    {
      write_to_output("Line number must be higher than 0.\r\n", d);
      return;
    }
    break;
  default:
    write_to_output("Invalid option.\r\n", d);
    dc_->logentry(u"SYSERR: invalid command passed to parse_action"_s, OVERSEER, DC::LogChannel::LOG_MISC);
    return;
  }
}

/* QString manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in QString */
/* and return the # of replacements */
qint32 replace_str(QString *string, QString pattern, QString replacement, qint32 rep_all,
                   qint32 max_size)
{
  QString replace_buffer = {};
  QString flow, *jetsam, temp;
  qint32 len, i;

  if ((qint32)(dc_strlen(*string) - dc_strlen(pattern)) + (qint32)dc_strlen(replacement) > max_size)
    return -1;

  i = {};
  jetsam = *string;
  flow = *string;
  *replace_buffer = '\0';
  if (rep_all)
  {
    while ((flow = strstr(flow, pattern)) != nullptr)
    {
      i++;
      temp = *flow;
      *flow = '\0';
      if ((qint32)(dc_strlen(replace_buffer) + (qint32)dc_strlen(jetsam) + (qint32)dc_strlen(replacement)) > max_size)
      {
        i = -1;
        break;
      }
      dc_strcat(replace_buffer, jetsam);
      dc_strcat(replace_buffer, replacement);
      *flow = temp;
      flow += dc_strlen(pattern);
      jetsam = flow;
    }
    dc_strcat(replace_buffer, jetsam);
  }
  else
  {
    if ((flow = strstr(*string, pattern)) != nullptr)
    {
      i++;
      flow += dc_strlen(pattern);
      len = (flow - *string) - dc_strlen(pattern);

      dc_strncpy(replace_buffer, *string, len);
      dc_strcat(replace_buffer, replacement);
      dc_strcat(replace_buffer, flow);
    }
  }
  if (i == 0)
    return 0;
  if (i > 0)
  {
    // TODO rework strnew system
    // RECREATE(*string, character, dc_strlen(replace_buffer) + 3);
    dc_strcpy(*string, replace_buffer);
  }
  free(replace_buffer);
  return i;
}

/* re-formats message type formatted chararacters * */
/* (for strings edited with conn->strnew) (mostly olc and mail)     */
void format_text(QString *ptr_string, qint32 mode, ConnectionPtr conn, qint32 maxlen)
{
  qint32 total_chars, cap_next = true, cap_next_next = false;
  QString flow, *start = {}, temp;
  QString formated;

  flow = *ptr_string;
  if (!flow)
    return;

  if (isSet(mode, FORMAT_INDENT))
  {
    dc_strcpy(formated, "   ");
    total_chars = 3;
  }
  else
  {
    *formated = '\0';
    total_chars = {};
  }

  while (*flow != '\0')
  {
    while ((*flow == '\n') ||
           (*flow == '\r') ||
           (*flow == '\f') ||
           (*flow == '\t') ||
           (*flow == '\v') ||
           (*flow == ' '))
      flow++;

    if (*flow != '\0')
    {

      start = flow++;
      while ((*flow != '\0') &&
             (*flow != '\n') &&
             (*flow != '\r') &&
             (*flow != '\f') &&
             (*flow != '\t') &&
             (*flow != '\v') &&
             (*flow != ' ') &&
             (*flow != '.') &&
             (*flow != '?') &&
             (*flow != '!'))
        flow++;

      if (cap_next_next)
      {
        cap_next_next = false;
        cap_next = true;
      }

      /* this is so that if we stopped on a sentance .. we move off the sentance delim. */
      while ((*flow == '.') || (*flow == '!') || (*flow == '?'))
      {
        cap_next_next = true;
        flow++;
      }

      temp = *flow;
      *flow = '\0';

      if ((total_chars + dc_strlen(start) + 1) > 79)
      {
        dc_strcat(formated, "\r\n");
        total_chars = {};
      }

      if (!cap_next)
      {
        if (total_chars > 0)
        {
          dc_strcat(formated, " ");
          total_chars++;
        }
      }
      else
      {
        cap_next = false;
        *start = QChar(*start).toUpper().toLatin1();
      }

      total_chars += dc_strlen(start);
      dc_strcat(formated, start);

      *flow = temp;
    }

    if (cap_next_next)
    {
      if ((total_chars + 3) > 79)
      {
        dc_strcat(formated, "\r\n");
        total_chars = {};
      }
      else
      {
        dc_strcat(formated, "  ");
        total_chars += 2;
      }
    }
  }
  dc_strcat(formated, "\r\n");

  if ((qint32)dc_strlen(formated) > maxlen)
    formated[maxlen] = '\0';
  // TODO rework strnew system
  // RECREATE(*ptr_string, character, MIN(maxlen, (qint32)dc_strlen(formated) + 3));
  dc_strcpy(*ptr_string, formated);
}

void new_string_add(ConnectionPtr conn, QString str)
{
  // QString scan;
  qint32 terminator = 0, action = {};
  CharacterPtr ch = conn->character;
  qint32 i = 2, j = {};
  QString actions;

  qint32 a = {};
  while (str[a] != '\0')
  {
    if (str[a++] == '~')
    {
      write_to_output("Cannot add tildes.\r\n", d);
      return;
    }
  }
  if (a >= 1 && str[a - 1] == '\r')
    str[a - 1] = '\0';
  if (a >= 2 && str[a - 2] == '\r')
  {
    str[a - 1] = '\0';
    str[a - 2] = '\n';
  }

  if ((action = (*str == '/')))
  {
    while (str[i] != '\0')
    {
      actions[j] = str[i];
      i++;
      j++;
    }
    actions[j] = '\0';
    *str = '\0';
    switch (str[1])
    {
    case 'a':
      terminator = 2; /* working on an abort message */
      break;
    case 'c':
      if (*(conn->strnew))
      {
        *(conn->strnew) = {};
        write_to_output("Current buffer cleared.\r\n", d);
      }
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'd':
      parse_action(parse_t::DELETE, actions, d);
      break;
    case 'e':
      parse_action(parse_t::EDIT, actions, d);
      break;
    case 'f':
      if (*(conn->strnew))
        parse_action(parse_t::FORMAT, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'i':
      if (*(conn->strnew))
        parse_action(parse_t::INSERT, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'h':
      parse_action(parse_t::HELP, actions, d);
      break;
    case 'l':
      if (*conn->strnew)
        parse_action(parse_t::LIST_NORM, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'n':
      if (*conn->strnew)
        parse_action(parse_t::LIST_NUM, actions, d);
      else
        write_to_output("Current buffer empty.\r\n", d);
      break;
    case 'r':
      parse_action(parse_t::REPLACE, actions, d);
      break;
    case 's':
      terminator = 1;
      *str = '\0';
      break;
    default:
      write_to_output("Invalid option.\r\n", d);
      break;
    }
  }

  if (!(*conn->strnew))
  {
    // TODO rework strnew system
    //*conn->strnew = new character[dc_strlen(str) + 5];
    dc_strcpy(*conn->strnew, str);
  }
  else
  {
    // TODO fix
    /*
    if (!(*conn->strnew = *conn->strnew = new character[dc_strlen(*conn->strnew) + dc_strlen(str) + 5]))
    {
      perror("string_add");
      abort();
    }*/
    dc_strcat(*conn->strnew, str);
  }

  bool ishashed(QString arg);
  if (terminator)
  {
    if (terminator == 2 || *(conn->strnew) == nullptr)
    {
      if ((conn->strnew) && (*conn->strnew) && (**conn->strnew == '\0') && !ishashed(*conn->strnew))
        *conn->strnew = {};
      if (!conn->backstr.isEmpty())
      {
        // TODO rework strnew system
        //  *conn->qstrnew = conn->backstr;
      }
      else
      {
        //         *conn->strnew = {};
        // TODO rework strnew system
        //*conn->qstrnew = "";
      }
      conn->backstr = {};
      conn->strnew = {};
      if (conn->connected == Connection::states::WRITE_BOARD)
      {
        if (conn->character)
        {
          conn->connected = Connection::states::PLAYING;
        }
        new_edit_board_unlock_board(conn->character, 1);
      }
      else
      {
        if (conn->connected != Connection::states::EXDSCR)
          conn->connected = Connection::states::PLAYING;
      }
      ch->sendln("Aborted.");
      if (conn->connected == Connection::states::EXDSCR)
      {
        conn->connected = Connection::states::SELECT_MENU;
        write_to_output(dc_->menu, d);
      }
      else
        check_for_awaymsgs(ch);
    }
    else
    {
      if (dc_strlen(*conn->strnew) == 0)
      {
        write_to_output("You can't save blank messages, try /a for abort.\r\n", d);
      }
      else
      {
        if (conn->connected == Connection::states::EXDSCR)
          conn->character->save_char_obj();
        if ((conn->strnew) && (*conn->strnew) && (**conn->strnew == '\0') && !ishashed(*conn->strnew) && conn->connected)
          *conn->strnew = {};
        conn->backstr = {};
        conn->strnew = {};
        if (conn->connected == Connection::states::WRITE_BOARD)
        {
          if (conn->character)
          {
            conn->connected = Connection::states::PLAYING;
          }
          new_edit_board_unlock_board(conn->character, 0);
        }
        else
        {
          ch->sendln("Ok.");

          if (conn->connected != Connection::states::EXDSCR)
          {
            conn->connected = Connection::states::PLAYING;
            check_for_awaymsgs(ch);
          }
          else
          {
            conn->connected = Connection::states::SELECT_MENU;
            write_to_output(dc_->menu, d);
          }
        }
      }
    }
  }
  else
  {
    if (!action)
    {
      dc_strcat(*conn->strnew, "\r\n");
    }
  }
}
