#include <QApplication>
#include "HexView.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    HexView view;
    view.setWindowTitle("Hex Infinite Canvas");
    view.show();
    return app.exec();
}
