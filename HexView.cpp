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

static const QPointF sideNormal[6] = {
    {  1.0f,  0.0f },
    {  0.5f,  0.8660254f },
    { -0.5f,  0.8660254f },
    { -1.0f,  0.0f },
    { -0.5f, -0.8660254f },
    {  0.5f, -0.8660254f }
};

static bool pointInsideHex(const QPointF& p, float R)
{
    const float d = R * 0.8660254f;
    for (int i = 0; i < 6; ++i)
        if (QPointF::dotProduct(p, sideNormal[i]) > d)
            return false;
    return true;
}

bool HexView::crossedSides(const QPointF& p, int& s1, int& s2) const
{
    const float d = hexRadius * 0.8660254f;
    constexpr float EPS = 1e-5f;

    float overflow[6];
    for (int i = 0; i < 6; ++i)
        overflow[i] = QPointF::dotProduct(p, sideNormal[i]) - d;

    s1 = -1;
    s2 = -1;

    float max1 = -1e9f, max2 = -1e9f;

    for (int i = 0; i < 6; ++i)
    {
        if (overflow[i] > max1)
        {
            max2 = max1; s2 = s1;
            max1 = overflow[i]; s1 = i;
        }
        else if (overflow[i] > max2)
        {
            max2 = overflow[i]; s2 = i;
        }
    }

    if (max1 >= -EPS && max2 >= -EPS)
        return true;    // пересечение по ребру (две грани)

    if (max1 >= -EPS)
    {
        s2 = -1;
        return true;    // пересечение по одной грани
    }

    return false;
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
    QPointF entryWorld = cursorWorldPos();

    HexGenerator::generate(grid, cur, hexRadius, entryWorld);
    score = 0;
    spawnApple();

    cursor = QPointF(0, 0);
    arrowDir = 0;
    zoom = 1.0f;
    path.Append({ cursorWorldPos() });
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
    QPointF worldCursor = axialToPixel(cur->q, cur->r) + cursor;

    QPointF screenCenter(width() / 2.0f, height() / 2.0f);
    camera = screenCenter - worldCursor * zoom + cameraDragOffset;
}

bool HexView::isAppleInHex(HexNode* h) const
{
    QPointF center = axialToPixel(h->q, h->r);
    QPointF local  = apple - center;
    return pointInsideHex(local, hexRadius);
}

