#include "state.hpp"
#include "game.hpp"
#include "graphics/overlay.hpp"


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


class OverworldState : public State {
public:
    OverworldState(bool camera_tracking) : camera_tracking_(camera_tracking)
    {
    }
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;
    void exit(Platform& pfrm, Game& game) override;

private:
    const bool camera_tracking_;
    std::optional<Text> room_clear_text_;
};


static class ActiveState : public OverworldState {
public:
    ActiveState(bool camera_tracking) : OverworldState(camera_tracking)
    {
    }
    void enter(Platform& pfrm, Game& game) override;
    void exit(Platform& pfrm, Game& game) override;
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_;
    std::optional<Text> score_;
    std::optional<SmallIcon> heart_icon_;
    std::optional<SmallIcon> coin_icon_;
} active_state(true);


static class FadeInState : public OverworldState {
public:
    FadeInState() : OverworldState(false)
    {
    }
    void enter(Platform& pfrm, Game& game) override;
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
} fade_in_state;


static class WarpInState : public OverworldState {
public:
    WarpInState() : OverworldState(true)
    {
    }
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    bool shook_ = false;
} warp_in_state;


static class PreFadePauseState : public OverworldState {
public:
    PreFadePauseState() : OverworldState(false)
    {
    }
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;
} pre_fade_pause_state;


static class GlowFadeState : public OverworldState {
public:
    GlowFadeState() : OverworldState(false)
    {
    }
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
} glow_fade_state;


static class FadeOutState : public OverworldState {
public:
    FadeOutState() : OverworldState(false)
    {
    }
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
} fade_out_state;


static class DeathFadeState : public OverworldState {
public:
    DeathFadeState() : OverworldState(false)
    {
    }
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

    Microseconds counter_ = 0;
} death_fade_state;


static class DeathContinueState : public State {
public:
    DeathContinueState()
    {
    }
    void enter(Platform& pfrm, Game& game) override;
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_;
    Microseconds counter_ = 0;
} death_continue_state;


static class InventoryState : public State {
public:
    void enter(Platform& pfrm, Game& game) override;
    void exit(Platform& pfrm, Game& game) override;
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    static constexpr const auto max_page = 3;

    void update_arrow_icons(Platform& pfrm);
    void update_item_description(Platform& pfrm, Game& game);
    void clear_items();
    void display_items(Platform& pfrm, Game& game);

    std::optional<Border> selector_;
    std::optional<SmallIcon> left_icon_;
    std::optional<SmallIcon> right_icon_;
    std::optional<Text> page_text_;
    std::optional<Text> item_description_;
    std::optional<MediumIcon> item_icons_[5][2];

    int page_ = 0;

    Microseconds selector_timer_ = 0;
    Microseconds fade_timer_ = 0;
    bool selector_shaded_ = false;

    Vec2<u8> selector_coord_ = {0, 0};

} inventory_state;


void OverworldState::exit(Platform&, Game&)
{
    room_clear_text_.reset();
}


