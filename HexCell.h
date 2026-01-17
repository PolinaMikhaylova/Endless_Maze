#pragma once
#include <array>

class HexCell {
public:
    int q = 0;
    int r = 0;

    bool generated = false;

    std::array<HexCell*, 6> neigh{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    HexCell(int q_, int r_) : q(q_), r(r_) {}

    void generate() {
        if (!generated) {
            generated = true;
        }
    }
};
