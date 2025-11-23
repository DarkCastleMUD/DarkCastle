#ifndef DC_DIRECTION_H
#define DC_DIRECTION_H
#include <QMap>
#include <QString>
#include <DC/common.h>

class Direction
{
public:
    enum Type : quint8
    {
        NORTH = 0,
        EAST = 1,
        SOUTH = 2,
        WEST = 3,
        UP = 4,
        DOWN = 5,
        UNDEFINED
    };
    Direction();
    Direction(Type value);
    Direction(QString string);
    constexpr operator Type() const
    {
        return value_;
    }
    [[nodiscard]] QString toString(void) const;
    [[nodiscard]] cmd_t toCommand(void) const;
    [[nodiscard]] Direction getReverse(void) const;

private:
    static const QMap<Type, QString> TypeToString_;
    static const QMap<Type, QString> TypeToStringAlt_;
    static const QMap<QString, Type> StringToType_;

    Type value_;
    bool valid_;
};

Direction reverse_direction(Direction dir);

#endif