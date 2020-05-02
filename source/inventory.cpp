#include "inventory.hpp"
#include "graphics/overlay.hpp"
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
                    static const auto nothing = "Nothing";
                    return nothing;

                } else {
                    return item_description(insert);
                }
            }();

            if (description) {

                NotificationStr str;

                str += "got \"";
                str += description;
                str += "\"";

                push_notification(pfrm, game, str);
            }

            return;
        }
    }

    NotificationStr str;
    str += "Inventory full";

    push_notification(pfrm, game, str);
}
