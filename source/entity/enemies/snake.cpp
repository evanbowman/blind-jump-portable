#include "snake.hpp"
#include "game.hpp"
#include "number/random.hpp"



static constexpr const Float x_move_rate = 0.000046f;

// Tiles are wider than they are tall. If we move with the same vertical rate as
// our horizontal rate, the snake segments will end up spaced out when the snake
// makes a right-angle turn (because the body segments need to cover less
// distance to traverse a tile).
static constexpr const Float y_move_rate = x_move_rate * (12.f / 16.f);



////////////////////////////////////////////////////////////////////////////////
// SnakeNode
////////////////////////////////////////////////////////////////////////////////



SnakeNode::SnakeNode(SnakeNode* parent) :
    parent_(parent),
    hitbox_{&position_, {16, 16}, {8, 8}}
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



void SnakeNode::update()
{
    tile_coord_ = to_quarter_tile_coord(position_.cast<s32>());
}



////////////////////////////////////////////////////////////////////////////////
// SnakeHead
////////////////////////////////////////////////////////////////////////////////



SnakeHead::SnakeHead(const Vec2<Float>& pos, Game& game)
    : SnakeNode(nullptr),
      dir_(Dir::left)
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

    game.enemies().spawn<SnakeBody>(pos, this, game, 5);
}



void SnakeHead::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto last_coord = tile_coord();

    SnakeNode::update();

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
            if (random_choice<5>() == 0) {
                dir_ = static_cast<Dir>(random_choice<4>());
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
                     u8 remaining) :
    SnakeNode(parent)
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

    SnakeNode::update();
    next_coord_ = tile_coord();

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
    SnakeNode::update();

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

    // const auto tile = game.tiles().get_tile(tile_coord_.x, tile_coord_.y);

    // if (tile not_eq Tile::ledge
    //     and tile not_eq Tile::grass_ledge
    //     and tile not_eq Tile::none) {
    //     shadow_.set_alpha(Sprite::Alpha::translucent);
    // } else {
    //     shadow_.set_alpha(Sprite::Alpha::transparent);
    // }
}



////////////////////////////////////////////////////////////////////////////////
// SnakeBody
////////////////////////////////////////////////////////////////////////////////


SnakeTail::SnakeTail(const Vec2<Float>& pos, SnakeNode* parent, Game& game) :
    SnakeBody(pos, parent, game, 0)
{
    // sprite_.set_texture_index(TextureMap::snake_tail);
}