State* OverworldState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    Player& player = game.player();

    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                it = entity_buf.erase(it);
            } else {
                (*it)->update(pfrm, game, delta);
                ++it;
            }
        }
    };

    game.effects().transform(update_policy);
    game.details().transform(update_policy);

    bool enemies_remaining = false;
    bool enemies_destroyed = false;
    game.enemies().transform([&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                it = entity_buf.erase(it);
                enemies_destroyed = true;
            } else {
                enemies_remaining = true;
                (*it)->update(pfrm, game, delta);
                if (camera_tracking_ &&
                    pfrm.keyboard().pressed<Key::action_1>()) {
                    // NOTE: snake body segments do not make much sense to
                    // center the camera on, so exclude them.
                    if constexpr (not std::is_same<decltype(**it),
                                                   SnakeBody>() and
                                  not std::is_same<decltype(**it),
                                                   SnakeHead>()) {
                        if ((*it)->visible()) {
                            game.camera().push_ballast((*it)->get_position());
                        }
                    }
                }
                ++it;
            }
        }
    });

    if (not room_clear_text_) {
        if (not enemies_remaining and enemies_destroyed) {
            room_clear_text_.emplace(pfrm, "level clear", OverlayCoord{0, 0});

            for (auto& chest : game.details().get<ItemChest>()) {
                chest->unlock();
            }

            game.on_timeout(seconds(4), [this](Platform&, Game&) {
                if (room_clear_text_) {
                    room_clear_text_->erase();
                }
            });
        }
    }

    game.camera().update(pfrm, delta, player.get_position());

    check_collisions(pfrm, game, player, game.enemies().get<Drone>());
    check_collisions(pfrm, game, player, game.enemies().get<Turret>());
    check_collisions(pfrm, game, player, game.enemies().get<Dasher>());
    check_collisions(pfrm, game, player, game.details().get<Item>());
    check_collisions(pfrm, game, player, game.effects().get<OrbShot>());
    check_collisions(pfrm, game, player, game.enemies().get<SnakeHead>());
    check_collisions(pfrm, game, player, game.enemies().get<SnakeBody>());
    check_collisions(
        pfrm, game, game.effects().get<Laser>(), game.enemies().get<Drone>());
    check_collisions(
        pfrm, game, game.effects().get<Laser>(), game.enemies().get<Dasher>());
    check_collisions(
        pfrm, game, game.effects().get<Laser>(), game.enemies().get<Turret>());
    check_collisions(pfrm,
                     game,
                     game.effects().get<Laser>(),
                     game.enemies().get<SnakeBody>());
    check_collisions(pfrm,
                     game,
                     game.effects().get<Laser>(),
                     game.enemies().get<SnakeHead>());
    check_collisions(pfrm,
                     game,
                     game.effects().get<Laser>(),
                     game.enemies().get<SnakeTail>());

    return this;
}


void ActiveState::enter(Platform& pfrm, Game& game)
{
    constexpr u32 overlay_tile_size = 8;
    auto screen_tiles = (pfrm.screen().size() / overlay_tile_size).cast<u8>();

    text_.emplace(pfrm, OverlayCoord{2, u8(screen_tiles.y - 3)});
    score_.emplace(pfrm, OverlayCoord{2, u8(screen_tiles.y - 2)});
    text_->assign(game.player().get_health());
    score_->assign(game.score());

    heart_icon_.emplace(pfrm, 145, OverlayCoord{1, u8(screen_tiles.y - 3)});
    coin_icon_.emplace(pfrm, 146, OverlayCoord{1, u8(screen_tiles.y - 2)});
}


void ActiveState::exit(Platform& pfrm, Game& game)
{
    OverworldState::exit(pfrm, game);

    text_.reset();
    score_.reset();
    heart_icon_.reset();
    coin_icon_.reset();
}


State* ActiveState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().update(pfrm, game, delta);

    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    OverworldState::update(pfrm, game, delta);

    if (last_health not_eq game.player().get_health()) {
        text_->assign(game.player().get_health());
    }

    if (last_score not_eq game.score()) {
        score_->assign(game.score());
    }

    if (game.player().get_health() == 0) {
        return &death_fade_state;
    }

    if (pfrm.keyboard().down_transition<Key::alt_1>()) {
        return &inventory_state;
    }

    const auto& t_pos = game.transporter().get_position() - Vec2<Float>{0, 22};
    if (manhattan_length(game.player().get_position(), t_pos) < 16) {
        game.player().move(t_pos);
        return &pre_fade_pause_state;
    } else {
        return this;
    }
}


void FadeInState::enter(Platform& pfrm, Game& game)
{
    game.player().set_visible(false);
}


State* FadeInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(800);
    if (counter_ > fade_duration) {
        counter_ = 0;
        pfrm.screen().fade(1.f, ColorConstant::electric_blue);
        pfrm.sleep(2);
        game.player().set_visible(true);
        return &warp_in_state;
    } else {
        pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, counter_));
        return this;
    }
}


State* WarpInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    if (counter_ > milliseconds(200) and not shook_) {
        game.camera().shake();
        shook_ = true;
    }

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        counter_ = 0;
        shook_ = false;
        pfrm.screen().fade(0.f, ColorConstant::electric_blue);
        return &active_state;
    } else {
        pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::electric_blue);
        return this;
    }
}


State* PreFadePauseState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.camera().set_speed(1.5f);

    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    if (manhattan_length(pfrm.screen().get_view().get_center() +
                             pfrm.screen().get_view().get_size() / 2.f,
                         game.player().get_position()) < 18) {
        game.camera().set_speed(1.f);
        return &glow_fade_state;
    } else {
        return this;
    }
}


State* GlowFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        counter_ = 0;
        pfrm.screen().fade(1.f, ColorConstant::electric_blue);
        return &fade_out_state;
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::electric_blue);
        return this;
    }
}


