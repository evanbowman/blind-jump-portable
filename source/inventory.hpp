#pragma once

#include "entity/details/item.hpp"
#include "number/numeric.hpp"
#include "util.hpp"


class Inventory {
public:
    static constexpr const u16 pages = 4;
    static constexpr const u16 rows = 2;
    static constexpr const u16 cols = 5;

    struct ItemInfo {
        Item::Type type_;
        u8 parameter_; // Misc parameter for setting various item properties
    };

    inline void push_item(Item::Type insert)
    {
        for (auto& item : data_) {
            if (item.type_ == Item::Type::null) {
                item.type_ = insert;
                item.parameter_ = 0;
                break;
            }
        }
    }

    inline Item::Type get_item(u16 page, u16 column, u16 row)
    {
        if (UNLIKELY(page == pages or column == cols or row == rows)) {
            return Item::Type::null;
        }
        return data_[page * 10 + row * 5 + column].type_;
    }

    inline bool has_item(Item::Type item)
    {
        for (auto& inventory_item : data_) {
            if (inventory_item.type_ == item) {
                return true;
            }
        }

        return false;
    }

    inline void remove_item(u16 page, u16 column, u16 row)
    {
        if (UNLIKELY(page == pages or column == cols or row == rows)) {
            return;
        }
        remove_item(&data_[page * 10 + row * 5 + column]);
    }

    inline void remove_non_persistent()
    {
        // NOTE: iterating in reverse would be more efficient, but this isn't a
        // frequent operation.
        ItemInfo* item = std::begin(data_);
        for (; item not_eq std::end(data_);) {
            if (not item_is_persistent(item->type_) and
                item->type_ not_eq Item::Type::null) {
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
        *(std::end(data_) - 1) = ItemInfo{Item::Type::null, 0};
    }

    ItemInfo data_[pages * rows * cols] = {};
};
