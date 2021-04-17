#include "state_impl.hpp"


void SignalJammerSelectorState::print(Platform& pfrm, const char* str)
{
    const bool bigfont = locale_requires_doublesize_font();

    auto margin =
        centered_text_margins(pfrm, utf8::len(str) * (bigfont ? 2 : 1));

    if (bigfont) {
        margin /= 2;
    }

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    text_.emplace(pfrm, OverlayCoord{}, font_conf);

    left_text_margin(*text_, margin);
    text_->append(str);
    right_text_margin(*text_, margin);
}


void SignalJammerSelectorState::enter(Platform& pfrm,
                                      Game& game,
                                      State& prev_state)
{
    cached_camera_ = game.camera();

    const auto player_pos = game.player().get_position();
    const auto ssize = pfrm.screen().size();
    game.camera().set_position(
        pfrm, {player_pos.x - ssize.x / 2, player_pos.y - ssize.y / 2});

    game.camera().set_speed(1.4f);

    print(pfrm, " ");
}


void SignalJammerSelectorState::exit(Platform& pfrm, Game& game, State&)
{
    game.effects().get<Reticule>().clear();
    game.effects().get<Proxy>().clear();
    game.camera() = cached_camera_;
    text_.reset();
}


StatePtr
SignalJammerSelectorState::update(Platform& pfrm, Game& game, Microseconds dt)
{
    MenuState::update(pfrm, game, dt);

    timer_ += dt;

    constexpr auto fade_duration = milliseconds(500);
    const auto target_fade = 0.75f;

    if (target_) {
        game.camera().push_ballast(game.player().get_position());
        game.camera().update(pfrm,
                             Settings::CameraMode::tracking_strong,
                             dt,
                             target_->get_position());
    }

    for (auto& rt : game.effects().get<Reticule>()) {
        rt->update(pfrm, game, dt);
    }

    for (auto& p : game.effects().get<Proxy>()) {
        p->update(pfrm, game, dt);
    }

    switch (mode_) {
    case Mode::fade_in: {
        const auto amount =
            1.f - target_fade * smoothstep(0.f, fade_duration, timer_);

        if (amount > (1.f - target_fade)) {
            pfrm.screen().fade(amount);
        }

        if (timer_ > fade_duration) {
            pfrm.screen().fade(
                1.f - target_fade, ColorConstant::rich_black, {}, false);
            timer_ = 0;
            if ((target_ = make_selector_target(pfrm, game))) {
                mode_ = Mode::update_selector;
                selector_start_pos_ = game.player().get_position();
                game.effects().spawn<Reticule>(selector_start_pos_);

                static const auto lstr = LocaleString::select_target_text;
                print(pfrm, locale_string(pfrm, lstr)->c_str());

            } else {
                return state_pool().create<InventoryState>(true);
            }
        }
        break;
    }

    case Mode::selected: {
        if (timer_ > milliseconds(100)) {
            timer_ = 0;

            if (flicker_anim_index_ > 9) {
                target_->make_allied(true);

                // Typically, we would have consumed the item already, in the
                // inventory item handler, but in some cases, we want to exit
                // out of this state without using the item.
                consume_selected_item(game);

                for (auto& p : game.effects().get<Proxy>()) {
                    p->colorize({ColorConstant::null, 0});
                }

                pfrm.sleep(8);

                return state_pool().create<InventoryState>(true);

            } else {
                if (flicker_anim_index_++ % 2 == 0) {
                    for (auto& p : game.effects().get<Proxy>()) {
                        p->colorize({ColorConstant::green, 180});
                    }
                } else {
                    for (auto& p : game.effects().get<Proxy>()) {
                        p->colorize({ColorConstant::silver_white, 255});
                    }
                }
            }
        }
        break;
    }

    case Mode::active: {
        if (pfrm.keyboard().down_transition(game.action2_key())) {
            if (target_) {
                mode_ = Mode::selected;
                timer_ = 0;
                pfrm.sleep(3);
            }
        } else if (pfrm.keyboard().down_transition(game.action1_key())) {
            return state_pool().create<InventoryState>(true);
        }

        if (pfrm.keyboard().down_transition<Key::right>() or
            pfrm.keyboard().down_transition<Key::left>()) {
            selector_index_ += 1;
            timer_ = 0;
            if ((target_ = make_selector_target(pfrm, game))) {
                mode_ = Mode::update_selector;
                selector_start_pos_ = [&] {
                    if (length(game.effects().get<Reticule>())) {
                        return (*game.effects().get<Reticule>().begin())
                            ->get_position();
                    }
                    return game.player().get_position();
                }();
            }
        }
        break;
    }

    case Mode::update_selector: {

        constexpr auto anim_duration = milliseconds(150);

        if (timer_ <= anim_duration) {
            const auto pos = interpolate(target_->get_position(),
                                         selector_start_pos_,
                                         Float(timer_) / anim_duration);

            for (auto& sel : game.effects().get<Reticule>()) {
                sel->move(pos);
            }
        } else {
            for (auto& sel : game.effects().get<Reticule>()) {
                sel->move(target_->get_position());
            }
            for (auto& p : game.effects().get<Proxy>()) {
                p->pulse(ColorConstant::green);
            }
            mode_ = Mode::active;
            timer_ = 0;
        }
        break;
    }
    }


    return null_state();
}


Enemy* SignalJammerSelectorState::make_selector_target(Platform& pfrm,
                                                       Game& game)
{
    constexpr u32 count = 8;

    // I wish it wasn't necessary to hold a proxy buffer locally, oh well...
    Buffer<Proxy, count> proxies;
    Buffer<Enemy*, count> targets;
    game.enemies().transform([&](auto& buf) {
        for (auto& element : buf) {
            using T = typename std::remove_reference<decltype(buf)>::type;

            using VT = typename T::ValueType::element_type;

            if (distance(element->get_position(),
                         game.player().get_position()) < 128) {
                if constexpr (std::is_base_of<Enemy, VT>()
                              // NOTE: Currently, I am not allowing hacking for
                              // enemies that do physical damage. On the Gameboy
                              // Advance, it simply isn't realistic to do
                              // collision checking between each enemy, and
                              // every other type of enemy. You also cannot turn
                              // bosses into allies.
                              and not std::is_same<VT, Drone>() and
                              not std::is_same<VT, SnakeBody>() and
                              not std::is_same<VT, SnakeTail>() and
                              not std::is_same<VT, SnakeHead>() and
                              not std::is_same<VT, Gatekeeper>() and
                              not std::is_same<VT, Wanderer>() and
                              not std::is_same<VT, Twin>() and
                              not std::is_same<VT, InfestedCore>()) {
                    targets.push_back(element.get());
                    proxies.emplace_back(*element.get());
                }
            }
        }
    });

    if (not targets.empty()) {
        if (selector_index_ >= static_cast<int>(targets.size())) {
            selector_index_ %= targets.size();
        }
        if (targets[selector_index_] not_eq target_) {
            game.effects().get<Proxy>().clear();
            game.effects().spawn<Proxy>(proxies[selector_index_]);
        }
        return targets[selector_index_];
    } else {
        return nullptr;
    }
}
