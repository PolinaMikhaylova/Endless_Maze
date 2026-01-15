#pragma once
#include <array>
#include <QPainter>
#include "ArraySequence.h"

enum class HexState {
    Linked,     // существует, но не сгенерирован
    Generated   // посещён
};

struct MazeCell
{
    QPointF pos;  // мировые координаты
    bool edge[4] = {false, false, false, false};  // 0=R,1=L,2=U,3=D
};

static inline uint64_t cellKey(const QPointF& p, float step)
{
    int x = int(std::round(p.x() / step));
    int y = int(std::round(p.y() / step));
    return (uint64_t(uint32_t(x)) << 32) | uint32_t(y);
}

struct HexNode {
    int q = 0;
    int r = 0;

    HexState state = HexState::Linked;

    // сколько соседей было известно до генерации
    int knownBeforeGen = -1;

    // 6 соседей по часовой стрелке
    // 0 Right
    // 1 Down-Right
    // 2 Down-Left
    // 3 Left
    // 4 Up-Left
    // 5 Up-Right
    HexNode* neigh[6] = {
        nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr
    };
    QPointF pending_apple;


};
