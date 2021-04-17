#include "localization.hpp"
#include "state_impl.hpp"


NotebookState::NotebookState(LocalizedText&& str)
    : str_(std::move(str)), page_(0)
{
}


void NotebookState::enter(Platform& pfrm, Game&, State&)
{
    // pfrm.speaker().play_sound("open_book", 0);

    pfrm.sleep(1); // Well, this is embarassing... basically, the fade function
                   // creates tearing on the gameboy, and we can mitigate the
                   // tearing with a sleep call, which is implemented to wait on
                   // a vblank interrupt. I could defer the fade on those
                   // platforms to run after the vblank, but fade is cpu
                   // intensive, and sometimes ends up exceeding the blank
                   // period, causing tearing anyway.

    pfrm.screen().fade(1.f, ColorConstant::rich_black, {}, true, true);
    pfrm.load_overlay_texture("overlay_journal");
    pfrm.screen().fade(1.f, ColorConstant::aged_paper, {}, true, true);

    auto screen_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm);

    page_number_.emplace(pfrm, OverlayCoord{0, u8(screen_tiles.y - 1)});

    if (locale_language_name(locale_get_language()) == "chinese") {
        pfrm.enable_expanded_glyph_mode(true);
        repaint_page(pfrm);
    } else {
        text_->assign(
            str_->c_str(),
            {1, 2},
            OverlayCoord{u8(screen_tiles.x - 2), u8(screen_tiles.y - 4)});
    }
}


Platform::TextureCpMapper locale_texture_map();


static u16 get_whitespace_tile(Platform& pfrm)
{
    const auto mapping_info = locale_texture_map()(' ');
    if (mapping_info) {
        return pfrm.map_glyph(' ', *mapping_info);
    }
    return 0;
}


void NotebookState::repaint_margin(Platform& pfrm)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    u16 notebook_margin_tile = get_whitespace_tile(pfrm);

    for (int x = 0; x < screen_tiles.x; ++x) {
        for (int y = 0; y < screen_tiles.y; ++y) {
            pfrm.set_tile(Layer::overlay, x, y, notebook_margin_tile);
        }
    }
}


void print_double_char(Platform& pfrm,
                       utf8::Codepoint c,
                       const OverlayCoord& coord,
                       const std::optional<FontColors>& colors = {});


static const int chinese_row_width = 14;
static const int chinese_row_count = 5;

static const int chinese_glyphs_per_page =
    chinese_row_width * chinese_row_count;


static int chinese_page_count(const char* str)
{
    const int len = utf8::len(str);
    return len / chinese_glyphs_per_page + 1;
}


void NotebookState::repaint_page(Platform& pfrm)
{
    const auto size = text_->size();

    page_number_->erase();
    repaint_margin(pfrm);
    page_number_->assign(page_ + 1);

    if (locale_language_name(locale_get_language()) == "chinese") {
        OverlayCoord pos{1, 2};

        int seen_count = 0;

        utf8::scan(
            [&](const utf8::Codepoint& cp, const char*, int) {
                if (seen_count++ < chinese_glyphs_per_page * page_) {
                    return;
                }

                if (pos.x > chinese_row_width * 2) {
                    pos.x = 1;
                    pos.y += 3;
                }

                if (pos.y - 2 >= chinese_row_count * 3) {
                    return;
                }

                print_double_char(pfrm, cp, pos);
                pos.x += 2;
            },
            str_->c_str(),
            str_len(str_->c_str()));
    } else {
        text_->assign(str_->c_str(), {1, 2}, size, page_ * (size.y / 2));
    }
}


void NotebookState::exit(Platform& pfrm, Game&, State&)
{
    pfrm.sleep(1);

    pfrm.enable_expanded_glyph_mode(false);

    pfrm.fill_overlay(0); // The TextView destructor cleans up anyway, but we
                          // have ways of clearing the screen faster than the
                          // TextView implementation is able to. The TextView
                          // class needs to loop through each glyph and
                          // individually zero them out, which can create
                          // tearing in the display. The fill_overlay() function
                          // doesn't need to work around sub-regions of the
                          // screen, so it can use faster methods, like a single
                          // memset, or special BIOS calls (depending on the
                          // platform) to clear out the screen.

    text_.reset();
}


StatePtr NotebookState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition(game.action2_key())) {
        return state_pool().create<InventoryState>(false);
    }

    switch (display_mode_) {
    case DisplayMode::after_transition:
        // NOTE: Loading a notebook page for the first time results in a large
        // amount of lag on some systems, which effectively skips the screen
        // fade. The first load of a new page causes the platform to load a
        // bunch of glyphs into memory for the first time, which means copying
        // over texture memory, possibly adjusting a glyph mapping table,
        // etc. So we're just going to sit for one update cycle, and let the
        // huge delta pass over.
        display_mode_ = DisplayMode::fade_in;
        break;

    case DisplayMode::fade_in: {
        static const auto fade_duration = milliseconds(200);
        timer_ += delta;
        if (timer_ < fade_duration) {

            pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::aged_paper,
                               {},
                               true,
                               true);
        } else {
            timer_ = 0;
            pfrm.screen().fade(0.f);
            display_mode_ = DisplayMode::show;
        }
        break;
    }

    case DisplayMode::show:
        if (pfrm.keyboard().down_transition<Key::down>() or
            pfrm.keyboard().down_transition<Key::right>()) {

            auto has_more_pages = [&]() -> bool {
                if (locale_language_name(locale_get_language()) == "chinese") {
                    return page_ < chinese_page_count(str_->c_str()) - 1;
                } else {
                    return text_->parsed() not_eq utf8::len(str_->c_str());
                }
            };

            if (has_more_pages()) {
                page_ += 1;
                timer_ = 0;
                display_mode_ = DisplayMode::fade_out;
            }

        } else if (pfrm.keyboard().down_transition<Key::up>() or
                   pfrm.keyboard().down_transition<Key::left>()) {
            if (page_ > 0) {
                page_ -= 1;
                timer_ = 0;
                display_mode_ = DisplayMode::fade_out;
            }
        }
        break;

    case DisplayMode::fade_out: {
        timer_ += delta;
        static const auto fade_duration = milliseconds(200);
        if (timer_ < fade_duration) {
            pfrm.screen().fade(smoothstep(0.f, fade_duration, timer_),
                               ColorConstant::aged_paper,
                               {},
                               true,
                               true);
        } else {
            timer_ = 0;
            pfrm.screen().fade(1.f, ColorConstant::aged_paper, {}, true, true);
            display_mode_ = DisplayMode::transition;
        }
        break;
    }

    case DisplayMode::transition:
        repaint_page(pfrm);
        pfrm.speaker().play_sound("open_book", 0);
        display_mode_ = DisplayMode::after_transition;
        break;
    }

    return null_state();
}
