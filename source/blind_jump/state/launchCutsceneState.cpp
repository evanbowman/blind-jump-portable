#include "state_impl.hpp"


void LaunchCutsceneState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    if (not dynamic_cast<IntroCreditsState*>(&prev_state)) {
        // The rocket launch sound starts playing during the intro credits. But
        // if we're running this cutscene, and the previous state was anything
        // other than the intro credits scene, we need to jump ahead in the
        // audio track to the proper position.
        pfrm.speaker().play_music("rocketlaunch",
                                  IntroCreditsState::music_offset());
    }

    for (int x = 0; x < TileMap::width; ++x) {
        for (int y = 0; y < TileMap::height; ++y) {
            pfrm.set_tile(Layer::map_0, x, y, 3);
            pfrm.set_tile(Layer::map_1, x, y, 0);
        }
    }

    auto clear_entities = [&](auto& buf) { buf.clear(); };

    game.enemies().transform(clear_entities);
    game.details().transform(clear_entities);
    game.effects().transform(clear_entities);


    game.camera() = Camera{};

    pfrm.screen().fade(1.f);

    pfrm.load_sprite_texture("spritesheet_launch_anim");
    pfrm.load_overlay_texture("overlay_cutscene");
    pfrm.load_tile0_texture("launch_flattened");
    pfrm.load_tile1_texture("tilesheet_top");

    // pfrm.screen().fade(0.f, ColorConstant::silver_white);

    const Vec2<Float> arbitrary_offscreen_location{1000, 1000};

    game.transporter().set_position(arbitrary_offscreen_location);
    game.player().set_visible(false);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    for (int i = 0; i < screen_tiles.x; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 112);
        pfrm.set_tile(Layer::overlay, i, 1, 112);

        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 1, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 2, 112);
        pfrm.set_tile(Layer::overlay, i, screen_tiles.y - 3, 112);
    }

    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 5; ++j) {
            pfrm.set_tile(Layer::background, i, j, 2);
        }
    }


    draw_image(pfrm, 61, 0, 5, 30, 12, Layer::background);

    Proxy::SpriteBuffer buf;

    for (int i = 0; i < 3; ++i) {
        buf.emplace_back();
        buf.back().set_position({136 - 24, 40 + 8});
        buf.back().set_size(Sprite::Size::w16_h32);
        buf.back().set_texture_index(i);
    }

    buf[1].set_origin({-16, 0});
    buf[2].set_origin({-32, 0});

    game.effects().spawn<Proxy>(buf);
    for (auto& p : game.effects().get<Proxy>()) {
        for (int i = 0; i < 3; ++i) {
            p->buffer()[i].set_texture_index(12 + i);
        }
    }
}


void LaunchCutsceneState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.fill_overlay(0);
    altitude_text_.reset();

    // pfrm.load_overlay_texture("overlay");

    game.details().transform([](auto& buf) { buf.clear(); });
    game.effects().transform([](auto& buf) { buf.clear(); });

    pfrm.speaker().stop_music();
}


