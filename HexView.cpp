#include "HexView.h"
#include "HexGenerator.h"
#include <QPainter>
#include <QKeyEvent>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>
#include <QPushButton>
#include <queue>


static const QPointF dirVec[4] = {
    {  1,  0 },  // R
    { -1,  0 },  // L
    {  0, -1 },  // U
    {  0,  1 }   // D
};

const float pi = acos(-1);
static const QPointF sideNormal[6] = {
    {  1.0f,  0.0f },
    {  0.5f,  std::cos(pi/6) },
    { -0.5f,  std::cos(pi/6) },
    { -1.0f,  0.0f },
    { -0.5f, -std::cos(pi/6)},
    {  0.5f, -std::cos(pi/6) }
};



static bool pointInsideHex(const QPointF& p, float hexRadius)
{
    const float d = hexRadius * std::cos(pi/6);
    for (int i = 0; i < 6; ++i)
        if (QPointF::dotProduct(p, sideNormal[i]) >= d)
            return false;
    return true;
}


bool HexView::crossedSides(const QPointF& p, ArraySequence<int>& side)
{
    const float d = hexRadius * std::cos(pi/6);
    for (int i = 0; i < 6; ++i){
        if (QPointF::dotProduct(p, sideNormal[i]) >= d){
            side.Append(i);
        }
    }
    return side.GetLength() != 0;
}


HexView::HexView(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(600, 480);


    setupNavMenu();
    setupNavButton();

    srand(time(NULL));
    cur = grid.root();

    HexGenerator::generate(grid, cur, hexRadius, {0, 0});
    cursor = grid.maze[grid.addCell({0, 0}, step)];
    score = 0;
    for (int i = 0; i < 3; ++i){
        spawnApple(i);
    }


    arrowDir = 0;
    zoom = 1.0f;
    path.Append( cursor.pos);
    centerCamera();

}

QPointF HexView::axialToPixel(int q, int r) const
{
    return {
        hexRadius * (2 * cos(pi/6) * q + cos(pi/6) * r),
        hexRadius * (1.5f * r)
    };
}

QPolygonF HexView::hexPolygonAt(const QPointF& c) const
{
    QPolygonF p;
    float r = hexRadius * zoom;
    for (int i = 0; i < 6; ++i)
    {
        float a = (60.0f * i - 30.0f) * pi / 180.0f;
        p << QPointF(
            c.x() + r * std::cos(a),
            c.y() + r * std::sin(a)
            );
    }
    return p;
}

void HexView::centerCamera()
{
    QPointF worldCursor = cursor.pos;

    QPointF screenCenter(width() / 2.0f, height() / 2.0f);
    camera = screenCenter - worldCursor * zoom + cameraDragOffset;
}

bool HexView::isAppleInHex(HexNode* h) const
{
    bool has = false;
    for (int i = 0; i < 3; ++i){
        QPointF hexCenter = axialToPixel(h->q, h->r);
        QPointF local  = apples[i] - hexCenter;
        has |= pointInsideHex(local, hexRadius);
    }
    return has;
}

void HexView::moveToNeighbor(int side, QPointF delta)
{
    QPointF entryWorld = cursor.pos;
    if (cur->neigh[side]->state != HexState::Generated)
    {
        grid.ensureNeighbors(cur->neigh[side]);
        if (isAppleInHex(cur->neigh[side])){
            HexGenerator::generate(
                grid,
                cur->neigh[side],
                hexRadius,
                entryWorld + delta,
                apples
                );
        }
        else{
            HexGenerator::generate(
                grid,
                cur->neigh[side],
                hexRadius,
                entryWorld + delta
                );
        }

    }
}

static int oppositeDir(int d)
{
    return d ^ 1;
}


