
/* action modes for parse_action */

enum class parse_t
{
    FORMAT,    // 0
    REPLACE,   // 1
    HELP,      // 2
    DELETE,    // 3
    INSERT,    // 4
    LIST_NORM, // 5
    LIST_NUM,  // 6
    EDIT       // 7
};

/* misc editor defines **************************************************/

/* format modes for format_text */
#define FORMAT_INDENT (1 << 0)

void parse_action(parse_t action, char *string, class Connection *d);