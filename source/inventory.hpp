#pragma once

#include "entity/details/item.hpp"
#include "number/numeric.hpp"
#include "util.hpp"
#include <optional>


class Platform;
class Game;


class Inventory {
public:
    static constexpr const u16 pages = 4;
    static constexpr const u16 rows = 2;
    static constexpr const u16 cols = 5;

    using ItemInfo = Item::Type;

    void push_item(Platform& pfrm,
                   Game& game,
                   Item::Type insert,
                   bool notify = true);

    inline Item::Type get_item(u16 page, u16 column, u16 row) const
    {
        if (UNLIKELY(page == pages or column == cols or row == rows)) {
            return Item::Type::null;
        }
        return data_[page * (rows * cols) + row * cols + column];
    }

    inline int item_count(Item::Type item) const
    {
        int count = 0;

        for (auto& inventory_item : data_) {
            if (inventory_item == item) {
                count += 1;
            }
        }
        return count;
    }

    inline void remove_item(u16 page, u16 column, u16 row)
    {
        if (UNLIKELY(page == pages or column == cols or row == rows)) {
            return;
        }
        remove_item(&data_[page * (rows * cols) + row * cols + column]);
    }

    inline void remove_non_persistent()
    {
        // NOTE: iterating in reverse would be more efficient, but this isn't a
        // frequent operation.
        ItemInfo* item = std::begin(data_);
        for (; item not_eq std::end(data_);) {
            if (not item_is_persistent(*item) and
                *item not_eq Item::Type::null and
                // NOTE: while the long_jump items are not persistent (story)
                // items, the whole point of the long jump items, is that you
                // can resume at a certain point after dying, so removing the
                // items from the inventory would defeat the purpose in this
                // case.
                *item not_eq Item::Type::long_jump_z2 and
                *item not_eq Item::Type::long_jump_z3 and
                *item not_eq Item::Type::long_jump_z4) {
                remove_item(item);
            } else {
                ++item;
            }
        }
    }

private:
    inline void remove_item(ItemInfo* item)
    {
        for (; item not_eq std::end(data_); ++item) {
            if (item + 1 not_eq std::end(data_)) {
                new (item) ItemInfo(std::move(*(item + 1)));
            }
        }
        *(std::end(data_) - 1) = ItemInfo{Item::Type::null};
    }

    ItemInfo data_[pages * rows * cols] = {};
};


Float items_collected_percentage(const Inventory& inventory,
                                 std::optional<int> zone = {});
