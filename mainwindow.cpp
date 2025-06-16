#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "CustomWidgetItem.h"

#include <QFileSystemModel>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QProcess>
#include <QTimer>
#include <QSplitter>
#include <QMenu>
#include <QFile>
#include <QUrl>
#include <QStatusBar>
#include <QDateTime>
#include <QDebug>
#include <QFileIconProvider>
#include <unistd.h>

const QString HOMEPATH = QDir::homePath();
const QString PROJECTROOTDIRNAME = "ProjectRootDir";
const QString PROJECTROOTDIRPATH = HOMEPATH + "/" + PROJECTROOTDIRNAME;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QTimer::singleShot(0, this, [this]() {
        ui->listWidget->setCurrentRow(0);
    });

    QMenu *stylesMenu = ui->menuBar->findChild<QMenu*>("menuStyle");

    if (!stylesMenu) {
        qWarning() << "Не найдено меню 'menuStyle'!";
        return;
    }

    QDir dir(":/styles");
    QStringList qssFiles = dir.entryList(QStringList() << "*.qss", QDir::Files);

    QList<QAction*> themeActions;

    for (const QString &fileName : qssFiles) {
        QAction *action = new QAction(fileName, this);

        connect(action, &QAction::triggered, this, [this, fileName]() {
            QFile file(":/styles/" + fileName);
            if (file.open(QFile::ReadOnly | QFile::Text)) {
                QString style = file.readAll();
                qApp->setStyleSheet(style);
            }
        });

        themeActions.append(action);
    }
    stylesMenu->addActions(themeActions);

    //applyDarkTheme();

    splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(ui->GroupFileWidget);
    splitter->addWidget(ui->treeView);
    ui->GroupFileWidget->setMinimumWidth(200);
    ui->verticalLayout_2->addWidget(splitter);
    splitter->setSizes(QList<int>() << 5 << 300);

    QSettings settings("PRZ", "FileManager");
    QVariant value = settings.value("splitterSizes");
    if (value.isValid()) {
        QVariantList variantSizes = value.toList();
        QList<int> sizes;
        for (const QVariant &var : variantSizes)
            sizes << var.toInt();
        splitter->setSizes(sizes);
    }

    model = new QFileSystemModel(this);
    model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    ui->treeView->setModel(model);
    model->setRootPath(QDir::rootPath());
    ui->treeView->setRootIndex(model->setRootPath(PROJECTROOTDIRPATH));

    createMainCategoiesDirs(PROJECTROOTDIRNAME, PROJECTROOTDIRPATH);
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    qDebug() << QStandardPaths::displayName(QStandardPaths::DesktopLocation) << desktopPath;
    createMainCategoiesDirs(QStandardPaths::displayName(QStandardPaths::DesktopLocation), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/");
    createMainCategoiesDirs(QStandardPaths::displayName(QStandardPaths::DocumentsLocation), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/");
    loadCategories();
    ui->listWidget->setMaximumHeight( ui->listWidget->sizeHintForRow(0) * ui->listWidget->count() + 2*ui->listWidget->frameWidth());

    ui->listWidget2->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->treeView->setDragEnabled(true);
    ui->treeView->setAcceptDrops(true);
    ui->treeView->setDropIndicatorShown(true);
    ui->treeView->setDragDropMode(QAbstractItemView::DragDrop);

    connect(ui->listWidget2, &QListWidget::customContextMenuRequested, this, &MainWindow::showListContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showTreeContextMenu);

    connect(ui->treeView, &QTreeView::doubleClicked, this, [=](const QModelIndex &index) {
        QString filePath = model->filePath(index);
        if (QFileInfo(filePath).isFile()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    });

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::updateStatusBarInfo);

    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem *item) {
        if (item) {
            QString newPath = HOMEPATH + "/" + item->text();
            qDebug() << newPath;
            ui->treeView->setRootIndex(model->index(item->text() == PROJECTROOTDIRNAME ? PROJECTROOTDIRPATH : newPath));

        }
    });

    connect(ui->listWidget2, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem *item) {
        if (item) {
            QString newPath = PROJECTROOTDIRPATH + "/" + item->text();
            ui->treeView->setRootIndex(model->index(newPath));
        }
    });

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::handlePathInput);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    foreach (const QUrl &url, event->mimeData()->urls()) {
        QString filePath = url.toLocalFile();
        qDebug() << "Dropped file:" << filePath;

    }
    event->acceptProposedAction();
}

