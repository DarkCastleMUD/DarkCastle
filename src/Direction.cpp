#include <expected>

#include <DC/Direction.h>

Direction::Direction()
    : value_(Type::UNDEFINED), valid_(false)
{
}

Direction::Direction(Type value)
    : value_(value), valid_(true)
{
    if (value_ == Type::UNDEFINED)
        valid_ = false;
}

Direction::Direction(QString dirstr)
{
    if (StringToType_.contains(dirstr))
    {
        value_ = StringToType_[dirstr];
        valid_ = true;
    }
    value_ = Type::UNDEFINED;
    valid_ = false;
}

QString Direction::toString(void) const
{
    if (!valid_)
        return QStringLiteral("Unknown");

    return TypeToString_.value(value_);
}

cmd_t Direction::toCommand(void) const
{
    if (!valid_)
        return cmd_t::UNDEFINED;

    switch (value_)
    {
    case Direction::NORTH:
        return cmd_t::NORTH;
        break;
    case Direction::EAST:
        return cmd_t::EAST;
        break;
    case Direction::SOUTH:
        return cmd_t::SOUTH;
        break;
    case Direction::WEST:
        return cmd_t::WEST;
        break;
    case Direction::UP:
        return cmd_t::UP;
        break;
    case Direction::DOWN:
        return cmd_t::DOWN;
        break;
    }

    return cmd_t::UNDEFINED;
}

const QMap<Direction::Type, QString> Direction::TypeToString_ =
    {
        {Direction::NORTH, QStringLiteral("north")},
        {Direction::EAST, QStringLiteral("east")},
        {Direction::SOUTH, QStringLiteral("south")},
        {Direction::WEST, QStringLiteral("west")},
        {Direction::UP, QStringLiteral("up")},
        {Direction::DOWN, QStringLiteral("down")}};

const QMap<Direction::Type, QString> Direction::TypeToStringAlt_ =
    {
        {Direction::NORTH, QStringLiteral("northward")},
        {Direction::EAST, QStringLiteral("eastward")},
        {Direction::SOUTH, QStringLiteral("southward")},
        {Direction::WEST, QStringLiteral("westward")},
        {Direction::UP, QStringLiteral("upward")},
        {Direction::DOWN, QStringLiteral("downward")}};

const QMap<QString, Direction::Type> Direction::StringToType_ =
    {
        {QStringLiteral("n"), Direction::NORTH},
        {QStringLiteral("no"), Direction::NORTH},
        {QStringLiteral("nor"), Direction::NORTH},
        {QStringLiteral("nort"), Direction::NORTH},
        {QStringLiteral("north"), Direction::NORTH},
        {QStringLiteral("e"), Direction::EAST},
        {QStringLiteral("ea"), Direction::EAST},
        {QStringLiteral("eas"), Direction::EAST},
        {QStringLiteral("east"), Direction::EAST},
        {QStringLiteral("s"), Direction::SOUTH},
        {QStringLiteral("so"), Direction::SOUTH},
        {QStringLiteral("sou"), Direction::SOUTH},
        {QStringLiteral("sout"), Direction::SOUTH},
        {QStringLiteral("south"), Direction::SOUTH},
        {QStringLiteral("w"), Direction::WEST},
        {QStringLiteral("we"), Direction::WEST},
        {QStringLiteral("wes"), Direction::WEST},
        {QStringLiteral("west"), Direction::WEST},
        {QStringLiteral("u"), Direction::UP},
        {QStringLiteral("up"), Direction::UP},
        {QStringLiteral("d"), Direction::DOWN},
        {QStringLiteral("do"), Direction::DOWN},
        {QStringLiteral("dow"), Direction::DOWN},
        {QStringLiteral("down"), Direction::DOWN}};

Direction Direction::getReverse(void) const
{
    switch (value_)
    {
    case Direction::NORTH:
        return Direction::SOUTH;
        break;
    case Direction::SOUTH:
        return Direction::NORTH;
        break;
    case Direction::EAST:
        return Direction::WEST;
        break;
    case Direction::WEST:
        return Direction::EAST;
        break;
    case Direction::UP:
        return Direction::DOWN;
        break;
    case Direction::DOWN:
        return Direction::UP;
        break;
    }
    return {};
}

auto getDirectionFromCommand(cmd_t cmd) -> std::expected<Direction, bool>
{
    switch (cmd)
    {
    case cmd_t::NORTH:
        return Direction::NORTH;
        break;
    case cmd_t::EAST:
        return Direction::EAST;
        break;
    case cmd_t::SOUTH:
        return Direction::SOUTH;
        break;
    case cmd_t::WEST:
        return Direction::WEST;
        break;
    case cmd_t::UP:
        return Direction::UP;
        break;
    case cmd_t::DOWN:
        return Direction::DOWN;
        break;
    default:
        break;
    }
    return std::unexpected(false);
}