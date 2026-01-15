#include "HexGrid.h"

static const int dq[6] = { +1,  0, -1, -1,  0, +1 };
static const int dr[6] = {  0, +1, +1,  0, -1, -1 };
// Right
// RightDown
// LeftDown
// Left
// LeftUp
// RightUp

HexGrid::HexGrid()
{
    start = createNode(0, 0);
    start->state = HexState::Generated;
    ensureNeighbors(start);
}

HexNode* HexGrid::root()
{
    return start;
}

const ArraySequence<HexNode*>& HexGrid::all() const
{
    return nodes;
}

HexNode* HexGrid::createNode(int q, int r)
{
    HexNode* n = new HexNode;
    n->q = q;
    n->r = r;
    nodes.Append(n);
    return n;
}

HexNode* HexGrid::getOrCreate(int q, int r)
{
    for (auto* n : nodes)
        if (n->q == q && n->r == r)
            return n;

    return createNode(q, r);
}

void HexGrid::ensureNeighbors(HexNode* n)
{
    for (int i = 0; i < 6; ++i)
    {
        if (n->neigh[i]) continue;

        int nq = n->q + dq[i];
        int nr = n->r + dr[i];

        HexNode* other = getOrCreate(nq, nr);
        n->neigh[i] = other;
        other->neigh[(i + 3) % 6] = n;
    }
}
