#include "state_impl.hpp"


static const std::array<LocaleString, 6> chat_messages = {
    LocaleString::chat_hello,
    LocaleString::yes,
    LocaleString::no,
    LocaleString::chat_q_got_oranges,
    LocaleString::chat_lets_go,
    LocaleString::chat_found_health};


void QuickChatState::display_time_remaining(Platform&, Game&)
{
}


void QuickChatState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    OverworldState::enter(pfrm, game, prev_state);
    update_text(pfrm, game);
}


void QuickChatState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);
    pfrm.fill_overlay(0);
}


StatePtr QuickChatState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    if (pfrm.keyboard().down_transition<inventory_key>()) {
        return state_pool().create<ActiveState>();
    }

    if (pfrm.keyboard().down_transition<Key::up>()) {
        if (msg_index_ > 0) {
            --msg_index_;
            update_text(pfrm, game);
            pfrm.speaker().play_sound("scroll", 1);
        }
    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        if (static_cast<u32>(msg_index_) < chat_messages.size() - 1) {
            ++msg_index_;
            update_text(pfrm, game);
            pfrm.speaker().play_sound("scroll", 1);
        }
    }

    if (pfrm.keyboard().down_transition(game.action2_key())) {
        net_event::QuickChat chat;
        chat.message_.set(static_cast<u32>(chat_messages[msg_index_]));

        net_event::transmit(pfrm, chat);

        pfrm.sleep(20);
    }

    return null_state();
}


void QuickChatState::update_text(Platform& pfrm, Game& game)
{
    const auto st = calc_screen_tiles(pfrm);

    text_.reset();

    const bool bigfont = locale_requires_doublesize_font();

    for (int i = 0; i < st.x; ++i) {
        if (bigfont) {
            pfrm.set_tile(Layer::overlay, i, st.y - 3, 425);
            pfrm.set_tile(Layer::overlay, i, st.y - 2, 112);
        } else {
            pfrm.set_tile(Layer::overlay, i, st.y - 2, 425);
        }
        pfrm.set_tile(Layer::overlay, i, st.y - 1, 112);
    }

    FontConfiguration font_conf;
    font_conf.double_size_ = bigfont;

    auto str = locale_string(pfrm, chat_messages[msg_index_]);

    text_.emplace(
        pfrm, OverlayCoord{2, u8(st.y - (bigfont ? 2 : 1))}, font_conf);
    text_->assign(": ");
    text_->append(str->c_str());

    if (msg_index_ == 0) {
        pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 1, 151);
    } else {
        pfrm.set_tile(Layer::overlay, st.x - 1, st.y - 1, 153);
    }

    if (static_cast<u32>(msg_index_) == chat_messages.size() - 1) {
        pfrm.set_tile(Layer::overlay, st.x - 2, st.y - 1, 152);
    } else {
        pfrm.set_tile(Layer::overlay, st.x - 2, st.y - 1, 154);
    }

    pfrm.set_tile(Layer::overlay, 0, st.y - 1, 391);
}
