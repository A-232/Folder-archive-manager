#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCreateFoldersClicked();

private:
    // Вспомогательная функция для запуска следующего архива
    void startNextArchive();
    // Завершение процесса распаковки
    void finishExtraction();

    // Переменные состояния для асинхронной распаковки
    QList<QString> m_archiveFiles;
    int m_currentIndex = 0;
    QString m_archivesPath;
    QString m_targetPath;
    QString m_baseName;
    int m_count = 0;
    int m_digitsCount = 0;
    QString m_archiverType;
    QString m_archiverPath;
    QStringList m_baseArguments;
    QString m_errorLog;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
