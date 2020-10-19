#pragma once


#include "memory/buffer.hpp"
#include "number/endian.hpp"


class Game;


class Powerup {
public:
    static constexpr const int max_ = 4;

    enum class Type { accelerator, lethargy, explosive_rounds } type_;

    HostInteger<s32> parameter_;
    bool dirty_;

    enum class DisplayMode {
        integer,
        timestamp
    } display_mode_ = DisplayMode::integer;

    inline int icon_index() const
    {
        return 257 + static_cast<int>(type_);
    }
};


using Powerups = Buffer<Powerup, Powerup::max_>;


void add_powerup(Game& game,
                 Powerup::Type t,
                 int param,
                 Powerup::DisplayMode mode = Powerup::DisplayMode::integer);


Powerup* get_powerup(Game& game, Powerup::Type t);
