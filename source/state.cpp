#include "state.hpp"
#include "game.hpp"
#include "graphics/overlay.hpp"
#include "number/random.hpp"
#include "string.hpp"


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

    std::optional<BossHealthBar> boss_health_bar_;

private:
    std::optional<Text> text_;
    std::optional<Text> score_;
    std::optional<SmallIcon> heart_icon_;
    std::optional<SmallIcon> coin_icon_;
};


void show_boss_health(Platform& pfrm, Game& game, Float percentage)
{
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        if (not state->boss_health_bar_) {
            state->boss_health_bar_.emplace(
                pfrm, 6, OverlayCoord{u8(pfrm.screen().size().x / 8 - 2), 1});
        }

        state->boss_health_bar_->set_health(percentage);
    }
}


void hide_boss_health(Game& game)
{
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        state->boss_health_bar_.reset();
    }
}


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
    void exit(Platform& pfrm, Game& game) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_;
    std::optional<Text> score_;
    std::optional<Text> highscore_;
    std::optional<Text> level_;
    Microseconds counter_ = 0;
    Microseconds counter2_ = 0;
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


// FIXME: this shouldn't be global...
static std::optional<Platform::Keyboard::KeyStates> restore_keystates;


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


class ImageViewState : public State {
public:
    ImageViewState(const char* image_name, ColorConstant background_color);

    void enter(Platform& pfrm, Game& game) override;
    void exit(Platform& pfrm, Game& game) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    const char* image_name_;
    ColorConstant background_color_;
};


class IntroCreditsState : public State {
public:
    void enter(Platform& pfrm, Game& game) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_;
    Microseconds timer_ = 0;
};


class NewLevelState : public State {
public:
    NewLevelState(Level next_level) : next_level_(next_level)
    {
    }

    void enter(Platform& pfrm, Game& game) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_[2];
    Level next_level_;
};


// This is a hidden game state intended for debugging. The user can enter
// various numeric codes, which trigger state changes within the game
// (e.g. jumping to a boss fight/level, spawing specific enemies, setting the
// random seed, etc.)
class CommandCodeState : public State {
public:
    void enter(Platform& pfrm, Game& game) override;
    void exit(Platform& pfrm, Game& game) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    bool handle_command_code(Platform& pfrm, Game& game);

    StringBuffer<10> input_;
    std::optional<Text> input_text_;
    std::optional<Border> entry_box_;
    std::optional<Border> selector_;
    Microseconds selector_timer_ = 0;
    bool selector_shaded_ = false;
    u8 selector_index_ = 0;
    Level next_level_;
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
                 NotebookState,
                 ImageViewState,
                 NewLevelState>
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
                (*it)->on_death(pfrm, game);
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
        enemy_timestep /= 2;
    }

    bool enemies_remaining = false;
    bool enemies_destroyed = false;
    game.enemies().transform([&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
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
                pfrm.set_overlay_tile(x, 0, 74);
            }
        }
        break;

    case NotificationStatus::exit: {
        notification_text_timer += delta;
        if (notification_text_timer > milliseconds(50)) {
            notification_text_timer -= delta;

            const auto tile = pfrm.get_overlay_tile(0, 0);
            if (tile < 74 + 7) {
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


    check_collisions(pfrm, game, player, game.details().get<Item>());

    if (not is_boss_level(game.level())) {
        check_collisions(pfrm, game, player, game.enemies().get<Drone>());
        check_collisions(pfrm, game, player, game.enemies().get<Turret>());
        check_collisions(pfrm, game, player, game.enemies().get<Dasher>());
        check_collisions(pfrm, game, player, game.effects().get<OrbShot>());
        check_collisions(pfrm, game, player, game.enemies().get<SnakeHead>());
        check_collisions(pfrm, game, player, game.enemies().get<SnakeBody>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Drone>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Dasher>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Turret>());
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
    } else {
        check_collisions(
            pfrm, game, player, game.enemies().get<TheFirstExplorer>());
        check_collisions(
            pfrm, game, player, game.effects().get<FirstExplorerBigLaser>());
        check_collisions(
            pfrm, game, player, game.effects().get<FirstExplorerSmallLaser>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<TheFirstExplorer>());
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// ActiveState
////////////////////////////////////////////////////////////////////////////////


void ActiveState::enter(Platform& pfrm, Game& game)
{
    if (restore_keystates) {
        pfrm.keyboard().restore_state(*restore_keystates);
        restore_keystates.reset();

    }

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
        pfrm.sleep(5);
        big_explosion(pfrm, game, game.player().get_position());

        return state_pool_.create<DeathFadeState>();
    }

    if (pfrm.keyboard().down_transition<Key::start, Key::alt_2>()) {

        // We don't update entities, e.g. the player entity, while in the
        // inventory state, so the player will not receive the up_transition
        // keystate unless we push the keystates, and restore after exiting the
        // inventory screen.
        restore_keystates = pfrm.keyboard().dump_state();

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
        pfrm.screen().fade(1.f, current_zone(game).energy_glow_color_);
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
        pfrm.screen().fade(0.f, current_zone(game).energy_glow_color_);
        return state_pool_.create<ActiveState>(true);
    } else {
        pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, counter_),
                           current_zone(game).energy_glow_color_);
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
        pfrm.screen().fade(1.f, current_zone(game).energy_glow_color_);
        return state_pool_.create<FadeOutState>();
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           current_zone(game).energy_glow_color_);
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

        Level next_level = game.level() + 1;

        // backdoor for debugging purposes.
        if (pfrm.keyboard().all_pressed<Key::alt_1, Key::alt_2, Key::start>()) {
            return state_pool_.create<CommandCodeState>();
        }

        return state_pool_.create<NewLevelState>(next_level);

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).energy_glow_color_);

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
                           current_zone(game).injury_glow_color_);
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

    enemy_lethargy_timer = 0;
    game.inventory().remove_non_persistent();
}


