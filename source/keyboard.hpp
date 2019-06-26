#pragma once

#include <array>
#include "numeric.hpp"


class Keyboard {
public:
    
    Keyboard();
    
    enum Key {
        action_1,
        action_2,
        start,
        left,
        right,
        up,
        down,
        count
    };
    
    void poll();

    template <Key k>
    bool pressed() const
    {
        return states_[k];
    }
    
private:
    std::array<bool, Key::count> states_;
};
