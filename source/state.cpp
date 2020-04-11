#include "state.hpp"
#include "game.hpp"
#include "graphics/overlay.hpp"
#include "string.hpp"
#include "number/random.hpp"


constexpr static const char* surveyor_logbook_str =
    "[2.11.2081]  We've been experiencing strange power surges lately on the "
    "station. The director reassured me that everthing will run smoothly from "
    "now on. The robots have been acting up ever since the last "
    "communications blackout though, I'm afraid next time they'll go mad...";


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


class OverworldState : public State {
public:
    OverworldState(bool camera_tracking) : camera_tracking_(camera_tracking)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
    void exit(Platform& pfrm, Game& game) override;

private:
    const bool camera_tracking_;
};


class ActiveState : public OverworldState {
public:
    ActiveState(bool camera_tracking) : OverworldState(camera_tracking)
    {
    }
    void enter(Platform& pfrm, Game& game) override;
    void exit(Platform& pfrm, Game& game) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_;
    std::optional<Text> score_;
    std::optional<SmallIcon> heart_icon_;
    std::optional<SmallIcon> coin_icon_;
};


static std::optional<Text> notification_text;
static Microseconds notification_text_timer;
static enum class NotificationStatus {
    display,
    exit,
    hidden
} notification_status;


void push_notification(Platform& pfrm,
                       Game& game,
                       Function<16, void(Text&)> notification_builder)
{
    if (dynamic_cast<OverworldState*>(game.state())) {

        notification_text_timer = seconds(3);
        notification_text.emplace(pfrm, OverlayCoord{0, 0});

        notification_status = NotificationStatus::display;

        notification_builder(*notification_text);
    }
}


