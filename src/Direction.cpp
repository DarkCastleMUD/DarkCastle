#include "DC/DC.h"
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
    return u"Unknown"_s;

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
        {Direction::NORTH, u"north"_s},
        {Direction::EAST, u"east"_s},
        {Direction::SOUTH, u"south"_s},
        {Direction::WEST, u"west"_s},
        {Direction::UP, u"up"_s},
        {Direction::DOWN, u"down"_s}};

const QMap<Direction::Type, QString> Direction::TypeToStringAlt_ =
    {
        {Direction::NORTH, u"northward"_s},
        {Direction::EAST, u"eastward"_s},
        {Direction::SOUTH, u"southward"_s},
        {Direction::WEST, u"westward"_s},
        {Direction::UP, u"upward"_s},
        {Direction::DOWN, u"downward"_s}};

const QMap<QString, Direction::Type> Direction::StringToType_ =
    {
        {u"n"_s, Direction::NORTH},
        {u"no"_s, Direction::NORTH},
        {u"nor"_s, Direction::NORTH},
        {u"nort"_s, Direction::NORTH},
        {u"north"_s, Direction::NORTH},
        {u"e"_s, Direction::EAST},
        {u"ea"_s, Direction::EAST},
        {u"eas"_s, Direction::EAST},
        {u"east"_s, Direction::EAST},
        {u"s"_s, Direction::SOUTH},
        {u"so"_s, Direction::SOUTH},
        {u"sou"_s, Direction::SOUTH},
        {u"sout"_s, Direction::SOUTH},
        {u"south"_s, Direction::SOUTH},
        {u"w"_s, Direction::WEST},
        {u"we"_s, Direction::WEST},
        {u"wes"_s, Direction::WEST},
        {u"west"_s, Direction::WEST},
        {u"u"_s, Direction::UP},
        {u"up"_s, Direction::UP},
        {u"d"_s, Direction::DOWN},
        {u"do"_s, Direction::DOWN},
        {u"dow"_s, Direction::DOWN},
        {u"down"_s, Direction::DOWN}};

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