void HexView::keyPressEvent(QKeyEvent* e)
{

    int dir = -1;
    QPointF delta;

    if (e->key() == Qt::Key_Right)      { dir = 0; delta = { step, 0 }; arrowDir = 0; }
    else if (e->key() == Qt::Key_Left)  { dir = 1; delta = { -step, 0 }; arrowDir = 2; }
    else if (e->key() == Qt::Key_Up)    { dir = 2; delta = { 0, -step }; arrowDir = 3; }
    else if (e->key() == Qt::Key_Down)  { dir = 3; delta = { 0, step }; arrowDir = 1; }
    else return;
    cameraDragOffset = {0, 0};

    if (cursor.edge[dir] == -1){
        return;
    }

    QPointF next = cursor.pos + delta;
    QPointF hexCenter = axialToPixel(cur->q, cur->r);
    ArraySequence<int> side;
    if (crossedSides(next - hexCenter, side))
    {

        moveToNeighbor(side[0], delta);
        if (side.GetLength() != 1){
            moveToNeighbor(side[1], delta);
        }
        cur = cur->neigh[side[0]];
    }

    cursor = grid.maze[cursor.edge[dir]];

    for (int i = 0; i < 3; ++i){
        if (cursor.pos == apples[i])
        {
            score++;
            spawnApple(i);
        }
    }

    path.Append(cursor.pos);

    centerCamera();
    update();
}


void HexView::wheelEvent(QWheelEvent* e)
{
    QPointF worldCenter =
        (QPointF(width()/2, height()/2) - camera) / zoom;

    zoom *= (e->angleDelta().y() > 0) ? 1.1f : 0.9f;
    zoom = std::clamp(zoom, 0.4f, 4.0f);

    camera = QPointF(width()/2, height()/2) - worldCenter * zoom;
    update();
}

void HexView::resizeEvent(QResizeEvent*)
{
    navButton->move(10, height() - navButton->height() - 10);
    centerCamera();
}


void HexView::mouseMoveEvent(QMouseEvent* e)
{
    if (dragging)
    {
        cameraDragOffset += e->pos() - lastMouse;
        lastMouse = e->pos();
        centerCamera();
        update();
    }
}

void HexView::mouseDoubleClickEvent(QMouseEvent* e)
{
    QPointF worldClick = (e->pos() - camera) / zoom;

    const float MAX_DIST2 = step * step;
    int best = -1;
    float bestDist2 = MAX_DIST2;

    for (int ind = 0; ind < grid.maze.GetLength();++ind) {
        QPointF d = grid.maze[ind].pos - worldClick;
        float dist2 = d.x()*d.x() + d.y()*d.y();
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            best = ind;
        }
    }


    if (best == -1) {
        goal = QPointF();
        update();
        return;
    }


    goal = grid.maze[best].pos;
    update();
}


void HexView::mouseReleaseEvent(QMouseEvent*)
{
    dragging = false;
}

QPointF HexView::randomPointInHex(const QPointF& hexCenter)
{

    QPointF center;
    center.setX(std::round(hexCenter.x() / step) * step);
    center.setY(std::round(hexCenter.y() / step) * step);

    while (true)
    {
        float angle = float(rand()) / RAND_MAX * 2.0f * pi;
        float dist  = std::sqrt(float(rand()) / RAND_MAX) * hexRadius;

        QPointF offset(
            std::cos(angle) * dist,
            std::sin(angle) * dist
            );

        QPointF world = center + offset;

        if (!pointInsideHex(world - hexCenter, hexRadius))
            continue;

        world.setX(std::round(world.x() / step) * step);
        world.setY(std::round(world.y() / step) * step);

        return world;
    }
}



void HexView::spawnApple(int i)
{
    ArraySequence<HexNode*> candidates;
    int N = grid.all().GetMaterializedCount();
    for (int i = 0; i < N; ++i) {
        HexNode* n = grid.all().Get(i);
        if (n->state != HexState::Generated){
            candidates.Append(n);
        }
    }

    HexNode* hex = candidates[rand() % candidates.GetLength()];

    QPointF hexCenter = axialToPixel(hex->q, hex->r);
    apples[i] = randomPointInHex(hexCenter);
}








