#include "inventory.hpp"
#include "graphics/overlay.hpp"
#include "state.hpp"


void Inventory::push_item(Platform& pfrm, Game& game, Item::Type insert)
{
    for (auto& item : data_) {
        if (item.type_ == Item::Type::null) {
            item.type_ = insert;
            item.parameter_ = 0;


            if (auto description = item_description(insert)) {
                push_notification(pfrm, game, [&description](Text& text) {
                    text.assign("got \"");
                    text.append(description);
                    text.append("\"");
                });
            }

            return;
        }
    }

    // Inventory full!
    push_notification(
        pfrm, game, [](Text& text) { text.append("Inventory full"); });
}
