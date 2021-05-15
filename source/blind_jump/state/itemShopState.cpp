#include "state_impl.hpp"


void ItemShopState::display_time_remaining(Platform&, Game&)
{
}


void ItemShopState::show_sidebar(Platform& pfrm)
{
    if (display_mode_ == DisplayMode::show_buy) {
        const auto screen_tiles = calc_screen_tiles(pfrm);
        for (int y = 0; y < screen_tiles.y; y += 5) {
            pfrm.set_tile(Layer::overlay, 0, y + 2, 129);
            pfrm.set_tile(Layer::overlay, 5, y + 2, 129);
        }
    } else {
        const auto screen_tiles = calc_screen_tiles(pfrm);
        for (int y = 0; y < screen_tiles.y; y += 5) {
            pfrm.set_tile(Layer::overlay, screen_tiles.x - 1, y + 2, 129);
            pfrm.set_tile(Layer::overlay, screen_tiles.x - 6, y + 2, 129);
        }
    }
}


void ItemShopState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);

    const auto st = calc_screen_tiles(pfrm);

    const u8 width = 6;
    buy_item_bar_.emplace(pfrm, width, st.y, OverlayCoord{});
    sell_item_bar_.emplace(pfrm, width, st.y, OverlayCoord{u8(st.x), 0});

    // for (int i = 0; i < st.x; ++i) {
    //     pfrm.set_tile(Layer::overlay, i, st.y - 1, 112);
    // }

    const bool bigfont = locale_requires_doublesize_font();

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    const auto str = locale_string(pfrm, LocaleString::scavenger_store);
    const auto heading_str_len = utf8::len(str->c_str()) * (bigfont ? 2 : 1);

    auto margin = centered_text_margins(pfrm, heading_str_len);

    if (bigfont) {
        margin /= 2;
    }

    heading_text_.emplace(
        pfrm, OverlayCoord{0, u8(st.y - 2 * (bigfont ? 2 : 1))}, font_conf);

    left_text_margin(*heading_text_, margin);
    heading_text_->append(str->c_str());
    right_text_margin(*heading_text_, margin);

    buy_sell_text_.emplace(
        pfrm, OverlayCoord{1, u8(st.y - 1 * (bigfont ? 2 : 1))}, font_conf);
    buy_sell_text_->assign(locale_string(pfrm, LocaleString::buy)->c_str());

    const auto fill_len =
        st.x - (2 +
                utf8::len(locale_string(pfrm, LocaleString::buy)->c_str()) *
                    (bigfont ? 2 : 1) +
                utf8::len(locale_string(pfrm, LocaleString::sell)->c_str()) *
                    (bigfont ? 2 : 1));


    for (u32 i = 0; i < fill_len / (bigfont ? 2 : 1); ++i) {
        buy_sell_text_->append(" ");
    }

    buy_sell_text_->append(locale_string(pfrm, LocaleString::sell)->c_str());

    pfrm.set_tile(Layer::overlay, 0, st.y - 1, 421);
    pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 1, 420);

    if (bigfont) {
        pfrm.set_tile(Layer::overlay, 0, st.y - 2, 112);
        pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 2, 112);
    }

    game.camera().set_speed(2.8f);

    if (display_mode_ == DisplayMode::animate_in_buy or
        display_mode_ == DisplayMode::animate_in_sell) {
        heading_text_.reset();
        buy_sell_text_.reset();
    } else {
        if (bigfont) {
            for (u32 i = 0; i < st.x; ++i) {
                pfrm.set_tile(Layer::overlay, i, st.y - 5, 425);
            }
        } else {
            for (u32 i = 0; i < st.x; ++i) {
                pfrm.set_tile(Layer::overlay, i, st.y - 3, 425);
            }
        }
    }
}


void ItemShopState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    buy_item_bar_.reset();
    buy_options_bar_.reset();
    sell_item_bar_.reset();
    sell_options_bar_.reset();
    buy_sell_text_.reset();
    heading_text_.reset();
    buy_sell_icon_.reset();
    info_icon_.reset();
    coin_icon_.reset();
    selector_.reset();
    score_.reset();

    buy_item_icons_.clear();
    sell_item_icons_.clear();

    game.camera().set_speed(1.f);

    pfrm.fill_overlay(0);
}


