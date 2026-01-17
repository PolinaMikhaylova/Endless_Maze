#include "HexGenerator.h"
#include <unordered_set>
#include <unordered_map>
#include <cstdlib>
#include <cmath>
#include <random>
#include <queue>


std::mt19937 rng(std::random_device{}());

const float CONTINUE_PROB = 0.40f;

const QPointF dirVec[4] = {
    {  1,  0 },
    { -1,  0 },
    {  0, -1 },
    {  0,  1 }
};

int opposite(int d) { return d ^ 1; }

uint64_t key(const QPointF& p, float step)
{
    int x = int(std::round(p.x() / step));
    int y = int(std::round(p.y() / step));
    return (uint64_t(uint32_t(x)) << 32) | uint32_t(y);
}

const float pi = acos(-1);
const QPointF sideNormal[6] = {
    {  1.0f,  0.0f },
    {  0.5f,  std::cos(pi/6) },
    { -0.5f,  std::cos(pi/6) },
    { -1.0f,  0.0f },
    { -0.5f, -std::cos(pi/6)},
    {  0.5f, -std::cos(pi/6) }
};

bool boundarySide(const QPointF& p,
                  float hexRadius,
                  HexNode* n,
                  ArraySequence<int>& sides)
{
    const float d = hexRadius * std::cos(pi / 6);
    bool hasGenerate = false;
    for (int i = 0; i < 6; ++i)
    {
        if (QPointF::dotProduct(p, sideNormal[i]) >= d)
        {
            sides.Append(i);
            HexNode* neigh = n->neigh[i];
            if (neigh && neigh->state == HexState::Generated)
                hasGenerate = true;
        }
    }
    return hasGenerate;
}

bool pointInsideHex(const QPointF& p, float hexRadius)
{
    const float d = hexRadius * std::cos(pi/6);
    for (int i = 0; i < 6; ++i)
        if (QPointF::dotProduct(p, sideNormal[i]) >= d)
            return false;
    return true;
}

QPointF axialToPixel(int q, int r, float hexRadius)
{
    return {
            hexRadius * (2 * cos(pi/6) * q + cos(pi/6) * r),
        hexRadius * (1.5f * r)
    };
}

bool bfsFrom(HexGrid& grid, HexNode* hex, const QPointF& hexCenter, int startId,
             bool connectOnly, const float hexRadius, const float step,
             std::unordered_map<int, int>& visited,  std::unordered_set<int>& visitedPlanB)
{

    std::queue<std::pair<int, int>> q, planB;

    int curDist = 0;
    int max_len = 1e9;
    int countEdge = 0;
    int maxCountEdge = 100 + rand() % (1000 - 100 + 1);
    int visit = connectOnly? 2: 1;
    visited[startId] = visit;
    q.push({startId, 0});
    while (true)

    {

        if (q.empty()){
            q = std::move(planB);
            visitedPlanB.clear();
        }

        auto [v, r] = q.front(); q.pop();
        curDist = r;

        if (max_len + 10 <= curDist) break;

        if (!pointInsideHex(grid.maze[v].pos - hexCenter, hexRadius))
        {
            if (max_len == 1e9 && countEdge > maxCountEdge && !connectOnly){
                max_len = curDist;
            }
            continue;
        }




        std::array<int,4> order = {0,1,2,3};
        std::shuffle(order.begin(), order.end(), rng);


        for (int d : order)
        {
            QPointF np = grid.maze[v].pos + dirVec[d] * step;

            if (!pointInsideHex(np - hexCenter, hexRadius))
            {
                if ((float)rand() / RAND_MAX < CONTINUE_PROB){
                    continue;
                }
                ArraySequence<int> sides;
                if (boundarySide(np, hexRadius, hex, sides)){
                    continue;
                }
                for (auto& side: sides){
                    HexNode* neigh = hex->neigh[side];
                    int to = grid.addCell(np, step);
                    int back = opposite(d);
                    grid.maze[v].edge[d] = to;
                    grid.maze[to].edge[back] = v;
                    ++countEdge;
                    visited[to] = 1;
                    q.push({to, r + 1});
                    if (connectOnly){
                        connectOnly = false;
                        neigh->pending_apple = np;
                    }
                }
                continue;


            }

            int to = grid.addCell(np, step);
            if ((float)rand() / RAND_MAX < CONTINUE_PROB)
            {
                if (!visitedPlanB.count(v)){
                    visitedPlanB.insert(v);
                    planB.push({v, r});
                }
                continue;
            }
            if (connectOnly && abs(visited[to]) == 1){
                connectOnly = false;
                grid.maze[v].edge[d] = to;
                grid.maze[to].edge[opposite(d)] = v;

                ++countEdge;
                if (visited[to] == -1){
                    q.push({to, r+1});
                    visited[to] = visit;
                }
                continue;

            }
            if (visited[to] > 0)
                continue;

            grid.maze[v].edge[d] = to;
            grid.maze[to].edge[opposite(d)] = v;
            ++countEdge;
            visited[to] = visit;
            q.push({to, r+1});
        }

    }
    return true;
}

void HexGenerator::generate(
    HexGrid& grid,
    HexNode* hex,
    const float hexRadius,
    const QPointF& start,
    const QPointF& apple
    )
{
    hex->state = HexState::Generated;

    const float step = hexRadius * 0.05f;
    QPointF hexCenter = axialToPixel(hex->q, hex->r, hexRadius);


    std::unordered_map<int, int> visited;
    std::unordered_set<int> visitedPlanB;
    visited.reserve(200000);
    visitedPlanB.reserve(200000);

    int startId = grid.addCell(start, step);
    bfsFrom(grid, hex, hexCenter, startId, (hex->knownBeforeGen == 6? true : false), hexRadius, step, visited, visitedPlanB);


    if (apple != zero){
        int appleId = grid.addCell(apple, step);
        if (visited[appleId] == 0){
            bfsFrom(grid, hex, hexCenter, appleId, true, hexRadius, step, visited, visitedPlanB);
        }

    }


    if (hex->pending_apple != zero){
        int appleId = grid.addCell(hex->pending_apple, step);
        if (visited[appleId] == 0){
            bfsFrom(grid, hex, hexCenter, appleId, true, hexRadius, step, visited, visitedPlanB);
        }
    }


}