void MainWindow::showTreeContextMenu(const QPoint &pos) {
    QMenu contextMenu;
    QPoint globalPos = ui->treeView->mapToGlobal(pos);
    QModelIndex index = ui->treeView->indexAt(pos);

    QAction *renameAction = nullptr;
    QAction *deleteAction = nullptr;

    if (index.isValid()) {
        QFileInfo file(model->filePath(index));
        renameAction = contextMenu.addAction("Переименовать");

        if (file.isDir()) {
            deleteAction = contextMenu.addAction("Удалить папку", this, &MainWindow::onDelete);
            deleteAction->setData(true);  // true == папка
        } else {
            deleteAction = contextMenu.addAction("Удалить файл", this, &MainWindow::onDelete);
            deleteAction->setData(false);  // false == папка
        }
    } else {
        contextMenu.addAction("Создать папку", this, [=]() { createFolder(1); });
        contextMenu.addAction("Создать файл", this, &MainWindow::createFile);
    }

    QAction *selectedAction = contextMenu.exec(globalPos);
    if (!selectedAction) return;

    if (selectedAction == renameAction) {
        // TODO: Реализовать rename
    } else if (selectedAction == deleteAction) {
        // TODO: Реализовать delete
    }
}

void MainWindow::showListContextMenu(const QPoint &pos) {
    QMenu contextMenu;
    QPoint globalPos = ui->listWidget2->mapToGlobal(pos);

    contextMenu.addAction("Переименовать");
    contextMenu.addAction("Удалить категорию с папкой", this, &MainWindow::on_DeleteToolButton_clicked);
    contextMenu.addAction("Добавить категорию", this, [=]() { createFolder(0); });
    contextMenu.addAction("Удалить директорию проекта", this, &MainWindow::on_DeleteRootFolderToolButton_clicked);

    contextMenu.exec(globalPos);
}

void MainWindow::loadCategories() {
    QSettings settings("PRZ", "FileManager");
    int size = settings.beginReadArray("categories");

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("name").toString();
        qDebug() << name;
        if (!name.isEmpty()) addSideBarItems(name, 1);
    }

    settings.endArray();
    settings.clear();
}

void MainWindow::saveAllCategories() {
    QSettings settings("PRZ", "FileManager");
    settings.beginWriteArray("categories");

    for (int i = 0; i < ui->listWidget2->count(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", ui->listWidget2->item(i)->text());
    }

    settings.endArray();
}

void MainWindow::mountAndDisplaySmb(const QString &smbPath, const QString &smbIp) {
    QString mountPath = QString("/run/user/%1/gvfs/smb-share:server=%2,share=public").arg(getuid()).arg(smbIp);
    QProcess::startDetached("gio mount smb://" + smbIp);

    QTimer::singleShot(1500, this, [=]() {
        if (QDir(mountPath).exists()) {
            ui->treeView->setRootIndex(model->index(mountPath));
        }
    });
}

void MainWindow::onDelete(bool isFile)
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action) return;

    bool isDir = action->data().toBool();

    bool isCategory = false;

    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QFileInfo fileInfo(path);

    if (isDir) {
        if (fileInfo.isDir()) {
            for (int i = ui->listWidget2->count() - 1; i >= 0; --i) {
                QListWidgetItem* item = ui->listWidget2->item(i);
                if (item->text() == fileInfo.fileName()) {
                    qDebug() << "it's a category" << item->text();
                    isCategory = true;
                    delete ui->listWidget2->takeItem(i);
                }
            }
            if (QDir(path).removeRecursively()) {
                qDebug() << "Папка удалена:" << path;
            } else {
                qDebug() << "Не удалось удалить папку:" << path;
            }
        }
    } else {
        if (fileInfo.isFile()) {
            if (QFile::remove(path)) {
                qDebug() << "Файл удалён:" << path;
            } else {
                qDebug() << "Не удалось удалить файл:" << path;
            }
        }
    }
    qDebug() << ui->treeView->currentIndex();
    //QDir(categoryPath).removeRecursively();
}

void MainWindow::addSideBarItems(const QString &name, int type) {
    if (name.isEmpty()) return;
    QFileIconProvider iconProvider;
    QIcon icon;
    if (name == "Desktop") {
        icon = QIcon(":/icons/desktop.png");
    } else if (name == "Documents") {
        icon = QIcon(":/icons/docs.png");
    } else if (name == "Downloads") {
        icon = QIcon(":/icons/docs.png");
    } else {
        icon = iconProvider.icon(QFileIconProvider::Desktop);
    }

    switch (type) {
    case 0: new QListWidgetItem(icon, name, ui->listWidget); break;
    case 1: new QListWidgetItem(name, ui->listWidget2); break;
    }
}

