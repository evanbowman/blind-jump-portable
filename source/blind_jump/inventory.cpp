#include "inventory.hpp"
#include "game.hpp"
#include "graphics/overlay.hpp"
#include "localization.hpp"
#include "state.hpp"
#include "string.hpp"


void Inventory::push_item(Platform& pfrm,
                          Game& game,
                          Item::Type insert,
                          bool notify)
{
    if (item_is_persistent(insert) and item_count(insert) > 0) {
        error(pfrm, "attempt to add duplicate persistent item to inventory");
        return;
    }

    if (static_cast<int>(insert) >= static_cast<int>(Item::Type::count)) {
        error(pfrm, "attempt to insert invalid item");
        return;
    }

    for (auto& item : data_) {
        if (item == Item::Type::null) {
            item = insert;

            if (notify) {

                auto description = [&]() -> StringBuffer<48> {
                    if (insert == Item::Type::null) {
                        // Technically, the description for null is Empty, but that
                        // doesn't make sense contextually, so lets use this text
                        // instead.
                        return locale_string(pfrm, LocaleString::nothing)
                            ->c_str();

                    } else {
                        return item_description(pfrm, insert)->c_str();
                    }
                }();

                if (not description.empty()) {

                    NotificationStr str;

                    str += locale_string(pfrm, LocaleString::got_item_before)
                               ->c_str();
                    str += description;
                    str += locale_string(pfrm, LocaleString::got_item_after)
                               ->c_str();

                    push_notification(pfrm, game.state(), str);
                }
            }
            return;
        }
    }

    if (notify) {
        NotificationStr str;
        str += locale_string(pfrm, LocaleString::inventory_full)->c_str();

        push_notification(pfrm, game.state(), str);
    }
}


Float items_collected_percentage(const Inventory& inventory,
                                 std::optional<int> zone)
{
    int total_persistent_items = 0;
    int collected_persistent_items = 0;

    for (Item::Type item = Item::Type::null; item < Item::Type::count;
         item = (Item::Type)((int)item + 1)) {
        if (zone) {
            const int level = [&] {
                switch (*zone) {
                case 0:
                    return boss_0_level - 1;
                case 1:
                    return boss_1_level - 1;
                case 2:
                    return boss_2_level - 1;
                case 3:
                    return boss_3_level - 1;
                default:
                    return 0;
                }
            }();

            auto range = level_range(item);
            if (not level_in_range(level, range)) {
                continue;
            }
        }
        if (item_is_persistent(item)) {
            total_persistent_items += 1;
            if (inventory.item_count(item) > 0) {
                collected_persistent_items += 1;
            }
        }
    }

    if (total_persistent_items not_eq 0) {
        return Float(collected_persistent_items) / total_persistent_items;
    } else {
        return 1.f;
    }
}
