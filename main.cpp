#include <QApplication>
#include <QStyleFactory>
#include <QPalette>
#include <QFont>

#include "stickie_manager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Stickies"));
    app.setOrganizationName(QStringLiteral("stickies"));
    app.setQuitOnLastWindowClosed(false);

    // Use Fusion style for consistent look across platforms
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Slightly modernised default font
    QFont f = app.font();
    f.setPointSize(10);
    app.setFont(f);

    StickieManager manager;

    return app.exec();
}