QIcon MainWindow::iconForLocation(StandardLocation location)
{
    switch (location) {
    case StandardLocation::Desktop:
        return QIcon(":/icons/desktop.png");
    case StandardLocation::Documents:
        return QIcon(":/icons/documents.png");
    case StandardLocation::Downloads:
        return QIcon(":/icons/downloads.png");
    case StandardLocation::Pictures:
        return QIcon(":/icons/pictures.png");
    case StandardLocation::Music:
        return QIcon(":/icons/music.png");
    case StandardLocation::Videos:
        return QIcon(":/icons/videos.png");
    default:
        return QIcon(":/icons/file.png");
    }
}


void MainWindow::createMainCategoiesDirs(const QString &name, const QString &path) {
    if (QDir(path).exists()) {
        addSideBarItems(name, 0);
    } else if (path == PROJECTROOTDIRPATH) {
        model->mkdir(model->index(HOMEPATH), PROJECTROOTDIRNAME);
        addSideBarItems(name, 0);
    }
}

void MainWindow::on_DeleteRootFolderToolButton_clicked() {
    if (QDir(PROJECTROOTDIRPATH).removeRecursively()) {
        model->setRootPath(QDir::rootPath());
        ui->treeView->setRootIndex(model->index(QDir::rootPath()));
    }
}

void MainWindow::on_ClearToolButton_clicked() {
    QSettings("PRZ", "FileManager").clear();
    ui->listWidget->clear();
}

void MainWindow::applyDarkTheme() {
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);

    qApp->setPalette(darkPalette);
}

void MainWindow::updateStatusBarInfo(const QModelIndex &current, const QModelIndex &)
{
    QFileInfo info(model->filePath(current));
    QString details;

    if (info.exists()) {
        details = QString("Имя: %1 | Размер: %2 байт | Изменён: %3 | Права: %4 | Тип: %5")
                      .arg(info.fileName())
                      .arg(info.size())
                      .arg(info.lastModified().toString("dd.MM.yyyy hh:mm"))
                      .arg(info.permissions(), 0, 8)
                      .arg(info.isDir() ? "Папка" : (info.isSymLink() ? "Симлинк" : "Файл"));
    }

    statusBar()->showMessage(details);
}


void MainWindow::on_DeleteToolButton_clicked() {
    QListWidgetItem *item = ui->listWidget2->currentItem();
    if (!item) return;

    QString categoryPath = model->filePath(ui->treeView->rootIndex()) + "/" + item->text();
    delete ui->listWidget2->takeItem(ui->listWidget2->row(item));
    QDir(categoryPath).removeRecursively();
}

void MainWindow::createFolder(int type) {
    bool ok;
    QString title = (type == 0) ? "Новая категория" : "Новая папка";
    QString text = QInputDialog::getText(this, title, "Введите название:", QLineEdit::Normal, "", &ok);

    if (!ok || text.isEmpty()) return;

    if (model->filePath(ui->treeView->rootIndex()) != PROJECTROOTDIRPATH) {
        QMessageBox::warning(this, "Ошибка", "Категории создаются только в корневой папке проекта.");
        return;
    }

    addSideBarItems(text, 1);
    model->mkdir(model->index(PROJECTROOTDIRPATH), text);
}

void MainWindow::createFile() {
    bool ok;
    QString fileName = QInputDialog::getText(this, "Новый файл", "Введите название файла:", QLineEdit::Normal, "", &ok);

    if (!ok || fileName.isEmpty()) return;

    QString newFilePath = model->filePath(ui->treeView->rootIndex()) + "/" + fileName;
    QFile file(newFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось создать файл.");
    }
}

void MainWindow::handlePathInput()
{
    const QString& str = ui->lineEdit->text();
    if (QDir(str).exists() && !ui->lineEdit->text().isEmpty()) {
        ui->treeView->setRootIndex(model->index(str));
    }
    else if (str.startsWith("smb://")) {
        qDebug() << "SMB Path detected: " << str;
        QString smbIp = str.mid(6); //вырезать первые 6 символов str
        qDebug() << smbIp;
        mountAndDisplaySmb(str, smbIp);
    } else {
        qDebug() << "Invalid path: " << str;
        ui->treeView->setRootIndex(model->setRootPath(PROJECTROOTDIRPATH));
        QMessageBox::warning(this, "Invalid Path", "The path you entered does not exist or is not valid.");
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveAllCategories();

    QSettings settings("PRZ", "FileManager");

    QList<int> sizes = splitter->sizes();
    QVariantList variantSizes;
    for (int size : sizes)
        variantSizes << size;
    settings.setValue("splitterSizes", variantSizes);  // <-- вот сохранение текущих размеров

    QMainWindow::closeEvent(event);
}
