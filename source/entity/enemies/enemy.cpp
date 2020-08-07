#include "enemy.hpp"
#include "game.hpp"


const Entity& Enemy::get_target(Game& game)
{
    if (not is_allied_) {
        if (game.peer()) {
            if (manhattan_length(position_, game.player().get_position()) >
                manhattan_length(position_, game.peer()->get_position())) {

                return *game.peer();
            } else {
                return game.player();
            }
        } else {
            return game.player();
        }

    } else {
        int dist = 1000000;
        Entity* result = nullptr;
        game.enemies().transform([&](auto& buf) {
            using T = typename std::remove_reference<decltype(buf)>::type;
            using VT = typename T::ValueType::element_type;

            if (not std::is_same<VT, SnakeBody>() and
                not std::is_same<VT, SnakeHead>()) {

                for (auto& enemy : buf) {
                    if (result == nullptr and not enemy->is_allied()) {
                        result = enemy.get();
                        dist =
                            manhattan_length(position_, enemy->get_position());
                    } else if (enemy->visible() and not enemy->is_allied()) {
                        const auto len =
                            manhattan_length(position_, enemy->get_position());
                        if (len < dist) {
                            dist = len;
                            result = enemy.get();
                        }
                    }
                }
            }
        });

        if (result) {
            return *result;
        } else {
            is_allied_ = false;
            return game.player();
        }
    }
}


OrbShot* Enemy::shoot(Platform& pfrm,
                      Game& game,
                      const Vec2<Float>& position,
                      const Vec2<Float>& target,
                      Float speed,
                      Microseconds duration)
{
    if (is_allied_) {
        if (game.effects().spawn<AlliedOrbShot>(
                position, target, speed, duration)) {
            return (*game.effects().get<AlliedOrbShot>().begin()).get();
        }
    } else {
        if (game.effects().spawn<OrbShot>(position, target, speed, duration)) {
            return (*game.effects().get<OrbShot>().begin()).get();
        }
    }
    return nullptr;
}


void Enemy::debit_health(Platform& pfrm, Health amount)
{
    Entity::debit_health(amount);

    if (pfrm.network_peer().is_connected()) {
        net_event::EnemyHealthChanged e;
        e.id_.set(id());
        e.new_health_.set(get_health());

        net_event::transmit(pfrm, e);
    }
}


void Enemy::health_changed(const net_event::EnemyHealthChanged& hc,
                           Platform& pfrm,
                           Game& game)
{
    if (hc.new_health_.get() < get_health()) {
        sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

        if (alive()) {
            pfrm.speaker().play_sound("click", 1, position_);
        }
    }

    Entity::set_health(std::min(hc.new_health_.get(), get_health()));
}
