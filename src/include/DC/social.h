#ifndef SOCIAL_H
#define SOCIAL_H
#include <QList>
#include <QMap>

class Social
{
public:
    QString name_;
    /* No argument was supplied */
    QString char_no_arg_;
    QString others_no_arg_;
    /* An argument was there, and a victim was found */
    QString char_found_; /* if nullptr, read no further, ignore args */
    QString others_found_;
    QString vict_found_;
    /* An argument was there, but no victim was found */
    QString not_found_;
    /* The victim turned out to be the character */
    QString char_auto_;
    QString others_auto_;

    int hide_ = {};
    position_t min_victim_position_ = {}; /* Position of victim */
};

class Socials
{
    QList<Social> socials_;
    QMap<QString, Social> abbreviated_socials_;

public:
    Socials(void);
    auto find(QString arg) -> std::expected<Social, search_error>;
    QStringList list(void);
};

#endif