class FadeInState : public OverworldState {
public:
    FadeInState() : OverworldState(false)
    {
    }
    void enter(Platform& pfrm, Game& game) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class WarpInState : public OverworldState {
public:
    WarpInState() : OverworldState(true)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    bool shook_ = false;
};


class PreFadePauseState : public OverworldState {
public:
    PreFadePauseState() : OverworldState(false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
};


class GlowFadeState : public OverworldState {
public:
    GlowFadeState() : OverworldState(false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class FadeOutState : public OverworldState {
public:
    FadeOutState() : OverworldState(false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class DeathFadeState : public OverworldState {
public:
    DeathFadeState() : OverworldState(false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    Microseconds counter_ = 0;
};


class DeathContinueState : public State {
public:
    DeathContinueState()
    {
    }
    void enter(Platform& pfrm, Game& game) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_;
    Microseconds counter_ = 0;
};


class InventoryState : public State {
public:
    InventoryState(bool fade_in);

    void enter(Platform& pfrm, Game& game) override;
    void exit(Platform& pfrm, Game& game) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    static int page_;
    static Vec2<u8> selector_coord_;

private:
    static constexpr const Microseconds fade_duration_ = milliseconds(400);

    void update_arrow_icons(Platform& pfrm);
    void update_item_description(Platform& pfrm, Game& game);
    void clear_items();
    void display_items(Platform& pfrm, Game& game);

    std::optional<Border> selector_;
    std::optional<SmallIcon> left_icon_;
    std::optional<SmallIcon> right_icon_;
    std::optional<Text> page_text_;
    std::optional<Text> item_description_;
    std::optional<MediumIcon> item_icons_[Inventory::cols][Inventory::rows];

    Microseconds selector_timer_ = 0;
    Microseconds fade_timer_ = 0;
    bool selector_shaded_ = false;
};


class NotebookState : public State {
public:
    // NOTE: The NotebookState class does not store a local copy of the text
    // string! Do not pass in pointers to a local buffer, only static strings!
    NotebookState(const char* text);

    void enter(Platform& pfrm, Game& game) override;
    void exit(Platform& pfrm, Game& game) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void repaint_page(Platform& pfrm);
    void repaint_margin(Platform& pfrm);

    std::optional<TextView> text_;
    std::optional<Text> page_number_;
    const char* str_;
    int page_;
};


class IntroCreditsState : public State {
public:
    void enter(Platform& pfrm, Game& game) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_;
    Microseconds timer_ = 0;
};


static void state_deleter(State* s);

static const StatePtr null_state()
{
    return {nullptr, state_deleter};
}

template <typename... States> class StatePool {
public:
    template <typename TState, typename... Args> StatePtr create(Args&&... args)
    {
        if (auto mem = pool_.get()) {
            new (mem) TState(std::forward<Args>(args)...);

            return {reinterpret_cast<TState*>(mem), state_deleter};
        } else {
            return null_state();
        }
    }

    Pool<std::max({sizeof(States)...}),
         // We should only need memory for two states at any given time: the
         // current state, and the next state.
         3,
         std::max({alignof(States)...})>
        pool_;
};
static StatePool<ActiveState,
                 FadeInState,
                 WarpInState,
                 PreFadePauseState,
                 GlowFadeState,
                 FadeOutState,
                 DeathFadeState,
                 InventoryState,
                 NotebookState>
    state_pool_;


static void state_deleter(State* s)
{
    if (s) {
        s->~State();
        state_pool_.pool_.post(reinterpret_cast<byte*>(s));
    }
}


////////////////////////////////////////////////////////////////////////////////
// OverworldState
////////////////////////////////////////////////////////////////////////////////


static Microseconds enemy_lethargy_timer;


void OverworldState::exit(Platform&, Game&)
{
    notification_text.reset();
    notification_status = NotificationStatus::hidden;
}


StatePtr OverworldState::update(Platform& pfrm, Game& game, Microseconds delta)
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

    auto enemy_timestep = delta;
    if (enemy_lethargy_timer > 0) {
        enemy_lethargy_timer -= delta;
        enemy_timestep /= 4;
    }

    bool enemies_remaining = false;
    bool enemies_destroyed = false;
    game.enemies().transform([&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                it = entity_buf.erase(it);
                enemies_destroyed = true;
            } else {
                enemies_remaining = true;

                (*it)->update(pfrm, game, enemy_timestep);

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

    if (not enemies_remaining and enemies_destroyed) {

        push_notification(pfrm, game, [&pfrm](Text& text) {
            static const auto str = "level clear";

            const auto margin = centered_text_margins(pfrm, str_len(str));

            left_text_margin(text, margin);
            text.append(str);
            right_text_margin(text, margin);
        });

        for (auto& chest : game.details().get<ItemChest>()) {
            chest->unlock();
        }
    }


    switch (notification_status) {
    case NotificationStatus::hidden:
        break;

    case NotificationStatus::display:
        if (notification_text_timer > 0) {
            notification_text_timer -= delta;

        } else {
            notification_text.reset();
            notification_text_timer = 0;
            notification_status = NotificationStatus::exit;

            for (int x = 0; x < 32; ++x) {
                pfrm.set_overlay_tile(x, 0, 73);
            }
        }
        break;

    case NotificationStatus::exit: {
        notification_text_timer += delta;
        if (notification_text_timer > milliseconds(50)) {
            notification_text_timer -= delta;

            const auto tile = pfrm.get_overlay_tile(0, 0);
            if (tile < 73 + 7) {
                for (int x = 0; x < 32; ++x) {
                    pfrm.set_overlay_tile(x, 0, tile + 1);
                }
            } else {
                notification_status = NotificationStatus::hidden;
            }
        }
        break;
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
    check_collisions(pfrm,
                     game,
                     game.effects().get<Laser>(),
                     game.enemies().get<Scarecrow>());


    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// ActiveState
////////////////////////////////////////////////////////////////////////////////


void ActiveState::enter(Platform& pfrm, Game& game)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

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


static void
big_explosion(Platform& pfrm, Game& game, const Vec2<Float>& position)
{
    for (int i = 0; i < 5; ++i) {
        game.effects().spawn<Explosion>(sample<48>(position));
    }

    game.on_timeout(milliseconds(60), [pos = position](Platform&, Game& game) {
        for (int i = 0; i < 4; ++i) {
            game.effects().spawn<Explosion>(sample<48>(pos));
        }
        game.on_timeout(milliseconds(60), [pos](Platform&, Game& game) {
            for (int i = 0; i < 3; ++i) {
                game.effects().spawn<Explosion>(sample<48>(pos));
            }
            game.on_timeout(milliseconds(60), [pos](Platform&, Game& game) {
                for (int i = 0; i < 2; ++i) {
                    game.effects().spawn<Explosion>(sample<48>(pos));
                }
            });
        });
    });

    game.camera().shake(Camera::ShakeMagnitude::two);
}


StatePtr ActiveState::update(Platform& pfrm, Game& game, Microseconds delta)
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
        big_explosion(pfrm, game, game.player().get_position());
        return state_pool_.create<DeathFadeState>();
    }

    if (pfrm.keyboard().down_transition<Key::start>()) {
        return state_pool_.create<InventoryState>(true);
    }

    const auto& t_pos = game.transporter().get_position() - Vec2<Float>{0, 22};
    if (manhattan_length(game.player().get_position(), t_pos) < 16) {
        game.player().move(t_pos);
        return state_pool_.create<PreFadePauseState>();
    } else {
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// FadeInState
////////////////////////////////////////////////////////////////////////////////


void FadeInState::enter(Platform& pfrm, Game& game)
{
    game.player().set_visible(false);
}


StatePtr FadeInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(800);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f, ColorConstant::electric_blue);
        pfrm.sleep(2);
        game.player().set_visible(true);
        return state_pool_.create<WarpInState>();
    } else {
        pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, counter_));
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// WarpInState
////////////////////////////////////////////////////////////////////////////////


StatePtr WarpInState::update(Platform& pfrm, Game& game, Microseconds delta)
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
        shook_ = false;
        pfrm.screen().fade(0.f, ColorConstant::electric_blue);
        return state_pool_.create<ActiveState>(true);
    } else {
        pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::electric_blue);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// PreFadePauseState
////////////////////////////////////////////////////////////////////////////////


StatePtr
PreFadePauseState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.camera().set_speed(1.5f);

    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    if (manhattan_length(pfrm.screen().get_view().get_center() +
                             pfrm.screen().get_view().get_size() / 2.f,
                         game.player().get_position()) < 18) {
        game.camera().set_speed(1.f);
        return state_pool_.create<GlowFadeState>();
    } else {
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// GlowFadeState
////////////////////////////////////////////////////////////////////////////////


StatePtr GlowFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f, ColorConstant::electric_blue);
        return state_pool_.create<FadeOutState>();
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::electric_blue);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// FadeOutState
////////////////////////////////////////////////////////////////////////////////


StatePtr FadeOutState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(670);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f);

        const auto s_tiles = calc_screen_tiles(pfrm);

        Text text(pfrm, {1, u8(s_tiles.y - 2)});

        pfrm.sleep(10);

        text.append("waypoint ");
        text.append(game.level() + 1);
        pfrm.sleep(55);

        game.next_level(pfrm);
        return state_pool_.create<FadeInState>();
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           ColorConstant::electric_blue);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// DeathFadeState
////////////////////////////////////////////////////////////////////////////////


StatePtr DeathFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = seconds(2);
    if (counter_ > fade_duration) {
        return state_pool_.create<DeathContinueState>();
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::aerospace_orange);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// DeathContinueState
////////////////////////////////////////////////////////////////////////////////


void DeathContinueState::enter(Platform& pfrm, Game& game)
{
    text_.emplace(pfrm, OverlayCoord{11, 4});
    text_->assign("you died");

    game.player().set_visible(false);

    game.inventory().remove_non_persistent();
}


StatePtr
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    constexpr auto fade_duration = seconds(1);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(
            1.f, ColorConstant::rich_black, ColorConstant::aerospace_orange);

        if (pfrm.keyboard().pressed<Key::action_2>() or
            pfrm.keyboard().pressed<Key::action_1>()) {

            game.score() = 0;
            game.player().revive();
            game.next_level(pfrm, 0);
            text_.reset();
            return state_pool_.create<FadeInState>();

        } else {
            return null_state();
        }
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           ColorConstant::aerospace_orange);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// InventoryState
////////////////////////////////////////////////////////////////////////////////


// These are static, because it's simply more convenient for the player when the inventory page and selector are persistent across opening/closing the inventory page.
int InventoryState::page_{0};
Vec2<u8> InventoryState::selector_coord_{0, 0};


struct InventoryItemHandler {
    Item::Type type_;
    int icon_;
    StatePtr (*callback_)(Platform& pfrm, Game& game);
    const char* (*description_)();
};


constexpr static const InventoryItemHandler inventory_handlers[] = {
    {Item::Type::null,
     0,
     [](Platform&, Game&) { return null_state(); },
     [] {
         static const auto str = "Empty";
         return str;
     }},
    {Item::Type::surveyor_logbook,
     177,
     [](Platform&, Game&) {
         return state_pool_.create<NotebookState>(surveyor_logbook_str);
     },
     [] {
         static const auto str = "Surveyor's logbook";
         return str;
     }},
    {Item::Type::blaster,
     181,
     [](Platform&, Game&) { return null_state(); },
     [] {
         static const auto str = "Blaster";
         return str;
     }},
    {Item::Type::accelerator,
     185,
     [](Platform&, Game& game) {
         game.inventory().remove_item(InventoryState::page_,
                                      InventoryState::selector_coord_.x,
                                      InventoryState::selector_coord_.y);

         game.player().weapon().accelerate(3, milliseconds(150));

         return null_state();
     },
     [] {
         static const auto str = "Accelerator (60 shots)";
         return str;
     }},
    {Item::Type::lethargy,
     189,
     [](Platform&, Game& game) {
         game.inventory().remove_item(InventoryState::page_,
                                      InventoryState::selector_coord_.x,
                                      InventoryState::selector_coord_.y);

         enemy_lethargy_timer = seconds(18);

         return null_state();
     },
     [] {
         static const auto str = "Lethargy (18 sec)";
         return str;
     }}};


static const InventoryItemHandler* inventory_item_handler(Item::Type type)
{
    for (auto& handler : inventory_handlers) {
        if (handler.type_ == type) {
            return &handler;
        }
    }
    return nullptr;
}


const char* item_description(Item::Type type)
{
    if (auto handler = inventory_item_handler(type)) {
        return handler->description_();

    } else {
        return nullptr;
    }
}


StatePtr InventoryState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::start>()) {
        return state_pool_.create<ActiveState>(true);
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
            if (page_ < Inventory::pages - 1) {
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

    } else if (pfrm.keyboard().down_transition<Key::action_1>()) {

        const auto item = game.inventory().get_item(
            page_, selector_coord_.x, selector_coord_.y);

        if (auto handler = inventory_item_handler(item)) {
            if (auto new_state = handler->callback_(pfrm, game)) {
                return new_state;
            } else {
                update_item_description(pfrm, game);
                display_items(pfrm, game);
            }
        }
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

    return null_state();
}


InventoryState::InventoryState(bool fade_in)
{
    if (not fade_in) {
        fade_timer_ = fade_duration_ - Microseconds{1};
    }
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

    constexpr static const OverlayCoord text_loc{3, 15};

    if (auto handler = inventory_item_handler(item)) {
        item_description_.emplace(pfrm, handler->description_(), text_loc);
        item_description_->append(".");
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
                if (auto handler = inventory_item_handler(item)) {
                    item_icons_[i][j].emplace(
                        pfrm,
                        handler->icon_,
                        OverlayCoord{static_cast<u8>(4 + i * 5),
                                     static_cast<u8>(4 + j * 5)});
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// NotebookState
////////////////////////////////////////////////////////////////////////////////


NotebookState::NotebookState(const char* text) : str_(text), page_(0)
{
}


void NotebookState::enter(Platform& pfrm, Game&)
{
    pfrm.screen().fade(1.f);
    pfrm.load_overlay_texture("bgr_overlay_journal");
    auto screen_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm);
    text_->assign(str_,
                  {1, 2},
                  OverlayCoord{u8(screen_tiles.x - 2), u8(screen_tiles.y - 4)});
    page_number_.emplace(pfrm, OverlayCoord{0, u8(screen_tiles.y - 1)});

    repaint_page(pfrm);
}


void NotebookState::repaint_margin(Platform& pfrm)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    for (int x = 0; x < screen_tiles.x; ++x) {
        for (int y = 0; y < screen_tiles.y; ++y) {
            if (x == 0 or y == 0 or y == 1 or x == screen_tiles.x - 1 or
                y == screen_tiles.y - 2 or y == screen_tiles.y - 1) {
                pfrm.set_overlay_tile(x, y, 39 + 26 + 6);
            }
        }
    }
}


void NotebookState::repaint_page(Platform& pfrm)
{
    const auto size = text_->size();

    page_number_->erase();
    repaint_margin(pfrm);
    page_number_->assign(page_ + 1);
    text_->assign(str_, {1, 2}, size, page_ * (size.y / 2));
}


void NotebookState::exit(Platform& pfrm, Game&)
{
    const auto screen_tiles = calc_screen_tiles(pfrm);

    // clear margin
    for (int x = 0; x < screen_tiles.x; ++x) {
        for (int y = 0; y < screen_tiles.y; ++y) {
            if (x == 0 or y == 0 or y == 1 or x == screen_tiles.x - 1 or
                y == screen_tiles.y - 2 or y == screen_tiles.y - 1) {
                pfrm.set_overlay_tile(x, y, 0);
            }
        }
    }

    text_.reset();
    pfrm.load_overlay_texture("bgr_overlay");
}


StatePtr NotebookState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::action_1>()) {
        return state_pool_.create<InventoryState>(false);
    }

    if (pfrm.keyboard().down_transition<Key::down>()) {
        if (text_->parsed() not_eq str_len(str_)) {
            page_ += 1;
            repaint_page(pfrm);
        }

    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (page_ > 0) {
            page_ -= 1;
            repaint_page(pfrm);
        }
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// IntroCreditsState
////////////////////////////////////////////////////////////////////////////////


void IntroCreditsState::enter(Platform& pfrm, Game& game)
{
    static const char* credits = "Evan Bowman presents";

    auto pos = (pfrm.screen().size() / u32(8)).cast<u8>();

    // Center horizontally, and place text vertically in top third of screen
    pos.x -= str_len(credits) + 1;
    pos.x /= 2;
    pos.y *= 0.35f;

    text_.emplace(pfrm, credits, pos);

    pfrm.screen().fade(1.f);
}


StatePtr
IntroCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    timer_ += delta;

    const auto skip = pfrm.keyboard().down_transition<Key::action_1>();

    if (timer_ > seconds(2) or skip) {
        text_.reset();

        if (not skip) {
            pfrm.sleep(70);
        }

        return state_pool_.create<FadeInState>();

    } else {
        return null_state();
    }
}


StatePtr State::initial()
{
    return state_pool_.create<IntroCreditsState>();
}
