#include "state_impl.hpp"


void EndingCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    next_y_ = screen_tiles.y + 2;

    game.on_timeout(pfrm, milliseconds(500), [](Platform& pfrm, Game&) {
        pfrm.speaker().play_music("clair_de_lune", false, 0);
    });
}


void EndingCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    lines_.clear();
    pfrm.set_overlay_origin(0, 0);
    pfrm.speaker().stop_music();
}


// NOTE: '%' characters in the credits will be filled with N '.' characters,
// such that the surrounding text is aligned to either edge of the screen.
//
// FIXME: localize the credits? Nah...
static const std::array<const char*, 32> credits_lines = {
    "Artwork and Source Code by",
    "Evan Bowman",
    "",
    "",
    "Music",
    "Hiraeth%Scott Buckley",
    "Omega%Scott Buckley",
    "Computations%Scott Buckley",
    "September%Kai Engel",
    "Clair De Lune%Chad Crouch",
    "",
    "",
    "Playtesting",
    "Benjamin Casler",
    "",
    "",
    "Special Thanks",
    "My Family",
    "Jasper Vijn (Tonc)",
    "The DevkitARM Project"
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "THE END"};


StatePtr
EndingCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    timer_ += delta;

    if (timer_ > milliseconds([&] {
            if (pfrm.keyboard().pressed<Key::action_2>()) {
                return 15;
            } else {
                return 60;
            }
        }())) {

        timer_ = 0;

        pfrm.set_overlay_origin(0, scroll_++);

        constexpr int tile_height = 8;
        const bool scrolled_two_lines = scroll_ % (tile_height * 2) == 0;

        if (scrolled_two_lines) {

            auto screen_tiles = calc_screen_tiles(pfrm);

            if (scroll_ > ((screen_tiles.y + 2) * tile_height) and
                not lines_.empty()) {
                lines_.erase(lines_.begin());
            }

            if (next_ < int{credits_lines.size() - 1}) {
                const u8 y =
                    next_y_ % 32; // The overlay tile layer is 32x32 tiles.
                next_y_ += 2;

                const auto str = credits_lines[next_++];


                bool contains_fill_char = false;
                utf8::scan(
                    [&contains_fill_char,
                     &pfrm](const utf8::Codepoint& cp, const char*, int) {
                        if (cp == '%') {
                            if (contains_fill_char) {
                                error(pfrm, "two fill chars not allowed");
                                pfrm.fatal();
                            }
                            contains_fill_char = true;
                        }
                    },
                    str,
                    str_len(str));

                if (utf8::len(str) > u32(screen_tiles.x - 2)) {
                    error(pfrm, "credits text too large");
                    pfrm.fatal();
                }

                if (contains_fill_char) {
                    const auto fill =
                        screen_tiles.x - ((utf8::len(str) - 1) + 2);
                    lines_.emplace_back(pfrm, OverlayCoord{1, y});
                    utf8::scan(
                        [this,
                         fill](const utf8::Codepoint&, const char* raw, int) {
                            if (str_len(raw) == 1 and raw[0] == '%') {
                                for (size_t i = 0; i < fill; ++i) {
                                    lines_.back().append(".");
                                }
                            } else {
                                lines_.back().append(raw);
                            }
                        },
                        str,
                        str_len(str));
                } else {
                    const auto len = utf8::len(str);
                    const u8 left_margin = (screen_tiles.x - len) / 2;
                    lines_.emplace_back(pfrm, OverlayCoord{left_margin, y});
                    lines_.back().assign(str);
                }


            } else if (lines_.size() == screen_tiles.y / 4) {
                pfrm.sleep(160);
                factory_reset(pfrm);
            }
        }
    }

    return null_state();
}