void HexView::drawApple(QPainter& p)
{
    for (int i = 0; i < 3; ++i){
        QPointF screen = apples[i] * zoom + camera;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(220, 40, 40));
        p.drawEllipse(screen, 2.0f * zoom, 2.0f * zoom);

        if (!isAppleOnScreen(i))
            drawApplePointer(p, i);
    }

}



bool HexView::isAppleOnScreen(int i) const
{
    QPointF s = apples[i] * zoom + camera;
    return rect().contains(s.toPoint());
}

void HexView::drawApplePointer(QPainter& p, int i)
{
    QPointF center(width()/2.0, height()/2.0);
    QPointF target = apples[i] * zoom + camera;

    QPointF v = target - center;
    double len = std::hypot(v.x(), v.y());

    QPointF dir = v / len;

    double R = std::min(width(), height()) * 0.45;
    QPointF pos = center + dir * R;

    double angle = std::atan2(dir.y(), dir.x()) * 180.0 / pi;

    QRectF arcRect(
        
        pos.x() - 18, pos.y() - 18,36, 36
        );

    p.setPen(QPen(QColor(220, 40, 40, 180), 4));
    p.setBrush(Qt::NoBrush);

    p.drawArc(
        arcRect,
        int((-angle - 60) * 16),
        int(120 * 16)
        );
}

HexNode* HexView::hexAtAxial(int q, int r)
{
    int N = grid.all().GetMaterializedCount();
    for (int i = 0; i < N; ++i) {
        HexNode* n = grid.all().Get(i);
        if (n->q == q && n->r == r)
            return n;
    }
    return nullptr;
}



HexNode* HexView::hexAtWorld(const QPointF& world)
{
    float qf = (std::sqrt(3.0f)/3 * world.x()
                - 1.0f/3 * world.y()) / hexRadius;
    float rf = (2.0f/3 * world.y()) / hexRadius;

    int q = int(std::round(qf));
    int r = int(std::round(rf));

    return HexView::hexAtAxial(q, r);
}


void HexView::tryTeleportToPath(const QPointF& screenPos)
{
    const float MAX_DIST2 = (step * 0.6f) * (step * 0.6f);

    QPointF worldClick = (screenPos - camera) / zoom;

    int best = -1;
    float bestDist2 = MAX_DIST2;

    for (size_t i = 0; i < path.GetLength(); ++i)
    {
        QPointF d = worldClick - path[i];
        float dist2 = d.x()*d.x() + d.y()*d.y();
        if (dist2 < bestDist2)
        {
            bestDist2 = dist2;
            best = int(i);
        }
    }

    if (best == -1)
        return;

    QPointF targetWorld = path[best];

    HexNode* targetHex = hexAtWorld(targetWorld);
    if (!targetHex || targetHex->state != HexState::Generated)
        return;

    cur = targetHex;

    QPointF hexCenter = axialToPixel(cur->q, cur->r);
    cursor = grid.maze[grid.addCell(targetWorld, step)];

    arrowDir = 0;

    cameraDragOffset = {0, 0};
    path.Append({step/2, step/2});
    path.Append(cursor.pos);
    centerCamera();
    update();
}


