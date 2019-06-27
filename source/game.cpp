#include "game.hpp"


Game::Game()
{
    test_.initialize(Sprite::Size::tall_16_32, 0);
    test_.set_position({50.f, 80.f});
}


bool last = false;
u32 frame = 1;


void Game::update(Platform& pfrm, Microseconds delta)
{
    // TEST
    if (pfrm.keyboard().pressed<Keyboard::Key::left>()) {
        auto pos2 = test_.get_position();
        test_.set_position({pos2.x - 1.f, pos2.y});
    }

    enemies_.transform([&](auto& enemy_buf) {
            for (auto it = enemy_buf.begin(); it != enemy_buf.end(); ++it) {
                (*it)->update(pfrm, *this, delta);
            }
        });

    bool current = pfrm.keyboard().pressed<Keyboard::Key::up>();
    if (current and not last) {
        test_.initialize(Sprite::Size::square_32_32, frame);
        test_.set_position({50.f, 80.f});
        frame += 1;
    }
    last = current;

    // TEST
    if (not pfrm.keyboard().pressed<Keyboard::Key::down>()) {
        pfrm.screen().draw(test_);
    }
}