void DeathContinueState::exit(Platform& pfrm, Game& game)
{
    text_.reset();
    score_.reset();
    highscore_.reset();
    level_.reset();
}


StatePtr
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    constexpr auto fade_duration = seconds(1);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(1.f,
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);

        constexpr auto stats_time = milliseconds(500);

        if (counter2_ < stats_time) {
            const bool show_stats = counter2_ + delta > stats_time;

            counter2_ += delta;

            if (show_stats) {
                score_.emplace(pfrm, Vec2<u8>{1, 8});
                highscore_.emplace(pfrm, Vec2<u8>{1, 10});
                level_.emplace(pfrm, Vec2<u8>{1, 12});

                for (auto& score : reversed(game.highscores())) {
                    if (score < game.score()) {
                        score = game.score();
                        break;
                    }
                }
                std::sort(game.highscores().rbegin(), game.highscores().rend());


                const auto screen_tiles = calc_screen_tiles(pfrm);

                auto print_metric =
                    [&](Text& target, const char* str, int num) {
                        target.append(str);

                        const auto iters =
                            screen_tiles.x -
                            (str_len(str) + 2 + integer_text_length(num));
                        for (u32 i = 0; i < iters; ++i) {
                            target.append(".");
                        }

                        target.append(num);
                    };

                print_metric(*score_, "score ", game.score());
                print_metric(*highscore_, "high score ", game.highscores()[0]);
                print_metric(*level_, "waypoints ", game.level());
            }
        }

        if (score_) {
            if (pfrm.keyboard().pressed<Key::action_1>() or
                pfrm.keyboard().pressed<Key::action_2>()) {

                game.score() = 0;
                game.player().revive();

                // This is how you would return the player to the beginning of
                // the last zone. Not sure yet whether I want to allow this
                // mechanic yet though...
                //
                // Level lv = game.level();
                // const ZoneInfo* zone = &zone_info(lv);
                // const ZoneInfo* last_zone = &zone_info(lv - 1);
                // while (*zone == *last_zone and lv > 0) {
                //     --lv;
                //     zone = &zone_info(lv);
                //     last_zone = &zone_info(lv - 1);
                // }
                // return state_pool_.create<NewLevelState>(lv);

                return state_pool_.create<NewLevelState>(Level{0});
            }
        }
        return null_state();

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);
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


static void consume_selected_item(Game& game)
{
    game.inventory().remove_item(InventoryState::page_,
                                 InventoryState::selector_coord_.x,
                                 InventoryState::selector_coord_.y);
}


