#ifndef DC_ARENA_H
#define DC_ARENA_H
#include <QtTypes>
#include "DC/types.h"

class Arena
{
public:
    static constexpr room_t ARENA_LOW = 14600;
    static constexpr room_t ARENA_HIGH = 14680;
    static constexpr room_t ARENA_DEATHTRAP = 14680;

    enum class Types
    {
        NORMAL,
        CHAOS,
        POTATO,
        PRIZE,
        HP,
        PLAYER_FREE,
        PLAYER_NOT_FREE
    };

    enum class Statuses
    {
        CLOSED,
        OPENED
    };

    auto Low(void) const -> level_t { return low_; }
    auto High(void) const -> level_t { return high_; }
    auto Number(void) const -> quint64 { return number_; }
    auto CurrentNumber(void) const -> quint64 { return current_number_; }
    auto IncrementCurrentNumber(void) -> void { current_number_++; }
    auto HPLimit(void) const -> quint64 { return hp_limit_; }
    auto Type(void) const -> Types { return type_; }
    auto Status(void) const -> Statuses { return status_; }
    auto EntryFee(void) const -> gold_t { return entry_fee_; }

    auto isNormal(void) const -> bool { return Type() == Types::NORMAL; }
    auto isChaos(void) const -> bool { return Type() == Types::CHAOS; }
    auto isPotato(void) const -> bool { return Type() == Types::POTATO; }
    auto isPrize(void) const -> bool { return Type() == Types::PRIZE; }
    auto isHP(void) const -> bool { return Type() == Types::HP; }
    auto isPlayerFree(void) const -> bool { return Type() == Types::PLAYER_FREE; }
    auto isPlayerNotFree(void) const -> bool { return Type() == Types::PLAYER_NOT_FREE; }

    auto isOpened(void) const -> bool { return Status() == Statuses::OPENED; }
    auto isClosed(void) const -> bool { return Status() == Statuses::CLOSED; }

private:
    level_t low_{};
    level_t high_{};
    quint64 number_{};
    quint64 current_number_{};
    quint64 hp_limit_{};
    Types type_{};
    Statuses status_{};
    gold_t entry_fee_{};
};
#endif