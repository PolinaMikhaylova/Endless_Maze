#pragma once
#include "ArraySequence.h"
#include "HexNode.h"
#include "LazySequence.h"

class HexGrid
{
public:
    HexGrid();
    HexNode* root();
    const LazySequence<HexNode*>& all() const;
    void ensureNeighbors(HexNode* n);
    ArraySequence<MazeCell> maze;

    std::unordered_map<uint64_t, int> index;
    int addCell(const QPointF& p, float step)
    {
        uint64_t k = cellKey(p, step);
        auto it = index.find(k);
        if (it != index.end())
            return it->second;

        MazeCell c;
        c.pos = p;

        int id = int(maze.GetLength());
        maze.Append(c);
        index[k] = id;
        return id;
    }

private:
    HexNode* createNode(int q, int r);
    HexNode* getOrCreate(int q, int r);

    HexNode* start = nullptr;
    LazySequence<HexNode*> nodes;

};
