#include "snake.hpp"
#include "blind_jump/game.hpp"
#include "common.hpp"
#include "number/random.hpp"


static constexpr const Float x_move_rate = 0.000040f;

// Tiles are wider than they are tall. If we move with the same vertical rate as
// our horizontal rate, the snake segments will end up spaced out when the snake
// makes a right-angle turn (because the body segments need to cover less
// distance to traverse a tile).
static constexpr const Float y_move_rate = x_move_rate * (12.f / 16.f);


////////////////////////////////////////////////////////////////////////////////
// SnakeNode
////////////////////////////////////////////////////////////////////////////////


SnakeNode::SnakeNode(SnakeNode* parent)
    : Enemy(1, position_, {{16, 16}, {8, 8}}), parent_(parent),
      destruct_timer_(0)
{
}


SnakeNode* SnakeNode::parent() const
{
    return parent_;
}


const Vec2<TIdx>& SnakeNode::tile_coord() const
{
    return tile_coord_;
}


void SnakeNode::update(Platform& pfrm, Game& game, Microseconds dt)
{
    Enemy::update(pfrm, game, dt);

    tile_coord_ = to_quarter_tile_coord(position_.cast<s32>());

    if (destruct_timer_) {
        destruct_timer_ -= dt;

        if (destruct_timer_ <= 0) {
            if (parent()) {
                static const Item::Type item_drop_vec[] = {Item::Type::null};
                on_enemy_destroyed(pfrm, game, 0, position_, 0, item_drop_vec);
                parent()->destroy();
            } else /* No parent, i.e.: we're the head */ {
                static const Item::Type item_drop_vec[] = {
                    Item::Type::heart, Item::Type::coin, Item::Type::null};
                on_enemy_destroyed(pfrm, game, 0, position_, 1, item_drop_vec);
            }
            game.score() += 8;

            kill();
        }
    }
}


void SnakeNode::destroy()
{
    destruct_timer_ = milliseconds(400);
}


////////////////////////////////////////////////////////////////////////////////
// SnakeHead
////////////////////////////////////////////////////////////////////////////////


SnakeHead::SnakeHead(const Vec2<Float>& pos, Game& game)
    : SnakeNode(nullptr), dir_(Dir::left)
{
    set_position(pos);

    sprite_.set_position(pos);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_texture_index(TextureMap::snake_head_profile);

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -11});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    game.enemies().spawn<SnakeBody>(pos, this, game, 4);
}


void SnakeHead::update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (UNLIKELY(state_ == State::sleep)) {
        return;
    }

    const auto last_coord = tile_coord();

    SnakeNode::update(pfrm, game, dt);

    if (last_coord not_eq tile_coord()) {

        const auto& player_pos = game.player().get_position();

        const auto player_tile = to_quarter_tile_coord(player_pos.cast<s32>());

        if (abs(position_.x - player_pos.x) > 80) {
            if (position_.x < player_pos.x) {
                dir_ = Dir::right;
            } else if (position_.x > player_pos.x) {
                dir_ = Dir::left;
            }
        } else if (abs(position_.y - player_pos.y) > 80) {
            if (position_.y < player_pos.y) {
                dir_ = Dir::down;
            } else if (position_.y > player_pos.y) {
                dir_ = Dir::up;
            }
        } else if (player_tile.x == tile_coord().x) {
            if (tile_coord().y < player_tile.y) {
                dir_ = Dir::down;
            } else {
                dir_ = Dir::up;
            }
        } else if (player_tile.y == tile_coord().y) {
            if (tile_coord().x < player_tile.x) {
                dir_ = Dir::right;
            } else {
                dir_ = Dir::left;
            }
        } else {
            if (rng::choice<5>(rng::critical_state) == 0) {
                dir_ = static_cast<Dir>(rng::choice<4>(rng::critical_state));
            }
        }

        // Stay within map bounds
        if (tile_coord().x == 0 and dir_ == Dir::left) {
            dir_ = Dir::right;
        } else if (tile_coord().y == 0 and dir_ == Dir::up) {
            dir_ = Dir::down;
        } else if (tile_coord().x == TileMap::width * 2 and
                   dir_ == Dir::right) {
            dir_ = Dir::left;
        } else if (tile_coord().y == TileMap::height * 2 and
                   dir_ == Dir::down) {
            dir_ = Dir::up;
        }
    }

    switch (dir_) {
    case Dir::up:
        sprite_.set_texture_index(TextureMap::snake_head_up);
        sprite_.set_flip({0, 0});
        position_.y -= y_move_rate * dt;
        break;

    case Dir::down:
        sprite_.set_texture_index(TextureMap::snake_head_down);
        sprite_.set_flip({0, 0});
        position_.y += y_move_rate * dt;
        break;

    case Dir::left:
        sprite_.set_texture_index(TextureMap::snake_head_profile);
        sprite_.set_flip({1, 0});
        position_.x -= x_move_rate * dt;
        break;

    case Dir::right:
        sprite_.set_texture_index(TextureMap::snake_head_profile);
        sprite_.set_flip({0, 0});
        position_.x += x_move_rate * dt;
        break;
    }

    sprite_.set_position(position_);
    shadow_.set_position(position_);
}


