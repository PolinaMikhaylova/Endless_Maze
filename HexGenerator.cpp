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
    {  1,  0 },  // R
    { -1,  0 },  // L
    {  0, -1 },  // U
    {  0,  1 }   // D
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
        if (QPointF::dotProduct(p, sideNormal[i]) > d)
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

bool isAppleInHex(const QPointF& hexCenter, const QPointF& apple, float hexRadius)
{
    QPointF local  = apple - hexCenter;
    return pointInsideHex(local, hexRadius);
}

bool bfsFrom(HexGrid& grid, HexNode* hex, const QPointF& hexCenter, int startId,
             bool connectOnly, const float hexRadius, const float step, int visit,
             std::unordered_map<int, int>& visited,  std::unordered_set<int>& visitedPlanB, int& countEdge)
{

    std::queue<int> q, planB;

    int maxCountEdge = 300 + rand() % (1000 - 300 + 1);
    bool beginConnect = connectOnly;
    visited[startId] = visit;
    visitedPlanB.insert(startId);
    planB.push(startId);
    q.push(startId);
    while (true)

    {

        if (q.empty()){
            q = planB;
        }

        int v = q.front(); q.pop();
        bool haveWay = !pointInsideHex(grid.maze[v].pos - hexCenter, hexRadius);
        if (connectOnly != beginConnect || haveWay)
        {
            if (connectOnly == false &&  countEdge > maxCountEdge){
                return true;
            }
            if (haveWay){
                continue;
            }

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
                if (boundarySide(np - hexCenter, hexRadius, hex, sides)){
                    continue;
                }
                for (auto& side: sides){
                    HexNode* neigh = hex->neigh[side];
                    int to = grid.addCell(np, step);
                    int back = opposite(d);
                    grid.maze[v].edge[d] = to;
                    grid.maze[to].edge[back] = v;
                    ++countEdge;
                    visited[to] = visit;
                    q.push(to);
                    if (connectOnly){
                        connectOnly = false;
                        for (int i = 0; i < 3; ++i){
                            if (neigh->pending_apple[i] == QPointF()){
                                neigh->pending_apple[i] = np;
                                break;
                            }
                        }

                    }
                }
                continue;


            }

            int to = grid.addCell(np, step);
            if ((float)rand() / RAND_MAX < CONTINUE_PROB)
            {
                if (!visitedPlanB.count(v)){
                    visitedPlanB.insert(v);
                    planB.push(v);
                }
                continue;
            }
            if (connectOnly && visited[to] != 0 && visited[to] != visit){
                connectOnly = false;
                grid.maze[v].edge[d] = to;
                grid.maze[to].edge[opposite(d)] = v;

                ++countEdge;
                if (visited[to] == 1){
                    q.push(to);
                    visited[to] = visit;
                }
                continue;

            }
            if (visited[to] == visit)
                continue;

            grid.maze[v].edge[d] = to;
            grid.maze[to].edge[opposite(d)] = v;
            ++countEdge;
            visited[to] = visit;
            q.push(to);
        }

    }
    return true;
}

void HexGenerator::generate(
    HexGrid& grid,
    HexNode* hex,
    const float hexRadius,
    const QPointF& start,
    const std::array<QPointF, 3>& apples
    )
{
    hex->state = HexState::Generated;

    const float step = hexRadius * 0.05f;
    QPointF hexCenter = axialToPixel(hex->q, hex->r, hexRadius);


    std::unordered_map<int, int> visited;
    std::unordered_set<int> visitedPlanB;
    visited.reserve(200000);
    visitedPlanB.reserve(200000);
    int countEdge = 0;
    for (int i = 0; i < grid.maze.GetLength(); ++i) {
        if (pointInsideHex(grid.maze[i].pos - hexCenter, hexRadius)) {
            visited[i] = 1;
        }
    }

    int visit = 2;
    int startId = grid.addCell(start, step);
    bfsFrom(grid, hex, hexCenter, startId, (hex->knownBeforeGen == 6? true : false), hexRadius, step, visit++, visited, visitedPlanB, countEdge);

    for (int i = 0; i < 3; ++i){
        if (apples[i] != zero && isAppleInHex(hexCenter, apples[i], hexRadius)){
            int appleId = grid.addCell(apples[i], step);
            if (visited[appleId] == 0){
                bfsFrom(grid, hex, hexCenter, appleId, true, hexRadius, step, visit++, visited, visitedPlanB, countEdge);
            }

        }
    }


    for (int i =0; i < 3; ++i){
        if (hex->pending_apple[i] != QPointF()){
            int appleId = grid.addCell(hex->pending_apple[i], step);
            if (visited[appleId] == 1){
                bfsFrom(grid, hex, hexCenter, appleId, true, hexRadius, step, visit++, visited, visitedPlanB, countEdge);
            }
        }
    }


    for (auto& i: hex->neigh){
        ++i->knownBeforeGen;
    }

}
