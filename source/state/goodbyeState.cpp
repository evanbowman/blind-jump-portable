#include "state_impl.hpp"


void GoodbyeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.speaker().stop_music();

    if (auto tm = pfrm.system_clock().now()) {
        game.persistent_data().timestamp_ = *tm;
    }
    pfrm.write_save_data(&game.persistent_data(),
                         sizeof game.persistent_data());

    const auto s_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm, OverlayCoord{1, u8(s_tiles.y - 2)});
    text_->append(locale_string(pfrm, LocaleString::goodbye_text)->c_str());
}


StatePtr GoodbyeState::update(Platform& pfrm, Game&, Microseconds delta)
{
    wait_timer_ += delta;
    if (wait_timer_ > seconds(6)) {
        pfrm.soft_exit();
        pfrm.sleep(10);
    }

    return null_state();
}
