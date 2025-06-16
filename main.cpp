#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString path = "://styles/dark.qss";
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        QString style = QLatin1String(file.readAll());
        qApp->setStyleSheet(style);
    } else {
        qDebug() << "Не удалось открыть QSS:" << path;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
