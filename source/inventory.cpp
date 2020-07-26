#include "inventory.hpp"
#include "graphics/overlay.hpp"
#include "localization.hpp"
#include "state.hpp"
#include "string.hpp"


void Inventory::push_item(Platform& pfrm, Game& game, Item::Type insert)
{
    if (item_is_persistent(insert) and item_count(insert) > 0) {
        error(pfrm, "attempt to add duplicate persistent item to inventory");
        return;
    }

    for (auto& item : data_) {
        if (item.type_ == Item::Type::null) {
            item.type_ = insert;

            auto description = [&] {
                if (insert == Item::Type::null) {
                    // Technically, the description for null is Empty, but that
                    // doesn't make sense contextually, so lets use this text
                    // instead.
                    return locale_string(LocaleString::nothing);

                } else {
                    return item_description(insert);
                }
            }();

            if (description) {

                NotificationStr str;

                str += locale_string(LocaleString::got_item_before);
                str += description;
                str += locale_string(LocaleString::got_item_after);

                push_notification(pfrm, game, str);
            }

            return;
        }
    }

    NotificationStr str;
    str += locale_string(LocaleString::inventory_full);

    push_notification(pfrm, game, str);
}


Float items_collected_percentage(const Inventory& inventory)
{
    int total_persistent_items = 0;
    int collected_persistent_items = 0;

    for (Item::Type item = Item::Type::null; item < Item::Type::count;
         item = (Item::Type)((int)item + 1)) {
        if (item_is_persistent(item)) {
            total_persistent_items += 1;
            if (inventory.item_count(item) > 0) {
                collected_persistent_items += 1;
            }
        }
    }

    return Float(collected_persistent_items) / total_persistent_items;
}
