#pragma once
#include <string>
enum class ItemType { CONSUMABLE, KEY, SCROLL, UNKNOWN };

struct Item {
    std::string name;
    ItemType type = ItemType::UNKNOWN;
    int value = 0; // heal amt, damage, etc.
};
