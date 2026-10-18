#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QApplication>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Установка иконки из файлов ресурсаов (искать в ресурсах файл иконки)
    this->setWindowIcon(QIcon(":/Icon.ico"));
    // Устанавливаем связь между кнопкой "Создать" и функцией onCreateFoldersClicked
    connect(ui->pushButtonCreate, &QPushButton::clicked, this, &MainWindow::onCreateFoldersClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onCreateFoldersClicked()
{
    // Эти параметры установлены для полного сброса состояния перед новым запуском
    m_archiveFiles.clear();
    m_currentIndex = 0;
    m_archivesPath.clear();
    m_targetPath.clear();
    m_baseName.clear();
    m_count = 0;
    m_digitsCount = 0;
    m_archiverType.clear();
    m_archiverPath.clear();
    m_baseArguments.clear();
    m_errorLog.clear();

    QString archiverChoice = ui->comboBoxArchiver->currentText();
    QString archivesPath = "";

    // 0. Выбор папки с архивами (если нужно)
    if (archiverChoice != "Не распаковывать") {
        archivesPath = QFileDialog::getExistingDirectory(this, "Где лежат архивы?");
        if (archivesPath.isEmpty()) return; // Пользователь нажал «Отмена»

        // Пытаемся автоматически подставить имя первой папки на основе имени первого архива
        QDir dir(archivesPath);
        QStringList filters;
        filters << "*.zip" << "*.rar" << "*.7z";
        dir.setNameFilters(filters);
        QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

        if (!files.isEmpty()) {
            QString defaultName = QFileInfo(files.first()).completeBaseName();
            // <-- ГЛАВНАЯ ЛОГИКА ИМЕНИ:
            // Если пользователь НЕ ввёл ничего в lineEditName, ставим авто-имя из архива.
            // Если ввёл — оставляем то, что ввёл.
            if (ui->lineEditName->text().isEmpty()) {
                ui->lineEditName->setText(defaultName);
            }
        }
    }

    // 1. Забираем данные из интерфейса
    int count = ui->spinBoxCount->value();
    QString baseName = ui->lineEditName->text();

    // <-- ДЕФОЛТНОЕ ИМЯ, ЕСЛИ ПОЛЬЗОВАТЕЛЬ НИЧЕГО НЕ ВПИСАЛ
    if (baseName.isEmpty()) {
        baseName = "Папка №";
    }

    // 2. Выбор целевой папки
    QString targetPath = QFileDialog::getExistingDirectory(this, "Куда создаем папки?");
    if (targetPath.isEmpty()) return;

    QDir targetDir(targetPath);
    int digitsCount = QString::number(count).length();

    // 3. Создаём папки (синхронно, это быстро)
    ui->labelStatus->setText("Создание папок...");
    ui->progressBar->setMaximum(count);
    ui->progressBar->setValue(0);
    qApp->processEvents();

    for (int i = 1; i <= count; ++i) {
        QString numStr = QString::number(i).rightJustified(digitsCount, '0');
        QString folderName = baseName + " " + numStr;
        targetDir.mkdir(folderName);

        ui->progressBar->setValue(i);
        qApp->processEvents();
    }

    // Если распаковывать не нужно — сразу финиш
    if (archiverChoice == "Не распаковывать") {
        ui->progressBar->setValue(0);
        ui->labelStatus->setText("Готово!");
        QMessageBox::information(this, "Всё готово", QString("Создано папок: %1").arg(count));
        return;
    }

    // --- Подготовка к асинхронной распаковке ---

    m_archivesPath = archivesPath;
    m_targetPath = targetPath;
    m_baseName = baseName;       // <-- Сюда уже попадает либо имя из архива, либо введённое, либо дефолт
    m_count = count;
    m_digitsCount = digitsCount;
    m_archiverType = archiverChoice;
    m_errorLog.clear();
    m_currentIndex = 0;

    // Определяем путь и базовые аргументы
    if (m_archiverType == "Распаковать при помощи WinRAR") {
        m_archiverPath = "C:\\Program Files\\WinRAR\\WinRAR.exe";
        if (!QFile::exists(m_archiverPath)) {
            m_archiverPath = "C:\\Program Files (x86)\\WinRAR\\WinRAR.exe";
        }
        m_baseArguments << "x" << "-ibck" << "-o+";
    } else { // 7-Zip
        m_archiverPath = "C:\\Program Files\\7-Zip\\7z.exe";
        if (!QFile::exists(m_archiverPath)) {
            m_archiverPath = "C:\\Program Files (x86)\\7-Zip\\7z.exe";
        }
        m_baseArguments << "x" << "-y";
    }

    if (!QFile::exists(m_archiverPath)) {
        QMessageBox::critical(this, "Ошибка", "Не найден архиватор по пути:\n" + m_archiverPath + "\nУстановите его или выберите другой.");
        ui->progressBar->setValue(0);
        ui->labelStatus->setText("Ошибка!");
        return;
    }

    // Собираем список архивов
    QDir archivesDir(m_archivesPath);
    QStringList filters;
    filters << "*.zip" << "*.rar" << "*.7z";
    archivesDir.setNameFilters(filters);

    m_archiveFiles = archivesDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    if (m_archiveFiles.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "В выбранной папке не найдено архивов (.zip, .rar, .7z).");
        ui->progressBar->setValue(0);
        ui->labelStatus->setText("Готово!");
        return;
    }

    m_archiveFiles.sort();

    // Ограничиваем количество архивов числом count
    if (m_archiveFiles.size() > m_count) {
        m_archiveFiles = m_archiveFiles.mid(0, m_count);
    }

    // Настраиваем прогресс-бар под количество архивов
    ui->progressBar->setMaximum(m_archiveFiles.size());
    ui->progressBar->setValue(0);
    ui->labelStatus->setText("Подготовка к распаковке...");
    qApp->processEvents();

    // Запускаем первый архив
    startNextArchive();
}

