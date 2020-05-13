#include "inventory.hpp"
#include "graphics/overlay.hpp"
#include "localization.hpp"
#include "state.hpp"
#include "string.hpp"


void Inventory::push_item(Platform& pfrm, Game& game, Item::Type insert)
{
    for (auto& item : data_) {
        if (item.type_ == Item::Type::null) {
            item.type_ = insert;
            item.parameter_ = 0;

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
