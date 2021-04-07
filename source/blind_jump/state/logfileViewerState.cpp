#include "state_impl.hpp"


void LogfileViewerState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    repaint(pfrm, 1);
    // The game's logfile is not localized, not worth the trouble.
    locale_set_language(0);
}


void LogfileViewerState::exit(Platform& pfrm, Game& game, State& next_state)
{
    locale_set_language(game.persistent_data().settings_.language_.get());
    pfrm.screen().fade(0.f);
    pfrm.fill_overlay(0);
}


StatePtr
LogfileViewerState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    auto screen_tiles = calc_screen_tiles(pfrm);

    if (pfrm.keyboard().down_transition<Key::alt_1>()) {
        return state_pool().create<ActiveState>();
    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        offset_ += screen_tiles.x;
        repaint(pfrm, offset_);
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (offset_ > 0) {
            offset_ -= screen_tiles.x;
            repaint(pfrm, offset_);
        }
    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        offset_ += screen_tiles.x * screen_tiles.y;
        repaint(pfrm, offset_);
    } else if (pfrm.keyboard().down_transition<Key::left>()) {
        if (offset_ >= screen_tiles.x * screen_tiles.y) {
            offset_ -= screen_tiles.x * screen_tiles.y;
            repaint(pfrm, offset_);
        }
    }

    return null_state();
}


Platform::TextureCpMapper locale_texture_map();


void LogfileViewerState::repaint(Platform& pfrm, int offset)
{
    constexpr int buffer_size = 600;
    u8 buffer[buffer_size];

    pfrm.logger().read(buffer, offset, buffer_size);

    auto screen_tiles = calc_screen_tiles(pfrm);

    int index = 0;
    for (int j = 0; j < screen_tiles.y; ++j) {
        for (int i = 0; i < screen_tiles.x; ++i) {
            // const int index = i + j * screen_tiles.x;
            if (index < buffer_size) {
                if (buffer[index] == '\n') {
                    for (; i < screen_tiles.x; ++i) {
                        // eat the rest of the space in the current line
                        pfrm.set_tile(Layer::overlay, i, j, 0);
                    }
                } else {
                    // FIXME: I broke the mapper when I was working on
                    // localization for asian charsets.
                    // const auto t =
                    //     pfrm.map_glyph(buffer[index], locale_texture_map());
                    // pfrm.set_tile(Layer::overlay, i, j, t);
                }
                index += 1;
            }
        }
    }
}