void HexView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        tryTeleportToPath(e->pos());
        dragging = true;
        lastMouse = e->pos();
    }
}
void HexView::drawGeneratedHex(QPainter& p){
    int N = grid.all().GetMaterializedCount();
    for (int i = 0; i < N; ++i) {
        HexNode* n = grid.all().Get(i);
        QPointF hexWorld = axialToPixel(n->q, n->r);
        QPointF c = hexWorld * zoom + camera;

        QPolygonF h = hexPolygonAt(c);

        p.setPen(Qt::NoPen);
        if (n->state == HexState::Generated){
            p.setBrush(QColor(215, 192, 149));
            p.drawPolygon(h);
        }

    }

}
void HexView::drawMaze(QPainter& p){


    float roadOuter = step * 0.70f;
    float roadInner = step * 0.50f;


    p.setPen(QPen(Qt::black, roadOuter * zoom));

    for (auto& c: grid.maze)
    {
        QPointF p0 = c.pos * zoom + camera;

        if (c.edge[0]!= -1)
            p.drawLine(p0, (c.pos + QPointF(step, 0)) * zoom + camera);
        if (c.edge[1]!= -1)
            p.drawLine(p0, (c.pos - QPointF(step, 0)) * zoom + camera);
        if (c.edge[2]!= -1)
            p.drawLine(p0, (c.pos - QPointF(0, step)) * zoom + camera);
        if (c.edge[3]!= -1)
            p.drawLine(p0, (c.pos + QPointF(0, step)) * zoom + camera);
    }

    p.setPen(QPen(QColor(245, 222, 179), roadInner * zoom));
    for (auto& c : grid.maze)
    {
        QPointF p0 = c.pos * zoom + camera;

        if (c.edge[0] != -1)
            p.drawLine(p0, (c.pos + QPointF(step, 0)) * zoom + camera);
        if (c.edge[1]!= -1)
            p.drawLine(p0, (c.pos - QPointF(step, 0)) * zoom + camera);
        if (c.edge[2]!= -1)
            p.drawLine(p0, (c.pos - QPointF(0, step)) * zoom + camera);
        if (c.edge[3]!= -1)
            p.drawLine(p0, (c.pos + QPointF(0, step)) * zoom + camera);
    }

}

void HexView::drawBlackHex(QPainter& p){
    int N = grid.all().GetMaterializedCount();
    for (int i = 0; i < N; ++i) {
        HexNode* n = grid.all().Get(i);
        QPointF hexWorld = axialToPixel(n->q, n->r);
        QPointF c = hexWorld * zoom + camera;

        QPolygonF h = hexPolygonAt(c);
        p.setPen(Qt::NoPen);
        if (n->state != HexState::Generated){
            p.setBrush(Qt::black);
            p.drawPolygon(h);
        }

    }
}

void HexView::drawBFS(QPainter& p){
    if (bfsPath.GetLength() > 1)
    {
        QPen pen(QColor(255, 100, 100));
        pen.setWidthF(2.0 * zoom);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath pp;
        pp.moveTo(bfsPath[0] * zoom + camera);
        for (size_t i = 1; i < bfsPath.GetLength(); ++i)
            pp.lineTo(bfsPath[i] * zoom + camera);
        p.drawPath(pp);
    }
}

void HexView::drawPathCursor(QPainter& p){
    QPointF zero = {step/2, step/2};
    QPen pen(QColor(50, 200, 255));
    pen.setWidthF(2.0 * zoom);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    QPainterPath pp;
    QPointF first = path[0] * zoom + camera;
    pp.moveTo(first);
    bool go = true;
    for (size_t i = 1; i < path.GetLength(); ++i)
    {
        QPointF pt = path[i] * zoom + camera;

        if (pt == zero * zoom + camera){
            go = false;
            continue;
        }
        if (!go){
            pp.moveTo(pt);
            go = true;
            continue;
        }
        pp.lineTo(pt);
    }

    p.drawPath(pp);
}

void HexView::drawCursor(QPainter& p){
    QPointF center =
        cursor.pos * zoom +
        camera;

    QPointF dir;
    if (arrowDir == 0) dir = { 1, 0 };
    else if (arrowDir == 1) dir = { 0, 1 };
    else if (arrowDir == 2) dir = { -1, 0 };
    else dir = { 0, -1 };

    float len = 4 * zoom;
    float w   = 1.5f  * zoom;

    QPolygonF arrow;
    arrow << center + dir * len
          << center + QPointF(-dir.y(), dir.x()) * w
          << center + QPointF(dir.y(), -dir.x()) * w;

    p.setBrush(Qt::red);
    p.setPen(Qt::NoPen);
    p.drawPolygon(arrow);

}