void MainWindow::startNextArchive()
{
    if (m_currentIndex >= m_archiveFiles.size()) {
        finishExtraction();
        return;
    }

    QString archiveName = m_archiveFiles.at(m_currentIndex);
    QString fullArchivePath = QDir::toNativeSeparators(m_archivesPath + "/" + archiveName);

    QString numStr = QString::number(m_currentIndex + 1).rightJustified(m_digitsCount, '0');
    QString folderName = m_baseName + " " + numStr;
    QString extractToPath = QDir::toNativeSeparators(m_targetPath + "/" + folderName);

    ui->labelStatus->setText("Распаковка: " + archiveName);

    // Обновляем прогресс ДО запуска процесса
    ui->progressBar->setValue(m_currentIndex + 1);
    qApp->processEvents();

    QProcess *process = new QProcess(this);

    connect(process, &QProcess::finished, this, [this, process](int exitCode) {
        if (m_currentIndex < m_archiveFiles.size()) {
            if (exitCode != 0) {
                QString errorDetails = QString::fromLocal8Bit(process->readAllStandardError());
                m_errorLog += "❌ Ошибка с '" + m_archiveFiles.at(m_currentIndex) + "':\n" + errorDetails + "\n\n";
            }
        }
        process->deleteLater();

        m_currentIndex++;
        startNextArchive();
    });

    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        Q_UNUSED(error);
        if (m_currentIndex < m_archiveFiles.size()) {
            QString errorDetails = QString::fromLocal8Bit(process->readAllStandardError());
            m_errorLog += "❌ Системная ошибка запуска архиватора:\n" + errorDetails + "\n\n";
        }
        process->deleteLater();

        m_currentIndex++;
        startNextArchive();
    });

    QStringList args = m_baseArguments;
    if (m_archiverType == "Распаковать при помощи 7-Zip") {
        args << fullArchivePath << ("-o" + extractToPath);
    } else {
        args << fullArchivePath << extractToPath;
    }

    process->start(m_archiverPath, args);
}

void MainWindow::finishExtraction()
{
    ui->progressBar->setValue(m_archiveFiles.size());
    ui->labelStatus->setText("Готово!");

    if (!m_errorLog.isEmpty()) {
        QMessageBox::warning(this, "Отчёт о распаковке", "Процесс завершён, но были ошибки:\n\n" + m_errorLog);
    } else {
        QMessageBox::information(this, "Успех", "Все папки созданы и архивы успешно распакованы!");
    }
}
