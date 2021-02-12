#include "state_impl.hpp"


////////////////////////////////////////////////////////////////////////////////
// CommandCodeState FIXME
//
// This code broke after the transition from ascii to unicode. May need some
// more work. But it's not essential to the game in any way.
//
////////////////////////////////////////////////////////////////////////////////


void CommandCodeState::push_char(Platform& pfrm, Game& game, char c)
{
    // input_.push_back(c);

    // auto screen_tiles = calc_screen_tiles(pfrm);

    // input_text_.emplace(pfrm,
    //                     OverlayCoord{u8(centered_text_margins(pfrm, 10)),
    //                                  u8(screen_tiles.y / 2 - 2)});

    // input_text_->assign(input_.c_str());

    // if (input_.full()) {

    //     // Why am I even bothering with the way that the cheat system's UI
    //     // will look!? No one will ever even see it... but I'll know how
    //     // good/bad it looks, and I guess that's a reason...
    //     while (not selector_shaded_) {
    //         update_selector(pfrm, milliseconds(20));
    //     }

    //     pfrm.sleep(25);

    //     input_text_->erase();

    //     if (handle_command_code(pfrm, game)) {
    //         input_text_->append(" ACCEPTED");

    //     } else {
    //         input_text_->append(" REJECTED");
    //     }

    //     pfrm.sleep(25);

    //     input_text_.reset();

    //     input_.clear();
    // }
}


void CommandCodeState::update_selector(Platform& pfrm, Microseconds dt)
{
    // selector_timer_ += dt;
    // if (selector_timer_ > milliseconds(75)) {
    //     selector_timer_ = 0;

    //     auto screen_tiles = calc_screen_tiles(pfrm);

    //     const auto margin = centered_text_margins(pfrm, 20) - 1;

    //     const OverlayCoord selector_pos{u8(margin + selector_index_ * 2),
    //                                     u8(screen_tiles.y - 4)};

    //     if (selector_shaded_) {
    //         selector_.emplace(pfrm, OverlayCoord{3, 3}, selector_pos, false, 8);
    //         selector_shaded_ = false;
    //     } else {
    //         selector_.emplace(
    //             pfrm, OverlayCoord{3, 3}, selector_pos, false, 16);
    //         selector_shaded_ = true;
    //     }
    // }
}


StatePtr
CommandCodeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    // update_selector(pfrm, delta);

    // if (pfrm.keyboard().down_transition<Key::left>()) {
    //     if (selector_index_ > 0) {
    //         selector_index_ -= 1;
    //     } else {
    //         selector_index_ = 9;
    //     }

    // } else if (pfrm.keyboard().down_transition<Key::right>()) {
    //     if (selector_index_ < 9) {
    //         selector_index_ += 1;
    //     } else {
    //         selector_index_ = 0;
    //     }

    // } else if (pfrm.keyboard().down_transition<Key::down>()) {
    //     if (not input_.empty()) {
    //         while (not input_.full()) {
    //             input_.push_back(*(input_.end() - 1));
    //         }

    //         push_char(pfrm, game, *(input_.end() - 1));
    //     }
    // } else if (pfrm.keyboard().down_transition(game.action2_key())) {
    //     push_char(pfrm, game, "0123456789"[selector_index_]);

    // } else if (pfrm.keyboard().down_transition(game.action1_key())) {
    //     if (input_.empty()) {
    //         return state_pool().create<NewLevelState>(next_level_);

    //     } else {
    //         input_.pop_back();
    //         input_text_->assign(input_.c_str());
    //     }
    // }

    return null_state();
}


