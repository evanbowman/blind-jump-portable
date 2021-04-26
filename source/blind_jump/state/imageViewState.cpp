#include "state_impl.hpp"


ImageViewState::ImageViewState(const StringBuffer<48>& image_name,
                               ColorConstant background_color)
    : image_name_(image_name), background_color_(background_color)
{
}


StatePtr ImageViewState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    MenuState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition(game.action2_key())) {
        return state_pool().create<InventoryState>(false);
    }

    return null_state();
}


void ImageViewState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(false);

    pfrm.sleep(1);
    pfrm.screen().fade(1.f, background_color_);
    pfrm.load_overlay_texture(image_name_.c_str());

    const auto screen_tiles = calc_screen_tiles(pfrm);

    draw_image(
        pfrm, 1, 1, 1, screen_tiles.x - 2, screen_tiles.y - 3, Layer::overlay);
}


void ImageViewState::exit(Platform& pfrm, Game& game, State&)
{
    pfrm.sleep(1);
    pfrm.fill_overlay(0);
    pfrm.sleep(1);

    pfrm.enable_glyph_mode(true);
}
