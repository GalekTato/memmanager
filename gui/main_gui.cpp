#include <QApplication>
#include <QFont>
#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Memory Manager");
    app.setApplicationVersion("1.0");

    // Try to use a monospace font
    QFont font("JetBrains Mono", 10);
    if (!font.exactMatch()) font.setFamily("Fira Code");
    if (!font.exactMatch()) font.setFamily("Monospace");
    app.setFont(font);

    MainWindow win;
    win.show();

    return app.exec();
}
