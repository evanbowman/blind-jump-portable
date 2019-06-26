#pragma once

#include "numeric.hpp"


class Sprite {
public:

    Sprite();


    Sprite(const Sprite&) = delete;


    ~Sprite();


    bool initialize(int x);


    void set_position(Vec2<float>& position);


    const Vec2<float>& get_position() const;


private:
    volatile void* data_;
};
