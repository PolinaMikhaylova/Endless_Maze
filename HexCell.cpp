#pragma once
#include <map>
#include "HexCell.h"

class HexGrid {
public:
    std::map<std::pair<int,int>, HexCell*> cells;

    static constexpr int DQ[6] = { +1, 0, -1, -1, 0, +1 };
    static constexpr int DR[6] = {  0, +1, +1, 0, -1, -1 };

    HexCell* getOrCreate(int q, int r) {
        auto key = std::make_pair(q, r);
        auto it = cells.find(key);
        if (it != cells.end())
            return it->second;

        HexCell* c = new HexCell(q,r);
        cells[key] = c;
        return c;
    }

    HexCell* ensureNeighbor(HexCell* c, int side) {
        if (c->neigh[side] != nullptr)
            return c->neigh[side];

        int nq = c->q + DQ[side];
        int nr = c->r + DR[side];

        HexCell* nb = getOrCreate(nq, nr);

        c->neigh[side] = nb;
        nb->neigh[(side + 3) % 6] = c;

        return nb;
    }

    HexCell* ensureNeighbor(int q, int r, int side) {
        HexCell* c = getOrCreate(q,r);
        return ensureNeighbor(c, side);
    }

    const std::map<std::pair<int,int>, HexCell*>& allCells() const {
        return cells;
    }
};