State* FadeOutState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(670);
    if (counter_ > fade_duration) {
        counter_ = 0;
        pfrm.screen().fade(1.f);
        game.next_level(pfrm);
        return &fade_in_state;
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           ColorConstant::electric_blue);
        return this;
    }
}


State* DeathFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = seconds(2);
    if (counter_ > fade_duration) {
        counter_ = 0;
        return &death_continue_state;
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::aerospace_orange);
        return this;
    }
}


void DeathContinueState::enter(Platform& pfrm, Game& game)
{
    text_.emplace(pfrm, OverlayCoord{11, 4});
    text_->assign("you died");

    game.player().set_visible(false);
}


State*
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    constexpr auto fade_duration = seconds(1);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(
            1.f, ColorConstant::rich_black, ColorConstant::aerospace_orange);
        if (pfrm.keyboard().pressed<Key::action_1>()) {
            counter_ = 0;
            game.score() = 0;
            game.player().revive();
            game.next_level(pfrm, 0);
            text_.reset();
            return &fade_in_state;
        } else {
            return this;
        }
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           ColorConstant::aerospace_orange);
        return this;
    }
}


State* InventoryState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::alt_1>()) {
        return &active_state;
    }

    if (pfrm.keyboard().down_transition<Key::left>()) {
        if (selector_coord_.x > 0) {
            selector_coord_.x -= 1;
        } else {
            if (page_ > 0) {
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
        } else {
            if (page_ < max_page) {
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
            selector_coord_.y += 1;
        }
        update_item_description(pfrm, game);

    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (selector_coord_.y > 0) {
            selector_coord_.y -= 1;
        }
        update_item_description(pfrm, game);
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

    return this;
}


void InventoryState::enter(Platform& pfrm, Game& game)
{
    update_arrow_icons(pfrm);
    for (int i = 0; i < 6; ++i) {
        pfrm.set_overlay_tile(2 + i * 5, 2, 176);
        pfrm.set_overlay_tile(2 + i * 5, 7, 176);
        pfrm.set_overlay_tile(2 + i * 5, 12, 176);
    }
}


void InventoryState::exit(Platform& pfrm, Game& game)
{
    pfrm.screen().fade(0.f);
    selector_.reset();
    left_icon_.reset();
    right_icon_.reset();
    page_text_.reset();
    item_description_.reset();
    fade_timer_ = 0;

    for (int i = 0; i < 6; ++i) {
        pfrm.set_overlay_tile(2 + i * 5, 2, 0);
        pfrm.set_overlay_tile(2 + i * 5, 7, 0);
        pfrm.set_overlay_tile(2 + i * 5, 12, 0);
    }

    clear_items();
}


void InventoryState::update_arrow_icons(Platform& pfrm)
{
    switch (page_) {
    case 0:
        right_icon_.emplace(pfrm, 172, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 175, OverlayCoord{1, 7});
        break;

    case max_page:
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

    static const OverlayCoord text_loc{3, 15};

    switch (static_cast<Item::Type>(item)) {
        // These items should never appear in the inventory.
    case Item::Type::heart:
    case Item::Type::coin:
        break;

    case Item::Type::null: {
        static const char* text = "Empty.";
        item_description_.emplace(pfrm, text, text_loc);
        break;
    }

    case Item::Type::journal: {
        static const char* text = "Jan's notebook.";
        item_description_.emplace(pfrm, text, text_loc);
        break;
    }

    case Item::Type::blaster: {
        static const char* text = "A powerful laser weapon.";
        item_description_.emplace(pfrm, text, text_loc);
        break;
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

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 2; ++j) {

            const auto item = game.inventory().get_item(page_, i, j);
            if (item not_eq Item::Type::null) {
                auto icon = [&] {
                    switch (item) {
                    case Item::Type::journal:
                        return 177;
                    case Item::Type::blaster:
                        return 181;
                    case Item::Type::heart:
                    case Item::Type::coin:
                    case Item::Type::null:
                        return 0;
                    };
                    return 0;
                }();
                item_icons_[i][j].emplace(
                    pfrm,
                    icon,
                    OverlayCoord{static_cast<u8>(4 + i * 5),
                                 static_cast<u8>(4 + j * 5)});
            }
        }
    }
}


State* State::initial()
{
    return &fade_in_state;
}
