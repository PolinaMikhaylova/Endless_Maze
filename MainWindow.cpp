#include "MainWindow.h"
#include "HexView.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    HexView* view = new HexView();
    setCentralWidget(view);
}
