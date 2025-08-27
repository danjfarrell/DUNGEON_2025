#pragma once
#include <vector>
#include "Item.h"

class Inventory {
public:
    explicit Inventory(int cap = 5) : cap_(cap) {}
    bool add(const Item& it) { if ((int)items_.size() >= cap_) return false; items_.push_back(it); return true; }
    void remove(int i) { if (i >= 0 && i < (int)items_.size()) items_.erase(items_.begin() + i); }
    const std::vector<Item>& items() const { return items_; }
private:
    int cap_;
    std::vector<Item> items_;
};

