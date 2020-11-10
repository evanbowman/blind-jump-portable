#include "state_impl.hpp"


void EndingCutsceneState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    game.details().clear();
    game.effects().clear();
    game.enemies().clear();

    game.camera() = Camera{};

    game.camera().update(pfrm,
                         milliseconds(100),
                         {(float)pfrm.screen().size().x / 2,
                          (float)pfrm.screen().size().y / 2});

    pfrm.set_overlay_origin(0, 0);

    pfrm.enable_glyph_mode(false);

    pfrm.load_tile0_texture("ending_scene_flattened");
    pfrm.load_tile1_texture("tilesheet_top");

    const auto screen_tiles = calc_screen_tiles(pfrm);

    draw_image(pfrm, 1, 0, 3, 30, 14, Layer::background);

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::background, i, 2, 9);
    }

    for (int i = 11; i < 23; ++i) {
        pfrm.set_tile(Layer::background, i, 3, 9);
    }

    for (int x = 0; x < TileMap::width; ++x) {
        for (int y = 0; y < TileMap::height; ++y) {
            pfrm.set_tile(Layer::map_0, x, y, 1);
            pfrm.set_tile(Layer::map_1, x, y, 0);
        }
    }

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 112);
        pfrm.set_tile(Layer::overlay, i, 1, 112);

        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 2, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 3, 112);
    }
}


void EndingCutsceneState::exit(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.fill_overlay(0);
    pfrm.enable_glyph_mode(true);
}


StatePtr EndingCutsceneState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    static const auto c = ColorConstant::rich_black;

    counter_ += delta;

    anim_counter_ += delta;

    if (anim_counter_ >= milliseconds(150)) {
        anim_counter_ = 0;
        if (anim_index_ == 1) {
            anim_index_ = 0;
            pfrm.load_tile0_texture("ending_scene_flattened");
        } else {
            anim_index_ = 1;
            pfrm.load_tile0_texture("ending_scene_2_flattened");
        }
    }

    switch (anim_state_) {
    case AnimState::fade_in: {
        constexpr auto fade_duration = milliseconds(1950);
        if (counter_ > fade_duration) {
            counter_ = 0;
            pfrm.screen().fade(0.f, c);
            anim_state_ = AnimState::hold;
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
            pfrm.screen().fade(amount, c, {}, true, true);
            return null_state();
        }
        break;
    }

    case AnimState::hold: {
        if (counter_ > seconds(10) or
            (counter_ > seconds(4) and pfrm.keyboard().any_pressed())) {
            counter_ = 0;
            anim_state_ = AnimState::fade_out;
        }
        break;
    }

    case AnimState::fade_out: {
        constexpr auto fade_duration = milliseconds(3950);
        if (counter_ > fade_duration) {
            pfrm.screen().fade(1.f, c);
            return state_pool().create<EndingCreditsState>();
        } else {
            const auto amount = smoothstep(0.f, fade_duration, counter_);
            pfrm.screen().fade(amount, c, {}, true, true);
            return null_state();
        }
        break;
    }
    }
    return null_state();
}
