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
    std::optional<TextView> text_;
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
            room_clear_text_.emplace(pfrm, "level clear", OverlayCoord{1, 1});

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

    heart_icon_.emplace(pfrm, 78, OverlayCoord{1, u8(screen_tiles.y - 3)});
    coin_icon_.emplace(pfrm, 79, OverlayCoord{1, u8(screen_tiles.y - 2)});
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

    return this;
}


void InventoryState::enter(Platform& pfrm, Game& game)
{
    pfrm.screen().fade(0.4f);
    // text_.emplace(pfrm);
    // text_->assign(
    //     "in these deep solitudes and awful cells, where heav'nly-pensive "
    //     "contemplation dwells, and ever-musing melancholy reigns; what means "
    //     "this tumult in a vestal's veins? why rove my thoughts beyond this "
    //     "last retreat? why feels my heart its long-forgotten heat? yet, yet i "
    //     "love!—from abelard it came, and eloisa yet must kiss the name. dear "
    //     "fatal name! rest ever unreveal'd, nor pass these lips in holy silence "
    //     "seal'd. hide it, my heart, within that close disguise, where mix'd "
    //     "with god's, his lov'd idea lies: o write it not, my hand—the name "
    //     "appears already written—wash it out, my tears! in vain lost eloisa "
    //     "weeps and prays, her heart still dictates, and her hand obeys.",
    //     {0, 0},
    //     (pfrm.screen().size() / u32(8)).cast<u8>(),
    //     0);
}


void InventoryState::exit(Platform& pfrm, Game& game)
{
    pfrm.screen().fade(0.f);
    text_.reset();
}


State* State::initial()
{
    return &fade_in_state;
}
