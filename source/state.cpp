#include "state.hpp"
#include "game.hpp"



bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);



class OverworldState : public State {
public:
    OverworldState(bool camera_tracking) : camera_tracking_(camera_tracking) {}
    State* update(Platform& pfrm, Microseconds delta, Game& game) override;
private:
    const bool camera_tracking_;
};



static class ActiveState : public OverworldState {
public:
    ActiveState(bool camera_tracking) : OverworldState(camera_tracking) {}
    State* update(Platform& pfrm, Microseconds delta, Game& game) override;
} active_state(true);



static class FadeInState : public OverworldState {
public:
    FadeInState() : OverworldState(false) {}
    State* update(Platform& pfrm, Microseconds delta, Game& game) override;
private:
    Microseconds counter_ = 0;
} fade_in_state;



static class PreFadePauseState : public OverworldState {
public:
    PreFadePauseState() : OverworldState(false) {}
    State* update(Platform& pfrm, Microseconds delta, Game& game) override;
} pre_fade_pause_state;



static class FadeOutState : public OverworldState {
public:
    FadeOutState() : OverworldState(false) {}
    State* update(Platform& pfrm, Microseconds delta, Game& game) override;
private:
    Microseconds counter_ = 0;
} fade_out_state;



static class DeathFadeState : public OverworldState {
public:
    DeathFadeState() : OverworldState(false) {}
    State* update(Platform& pfrm, Microseconds delta, Game& game) override;
private:
    Microseconds counter_ = 0;
} death_fade_state;



State* OverworldState::update(Platform& pfrm, Microseconds delta, Game& game)
{
    Player& player = game.player();

    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                entity_buf.erase(it);
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
                entity_buf.erase(it);
            } else {
                (*it)->update(pfrm, game, delta);
                if (camera_tracking_) {
                    if (within_view_frustum(pfrm.screen(), (*it)->get_position())) {
                        game.camera().push_ballast((*it)->get_position());
                    }
                }
                ++it;
            }
        }
    });

    game.camera().update(pfrm, delta, player.get_position());

    check_collisions(pfrm, game, player, game.enemies().get<Turret>());
    check_collisions(pfrm, game, player, game.enemies().get<Dasher>());
    check_collisions(pfrm, game, player, game.effects().get<Item>());

    return this;
}



State* ActiveState::update(Platform& pfrm, Microseconds delta, Game& game)
{
    game.player().update(pfrm, game, delta);

    OverworldState::update(pfrm, delta, game);

    if (game.player().get_health() == 0) {
        return &death_fade_state;
    }

    const auto& t_pos = game.transporter().get_position() - Vec2<Float>{0, 22};
    if (manhattan_length(game.player().get_position(), t_pos) < 16) {
        game.player().move(t_pos);
        return &pre_fade_pause_state;
    } else {
        return this;
    }
}



State* FadeInState::update(Platform& pfrm, Microseconds delta, Game& game)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, delta, game);

    counter_ += delta;

    static const Microseconds fade_duration = 950000;
    if (counter_ > fade_duration) {
        counter_ = 0;
        pfrm.screen().fade(0.f);
        return &active_state;
    } else {
        pfrm.screen().fade(1.f - smoothstep(0.f, fade_duration, counter_));
        return this;
    }
}



State* PreFadePauseState::update(Platform& pfrm,
                                       Microseconds delta,
                                       Game& game)
{
    game.camera().set_speed(1.5f);

    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, delta, game);

    if (manhattan_length(pfrm.screen().get_view().get_center() +
                         pfrm.screen().get_view().get_size() / 2.f,
                         game.player().get_position()) < 18) {
        game.camera().set_speed(1.f);
        return &fade_out_state;
    } else {
        return this;
    }
}



State* FadeOutState::update(Platform& pfrm, Microseconds delta, Game& game)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, delta, game);

    counter_ += delta;

    static const Microseconds fade_duration = 950000;
    if (counter_ > fade_duration) {
        counter_ = 0;
        pfrm.screen().fade(1.f);
        game.next_level(pfrm);
        return &fade_in_state;
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_));
        return this;
    }
}



State* DeathFadeState::update(Platform& pfrm, Microseconds delta, Game& game)
{
    // game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, delta, game);

    counter_ += delta;

    static const Microseconds fade_duration = 1500000;
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(1.f, ColorConstant::coquelicot);
        if (pfrm.keyboard().pressed<Key::action_1>()) {
            counter_ = 0;
            game.player().revive();
            game.next_level(pfrm);
            return &fade_in_state;
        } else {
            return this;
        }
    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::coquelicot);
        return this;
    }
}



State* State::initial()
{
    return &fade_in_state;
}
