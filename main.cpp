#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("Создатель электронных папок"); // ← ДОБАВЬ ЭТУ СТРОКУ
    w.show();
    return QApplication::exec();
}