void HexView::moveToNeighbor(int side, QPointF delta, bool b = false)
{
    QPointF entryWorld = cursorWorldPos();
    if (!b){
        cur = cur->neigh[side];
        if (cur->state != HexState::Generated)
        {
            grid.ensureNeighbors(cur);
            // можно выходить только к НЕ сгенерированным соседям
            if (isAppleInHex(cur)){
                HexGenerator::generate(
                    grid,
                    cur,
                    hexRadius,
                    entryWorld + delta,
                    apple
                    );
            }
            else{
                HexGenerator::generate(
                    grid,
                    cur,
                    hexRadius,
                    entryWorld + delta
                    );
            }
        }
    }
    else if (cur->neigh[side]->state != HexState::Generated)
    {
        grid.ensureNeighbors(cur->neigh[side]);
        // можно выходить только к НЕ сгенерированным соседям
        if (isAppleInHex(cur->neigh[side])){
            HexGenerator::generate(
                grid,
                cur->neigh[side],
                hexRadius,
                entryWorld + delta,
                apple
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

    // перенос точки строго на противоположную грань

    if(!b){
        cursor -= sideNormal[side] * (hexRadius * 1.7320508f);
    }
}

MazeCell* HexView::cellAt(const QPointF& worldPos)
{
    const float step = hexRadius * 0.05f;
    for (auto& c : grid.maze)
    {
        if (QLineF(c.pos, worldPos).length() < step*0.5f)
            return &c;
    }
    return nullptr;
}

static int oppositeDir(int d)
{
    // 0=R,1=L,2=U,3=D
    static int opp[4] = {1, 0, 3, 2};
    return opp[d];
}


bool go = true;
void HexView::keyPressEvent(QKeyEvent* e)
{
    const float step = hexRadius * 0.05f;

    int dir = -1;
    QPointF delta;

    if (e->key() == Qt::Key_Right)      { dir = 0; delta = { step, 0 }; arrowDir = 0; }
    else if (e->key() == Qt::Key_Left)  { dir = 1; delta = { -step, 0 }; arrowDir = 2; }
    else if (e->key() == Qt::Key_Up)    { dir = 2; delta = { 0, -step }; arrowDir = 3; }
    else if (e->key() == Qt::Key_Down)  { dir = 3; delta = { 0, step }; arrowDir = 1; }
    else return;
    cameraDragOffset = {0, 0};
    MazeCell* cell = cellAt(cursorWorldPos());
    if (!cell) return;

    // движение только по графу
    if (!cell->edge[dir]){
        //update();
        return;
    }

    QPointF next = cursor + delta;

    int s1, s2;
    bool crossed = crossedSides(next, s1, s2);

    if (crossed)
    {
        moveToNeighbor(s1, delta, s2 != -1);
        if (s2 != -1)
            moveToNeighbor(s2, delta);

        cursor += delta;
    }
    else
    {
        cursor = next;
    }

    // яблоко
    QPointF d = cursorWorldPos() - apple;
    if (QLineF(QPointF(0,0), d).length() < step * 0.9f)
    {
        score++;
        spawnApple();
    }
    path.Append(cursorWorldPos());

    centerCamera();
    update();
}


QPointF HexView::cursorWorldPos() const
{
    return axialToPixel(cur->q, cur->r) + cursor;
}


void HexView::wheelEvent(QWheelEvent* e)
{
    // мировая точка под центром экрана
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

void HexView::mouseReleaseEvent(QMouseEvent*)
{
    dragging = false;
}

QPointF HexView::randomPointInHex(const QPointF& hexCenter)
{
    const float R = hexRadius;
    const float gridstep = hexRadius * 0.05f;

    // округляем центр гекса к сетке точек
    QPointF center;
    center.setX(std::round(hexCenter.x() / gridstep) * gridstep);
    center.setY(std::round(hexCenter.y() / gridstep) * gridstep);

    while (true)
    {
        // случайное направление и расстояние
        float angle = float(rand()) / RAND_MAX * 2.0f * M_PI;
        float dist  = std::sqrt(float(rand()) / RAND_MAX) * R;

        QPointF offset(
            std::cos(angle) * dist,
            std::sin(angle) * dist
            );

        QPointF world = center + offset;

        // проверка, что точка внутри гекса
        if (!pointInsideHex(world - hexCenter, R))
            continue;

        // финальная привязка к сетке точек
        world.setX(std::round(world.x() / gridstep) * gridstep);
        world.setY(std::round(world.y() / gridstep) * gridstep);

        return world; // мировые координаты
    }
}



void HexView::spawnApple()
{
    // берём случайный НЕ сгенерированный гекс
    std::vector<HexNode*> candidates;
    for (auto* n : grid.all())
        if (n->state != HexState::Generated)
            candidates.push_back(n);

    if (candidates.empty())
        return;

    HexNode* hex = candidates[rand() % candidates.size()];
    if (!hex) return;

    QPointF hexCenter = axialToPixel(hex->q, hex->r);
    apple = randomPointInHex(hexCenter);
}








void HexView::drawApple(QPainter& p)
{
    QPointF screen = apple * zoom + camera;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(220, 40, 40));
    p.drawEllipse(screen, 2.0f * zoom, 2.0f * zoom);

    if (!isAppleOnScreen())
        drawApplePointer(p);
}



bool HexView::isAppleOnScreen() const
{
    QPointF s = apple * zoom + camera;
    return rect().contains(s.toPoint());
}

void HexView::drawApplePointer(QPainter& p)
{
    QPointF center(width()/2.0, height()/2.0);
    QPointF target = apple * zoom + camera;

    QPointF v = target - center;
    double len = std::hypot(v.x(), v.y());
    if (len < 1.0) return;

    QPointF dir = v / len;

    double R = std::min(width(), height()) * 0.45;
    QPointF pos = center + dir * R;

    // угол направления
    double angle = std::atan2(dir.y(), dir.x()) * 180.0 / M_PI;

    QRectF arcRect(
        pos.x() - 18, pos.y() - 18,
        36, 36
        );

    p.setPen(QPen(QColor(220, 40, 40, 180), 4));
    p.setBrush(Qt::NoBrush);

    // дуга смотрит в сторону яблока
    p.drawArc(
        arcRect,
        int((-angle - 60) * 16),
        int(120 * 16)
        );
}

HexNode* HexView::hexAtAxial(int q, int r)
{
    for (HexNode* n : grid.all())
    {
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
        return; // не попали точно по пути

    // целевая точка
    QPointF targetWorld = path[best];

    // находим гекс
    HexNode* targetHex = hexAtWorld(targetWorld);
    if (!targetHex || targetHex->state != HexState::Generated)
        return; // защита от мусора

    // обновляем текущий гекс
    cur = targetHex;

    // восстанавливаем текущую позицию
    QPointF hexCenter = axialToPixel(cur->q, cur->r);
    cursor = targetWorld - hexCenter;

    // очистка хвостов
    arrowDir = 0;

    // можно обрезать путь до этой точки
    // path.resize(best + 1);
    cameraDragOffset = {0, 0};
    path.Append({step/2, step/2});
    path.Append(cursorWorldPos());
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


void HexView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(30, 30, 30));


    // гексы и точки
    for (auto* n : grid.all())
    {
        QPointF hexWorld = axialToPixel(n->q, n->r);
        QPointF c = hexWorld * zoom + camera;

        QPolygonF h = hexPolygonAt(c);

        p.setPen(Qt::NoPen);
        if (n->state == HexState::Generated){
            p.setBrush( QColor(245, 222, 179));
            p.drawPolygon(h);
        }

    }





    // внутренний лабиринт
    const float step = hexRadius * 0.05f;
    QPointF zero = {step/2, step/2};


    float roadOuter = step * 0.35f; // общая ширина дороги (чёрный)
    float roadInner = step * 0.20f; // внутренняя (цвет гекса)


    p.setPen(QPen(Qt::black, roadOuter * zoom));

    for (auto& c: grid.maze)
    {
        QPointF p0 = c.pos * zoom + camera;

        if (c.edge[0])
            p.drawLine(p0, (c.pos + QPointF(step, 0)) * zoom + camera);
        if (c.edge[1])
            p.drawLine(p0, (c.pos - QPointF(step, 0)) * zoom + camera);
        if (c.edge[2])
            p.drawLine(p0, (c.pos - QPointF(0, step)) * zoom + camera);
        if (c.edge[3])
            p.drawLine(p0, (c.pos + QPointF(0, step)) * zoom + camera);
    }

    p.setPen(QPen(QColor(245, 222, 179), roadInner * zoom));
    for (auto& c : grid.maze)
    {
        QPointF p0 = c.pos * zoom + camera;

        if (c.edge[0])
            p.drawLine(p0, (c.pos + QPointF(step, 0)) * zoom + camera);
        if (c.edge[1])
            p.drawLine(p0, (c.pos - QPointF(step, 0)) * zoom + camera);
        if (c.edge[2])
            p.drawLine(p0, (c.pos - QPointF(0, step)) * zoom + camera);
        if (c.edge[3])
            p.drawLine(p0, (c.pos + QPointF(0, step)) * zoom + camera);
    }

    for (auto* n : grid.all())
    {
        QPointF hexWorld = axialToPixel(n->q, n->r);
        QPointF c = hexWorld * zoom + camera;

        QPolygonF h = hexPolygonAt(c);
        p.setPen(Qt::NoPen);
        if (n->state != HexState::Generated){
            p.setBrush(Qt::black);
            p.drawPolygon(h);
        }

    }


    // BFS путь
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



    // путь курсора
    if (path.GetLength() > 1)
    {
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
            }
            pp.lineTo(pt);
        }

        p.drawPath(pp);
    }
    QPointF center =
        axialToPixel(cur->q, cur->r) * zoom +
        cursor * zoom +
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

    drawApple(p);

    // панель очков
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


    if (messageTimer.isValid() &&
        messageTimer.elapsed() < MESSAGE_TIME_MS)
    {
        QString text;
        QColor bg;

        if (showNoAppleHere)
        {
            text = "Яблоко не в этом гексе";
            bg = QColor(220, 180, 50, 160); // полупрозрачный
        }
        else if (showNoPath)
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
        f.setPointSizeF(14); // можно без zoom
        p.setFont(f);

        p.setPen(QColor(255, 255, 255, 220)); // слегка прозрачный текст
        p.drawText(box, Qt::AlignCenter, text);
    }
}


int targetSide = -1;

bool HexView::isExitToNeighbor(int cellId)
{
    const float step = hexRadius * 0.05f;
    const MazeCell& c = grid.maze[cellId];

    QPointF np = c.pos;

    // вышли за текущий гекс
    if (!pointInsideHex(np - axialToPixel(cur->q, cur->r), hexRadius))
    {
        int s1, s2;
        QPointF local = np - axialToPixel(cur->q, cur->r);
        if (crossedSides(local, s1, s2))
        {
            if (s1 == targetSide || s2 == targetSide)
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


ArraySequence<QPointF> bfsInHex(
    HexGrid& grid,
    HexNode* hex,
    int startId,
    std::function<bool(int)> isTarget,
    float hexRadius
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
            ArraySequence<QPointF> res;
            for (int cur = v; cur != -1; cur = parent[cur])
                res.Append(grid.maze[cur].pos);
            return res;
        }

        for (int d = 0; d < 4; ++d)
        {
            if (!grid.maze[v].edge[d])
                continue;

            QPointF np = grid.maze[v].pos + dirVec[d] * (hexRadius * 0.05f);
            auto it = grid.index.find(cellKey(np, hexRadius * 0.05f));
            if (it == grid.index.end())
                continue;

            int to = it->second;
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

    int startId = grid.addCell(cursorWorldPos(), step);


    bfsPath = bfsInHex(
        grid,
        cur,
        startId,
        [this](int id){ return isExitToNeighbor(id); },
        hexRadius
        );
    if (bfsPath.GetLength() > 1)
    {
        // путь найден — ничего не показываем
        showNoAppleHere = false;
        showNoPath = false;
    }
    else{
        showNoAppleHere = false;
        showNoPath = true;
        messageTimer.restart();
    }

}

void HexView::runBfsToApple()
{
    int startId = grid.addCell(cursorWorldPos(), step);

    bfsPath = bfsInHex(
        grid,
        cur,
        startId,
        [this](int id){
            return QLineF(grid.maze[id].pos, apple).length()
            < hexRadius * 0.05f;
        },
        hexRadius
        );
    if (bfsPath.GetLength() > 1)
    {
        // путь найден — ничего не показываем
        showNoAppleHere = false;
        showNoPath = false;
    }
    else
    {
        if (!isAppleInHex(cur))
        {
            showNoAppleHere = true;
            showNoPath = false;
        }
        else
        {
            showNoAppleHere = false;
            showNoPath = true;
        }
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
        findApple = true;
        update();
    });
}