bool CommandCodeState::handle_command_code(Platform& pfrm, Game& game)
{
    // // The command interface interprets the leading three digits as a decimal
    // // opcode. Each operation will parse the rest of the trailing digits
    // // uniquely, if at all.
    // const char* input_str = input_.c_str();

    // auto to_num = [](char c) { return int{c} - 48; };

    // auto opcode = to_num(input_str[0]) * 100 + to_num(input_str[1]) * 10 +
    //               to_num(input_str[2]);

    // auto single_parameter = [&]() {
    //     return to_num(input_str[3]) * 1000000 + to_num(input_str[4]) * 100000 +
    //            to_num(input_str[5]) * 10000 + to_num(input_str[6]) * 1000 +
    //            to_num(input_str[7]) * 100 + to_num(input_str[8]) * 10 +
    //            to_num(input_str[9]);
    // };

    // switch (opcode) {
    // case 222: // Jump to next zone
    //     for (Level level = next_level_; level < 1000; ++level) {
    //         auto current_zone = zone_info(level);
    //         auto next_zone = zone_info(level + 1);

    //         if (not(current_zone == next_zone)) {
    //             next_level_ = level + 1;
    //             return true;
    //         }
    //     }
    //     return false;

    // case 999: // Jump to the next boss
    //     for (Level level = next_level_; level < 1000; ++level) {
    //         if (is_boss_level(level)) {
    //             next_level_ = level;
    //             return true;
    //         }
    //     }
    //     return false;

    // // For testing boss levels:
    // case 1:
    //     debug_boss_level(pfrm, boss_0_level);
    // case 2:
    //     debug_boss_level(pfrm, boss_1_level);
    // case 3:
    //     debug_boss_level(pfrm, boss_2_level);

    // case 100: // Add player health. The main reason that this command doesn't
    //           // accept a parameter, is that someone could accidentally overflow
    //           // the health variable and cause bugs.
    //     game.player().add_health(100);
    //     return true;

    // case 105: // Maybe you want to add some health, but not affect the gameplay
    //           // too much.
    //     game.player().add_health(5);
    //     return true;

    // case 200: { // Get item
    //     const auto item = single_parameter();
    //     if (item >= static_cast<int>(Item::Type::count) or
    //         item <= static_cast<int>(Item::Type::coin)) {
    //         return false;
    //     }
    //     game.inventory().push_item(pfrm, game, static_cast<Item::Type>(item));
    //     return true;
    // }

    // case 300: // Get all items
    //     for (int i = static_cast<int>(Item::Type::coin) + 1;
    //          i < static_cast<int>(Item::Type::count);
    //          ++i) {

    //         const auto item = static_cast<Item::Type>(i);

    //         if (game.inventory().item_count(item) == 0) {
    //             game.inventory().push_item(pfrm, game, item);
    //         }
    //     }
    //     return true;

    // case 404: // Drop all items
    //     while (game.inventory().get_item(0, 0, 0) not_eq Item::Type::null) {
    //         game.inventory().remove_item(0, 0, 0);
    //     }
    //     return true;


    // default:
    return false;

    // case 592:
    //     factory_reset(pfrm);
    // }
}


void CommandCodeState::enter(Platform& pfrm, Game& game, State&)
{
    // auto screen_tiles = calc_screen_tiles(pfrm);

    // const auto margin = centered_text_margins(pfrm, 20) - 1;

    // numbers_.emplace(pfrm,
    //                  OverlayCoord{u8(margin + 1), u8(screen_tiles.y - 3)});

    // for (int i = 0; i < 10; ++i) {
    //     numbers_->append(i);
    //     numbers_->append(" ");
    // }

    // entry_box_.emplace(pfrm,
    //                    OverlayCoord{12, 3},
    //                    OverlayCoord{u8(centered_text_margins(pfrm, 12)),
    //                                 u8(screen_tiles.y / 2 - 3)},
    //                    false,
    //                    16);

    // next_level_ = game.level() + 1;
}


void CommandCodeState::exit(Platform& pfrm, Game& game, State&)
{
    // input_text_.reset();
    // entry_box_.reset();
    // selector_.reset();
    // numbers_.reset();
    // pfrm.fill_overlay(0);
}
