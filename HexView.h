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
    bool crossedSides(const QPointF& p, ArraySequence<int>& res);
    void moveToNeighbor(int side, QPointF delta);
    void centerCamera();
    QPointF cursorWorldPos() const;
    void spawnApple();
    bool isAppleOnScreen() const;
    void drawApple(QPainter& p);
    void drawApplePointer(QPainter& p);
    QPointF randomPointInHex(const QPointF& hexCenter);
    bool isAppleInHex(HexNode* h) const;
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
    ArraySequence<QPointF> bfsInHex(HexGrid& grid, HexNode* hex, int startId, std::function<bool(int)> isTarget, float hexRadius, bool apple);
    void drawGeneratedHex(QPainter& p);
    void drawMaze(QPainter& p);
    void drawBlackHex(QPainter& p);
    void drawBFS(QPainter& p);
    void drawPathCursor(QPainter& p);
    void drawCursor(QPainter& p);
    void drawScore(QPainter& p);
    void drawMessange(QPainter& p);

    QPointF apple;
    int score = 0;


    ArraySequence<QPointF> path;
    ArraySequence<QPointF>bfsPath;
    HexGrid grid;
    HexNode* cur = nullptr;
    MazeCell cursor;

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
    bool showNoPath = false;

    QElapsedTimer messageTimer;
    const int MESSAGE_TIME_MS = 1000;



};