StatePtr
LaunchCutsceneState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    timer_ += delta;

    if (static_cast<int>(scene_) < static_cast<int>(Scene::within_clouds)) {
        game.camera().update(pfrm,
                             game.persistent_data().settings_.camera_mode_,
                             delta,
                             {(float)pfrm.screen().size().x / 2,
                              (float)pfrm.screen().size().y / 2});
    }


    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
                it = entity_buf.erase(it);
            } else {
                (*it)->update(pfrm, game, delta);
                ++it;
            }
        }
    };

    auto do_camera_shake = [&](int magnitude) {
        camera_shake_timer_ -= delta;

        if (camera_shake_timer_ <= 0) {
            camera_shake_timer_ = milliseconds(80);
            game.camera().shake(magnitude);
        }
    };


    game.details().transform(update_policy);

    if (static_cast<int>(scene_) > static_cast<int>(Scene::fade_transition0)) {
        altitude_ += delta * speed_;

        if (--altitude_update_ == 0) {

            auto screen_tiles = calc_screen_tiles(pfrm);

            altitude_update_ = 30;

            const auto units =
                locale_string(pfrm, LocaleString::distance_units);

            const bool lang_english =
                (locale_language_name(locale_get_language()) == "english");

            static const auto metric_conversion{0.3048f};

            // Ok, I know, not all english speakers use British Imperial
            // Units. Distance units should probably be in the init.lisp
            // script, alongside the language definitions. But, we certainly
            // don't plan on maintaining two separate language definition
            // files for English just so that we can give
            // non-American-english speakers metric units. And we certainly
            // aren't going to assume that Americans know what a meter
            // is. I've lived in America my whole live, and I know for a
            // fact that many Americans will not know what they're looking
            // at if they see a number with an 'm' unit after it.
            const auto altitude =
                lang_english ? altitude_ : altitude_ * metric_conversion;

            auto len =
                integer_text_length(altitude) + utf8::len(units->c_str());

            if (not altitude_text_ or
                (altitude_text_ and altitude_text_->len() not_eq len)) {
                altitude_text_.emplace(
                    pfrm,
                    OverlayCoord{u8((screen_tiles.x - len) / 2),
                                 u8(screen_tiles.y - 2)});
            }

            altitude_text_->assign(altitude);

            altitude_text_->append(units->c_str());
        }
    }

    auto spawn_cloud = [&] {
        const auto swidth = pfrm.screen().size().x;

        // NOTE: just doing this because random numbers are not naturally well
        // distributed, and we don't want the clouds to bunch up too much.
        if (cloud_lane_ == 0) {
            cloud_lane_ = 1;
            const auto offset =
                rng::choice(swidth / 2, rng::critical_state) + 32;
            game.details().spawn<CutsceneCloud>(
                Vec2<Float>{Float(offset), -80});
        } else {
            const auto offset =
                rng::choice(swidth / 2, rng::critical_state) + swidth / 2 + 32;
            game.details().spawn<CutsceneCloud>(
                Vec2<Float>{Float(offset), -80});
            cloud_lane_ = 0;
        }
    };

    auto animate_ship = [&] {
        anim_timer_ += delta;
        static const auto frame_time = milliseconds(170);
        if (anim_timer_ >= frame_time) {
            anim_timer_ -= frame_time;

            if (anim_index_ < 28) {
                anim_index_ += 1;
                for (auto& p : game.effects().get<Proxy>()) {
                    for (int i = 0; i < 3; ++i) {
                        p->buffer()[i].set_texture_index(12 + anim_index_ * 3 +
                                                         i);
                    }
                }
            }
        }
    };

    switch (scene_) {
    case Scene::fade_in0: {
        constexpr auto fade_duration = milliseconds(400);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);
            scene_ = Scene::wait;
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount);
        }
        animate_ship();
        break;
    }

    case Scene::wait:
        if (timer_ > seconds(4) + milliseconds(510)) {
            timer_ = 0;
            scene_ = Scene::fade_transition0;
        }
        animate_ship();
        break;

    case Scene::fade_transition0: {
        animate_ship();
        constexpr auto fade_duration = milliseconds(680);
        if (timer_ <= fade_duration) {
            const auto amount = smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount);
        }

        if (timer_ > fade_duration) {
            timer_ = 0;
            scene_ = Scene::fade_in;

            rng::critical_state = 2017;

            pfrm.screen().fade(1.f, ColorConstant::rich_black, {}, true, true);

            game.effects().transform([](auto& buf) { buf.clear(); });

            pfrm.load_tile0_texture("tilesheet_intro_cutscene_flattened");
            pfrm.load_sprite_texture("spritesheet_intro_clouds");

            for (int i = 0; i < 32; ++i) {
                for (int j = 0; j < 32; ++j) {
                    pfrm.set_tile(Layer::background, i, j, 4);
                }
            }

            const auto screen_tiles = calc_screen_tiles(pfrm);

            for (int i = 0; i < screen_tiles.x; ++i) {
                pfrm.set_tile(Layer::background, i, screen_tiles.y - 2, 18);
            }

            speed_ = 0.0050000f;

            game.details().spawn<CutsceneCloud>(Vec2<Float>{70, 20});
            game.details().spawn<CutsceneCloud>(Vec2<Float>{150, 60});

            game.on_timeout(pfrm, milliseconds(500), [](Platform&, Game& game) {
                game.details().spawn<CutsceneBird>(Vec2<Float>{210, -15}, 0);
                game.details().spawn<CutsceneBird>(Vec2<Float>{180, -20}, 3);
                game.details().spawn<CutsceneBird>(Vec2<Float>{140, -30}, 6);
            });
        }
        break;
    }

    case Scene::fade_in: {
        constexpr auto fade_duration = milliseconds(600);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(
                amount, ColorConstant::rich_black, {}, true, true);
        }

        if (timer_ > milliseconds(600)) {
            scene_ = Scene::rising;
        }

        // do_camera_shake(2);

        cloud_spawn_timer_ -= delta;
        if (cloud_spawn_timer_ <= 0) {
            cloud_spawn_timer_ = milliseconds(450);

            spawn_cloud();
        }
        break;
    }

    case Scene::rising: {
        if (timer_ > seconds(5)) {
            timer_ = 0;
            scene_ = Scene::enter_clouds;
            speed_ = 0.02f;
        }

        cloud_spawn_timer_ -= delta;
        if (cloud_spawn_timer_ <= 0) {
            cloud_spawn_timer_ = milliseconds(350);

            spawn_cloud();
        }

        break;
    }

    case Scene::enter_clouds: {

        if (timer_ > seconds(3)) {
            do_camera_shake(7);

            pfrm.screen().pixelate(
                interpolate(0, 30, Float(timer_ - seconds(3)) / seconds(1)),
                false);

        } else {
            do_camera_shake(3);
        }

        constexpr auto fade_duration = seconds(4);
        if (timer_ > fade_duration) {
            speed_ = 0.1f;

            pfrm.screen().fade(1.f, ColorConstant::silver_white);
            pfrm.screen().pixelate(0);

            timer_ = 0;
            scene_ = Scene::within_clouds;

            for (int i = 0; i < 32; ++i) {
                for (int j = 0; j < 32; ++j) {
                    pfrm.set_tile(Layer::background, i, j, 1);
                }
            }

            draw_image(pfrm, 151, 0, 8, 30, 9, Layer::background);
            int moon_x = 7;
            int moon_y = 1;
            pfrm.set_tile(Layer::background, moon_x, moon_y, 2);
            pfrm.set_tile(Layer::background, moon_x + 1, moon_y, 3);
            pfrm.set_tile(Layer::background, moon_x, moon_y + 1, 32);
            pfrm.set_tile(Layer::background, moon_x + 1, moon_y + 1, 33);

            game.details().transform([](auto& buf) { buf.clear(); });

        } else {
            const auto amount = smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount, ColorConstant::silver_white);

            cloud_spawn_timer_ -= delta;
            if (cloud_spawn_timer_ <= 0) {
                cloud_spawn_timer_ = milliseconds(70);

                spawn_cloud();
            }
        }
        break;
    }

    case Scene::within_clouds:
        if (timer_ > seconds(2) + milliseconds(300)) {
            timer_ = 0;
            scene_ = Scene::exit_clouds;
        }
        break;

    case Scene::exit_clouds: {
        camera_offset_ = interpolate(-50.f, camera_offset_, delta * 0.0000005f);

        game.camera().set_position(pfrm, {0, camera_offset_});

        if (timer_ > seconds(3)) {
            scene_ = Scene::scroll;
        } else {
            speed_ =
                interpolate(0.00900f, 0.095000f, timer_ / Float(seconds(3)));
        }

        constexpr auto fade_duration = milliseconds(1000);
        if (timer_ > fade_duration) {
            pfrm.screen().fade(0.f);
        } else {
            const auto amount = 1.f - smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(amount, ColorConstant::silver_white);
        }
        break;
    }

    case Scene::scroll: {

        camera_offset_ -= delta * 0.0000014f;

        game.camera().set_position(pfrm, {0, camera_offset_});

        if (timer_ > seconds(8)) {
            timer_ = 0;
            scene_ = Scene::fade_out;
        }

        break;
    }

    case Scene::fade_out: {

        constexpr auto fade_duration = milliseconds(2000);
        if (timer_ > fade_duration) {
            altitude_text_.reset();

            pfrm.sleep(5);

            return state_pool().create<NewLevelState>(Level{0});

        } else {
            camera_offset_ -= delta * 0.0000014f;
            game.camera().set_position(pfrm, {0, camera_offset_});

            const auto amount = smoothstep(0.f, fade_duration, timer_);
            pfrm.screen().fade(
                amount, ColorConstant::rich_black, {}, true, true);
        }
        break;
    }
    }

    if (pfrm.keyboard().down_transition(game.action2_key()) and
        static_cast<int>(scene_) > static_cast<int>(Scene::fade_transition0)) {
        pfrm.speaker().play_sound("select", 1);
        return state_pool().create<NewLevelState>(Level{0});
    }

    return null_state();
}
