#include "state_impl.hpp"


// These are static, because it's simply more convenient for the player when the
// inventory page and selector are persistent across opening/closing the
// inventory page.
int InventoryState::page_{0};
Vec2<u8> InventoryState::selector_coord_{0, 0};


void consume_selected_item(Game& game)
{
    game.inventory().remove_item(InventoryState::page_,
                                 InventoryState::selector_coord_.x,
                                 InventoryState::selector_coord_.y);
}


LocalizedText item_description(Platform& pfrm, Item::Type type)
{
    if (auto handler = inventory_item_handler(type)) {
        return locale_string(pfrm, handler->description_);

    } else {
        pfrm.fatal("corrupt item memory");
    }
}


StatePtr InventoryState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition<inventory_key>()) {
        return state_pool().create<ActiveState>();
    }

    if (pfrm.keyboard().down_transition<Key::start>() and
        not is_boss_level(game.level())) {

        if (restore_keystates) {
            restore_keystates->set(static_cast<int>(inventory_key), false);
            restore_keystates->set(static_cast<int>(Key::start), true);
        }
        return state_pool().create<PauseScreenState>(false);
    }

    selector_timer_ += delta;

    constexpr auto fade_duration = milliseconds(400);
    if (fade_timer_ < fade_duration) {
        fade_timer_ += delta;

        if (fade_timer_ >= fade_duration) {
            page_text_.emplace(pfrm, OverlayCoord{1, 1});
            page_text_->assign(page_ + 1);
            update_item_description(pfrm, game);
            display_items(pfrm, game);
        }

        pfrm.screen().fade(smoothstep(0.f, fade_duration, fade_timer_));
    }

    if (fade_timer_ > fade_duration / 2) {

        if (pfrm.keyboard().down_transition<Key::left>()) {
            if (selector_coord_.x > 0) {
                selector_coord_.x -= 1;
                pfrm.speaker().play_sound("scroll", 1);
            } else {
                if (page_ > 0) {
                    pfrm.speaker().play_sound("scroll", 1);
                    page_ -= 1;
                    selector_coord_.x = 4;
                    if (page_text_) {
                        page_text_->assign(page_ + 1);
                    }
                    update_arrow_icons(pfrm);
                    display_items(pfrm, game);
                }
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::right>()) {
            if (selector_coord_.x < 4) {
                selector_coord_.x += 1;
                pfrm.speaker().play_sound("scroll", 1);
            } else {
                if (page_ < Inventory::pages - 1) {
                    pfrm.speaker().play_sound("scroll", 1);
                    page_ += 1;
                    selector_coord_.x = 0;
                    if (page_text_) {
                        page_text_->assign(page_ + 1);
                    }
                    update_arrow_icons(pfrm);
                    display_items(pfrm, game);
                }
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::down>()) {
            if (selector_coord_.y < 1) {
                pfrm.speaker().play_sound("scroll", 1);
                selector_coord_.y += 1;
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::up>()) {
            if (selector_coord_.y > 0) {
                pfrm.speaker().play_sound("scroll", 1);
                selector_coord_.y -= 1;
            }
            update_item_description(pfrm, game);
        }
    }

    if (pfrm.keyboard().down_transition(game.action2_key())) {

        const auto item = game.inventory().get_item(
            page_, selector_coord_.x, selector_coord_.y);

        if (auto handler = inventory_item_handler(item)) {

            if (handler->single_use_ == InventoryItemHandler::yes) {
                consume_selected_item(game);
            }

            if (auto new_state = handler->callback_(pfrm, game)) {
                return new_state;
            } else {
                update_item_description(pfrm, game);
                display_items(pfrm, game);
            }
        }
    }

    if (selector_timer_ > milliseconds(75)) {
        selector_timer_ = 0;
        const OverlayCoord pos{static_cast<u8>(3 + selector_coord_.x * 5),
                               static_cast<u8>(3 + selector_coord_.y * 5)};
        if (selector_shaded_) {
            selector_.emplace(pfrm, OverlayCoord{4, 4}, pos, false, 8);
            selector_shaded_ = false;
        } else {
            selector_.emplace(pfrm, OverlayCoord{4, 4}, pos, false, 16);
            selector_shaded_ = true;
        }
    }

    return null_state();
}


InventoryState::InventoryState(bool fade_in)
{
    if (not fade_in) {
        fade_timer_ = fade_duration_ - Microseconds{1};
    }
}


static void draw_dot_grid(Platform& pfrm)
{
    for (int i = 0; i < 6; ++i) {
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 2, 176);
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 7, 176);
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 12, 176);
    }
}


