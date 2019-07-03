#include "game.hpp"


Game::Game() : player_(Player::pool().get())
{

}


void Game::update(Platform& pfrm, Microseconds delta)
{
    player_->update(pfrm, *this, delta);

    enemies_.transform([&](auto& enemy_buf) {
            for (auto it = enemy_buf.begin(); it != enemy_buf.end(); ++it) {
                (*it)->update(pfrm, *this, delta);
            }
        });


    pfrm.screen().draw(player_->get_sprite());
}
