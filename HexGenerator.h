#pragma once
#include "HexGrid.h"
#include <QPointF>
static QPointF zero = {0.666f, 0.666f};
class HexGenerator
{
public:
    static void generate(
        HexGrid& grid,
        HexNode* hex,
        const float hexRadius,
        const QPointF& start,
        const QPointF& apple = zero
        );
};