void HexView::drawScore(QPainter& p){
    QString text = QString("Score: %1").arg(score);

    QFont f = p.font();
    f.setPointSize(14);
    f.setBold(true);
    p.setFont(f);

    // размеры текста
    QFontMetrics fm(f);
    QSize textSize = fm.size(Qt::TextSingleLine, text);

    // отступы внутри рамки
    int padX = 10;
    int padY = 6;

    // прямоугольник панели
    QRectF panel(
        10,
        10,
        textSize.width()  + padX * 2,
        textSize.height() + padY * 2
        );

    // фон (полупрозрачный)
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 160));
    p.drawRoundedRect(panel, 8, 8);

    // рамка
    p.setPen(QPen(QColor(0, 200, 255), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(panel, 8, 8);

    // текст
    p.setPen(Qt::white);
    p.drawText(
        panel.adjusted(padX, padY, -padX, -padY),
        Qt::AlignLeft | Qt::AlignVCenter,
        text
        );
}

void HexView::drawMessange(QPainter& p){
    if (messageTimer.isValid() &&
        messageTimer.elapsed() < MESSAGE_TIME_MS)
    {
        QString text;
        QColor bg;

        if (showNoPath)
        {
            text = "❌ Пути нет";
            bg = QColor(200, 60, 60, 160); // полупрозрачный
        }
        else
            return;

        const qreal w = 300;
        const qreal h = 44;

        QRectF box(
            (width()  - w) * 0.5,
            (height() - h) * 0.5,
            w,
            h
            );

        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(box, 10, 10);

        QFont f = p.font();
        f.setBold(true);
        f.setPointSizeF(14);
        p.setFont(f);

        p.setPen(QColor(255, 255, 255, 220)); // слегка прозрачный текст
        p.drawText(box, Qt::AlignCenter, text);
    }
}

void HexView::drawGoal(QPainter& p) {
    if (goal.isNull()) return;

    float worldSize = step * 0.2f;
    float size = worldSize * zoom;

    QPen pen(Qt::red, 2);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    QPointF screenGoal = goal * zoom + camera;

    p.drawLine(screenGoal.x() - size, screenGoal.y() - size,
               screenGoal.x() + size, screenGoal.y() + size);

    p.drawLine(screenGoal.x() + size, screenGoal.y() - size,
               screenGoal.x() - size, screenGoal.y() + size);
}





void HexView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(30, 30, 30));


    // гексы
    drawGeneratedHex(p);


    // внутренний лабиринт
    drawMaze(p);

    // гексы
    drawBlackHex(p);

    // bfs путь
    drawBFS(p);


    // путь курсора
    drawPathCursor(p);

    // курсор
    drawCursor(p);

    // яблоко
    drawApple(p);

    // цель
    drawGoal(p);

    // панель очков
    drawScore(p);

    // сообщение
    drawMessange(p);
}


int targetSide = -1;

bool HexView::isExitToNeighbor(int cellId)
{
    MazeCell c = grid.maze[cellId];

    QPointF np = c.pos;

    if (!pointInsideHex(np - axialToPixel(cur->q, cur->r), hexRadius))
    {
        QPointF local = np - axialToPixel(cur->q, cur->r);

        ArraySequence<int> side;
        if (crossedSides(local, side))
        {
            if (side[0] == targetSide ||(side.GetLength()!=1 &&  side[1] == targetSide))
                return true;
        }
    }
    return false;
}


void HexView::setupNavButton()
{
    navButton = new QPushButton("Навигация", this);
    navButton->setFixedSize(120, 28);
    navButton->raise();

    connect(navButton, &QPushButton::clicked, this, [this]() {
        QPoint p = navButton->mapToGlobal(QPoint(0, -navMenu->sizeHint().height()));
        navMenu->exec(p);
    });
}


