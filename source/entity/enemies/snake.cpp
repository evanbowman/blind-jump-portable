#include "snake.hpp"
#include "game.hpp"



void Snake::init(const Vec2<Float>& pos)
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

    tile_coord_ = to_quarter_tile_coord(position_.cast<s32>());
}



Snake::Snake(const Vec2<Float>& pos, Game& game)
    : parent_(nullptr)
{
    init(pos);

    game.enemies().spawn<Snake>(pos, this, game, 3);
}



Snake::Snake(const Vec2<Float>& pos, Snake* parent, Game& game, u8 remaining) :
    parent_(parent)
{
    init(pos);

    if (remaining) {
        game.enemies().spawn<Snake>(pos, this, game, --remaining);
    }
}



void Snake::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto last_coord = tile_coord_;

    tile_coord_ = to_quarter_tile_coord(position_.cast<s32>());

    constexpr Float rate(0.000008f);

    if (parent_) {

        if (tile_coord_ not_eq parent_->tile_coord_) {

            if (tile_coord_.x > parent_->tile_coord_.x) {
                position_.x -= rate * dt;
            } else if (tile_coord_.x < parent_->tile_coord_.x) {
                position_.x += rate * dt;
            }

            if (tile_coord_.y > parent_->tile_coord_.y) {
                position_.y -= rate * dt;
            } else if (tile_coord_.y < parent_->tile_coord_.y) {
                position_.y += rate * dt;
            }
        }
    } else /* we're the parent node */ {
        position_ = {
            position_.x - rate * dt,
            position_.y
        };

        if (last_coord not_eq tile_coord_) {
            // TODO: do a direction change?
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
