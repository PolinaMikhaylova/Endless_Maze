#pragma once
#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include<QElapsedTimer>
#include "HexGrid.h"


enum class Neighbor {
    LeftUp = 4,
    Left   = 3,
    LeftDown = 2,
    RightUp = 5,
    Right  = 0,
    RightDown = 1
};


class HexView : public QWidget {
    Q_OBJECT
public:
    explicit HexView(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void paintEvent(QPaintEvent*) override;


private:
    QPointF axialToPixel(int q, int r) const;
    QPolygonF hexPolygonAt(const QPointF& center) const;
    int sideFromVector(const QPointF& v) const;
    bool crossedSides(const QPointF& p, int& s1, int& s2) const;
    void moveToNeighbor(int side, QPointF delta, bool b);
    void centerCamera();
    QPointF cursorWorldPos() const;
    void spawnApple();
    bool isAppleOnScreen() const;
    void drawApple(QPainter& p);
    void drawApplePointer(QPainter& p);
    QPointF randomPointInHex(const QPointF& hexCenter);
    bool isAppleInHex(HexNode* h) const;
    MazeCell* cellAt(const QPointF& worldPos);
    void tryTeleportToPath(const QPointF& screenPos);
    HexNode* hexAtWorld(const QPointF& world);
    HexNode* hexAtAxial(int q, int r);
    bool hasEdgeBetween(const QPointF& a, const QPointF& b);
    bool hasDirectedEdge(const QPointF& from, const QPointF& to);
    void setupNavButton();
    void setupNavMenu();
    void runBfsToNeighbor(Neighbor n);
    bool isExitToNeighbor(int cellId);
    void runBfsToApple();

    QPointF apple;
    int score = 0;


    ArraySequence<QPointF> path;
    ArraySequence<QPointF>bfsPath;
    HexGrid grid;
    HexNode* cur = nullptr;
    QPointF cursor;

    QPointF camera;
    QPointF cameraDragOffset;
    QPointF lastMouse;
    bool dragging = false;

    const float hexRadius = 120.0f;
    const float step = hexRadius * 0.05f;
    float zoom = 1.0f;
    int arrowDir = 0;
    const float pi = acos(-1);


    QPushButton* navButton;
    QMenu* navMenu;

    bool findApple = false;
    // сообщения
    bool showNoAppleHere = false;
    bool showNoPath = false;

    QElapsedTimer messageTimer;
    const int MESSAGE_TIME_MS = 1000;



};