void InventoryState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    if (not dynamic_cast<ActiveState*>(&prev_state)) {
        pfrm.screen().fade(1.f);
    }

    pfrm.load_overlay_texture("overlay");

    update_arrow_icons(pfrm);
    draw_dot_grid(pfrm);
}


void InventoryState::exit(Platform& pfrm, Game& game, State& next_state)
{
    // Un-fading and re-fading when switching from the inventory view to an item
    // view creates tearing. Only unfade the screen when switching back to the
    // active state.
    selector_.reset();
    left_icon_.reset();
    right_icon_.reset();
    page_text_.reset();
    item_description_.reset();
    item_description2_.reset();
    label_.reset();

    pfrm.fill_overlay(0);

    clear_items();
}


void InventoryState::update_arrow_icons(Platform& pfrm)
{
    switch (page_) {
    case 0:
        right_icon_.emplace(pfrm, 172, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 175, OverlayCoord{1, 7});
        break;

    case Inventory::pages - 1:
        right_icon_.emplace(pfrm, 174, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 173, OverlayCoord{1, 7});
        break;

    default:
        right_icon_.emplace(pfrm, 172, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 173, OverlayCoord{1, 7});
        break;
    }
}


void InventoryState::update_item_description(Platform& pfrm, Game& game)

{
    const auto item =
        game.inventory().get_item(page_, selector_coord_.x, selector_coord_.y);

    OverlayCoord text_loc{3, 15};

    const bool bigfont = locale_requires_doublesize_font();

    if (bigfont) {
        text_loc.y -= 1;
    }

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    if (auto handler = inventory_item_handler(item)) {
        item_description_.emplace(
            pfrm,
            locale_string(pfrm, handler->description_)->c_str(),
            text_loc,
            font_conf);
        // item_description_->append(
        //     locale_string(pfrm, LocaleString::punctuation_period)->c_str());

        if (handler->single_use_) {

            if (bigfont) {
                text_loc.y += 1;
            }

            item_description2_.emplace(
                pfrm, OverlayCoord{text_loc.x, u8(text_loc.y + 2)}, font_conf);

            item_description2_->assign(
                locale_string(pfrm, LocaleString::single_use_warning)->c_str(),
                FontColors{ColorConstant::med_blue_gray,
                           ColorConstant::rich_black});
        } else {
            item_description2_.reset();
        }
    }
}


void InventoryState::clear_items()
{
    for (auto& items : item_icons_) {
        for (auto& item : items) {
            item.reset();
        }
    }
}


void InventoryState::display_items(Platform& pfrm, Game& game)
{
    clear_items();

    auto screen_tiles = calc_screen_tiles(pfrm);

    auto label_str = locale_string(pfrm, LocaleString::items);

    const bool bigfont = locale_requires_doublesize_font();

    if (not bigfont) {
        label_.emplace(
            pfrm,
            OverlayCoord{
                u8(screen_tiles.x - (utf8::len(label_str->c_str()) + 1)), 1});
        label_->assign(label_str->c_str());
    }

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 2; ++j) {

            const auto item = game.inventory().get_item(page_, i, j);

            const OverlayCoord coord{static_cast<u8>(4 + i * 5),
                                     static_cast<u8>(4 + j * 5)};

            if (item not_eq Item::Type::null) {
                if (auto handler = inventory_item_handler(item)) {
                    item_icons_[i][j].emplace(pfrm, handler->icon_, coord);
                }
            }
        }
    }
}
