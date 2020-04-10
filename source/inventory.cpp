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

                push_notification(pfrm, game, [description, &pfrm](Text& text) {
                    static const auto prefix = "got \"";
                    static const auto suffix = "\"";

                    const auto width = str_len(prefix) + str_len(description) +
                                       str_len(suffix);

                    const auto margin = centered_text_margins(pfrm, width);


                    left_text_margin(text, margin);

                    text.append(prefix);
                    text.append(description);
                    text.append(suffix);

                    right_text_margin(text, margin);
                });
            }

            return;
        }
    }

    // Inventory full!
    push_notification(pfrm, game, [&pfrm](Text& text) {
        static const auto str = "Inventory full";

        const auto margin = centered_text_margins(pfrm, str_len(str));

        left_text_margin(text, margin);
        text.append(str);
        right_text_margin(text, margin);
    });
}
