#include <algorithm>
#include "game.hpp"


Game::Game() : player_(Player::pool().get())
{
    details_.get<0>().push_back(ItemChest::pool().get());
}


void Game::update(Platform& pfrm, Microseconds delta)
{
    player_->update(pfrm, *this, delta);

    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            (*it)->update(pfrm, *this, delta);
        }
    };

    enemies_.transform(update_policy);
    details_.transform(update_policy);
    effects_.transform(update_policy);

    camera_.update(pfrm, delta, player_->get_position());

    display_buffer.push_back(&player_->get_sprite());

    auto render_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            display_buffer.push_back(&(*it)->get_sprite());
        }
    };

    enemies_.transform(render_policy);
    details_.transform(render_policy);
    effects_.transform(render_policy);

    std::sort(display_buffer.begin(), display_buffer.end(),
              [](const auto& l, const auto& r) {
                  return l->get_position().y > r->get_position().y;
              });
}


void Game::render(Platform& pfrm)
{
    for (auto spr : display_buffer) {
        pfrm.screen().draw(*spr);
    }

    display_buffer.clear();
}