ArraySequence<QPointF> HexView::bfsInHex(
    HexGrid& grid,
    HexNode* hex,
    int startId,
    std::function<bool(int)> isTarget,
    float hexRadius,
    bool apple
    )
{
    std::queue<int> q;
    std::unordered_map<int, int> parent;
    q.push(startId);
    parent[startId] = -1;

    while (!q.empty())
    {
        int v = q.front(); q.pop();

        if (isTarget(v))
        {
            ArraySequence<QPointF> side;
            for (int cur = v; cur != -1; cur = parent[cur])
                side.Append(grid.maze[cur].pos);
            return side;
        }

        if (!apple && !pointInsideHex(grid.maze[v].pos - axialToPixel(cur->q, cur->r), hexRadius)){
            continue;
        }
        for (int d = 0; d < 4; ++d)
        {
            if (grid.maze[v].edge[d] == -1)
                continue;

            QPointF np = grid.maze[v].pos + dirVec[d] * step;
            int to = grid.addCell(np, step);

            if (parent.count(to))
                continue;

            parent[to] = v;
            q.push(to);
        }
    }
    return {};
}

void HexView::runBfsToNeighbor(Neighbor n)
{
    targetSide = static_cast<int>(n);

    int startId = grid.addCell(cursor.pos, step);


    bfsPath = bfsInHex(
        grid,
        cur,
        startId,
        [this](int id){ return isExitToNeighbor(id); },
        hexRadius,
        false
        );
    if (bfsPath.GetLength() > 1)
    {
        showNoPath = false;
    }
    else{
        showNoPath = true;
        messageTimer.restart();
    }

}

void HexView::runBfsToApple()
{
    int startId = grid.addCell(cursor.pos, step);

    bfsPath = bfsInHex(
        grid,
        cur,
        startId,
        [this](int id){
            for (int i = 0; i < 3; ++i){
                if (grid.maze[id].pos == apples[i]){
                    return true;
                }
            }
            return false;
        },
        hexRadius,
        true
        );
    if (bfsPath.GetLength() > 1)
    {
        showNoPath = false;
    }
    else
    {
        showNoPath = true;
        messageTimer.restart();
    }


}

void HexView::runBfsToGoal()
{
    int startId = grid.addCell(cursor.pos, step);

    bfsPath = bfsInHex(
        grid,
        cur,
        startId,
        [this](int id){
            return grid.maze[id].pos == goal;
        },
        hexRadius,
        true
        );
    if (bfsPath.GetLength() > 1)
    {
        showNoPath = false;
    }
    else
    {
        showNoPath = true;
        messageTimer.restart();
    }


}


void HexView::setupNavMenu()
{
    navMenu = new QMenu(this);

    QAction* toLU = navMenu->addAction("Дойти до левого верхнего");
    QAction* toL  = navMenu->addAction("Дойти до левого");
    QAction* toLD = navMenu->addAction("Дойти до левого нижнего");

    navMenu->addSeparator();

    QAction* toRU = navMenu->addAction("Дойти до правого верхнего");
    QAction* toR  = navMenu->addAction("Дойти до правого");
    QAction* toRD = navMenu->addAction("Дойти до правого нижнего");

    navMenu->addSeparator();

    QAction* toPoint = navMenu->addAction("Построить маршрут до точки");


    navMenu->addSeparator();
    QAction* toApple = navMenu->addAction("Дойти до яблока");


    connect(toLU, &QAction::triggered, this, [this]() {
        runBfsToNeighbor(Neighbor::LeftUp);
        update();
    });
    connect(toL, &QAction::triggered, this, [this]() {
        runBfsToNeighbor(Neighbor::Left);
        update();
    });
    connect(toLD, &QAction::triggered, this, [this]() {
        runBfsToNeighbor(Neighbor::LeftDown);
        update();
    });

    connect(toRU, &QAction::triggered, this, [this]() {
        runBfsToNeighbor(Neighbor::RightUp);
        update();
    });
    connect(toR, &QAction::triggered, this, [this]() {
        runBfsToNeighbor(Neighbor::Right);
        update();
    });
    connect(toRD, &QAction::triggered, this, [this]() {
        runBfsToNeighbor(Neighbor::RightDown);
        update();
    });

    connect(toApple, &QAction::triggered, this, [this]() {
        runBfsToApple();
        update();
    });
    connect(toPoint, &QAction::triggered, this, [this]() {
        runBfsToGoal();
        update();
    });
}