constexpr static const InventoryItemHandler inventory_handlers[] = {
    {Item::Type::null,
     0,
     [](Platform&, Game&) { return null_state(); },
     [] {
         static const auto str = "Empty";
         return str;
     }},
    {Item::Type::old_poster_1,
     193,
     [](Platform&, Game&) {
         static const auto str = "old_poster_flattened";
         return state_pool_.create<ImageViewState>(str,
                                                   ColorConstant::steel_blue);
     },
     [] {
         static const auto str = "Old poster (1)";
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
         consume_selected_item(game);

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
         consume_selected_item(game);

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
    if (pfrm.keyboard().down_transition<Key::start, Key::alt_2>()) {
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

constexpr u16 notebook_margin_tile = 39 + 26 + 6;

NotebookState::NotebookState(const char* text) : str_(text), page_(0)
{
}


void NotebookState::enter(Platform& pfrm, Game&)
{
    pfrm.screen().fade(1.f);
    pfrm.load_overlay_texture("overlay_journal");

    // This is to eliminate display tearing, see other comments on
    // fill_overlay() calls.
    pfrm.fill_overlay(notebook_margin_tile);

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
                pfrm.set_overlay_tile(x, y, notebook_margin_tile);
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
    pfrm.load_overlay_texture("overlay");
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
// ImageViewState
////////////////////////////////////////////////////////////////////////////////


ImageViewState::ImageViewState(const char* image_name,
                               ColorConstant background_color)
    : image_name_(image_name), background_color_(background_color)
{
}


StatePtr ImageViewState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::action_1>()) {
        return state_pool_.create<InventoryState>(false);
    }

    return null_state();
}


void ImageViewState::enter(Platform& pfrm, Game& game)
{
    pfrm.screen().fade(1.f, background_color_);
    pfrm.load_overlay_texture(image_name_);

    const auto screen_tiles = calc_screen_tiles(pfrm);
    for (int x = 0; x < screen_tiles.x - 2; ++x) {
        for (int y = 0; y < screen_tiles.y - 3; ++y) {
            pfrm.set_overlay_tile(x + 1, y + 1, y * 28 + x + 1);
        }
    }
}


void ImageViewState::exit(Platform& pfrm, Game& game)
{
    pfrm.fill_overlay(0);

    pfrm.screen().fade(1.f);
    pfrm.load_overlay_texture("overlay");
}


////////////////////////////////////////////////////////////////////////////////
// NewLevelState
////////////////////////////////////////////////////////////////////////////////


void NewLevelState::enter(Platform& pfrm, Game& game)
{
    pfrm.sleep(15);

    pfrm.screen().fade(1.f);

    const auto s_tiles = calc_screen_tiles(pfrm);

    auto zone = zone_info(next_level_);
    auto last_zone = zone_info(next_level_ - 1);
    if (not(zone == last_zone) or next_level_ == 0) {

        auto pos = OverlayCoord{1, u8(s_tiles.y * 0.3f)};
        text_[0].emplace(pfrm, pos);

        pos.y += 2;

        text_[1].emplace(pfrm, pos);

        const auto margin =
            centered_text_margins(pfrm, str_len(zone.title_line_1));
        left_text_margin(*text_[0], std::max(0, int{margin} - 1));

        text_[0]->append(zone.title_line_1);

        const auto margin2 =
            centered_text_margins(pfrm, str_len(zone.title_line_2));
        left_text_margin(*text_[1], std::max(0, int{margin2} - 1));

        text_[1]->append(zone.title_line_2);

        pfrm.sleep(120);

    } else {
        text_[0].emplace(pfrm, OverlayCoord{1, u8(s_tiles.y - 2)});
        text_[0]->append("waypoint ");
        text_[0]->append(next_level_);
        pfrm.sleep(60);
    }

    game.next_level(pfrm, next_level_);
}


StatePtr NewLevelState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    return state_pool_.create<FadeInState>();
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

        return state_pool_.create<NewLevelState>(Level{0});

    } else {
        return null_state();
    }
}


StatePtr State::initial()
{
    return state_pool_.create<IntroCreditsState>();
}


////////////////////////////////////////////////////////////////////////////////
// CommandCodeState
////////////////////////////////////////////////////////////////////////////////


StatePtr
CommandCodeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto update_selector = [&] {
        selector_timer_ += delta;
        if (selector_timer_ > milliseconds(75)) {
            selector_timer_ = 0;

            auto screen_tiles = calc_screen_tiles(pfrm);

            const auto margin = centered_text_margins(pfrm, 20) - 1;

            const OverlayCoord selector_pos{u8(margin + selector_index_ * 2),
                                            u8(screen_tiles.y - 4)};

            if (selector_shaded_) {
                selector_.emplace(
                    pfrm, OverlayCoord{3, 3}, selector_pos, false, 8);
                selector_shaded_ = false;
            } else {
                selector_.emplace(
                    pfrm, OverlayCoord{3, 3}, selector_pos, false, 16);
                selector_shaded_ = true;
            }
        }
    };

    update_selector();

    if (pfrm.keyboard().down_transition<Key::left>()) {
        if (selector_index_ > 0) {
            selector_index_ -= 1;
        } else {
            selector_index_ = 9;
        }

    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        if (selector_index_ < 9) {
            selector_index_ += 1;
        } else {
            selector_index_ = 0;
        }

    } else if (pfrm.keyboard().down_transition<Key::action_1>()) {
        input_.push_back("0123456789"[selector_index_]);

        auto screen_tiles = calc_screen_tiles(pfrm);

        input_text_.emplace(pfrm,
                            OverlayCoord{u8(centered_text_margins(pfrm, 10)),
                                         u8(screen_tiles.y / 2 - 2)});

        input_text_->append(input_.c_str());

        if (input_.full()) {

            // Why am I even bothering with the way that the cheat system's UI
            // will look!? No one will ever even see it... but I'll know how
            // good/bad it looks, and I guess that's a reason...
            while (not selector_shaded_) {
                update_selector();
            }

            pfrm.sleep(60);

            input_text_->erase();

            if (handle_command_code(pfrm, game)) {
                input_text_->append(" ACCEPTED");

            } else {
                input_text_->append(" REJECTED");
            }

            pfrm.sleep(60);

            input_text_.reset();

            input_.clear();
        }

    } else if (pfrm.keyboard().down_transition<Key::action_2>()) {
        if (input_.empty()) {
            return state_pool_.create<NewLevelState>(next_level_);

        } else {
            input_.pop_back();
            input_text_->assign(input_.c_str());
        }
    }

    return null_state();
}


