#pragma once

#include "numeric.hpp"
#include "sprite.hpp"
#include "color.hpp"


class Screen {
public:

    Screen();

    void draw(const Sprite& spr);

    void clear();
    
    void display();

    const Vec2<u32>& size() const;
    
private:    
    void* userdata_;
};
