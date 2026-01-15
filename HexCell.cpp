#pragma once
#include <map>
#include "HexCell.h"

class HexGrid {
public:
    // Хранилище всех гексов по axial координатам
    std::map<std::pair<int,int>, HexCell*> cells;

    // axial смещения соседей
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

    // Создать соседа, если нет, и связать их
    HexCell* ensureNeighbor(HexCell* c, int side) {
        if (c->neigh[side] != nullptr)
            return c->neigh[side];

        int nq = c->q + DQ[side];
        int nr = c->r + DR[side];

        HexCell* nb = getOrCreate(nq, nr);

        // связать обоих
        c->neigh[side] = nb;
        nb->neigh[(side + 3) % 6] = c; // обратная сторона

        return nb;
    }

    // То же, но по q,r
    HexCell* ensureNeighbor(int q, int r, int side) {
        HexCell* c = getOrCreate(q,r);
        return ensureNeighbor(c, side);
    }

    const std::map<std::pair<int,int>, HexCell*>& allCells() const {
        return cells;
    }
};