bool CommandCodeState::handle_command_code(Platform& pfrm, Game& game)
{
    // The command interface interprets the leading three digits as a decimal
    // opcode. Each operation will parse the rest of the trailing digits
    // uniquely, if at all.
    const char* input_str = input_.c_str();

    auto to_num = [](char c) { return int{c} - 48; };

    auto opcode = to_num(input_str[0]) * 100 + to_num(input_str[1]) * 10 +
                  to_num(input_str[2]);

    auto single_parameter = [&]() {
        return to_num(input_str[3]) * 1000000 + to_num(input_str[4]) * 100000 +
               to_num(input_str[5]) * 10000 + to_num(input_str[6]) * 1000 +
               to_num(input_str[7]) * 100 + to_num(input_str[8]) * 10 +
               to_num(input_str[9]);
    };

    switch (opcode) {
    case 222: // Jump to next zone
        for (Level level = next_level_; level < 1000; ++level) {
            auto current_zone = zone_info(level);
            auto next_zone = zone_info(level + 1);

            if (not(current_zone == next_zone)) {
                next_level_ = level + 1;
                return true;
            }
        }
        return false;

    case 999: // Jump to the next boss
        for (Level level = next_level_; level < 1000; ++level) {
            if (is_boss_level(level)) {
                next_level_ = level;
                return true;
            }
        }
        return false;

    case 100: // Add player health. The main reason that this command doesn't
              // accept a parameter, is that someone could accidentally overflow
              // the health variable and cause bugs.
        game.player().add_health(100);
        return true;

    case 105: // Maybe you want to add some health, but not affect the gameplay
              // too much.
        game.player().add_health(5);
        return true;

    case 200: { // Get item
        const auto item = single_parameter();
        if (item >= static_cast<int>(Item::Type::count) or
            item <= static_cast<int>(Item::Type::coin)) {
            return false;
        }
        game.inventory().push_item(pfrm, game, static_cast<Item::Type>(item));
        return true;
    }

    case 300: // Get all items
        for (int i = static_cast<int>(Item::Type::coin) + 1;
             i < static_cast<int>(Item::Type::count);
             ++i) {

            const auto item = static_cast<Item::Type>(i);

            if (not game.inventory().has_item(item)) {
                game.inventory().push_item(pfrm, game, item);
            }
        }
        return true;

    default:
        return false;
    }
}


void CommandCodeState::enter(Platform& pfrm, Game& game)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    const auto margin = centered_text_margins(pfrm, 20) - 1;

    u8 write_xpos = margin + 1;
    for (int i = 1; i < 11; ++i, write_xpos += 2) {
        pfrm.set_overlay_tile(write_xpos, screen_tiles.y - 3, i);
    }

    entry_box_.emplace(pfrm,
                       OverlayCoord{12, 3},
                       OverlayCoord{u8(centered_text_margins(pfrm, 12)),
                                    u8(screen_tiles.y / 2 - 3)},
                       false,
                       16);

    next_level_ = game.level() + 1;
}


void CommandCodeState::exit(Platform& pfrm, Game& game)
{
    pfrm.fill_overlay(0);
}