void ItemShopState::show_buy_icons(Platform& pfrm, Game& game)
{
    if (not game.scavenger()) {
        pfrm.fatal("in shop, but scavenger missing??");
    }

    buy_item_icons_.clear();

    buy_items_remaining_ = false;

    buy_item_bar_->set_display_percentage(1.f);
    show_sidebar(pfrm);

    int skip = buy_page_num_ * item_display_count_;

    const auto st = calc_screen_tiles(pfrm);

    for (u32 i = skip; i < game.scavenger()->inventory_.size(); ++i) {
        if (buy_item_icons_.full()) {
            buy_items_remaining_ = true;
            break;
        }

        auto item = game.scavenger()->inventory_[i];

        const OverlayCoord coord{
            2, static_cast<u8>(4 + buy_item_icons_.size() * 5)};

        if (auto handler = inventory_item_handler(item)) {
            buy_item_icons_.emplace_back(pfrm, handler->icon_, coord);
        }
    }

    if (buy_items_remaining_) {
        pfrm.set_tile(Layer::overlay, 2, st.y - 2, 154);
    } else {
        pfrm.set_tile(Layer::overlay, 2, st.y - 2, 152);
    }

    if (buy_page_num_ > 0) {
        pfrm.set_tile(Layer::overlay, 2, 1, 153);
    } else {
        pfrm.set_tile(Layer::overlay, 2, 1, 151);
    }
}


void ItemShopState::show_sell_icons(Platform& pfrm, Game& game)
{
    sell_item_icons_.clear();

    sell_item_bar_->set_display_percentage(1.f);
    show_sidebar(pfrm);

    int skip = sell_page_num_ * item_display_count_;

    sell_items_remaining_ = false;

    const auto screen_tiles = calc_screen_tiles(pfrm);

    for (int page = 0; page < Inventory::pages; ++page) {
        for (int row = 0; row < Inventory::rows; ++row) {
            for (int col = 0; col < Inventory::cols; ++col) {
                if (skip > 0) {
                    --skip;
                    continue;
                }
                const auto item = game.inventory().get_item(page, col, row);

                if (item == Item::Type::null) {
                    continue;
                }

                if (sell_item_icons_.full()) {
                    sell_items_remaining_ = true;
                    continue;
                }

                const OverlayCoord coord{
                    static_cast<u8>(screen_tiles.x - 4),
                    static_cast<u8>(4 + sell_item_icons_.size() * 5)};

                if (auto handler = inventory_item_handler(item)) {
                    sell_item_icons_.emplace_back(pfrm, handler->icon_, coord);
                }
            }
        }
    }

    if (sell_items_remaining_) {
        pfrm.set_tile(
            Layer::overlay, screen_tiles.x - 3, screen_tiles.y - 2, 154);
    } else {
        pfrm.set_tile(
            Layer::overlay, screen_tiles.x - 3, screen_tiles.y - 2, 152);
    }

    if (sell_page_num_ > 0) {
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 3, 1, 153);
    } else {
        pfrm.set_tile(Layer::overlay, screen_tiles.x - 3, 1, 151);
    }
}


void ItemShopState::hide_label(Platform& pfrm)
{
    if (buy_sell_text_) {
        // Erase the border that sits atop of the label
        const auto c = buy_sell_text_->coord();
        for (u32 i = 0; i < buy_sell_text_->len(); ++i) {
            pfrm.set_tile(Layer::overlay, c.x + i, c.y - 1, 0);
        }

        if (coin_icon_) {
            coin_icon_.reset();
            pfrm.set_tile(
                Layer::overlay, c.x + buy_sell_text_->len(), c.y - 1, 0);
        }
    }

    buy_sell_text_.reset();
}


