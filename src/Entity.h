#ifndef ENTITY_H
#define ENTITY_H

#include <QString>
#include <QUuid>
#include <sstream>

class Entity
{
public:
    inline QString getName(void) const { return name_; }
    inline void setName(QString name) { name_ = name; }
    inline void setName(std::stringstream &ss) { name_ = ss.str().c_str(); }
    inline void setName(std::string &ss) { name_ = ss.c_str(); }

    inline uint64_t getNumber(void) const { return number_; }
    inline void setNumber(uint64_t number) { number_ = number; }
    inline void incrementNumber(void)
    {
        if (number_ + 1 > number_)
            number_++;
    }
    inline void decrementNumber(void)
    {
        if (number_ > 0)
            number_--;
    }

    inline uint64_t getVirtualNumber(void) const { return virtual_number_; }
    inline void setVirtualNumber(uint64_t number) { virtual_number_ = number; }
    inline void incrementVirtualNumber(void)
    {
        if (virtual_number_ + 1 > virtual_number_)
            virtual_number_++;
    }
    inline void decrementVirtualNumber(void)
    {
        if (virtual_number_ > 0)
            virtual_number_--;
    }

    inline QUuid getUUID(void) const { return uuid_; }
    inline void setUUID(QUuid &uuid) { uuid_ = uuid; }

protected:
    QString name_ = {};
    uint64_t number_ = {};
    uint64_t virtual_number_ = {};
    QUuid uuid_ = {};
};
#endif