#include "game.hpp"


Game::Game()
{
    test_.initialize();
    test_.set_position({50.f, 80.f});
}


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

    // TEST
    if (not pfrm.keyboard().pressed<Keyboard::Key::down>()) {
        pfrm.screen().draw(test_);
    }
}