void ItemShopState::show_label(Platform& pfrm,
                               Game& game,
                               const char* str,
                               bool anchor_right,
                               bool append_coin_icon)
{
    const auto st = calc_screen_tiles(pfrm);

    const bool bigfont = locale_requires_doublesize_font();

    u8 xoff = 0;
    if (anchor_right) {
        xoff = st.x - (utf8::len(str) * (bigfont ? 2 : 1) +
                       (append_coin_icon ? 1 : 0));
    }

    if (buy_sell_text_) {

        // Erase tiles, to make up for the difference in the width of the line
        // that sits atop of the text.
        const u32 extra = not append_coin_icon and coin_icon_ ? 1 : 0;

        if (anchor_right) {
            for (u32 i = 0;
                 i < buy_sell_text_->len() * (bigfont ? 2 : 1) + extra;
                 ++i) {
                pfrm.set_tile(Layer::overlay,
                              (st.x - 1) - i,
                              st.y - (2 + (bigfont ? 1 : 0)),
                              0);
            }
        } else {
            for (u32 i = utf8::len(str);
                 i < buy_sell_text_->len() * (bigfont ? 2 : 1) + extra;
                 ++i) {
                pfrm.set_tile(Layer::overlay,
                              xoff + i,
                              st.y - (2 + (bigfont ? 1 : 0)),
                              0);
            }
        }

        buy_sell_text_->erase();

        if (not append_coin_icon and coin_icon_) {
            coin_icon_.reset();
        }
    }

    if (append_coin_icon) {
        coin_icon_.emplace(
            pfrm, 146, OverlayCoord{u8(xoff + utf8::len(str)), u8(st.y - 1)});
    }

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    buy_sell_text_.emplace(
        pfrm,
        OverlayCoord{xoff, u8(st.y - (1 + (bigfont ? 1 : 0)))},
        font_conf);
    buy_sell_text_->assign(str);

    for (u32 i = 0;
         i < utf8::len(str) * (bigfont ? 2 : 1) + (append_coin_icon ? 1 : 0);
         ++i) {
        pfrm.set_tile(
            Layer::overlay, xoff + i, st.y - (2 + (bigfont ? 1 : 0)), 425);
    }
}


static Item::Type get_sale_item(Game& game, int page, int offset)
{
    int skip = page * 3 + offset;

    for (int page = 0; page < Inventory::pages; ++page) {
        for (int row = 0; row < Inventory::rows; ++row) {
            for (int col = 0; col < Inventory::cols; ++col) {
                if (skip > 0) {
                    --skip;
                    continue;
                } else if (skip == 0) {
                    return game.inventory().get_item(page, col, row);
                }
            }
        }
    }

    return Item::Type::null;
}


void ItemShopState::show_sell_option_label(Platform& pfrm, Game& game)
{
    if (selector_x_ == 1) {
        StringBuffer<32> format;
        format += locale_string(pfrm, LocaleString::store_sell)->c_str();

        char int_str[32];
        locale_num2str(
            base_price(get_sale_item(game, sell_page_num_, selector_pos_)),
            int_str,
            10);

        format += int_str;

        show_label(pfrm, game, format.c_str(), false, true);
    } else if (selector_x_ == 2) {
        show_label(pfrm,
                   game,
                   locale_string(pfrm, LocaleString::store_info)->c_str(),
                   false);
    }
}


static Item::Type get_buy_item(Game& game, int page, int offset)
{
    if (game.scavenger()) {
        int skip = page * 3 + offset;
        return game.scavenger()->inventory_[skip];
    }
    return Item::Type::null;
}


// The scavenger charges a markup for everything he sells you. Hey, he's trying
// to run a business you know.
static int scavenger_markup_price(Item::Type item)
{
    const auto bp = base_price(item);
    return bp + bp * 0.5f;
}


void ItemShopState::show_buy_option_label(Platform& pfrm, Game& game)
{
    if (selector_x_ == 1) {
        StringBuffer<32> format;
        format += locale_string(pfrm, LocaleString::store_buy)->c_str();

        char int_str[32];
        locale_num2str(scavenger_markup_price(
                           get_buy_item(game, buy_page_num_, selector_pos_)),
                       int_str,
                       10);

        format += int_str;

        show_label(pfrm, game, format.c_str(), true, true);
    } else if (selector_x_ == 2) {
        show_label(pfrm,
                   game,
                   locale_string(pfrm, LocaleString::store_info)->c_str(),
                   true);
    }
}


