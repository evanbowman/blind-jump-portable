#pragma once

#include "number/numeric.hpp"
#include "entity/details/item.hpp"


class Inventory {
public:

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

    inline Item::Type get_item(int page, int column, int row)
    {
        return data_[page * 10 + row * 5 + column].type_;
    }

private:
    ItemInfo data_[40] = {};
};
