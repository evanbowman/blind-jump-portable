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

private:
    const bool camera_tracking_;
};


static class ActiveState : public OverworldState {
public:
    ActiveState(bool camera_tracking) : OverworldState(camera_tracking)
    {
    }
    void enter(Platform& pfrm, Game& game) override;
    State* update(Platform& pfrm, Game& game, Microseconds delta) override;
    std::optional<Text> text_;
    std::optional<Text> score_;
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
    game.enemies().transform([&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                it = entity_buf.erase(it);
            } else {
                (*it)->update(pfrm, game, delta);
                if (camera_tracking_ &&
                    pfrm.keyboard().pressed<Key::action_1>()) {
                    // NOTE: snake body segments do not make much sense to
                    // center the camera on, so exclude them.
                    if constexpr (not std::is_same<decltype(**it),
                                                   SnakeBody>() and
                                  not std::is_same<decltype(**it),
                                                   SnakeHead>()) {
                        if (within_view_frustum(pfrm.screen(),
                                                (*it)->get_position())) {
                            game.camera().push_ballast((*it)->get_position());
                        }
                    }
                }
                ++it;
            }
        }
    });

    game.camera().update(pfrm, delta, player.get_position());

    check_collisions(pfrm, game, player, game.enemies().get<Turret>());
    check_collisions(pfrm, game, player, game.enemies().get<Dasher>());
    check_collisions(pfrm, game, player, game.details().get<Item>());
    check_collisions(pfrm, game, player, game.effects().get<OrbShot>());
    check_collisions(pfrm, game, player, game.enemies().get<SnakeHead>());
    check_collisions(pfrm, game, player, game.enemies().get<SnakeBody>());
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
    text_.emplace(pfrm, OverlayCoord{1, 17});
    score_.emplace(pfrm, OverlayCoord{1, 18});
    text_->assign(game.player().get_health());
    score_->assign(game.score());
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
        text_.reset();
        score_.reset();
        return &death_fade_state;
    }

    const auto& t_pos = game.transporter().get_position() - Vec2<Float>{0, 22};
    if (manhattan_length(game.player().get_position(), t_pos) < 16) {
        game.player().move(t_pos);
        text_.reset();
        score_.reset();
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

    constexpr auto fade_duration = seconds(3);
    if (counter_ > fade_duration) {
        counter_ = 0;
        return &death_continue_state;
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::coquelicot);
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

    constexpr auto fade_duration = seconds(2);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(
            1.f, ColorConstant::rich_black, ColorConstant::coquelicot);
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
                           ColorConstant::coquelicot);
        return this;
    }
}


State* State::initial()
{
    return &fade_in_state;
}