////////////////////////////////////////////////////////////////////////////////
// SnakeBody
////////////////////////////////////////////////////////////////////////////////


SnakeBody::SnakeBody(const Vec2<Float>& pos,
                     SnakeNode* parent,
                     Game& game,
                     u8 remaining)
    : SnakeNode(parent)
{
    set_position(pos);

    sprite_.set_position(pos);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    sprite_.set_texture_index(TextureMap::snake_body);

    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_origin({8, -11});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    // SnakeNode::update(game, 0);
    next_coord_ = to_quarter_tile_coord(position_.cast<s32>());


    if (remaining) {
        if (remaining == 1) {
            game.enemies().spawn<SnakeTail>(pos, this, game);
        } else {
            game.enemies().spawn<SnakeBody>(pos, this, game, --remaining);
        }
    }
}


void SnakeBody::update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (UNLIKELY(state_ == State::sleep)) {
        return;
    }

    SnakeNode::update(pfrm, game, dt);

    if (tile_coord() == next_coord_) {
        next_coord_ = parent()->tile_coord();
    }

    if (tile_coord() not_eq next_coord_) {

        if (tile_coord().x > next_coord_.x) {
            position_.x -= x_move_rate * dt;
        } else if (tile_coord().x < next_coord_.x) {
            position_.x += x_move_rate * dt;
        }

        if (tile_coord().y > next_coord_.y) {
            position_.y -= y_move_rate * dt;
        } else if (tile_coord().y < next_coord_.y) {
            position_.y += y_move_rate * dt;
        }
    }

    sprite_.set_position(position_);
    shadow_.set_position(position_);
}


////////////////////////////////////////////////////////////////////////////////
// SnakeBody
////////////////////////////////////////////////////////////////////////////////


static const Microseconds tail_drop_time = seconds(8);


SnakeTail::SnakeTail(const Vec2<Float>& pos, SnakeNode* parent, Game& game)
    : SnakeBody(pos, parent, game, 0), sleep_timer_(seconds(2)),
      drop_timer_(tail_drop_time)
{
    add_health(10);
}


void SnakeTail::update(Platform& pfrm, Game& game, Microseconds dt)
{
    if (UNLIKELY(state_ == State::sleep)) {

        sleep_timer_ -= dt;

        if (sleep_timer_ <= 0) {
            state_ = State::active;

            SnakeNode* current = parent();
            while (current) {
                current->state_ = State::active;
                current = current->parent();
            }
        }
        return;
    }

    drop_timer_ -= dt;
    if (drop_timer_ < 0) {
        if (visible() or pfrm.network_peer().is_connected()) {
            drop_timer_ = tail_drop_time;
            if (length(game.enemies().get<Sinkhole>()) == 0) {
                game.enemies().spawn<Sinkhole>(position_);
            }
        } else {
            drop_timer_ = seconds(1);
        }
    }

    SnakeBody::update(pfrm, game, dt);

    if (tile_coord().x > next_coord_.x) {
        sprite_.set_texture_index(90);
        sprite_.set_flip({true, false});
    } else if (tile_coord().x < next_coord_.x) {
        sprite_.set_texture_index(90);
        sprite_.set_flip({false, false});
    } else if (tile_coord().y > next_coord_.y) {
        sprite_.set_texture_index(91);
        sprite_.set_flip({false, false});
    } else if (tile_coord().y < next_coord_.y) {
        sprite_.set_texture_index(92);
        sprite_.set_flip({false, false});
    }

    const auto& mix = sprite_.get_mix();

    if (mix.color_ not_eq ColorConstant::null) {

        fade_color_anim_.advance(sprite_, dt);

        SnakeNode* current = parent();
        while (current) {
            current->sprite_.set_mix(mix);
            current = current->parent();
        }
    }
}


void SnakeTail::on_death(Platform& pf, Game& game)
{
    pf.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::null};
    on_enemy_destroyed(pf, game, 0, position_, 0, item_drop_vec);

    SnakeNode* current = parent();
    while (current) {
        current->sprite_.set_mix({});
        current = current->parent();
    }

    parent()->destroy();
}


void SnakeTail::on_collision(Platform& pf, Game& game, Laser&)
{
    sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

    damage_ += 1;

    debit_health(pf);
}


void SnakeTail::on_collision(Platform& pf, Game& game, LaserExplosion&)
{
    sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

    damage_ += 8;

    debit_health(pf, 8);
}


void SnakeTail::on_collision(Platform& pf, Game& game, AlliedOrbShot&)
{
    if (not is_allied()) {
        if (sprite_.get_mix().amount_ < 180) {
            pf.sleep(2);
        }

        sprite_.set_mix({current_zone(game).injury_glow_color_, 255});

        damage_ += 1;

        debit_health(pf);
    }
}
