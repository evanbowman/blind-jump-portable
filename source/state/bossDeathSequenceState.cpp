#include "state_impl.hpp"


void BossDeathSequenceState::enter(Platform& pfrm,
                                   Game& game,
                                   State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);
    repaint_powerups(
        pfrm, game, true, &health_, &score_, &powerups_, UIMetric::Align::left);
}


void BossDeathSequenceState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);
    powerups_.clear();
    health_.reset();
    score_.reset();
}


StatePtr
BossDeathSequenceState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    game.player().update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    update_ui_metrics(pfrm,
                      game,
                      delta,
                      &health_,
                      &score_,
                      &powerups_,
                      last_health,
                      last_score,
                      UIMetric::Align::left);

    counter_ += delta;

    switch (anim_state_) {
    case AnimState::init: {
        pfrm.speaker().stop_music();

        big_explosion(pfrm, game, boss_position_);

        const auto off = 50.f;

        big_explosion(
            pfrm, game, {boss_position_.x - off, boss_position_.y - off});
        big_explosion(
            pfrm, game, {boss_position_.x + off, boss_position_.y + off});
        counter_ = 0;
        anim_state_ = AnimState::explosion_wait1;
        break;
    }

    case AnimState::explosion_wait1:
        if (counter_ > milliseconds(300)) {
            big_explosion(pfrm, game, boss_position_);
            const auto off = -50.f;

            big_explosion(
                pfrm, game, {boss_position_.x - off, boss_position_.y + off});
            big_explosion(
                pfrm, game, {boss_position_.x + off, boss_position_.y - off});

            anim_state_ = AnimState::explosion_wait2;

            counter_ = 0;
        }
        break;

    case AnimState::explosion_wait2:
        if (counter_ > milliseconds(100)) {
            counter_ = 0;
            anim_state_ = AnimState::fade;

            for (int i = 0; i <
                            [&] {
                                switch (game.difficulty()) {
                                case Settings::Difficulty::count:
                                case Settings::Difficulty::normal:
                                    break;

                                case Settings::Difficulty::survival:
                                case Settings::Difficulty::hard:
                                    return 2;
                                }
                                return 3;
                            }();
                 ++i) {
                game.details().spawn<Item>(
                    rng::sample<32>(boss_position_, rng::utility_state),
                    pfrm,
                    Item::Type::heart);
            }
            game.transporter().set_position(boss_position_);
        }
        break;

    case AnimState::fade:
        constexpr auto fade_duration = seconds(4) + milliseconds(500);
        if (counter_ > seconds(1) + milliseconds(700) and
            not pushed_notification_) {
            pushed_notification_ = true;
            push_notification(pfrm, this, locale_string(defeated_text_));
        }
        if (counter_ > fade_duration) {
            pfrm.screen().fade(0.f);
            return state_pool().create<ActiveState>(game);

        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
            pfrm.screen().fade(amount, ColorConstant::silver_white);
        }
        break;
    }


    return null_state();
}