void ItemShopState::show_score(Platform& pfrm,
                               Game& game,
                               UIMetric::Align align)
{
    const auto screen_tiles = calc_screen_tiles(pfrm);

    const u8 x = align == UIMetric::Align::right ? screen_tiles.x - 2 : 1;

    score_.emplace(pfrm, OverlayCoord{x, 1}, 146, game.score(), align);
}


StatePtr ItemShopState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    const auto last_score = game.score();

    if (auto new_state = OverworldState::update(pfrm, game, delta)) {
        return new_state;
    }


    if (score_) {
        score_->update(pfrm, delta);
    }

    const auto action_key = game.persistent_data().settings_.action2_key_;

    const auto player_pos = game.player().get_position();

    auto redraw_selector = [&](TileDesc erase_color, bool anchor_right) {
        auto screen_tiles = calc_screen_tiles(pfrm);

        int x_off = anchor_right ? screen_tiles.x : 6;
        if (anchor_right) {
            x_off -= selector_x_ * 5;
        } else {
            x_off += selector_x_ * 5;
        }

        const OverlayCoord pos{static_cast<u8>(x_off - 5),
                               static_cast<u8>(3 + selector_pos_ * 5)};
        if (selector_shaded_) {
            selector_.emplace(
                pfrm, OverlayCoord{4, 4}, pos, false, 8 + 278, erase_color);
            selector_shaded_ = false;
        } else {
            selector_.emplace(
                pfrm, OverlayCoord{4, 4}, pos, false, 16 + 278, erase_color);
            selector_shaded_ = true;
        }
    };

    switch (display_mode_) {
    case DisplayMode::wait:
        if (pfrm.keyboard().down_transition<Key::left>()) {
            display_mode_ = DisplayMode::animate_in_buy;
            heading_text_.reset();
            buy_sell_text_.reset();

            pfrm.fill_overlay(0);

        } else if (pfrm.keyboard().down_transition<Key::right>()) {
            display_mode_ = DisplayMode::animate_in_sell;
            heading_text_.reset();
            buy_sell_text_.reset();

            pfrm.fill_overlay(0);
        }
        break;

    case DisplayMode::animate_in_sell: {
        timer_ += delta;

        static const auto anim_duration = milliseconds(150);

        const auto amount = smoothstep(0.f, anim_duration, timer_);
        sell_item_bar_->set_display_percentage(amount);

        if (timer_ > anim_duration) {
            timer_ = 0;

            sell_item_bar_->set_display_percentage(1.f);

            show_score(pfrm, game, UIMetric::Align::left);

            display_mode_ = DisplayMode::show_sell;

            show_label(
                pfrm,
                game,
                locale_string(pfrm, LocaleString::store_sell_items)->c_str(),
                false);

            show_sell_icons(pfrm, game);
        }

        game.camera().push_ballast({player_pos.x + 25, player_pos.y});

    } break;

    case DisplayMode::animate_in_buy: {
        timer_ += delta;

        static const auto anim_duration = milliseconds(150);

        const auto amount = smoothstep(0.f, anim_duration, timer_);
        buy_item_bar_->set_display_percentage(amount);
        // sell_item_bar_->set_display_percentage(amount);

        if (timer_ > anim_duration) {
            timer_ = 0;

            buy_item_bar_->set_display_percentage(1.f);
            // sell_item_bar_->set_display_percentage(1.f);

            show_score(pfrm, game, UIMetric::Align::right);

            display_mode_ = DisplayMode::show_buy;

            show_label(
                pfrm,
                game,
                locale_string(pfrm, LocaleString::store_buy_items)->c_str(),
                true);

            show_buy_icons(pfrm, game);
        }

        game.camera().push_ballast({player_pos.x - 25, player_pos.y});

    } break;

    case DisplayMode::inflate_buy_options: {
        timer_ += delta;
        const auto anim_time = milliseconds(200);

        const auto amount = smoothstep(0.f, anim_time, timer_);
        buy_options_bar_->set_display_percentage(amount);

        if (timer_ > anim_time) {
            timer_ = 0;

            // redraw_selector(0, false);
            // selector_.reset();

            buy_options_bar_->set_display_percentage(1.f);

            const auto y = u8(2 + selector_pos_ * 5);

            pfrm.set_tile(Layer::overlay, 15, y, 129);
            pfrm.set_tile(Layer::overlay, 10, y, 129);
            pfrm.set_tile(Layer::overlay, 15, y + 5, 129);
            pfrm.set_tile(Layer::overlay, 10, y + 5, 129);

            buy_sell_icon_.emplace(pfrm, 412, OverlayCoord{7, u8(y + 2)});

            info_icon_.emplace(pfrm, 416, OverlayCoord{12, u8(y + 2)});


            display_mode_ = DisplayMode::show_buy_options;
            pfrm.speaker().play_sound("scroll", 1);

            show_buy_option_label(pfrm, game);
        }

        game.camera().push_ballast({player_pos.x - 60, player_pos.y});
    } break;

    case DisplayMode::show_buy_options: {
        timer_ += delta;

        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            redraw_selector(112, false);
        }

        if (pfrm.keyboard().down_transition<Key::right>()) {
            if (selector_x_ < 2) {
                ++selector_x_;
                pfrm.speaker().play_sound("scroll", 1);
                show_buy_option_label(pfrm, game);
            }
        } else if (pfrm.keyboard().down_transition<Key::left>() or
                   pfrm.keyboard().down_transition(
                       game.persistent_data().settings_.action1_key_)) {
            if (selector_x_ > 1) {
                --selector_x_;
                pfrm.speaker().play_sound("scroll", 1);
                show_buy_option_label(pfrm, game);
            } else {
                display_mode_ = DisplayMode::deflate_buy_options;
                timer_ = 0;
            }
        }
        if (pfrm.keyboard().down_transition(action_key)) {
            if (selector_x_ == 1) {
                const auto item =
                    get_buy_item(game, buy_page_num_, selector_pos_);

                const auto price = scavenger_markup_price(item);

                auto offset = buy_page_num_ * 3 + selector_pos_;
                if (offset < (int)game.scavenger()->inventory_.size()) {
                    if (price not_eq 0 and game.score() >= (u64)price) {
                        game.score() -= price;

                        auto it = game.scavenger()->inventory_.begin() + offset;
                        game.scavenger()->inventory_.erase(it);

                        game.inventory().push_item(pfrm, game, item, false);
                    }
                }
            } else if (selector_x_ == 2) {
                auto resume_state = make_deferred_state<ItemShopState>(
                    DisplayMode::animate_in_buy, selector_pos_, buy_page_num_);

                const auto item =
                    get_buy_item(game, buy_page_num_, selector_pos_);

                if (auto handler = inventory_item_handler(item)) {
                    const LocaleString* dialog = handler->scavenger_dialog_;
                    return state_pool().create<DialogState>(resume_state,
                                                            dialog);
                }
            }
            display_mode_ = DisplayMode::deflate_buy_options;
            timer_ = 0;
        }

        game.camera().push_ballast({player_pos.x - 60, player_pos.y});
    } break;

    case DisplayMode::deflate_buy_options: {
        timer_ += delta;
        const auto anim_time = milliseconds(100);

        const auto amount = smoothstep(0.f, anim_time, timer_);
        buy_options_bar_->set_display_percentage(1.f - amount);

        if (timer_ > anim_time) {
            timer_ = 0;

            redraw_selector(0, false);
            selector_.reset();

            buy_options_bar_->set_display_percentage(0.f);

            display_mode_ = DisplayMode::show_buy;

            show_label(
                pfrm,
                game,
                locale_string(pfrm, LocaleString::store_buy_items)->c_str(),
                true);

            show_buy_icons(pfrm, game);

            selector_x_ = 0;
            pfrm.speaker().play_sound("scroll", 1);
        }

        game.camera().push_ballast({player_pos.x - 25, player_pos.y});

    } break;

    case DisplayMode::show_buy:
        if (pfrm.keyboard().down_transition<Key::right>()) {
            display_mode_ = DisplayMode::to_sell_menu;
            hide_label(pfrm);
            timer_ = 0;
        } else if (pfrm.keyboard().down_transition<Key::left>()) {
            timer_ = 0;
            display_mode_ = DisplayMode::exit_left;
        } else if (pfrm.keyboard().down_transition(action_key)) {
            auto item = get_sale_item(game, sell_page_num_, selector_pos_);
            if (item not_eq Item::Type::null) {
                timer_ = 0;
                display_mode_ = DisplayMode::inflate_buy_options;
                while (not selector_shaded_) {
                    redraw_selector(112, false);
                }
                selector_x_ = 1;
                const auto c = OverlayCoord{6, u8(2 + selector_pos_ * 5)};
                buy_options_bar_.emplace(pfrm, 10, 6, c);
            }
        } else if (pfrm.keyboard().down_transition(
                       game.persistent_data().settings_.action1_key_)) {
            timer_ = 0;
            display_mode_ = DisplayMode::exit_left;
        } else if (pfrm.keyboard().down_transition<Key::down>()) {
            if (selector_pos_ < 2) {
                ++selector_pos_;
                pfrm.speaker().play_sound("scroll", 1);
            } else if (buy_items_remaining_) {
                buy_page_num_++;
                selector_pos_ = 0;
                show_buy_icons(pfrm, game);
                pfrm.speaker().play_sound("scroll", 1);
            }
        } else if (pfrm.keyboard().down_transition<Key::up>()) {
            if (selector_pos_ > 0) {
                --selector_pos_;
                pfrm.speaker().play_sound("scroll", 1);
            } else if (buy_page_num_ > 0) {
                buy_page_num_--;
                selector_pos_ = 2;
                show_buy_icons(pfrm, game);
                pfrm.speaker().play_sound("scroll", 1);
            }
        }

        timer_ += delta;
        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            redraw_selector(112, false);
        }

        game.camera().push_ballast({player_pos.x - 25, player_pos.y});
        break;

    case DisplayMode::to_sell_menu: {
        timer_ += delta;

        static const auto anim_duration = milliseconds(150);

        const auto amount = smoothstep(0.f, anim_duration, timer_);
        buy_item_bar_->set_display_percentage(1.f - amount);
        sell_item_bar_->set_display_percentage(amount);

        if (timer_ > anim_duration) {
            timer_ = 0;

            redraw_selector(0, false);
            selector_.reset();

            buy_item_bar_->set_display_percentage(0.f);
            sell_item_bar_->set_display_percentage(1.f);

            display_mode_ = DisplayMode::show_sell;

            show_label(
                pfrm,
                game,
                locale_string(pfrm, LocaleString::store_sell_items)->c_str(),
                false);

            show_score(pfrm, game, UIMetric::Align::left);

            show_sell_icons(pfrm, game);
        }

        game.camera().push_ballast({player_pos.x + 25, player_pos.y});

    } break;

    case DisplayMode::inflate_sell_options: {
        timer_ += delta;
        const auto anim_time = milliseconds(200);

        const auto amount = smoothstep(0.f, anim_time, timer_);
        sell_options_bar_->set_display_percentage(amount);

        if (timer_ > anim_time) {
            timer_ = 0;

            // redraw_selector(0, false);
            // selector_.reset();

            sell_options_bar_->set_display_percentage(1.f);

            const auto y = u8(2 + selector_pos_ * 5);

            const auto st = calc_screen_tiles(pfrm);

            pfrm.set_tile(Layer::overlay, st.x - 16, y, 129);
            pfrm.set_tile(Layer::overlay, st.x - 11, y, 129);
            pfrm.set_tile(Layer::overlay, st.x - 16, y + 5, 129);
            pfrm.set_tile(Layer::overlay, st.x - 11, y + 5, 129);

            buy_sell_icon_.emplace(
                pfrm, 412, OverlayCoord{u8(st.x - 9), u8(y + 2)});

            info_icon_.emplace(
                pfrm, 416, OverlayCoord{u8(st.x - 14), u8(y + 2)});


            display_mode_ = DisplayMode::show_sell_options;
            pfrm.speaker().play_sound("scroll", 1);

            show_sell_option_label(pfrm, game);
        }

        game.camera().push_ballast({player_pos.x + 60, player_pos.y});

    } break;

    case DisplayMode::show_sell_options: {
        timer_ += delta;

        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            redraw_selector(112, true);
        }

        if (pfrm.keyboard().down_transition<Key::left>()) {
            if (selector_x_ < 2) {
                ++selector_x_;
                pfrm.speaker().play_sound("scroll", 1);
                show_sell_option_label(pfrm, game);
            }
        } else if (pfrm.keyboard().down_transition<Key::right>() or
                   pfrm.keyboard().down_transition(
                       game.persistent_data().settings_.action1_key_)) {
            if (selector_x_ > 1) {
                --selector_x_;
                pfrm.speaker().play_sound("scroll", 1);
                show_sell_option_label(pfrm, game);
            } else {
                display_mode_ = DisplayMode::deflate_sell_options;
                timer_ = 0;
            }
        }
        if (pfrm.keyboard().down_transition(action_key)) {
            if (selector_x_ == 1) {

                const auto sale_item =
                    get_sale_item(game, sell_page_num_, selector_pos_);

                int skip = sell_page_num_ * 3 + selector_pos_;

                const auto price = base_price(sale_item);
                if (price == 0) {
                    goto DONE;
                }

                for (int page = 0; page < Inventory::pages; ++page) {
                    for (int row = 0; row < Inventory::rows; ++row) {
                        for (int col = 0; col < Inventory::cols; ++col) {
                            if (skip > 0) {
                                --skip;
                                continue;
                            } else if (skip == 0) {
                                if (game.scavenger()) {
                                    game.scavenger()->inventory_.push_back(
                                        sale_item);
                                }
                                game.inventory().remove_item(page, col, row);
                                game.player().get_score(pfrm, game, price);
                                goto DONE;
                            }
                        }
                    }
                }
            DONE:;

            } else if (selector_x_ == 2) {
                const auto mode_resume = DisplayMode::animate_in_sell;
                auto resume_state = make_deferred_state<ItemShopState>(
                    mode_resume, selector_pos_, sell_page_num_);

                const auto sale_item =
                    get_sale_item(game, sell_page_num_, selector_pos_);

                if (auto handler = inventory_item_handler(sale_item)) {
                    const LocaleString* dialog = handler->scavenger_dialog_;
                    return state_pool().create<DialogState>(resume_state,
                                                            dialog);
                }
            }
            display_mode_ = DisplayMode::deflate_sell_options;
            timer_ = 0;
        }

        game.camera().push_ballast({player_pos.x + 60, player_pos.y});

    } break;

    case DisplayMode::deflate_sell_options: {
        timer_ += delta;
        const auto anim_time = milliseconds(100);

        const auto amount = smoothstep(0.f, anim_time, timer_);
        sell_options_bar_->set_display_percentage(1.f - amount);

        if (timer_ > anim_time) {
            timer_ = 0;

            redraw_selector(0, false);
            selector_.reset();

            sell_options_bar_->set_display_percentage(0.f);

            show_label(
                pfrm,
                game,
                locale_string(pfrm, LocaleString::store_sell_items)->c_str(),
                false);

            show_sell_icons(pfrm, game);

            display_mode_ = DisplayMode::show_sell;
            selector_x_ = 0;

            // Basically, do not play the scroll sound if we just gave the
            // player some score, because the score-added sound would still be
            // playing.
            if (game.player().get_sprite().get_mix().amount_ == 0) {
                pfrm.speaker().play_sound("scroll", 1);
            }
        }

        game.camera().push_ballast({player_pos.x + 25, player_pos.y});

    } break;

    case DisplayMode::show_sell:
        if (pfrm.keyboard().down_transition<Key::left>()) {
            display_mode_ = DisplayMode::to_buy_menu;
            hide_label(pfrm);
            timer_ = 0;
        } else if (pfrm.keyboard().down_transition<Key::right>()) {
            display_mode_ = DisplayMode::exit_right;
            timer_ = 0;
        } else if (pfrm.keyboard().down_transition(action_key)) {
            while (not selector_shaded_) {
                redraw_selector(112, true);
            }
            display_mode_ = DisplayMode::inflate_sell_options;
            selector_x_ = 1;
            const auto st = calc_screen_tiles(pfrm);
            sell_options_bar_.emplace(
                pfrm,
                10,
                6,
                OverlayCoord{u8(st.x - 6), u8(2 + selector_pos_ * 5)});
        } else if (pfrm.keyboard().down_transition(
                       game.persistent_data().settings_.action1_key_)) {
            timer_ = 0;
            display_mode_ = DisplayMode::exit_left;
        } else if (pfrm.keyboard().down_transition<Key::down>()) {
            if (selector_pos_ < 2) {
                ++selector_pos_;
                pfrm.speaker().play_sound("scroll", 1);
            } else if (sell_items_remaining_) {
                sell_page_num_++;
                selector_pos_ = 0;
                show_sell_icons(pfrm, game);
                pfrm.speaker().play_sound("scroll", 1);
            }
        } else if (pfrm.keyboard().down_transition<Key::up>()) {
            if (selector_pos_ > 0) {
                --selector_pos_;
                pfrm.speaker().play_sound("scroll", 1);
            } else if (sell_page_num_ > 0) {
                sell_page_num_--;
                selector_pos_ = 2;
                show_sell_icons(pfrm, game);
                pfrm.speaker().play_sound("scroll", 1);
            }
        }

        timer_ += delta;
        if (timer_ > milliseconds(75)) {
            timer_ = 0;
            redraw_selector(112, true);
        }

        game.camera().push_ballast({player_pos.x + 25, player_pos.y});

        break;

    case DisplayMode::to_buy_menu: {
        timer_ += delta;

        static const auto anim_duration = milliseconds(150);

        const auto amount = smoothstep(0.f, anim_duration, timer_);
        buy_item_bar_->set_display_percentage(amount);
        sell_item_bar_->set_display_percentage(1.f - amount);

        if (timer_ > anim_duration) {
            timer_ = 0;

            redraw_selector(0, true);
            selector_.reset();

            buy_item_bar_->set_display_percentage(1.f);
            sell_item_bar_->set_display_percentage(0.f);

            display_mode_ = DisplayMode::show_buy;

            show_score(pfrm, game, UIMetric::Align::right);

            show_label(
                pfrm,
                game,
                locale_string(pfrm, LocaleString::store_buy_items)->c_str(),
                true);

            show_buy_icons(pfrm, game);
        }

        game.camera().push_ballast({player_pos.x + 25, player_pos.y});
    } break;

    case DisplayMode::exit_left: {
        timer_ += delta;

        static const auto anim_duration = milliseconds(110);

        const auto amount = smoothstep(0.f, anim_duration, timer_);
        buy_item_bar_->set_display_percentage(1.f - amount);

        if (timer_ > anim_duration) {
            timer_ = 0;

            buy_item_bar_->set_display_percentage(0.f);

            return state_pool().create<ActiveState>();
        }
    } break;

    case DisplayMode::exit_right: {
        timer_ += delta;

        static const auto anim_duration = milliseconds(110);

        const auto amount = smoothstep(0.f, anim_duration, timer_);
        sell_item_bar_->set_display_percentage(1.f - amount);

        if (timer_ > anim_duration) {
            timer_ = 0;

            sell_item_bar_->set_display_percentage(0.f);

            return state_pool().create<ActiveState>();
        }
    } break;

    default:
        break;
    }

    if (score_ and last_score not_eq game.score()) {
        score_->set_value(game.score());
    }

    return null_state();
}
