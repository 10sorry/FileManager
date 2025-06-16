#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "CustomWidgetItem.h"
#include <QFileSystemModel>
#include <QStandardPaths>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <unistd.h>
#include <QProcess>
#include <QTimer>
#include <QSplitter>
#include <QMenu>

const QString HOMEPATH = QDir::homePath();
const QString PROJECTROOTDIRNAME = "ProjectRootDir";
const QString PROJECTROOTDIRPATH = QDir::homePath() + "/" + PROJECTROOTDIRNAME;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    splitter->addWidget(ui->GroupFileWidget);
    splitter->addWidget(ui->treeView);

    ui->GroupFileWidget->setMinimumWidth(200);
    int tmp = ui->listWidget->sizeHintForRow(0) * ui->listWidget->count() + 2*ui->listWidget->frameWidth();
    qDebug() << ui->listWidget->sizeHintForRow(1) << ui->listWidget->count() << 2*ui->listWidget->frameWidth() << tmp;
    qDebug() << ui->listWidget->count() << ui->listWidget2->count();


    //splitter->setSizePolicy();

    ui->verticalLayout_2->addWidget(splitter);
    splitter->setSizes(QList<int>() << 5 << 300);

    model = new QFileSystemModel;
    model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    ui->treeView->setModel(model);
    model->setRootPath(QDir::rootPath());
    ui->treeView->setRootIndex(model->setRootPath(PROJECTROOTDIRPATH));
    createMainCategoiesDirs("ProjectRootDir", PROJECTROOTDIRPATH);
    loadCategories();

    ui->listWidget->setMaximumHeight( ui->listWidget->sizeHintForRow(0) * ui->listWidget->count() + 2*ui->listWidget->frameWidth());
    ui->listWidget2->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listWidget2, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showListContextMenu);

    connect(ui->treeView, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showTreeContextMenu);

    connect(ui->treeView, &QTreeView::doubleClicked, this, [=] (const QModelIndex& index){
        QString filePath = model->filePath(index);
        QFileInfo info(filePath);
        if (info.isFile()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    });


    connect(ui->listWidget, &QListWidget::itemDoubleClicked,
            this, [=](QListWidgetItem *current) {
                if (current) {
                    QString newPath = PROJECTROOTDIRPATH + "/" + current->text();
                    if (current->text() != PROJECTROOTDIRNAME) {
                        ui->treeView->setRootIndex(model->index(newPath));
                    } else {
                        ui->treeView->setRootIndex(model->index(PROJECTROOTDIRPATH));
                    }
                }
            });

    connect(ui->listWidget2, &QListWidget::itemDoubleClicked,
            this, [=](QListWidgetItem *current) {
                if (current) {
                    qDebug() << "Текущий элемент:" << current->text();
                    QString newPath = PROJECTROOTDIRPATH + "/" + current->text();
                    qDebug() << "NewPath: " << newPath;
                    if (current->text() != PROJECTROOTDIRNAME) {
                        ui->treeView->setRootIndex(model->index(newPath));
                    } else {
                        ui->treeView->setRootIndex(model->index(PROJECTROOTDIRPATH));
                    }
                }
            });

//    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [=] {
//        const QString path = ui->lineEdit->text();
//        if (QDir(path).exists() && !ui->lineEdit->text().isEmpty()) {
//            qDebug() << "Путь существует:" << path;
//        ui->treeView->setRootIndex(model->setRootPath(path));
//        } else { ui->treeView->setRootIndex(model->setRootPath(PROJECTROOTDIRPATH));}
//    });

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [=]() {
        const QString& str = ui->lineEdit->text();
        // Проверка, существует ли путь как директория
        if (QDir(str).exists() && !ui->lineEdit->text().isEmpty()) {
            ui->treeView->setRootIndex(model->index(str));
        }
        // Если это smb-путь
        else if (str.startsWith("smb://")) {
            // Ваш код для обработки SMB пути (например, монтирование или использование других инструментов для работы с ним)
            qDebug() << "SMB Path detected: " << str;
            QString smbIp = str.mid(6); //вырезать первые 6 символов str
            qDebug() << smbIp;
            mountAndDisplaySmb(str, smbIp);
        } else {
            qDebug() << "Invalid path: " << str;
            ui->treeView->setRootIndex(model->setRootPath(PROJECTROOTDIRPATH));
            QMessageBox::warning(this, "Invalid Path", "The path you entered does not exist or is not valid.");
        }
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showTreeContextMenu(const QPoint &pos)
{
    QAction *createFolderAction;
    QAction *createFileAction;
    QAction *renameAction;
    QAction *deleteAction;
    QAction *selectedAction;
    QPoint globalPos = ui->treeView->mapToGlobal(pos);

    QMenu contextMenu;
    QModelIndex index = ui->treeView->indexAt(pos);
    if (index.isValid()) {
        QFileInfo file(model->filePath(index));
        if (file.exists()) {
            renameAction = contextMenu.addAction("Пермеименовать");
            if (file.isDir()) {
                qDebug() << "This is dir";
                deleteAction = contextMenu.addAction("Удалить категорию вместе с папкой");
            } else {
                qDebug() << "This is file";
                deleteAction = contextMenu.addAction("Удалить файл");
            }
        }
    } else {
        createFolderAction = contextMenu.addAction("Создать папку");
        createFileAction = contextMenu.addAction("Создать файл");
    }


    selectedAction = contextMenu.exec(globalPos);

    if (selectedAction == renameAction) {
        qDebug() << "Rename";
    } else if (selectedAction == deleteAction) {
        qDebug() << "Remove";

    } else if (selectedAction == createFolderAction) {
        createFolder(1);
        qDebug() << "Add";
    } else if (selectedAction == createFileAction) {
        createFile();
        qDebug() << "Add";
    }
}


void MainWindow::showListContextMenu(const QPoint &pos)
{
    // Приводим локальную позицию к глобальной
    QPoint globalPos = ui->listWidget2->mapToGlobal(pos);

    QMenu contextMenu;

    QAction *renameAction = contextMenu.addAction("Пермеименовать");
    QAction *deleteAction = contextMenu.addAction("Удалить категорию вместе с папкой");
    QAction *addAction = contextMenu.addAction("Добавить категорию");
    QAction *deleteRootAction = contextMenu.addAction("Удалить директорю проекта");

    QAction *selectedAction = contextMenu.exec(globalPos);

    if (selectedAction == renameAction) {
        qDebug() << "Rename";
    } else if (selectedAction == deleteAction) {
        qDebug() << "Remove";
        emit on_DeleteToolButton_clicked();
    } else if (selectedAction == addAction) {
        qDebug() << "Add";
        createFolder(0);
    } else if (selectedAction == deleteRootAction) {
        qDebug() << "Add";
        emit on_DeleteRootFolderToolButton_clicked();
    }
}

void MainWindow::loadCategories()
{
    QSettings settings("PRZ", "FileManager");
    int size = settings.beginReadArray("categories");

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("name").toString();
//        QString path = settings.value("path").toString();
        addSideBarItems(name, 1);
        qDebug() << name;
    }

    settings.endArray();
    settings.clear();
}

void MainWindow::saveAllCategories()
{
    QSettings settings("PRZ", "FileManager");
    settings.beginWriteArray("categories");
    int count = ui->listWidget2->count();

    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QListWidgetItem* item = ui->listWidget2->item(i);
        settings.setValue("name", item->text());
    }
    settings.endArray();
}

void MainWindow::mountAndDisplaySmb(const QString& smbPath, const QString &smbIp)
{
    // Исправленный путь для монтирования
    QString mountedPath = "/run/user/" + QString::number(getuid()) + "/gvfs/smb-share:server=" + smbIp + ",share=public";
    qDebug() << "MOUNTED PATH: " << mountedPath;

    QString bashCmd = "gio mount smb://" + smbIp;
    qDebug() << "BASH COMMAND: " << bashCmd;
    QProcess::startDetached(bashCmd);
    QTimer::singleShot(1500, this, [=]() {
        if (QDir(mountedPath).exists()) {
            qDebug() << "Mounted!";
            model->setRootPath(mountedPath);
            ui->treeView->setRootIndex(model->index(mountedPath));
        } else {
            qDebug() << "Still not exists";
        }
    });

    // Проверка текущих смонтированных ресурсов
    QProcess process;
    process.start("gio", QStringList() << "mount" << "-l");
    process.waitForFinished();
    qDebug() << "Mounted resources: " << process.readAllStandardOutput();
}

void MainWindow::addSideBarItems(const QString &name, int type) {
    switch (type) {
        case 0:
            new QListWidgetItem(name, ui->listWidget);
            break;
        case 1:
            new QListWidgetItem(name, ui->listWidget2);
            break;
        default:
            break;
    }
}

void MainWindow::addSideBarItems(const QString &name, const QString &path)
{
    if(path.isEmpty() || !QFileInfo(path).exists()) return;
    CustomWidgetItem* item = new CustomWidgetItem(name, path);
    ui->listWidget2->addItem(item);
}

void MainWindow::on_AddToolButton_clicked()
{
    bool ok;
    QString categoryText = QInputDialog::getText(this,
                                         "Новая категория",
                                         "Назовите вашу категорию:",
                                         QLineEdit::Normal,
                                         "", &ok);
    if (ok && !categoryText.isEmpty()) {
        qDebug() << "New category: "  << categoryText;
    }
    qDebug() << "PROJECTROOTDIRPATH: " << PROJECTROOTDIRPATH << "FilePath: " << model->filePath(ui->treeView->rootIndex());
    if (model->filePath(ui->treeView->rootIndex()) != PROJECTROOTDIRPATH) {
        QMessageBox::warning(this, " ", "Корневая папка проекта не создана, категории не будут созданы");
        return;
    }

    addSideBarItems(categoryText, 1);
    model->mkdir(model->index(PROJECTROOTDIRPATH), categoryText);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveAllCategories();
    QMainWindow::closeEvent(event);
}

void MainWindow::createMainCategoiesDirs(const QString& name, const QString& path)
{
    QModelIndex index = model->index(path);
    if (QDir(path).exists()) {
        qDebug() << name << "is present in filesystem";
        //ui->treeView->setRootIndex(rootIndex);
        addSideBarItems(name, 0);
    } else if (path == PROJECTROOTDIRPATH) {
        qDebug() << "Creating root dir..." << PROJECTROOTDIRPATH;
        QModelIndex parentIndex = model->index(QDir::homePath());
        QModelIndex newDirIndex = model->mkdir(parentIndex, PROJECTROOTDIRNAME);
        ui->treeView->setRootIndex(newDirIndex);
        addSideBarItems(name, 0);
    } else {
        qDebug() << "Error: strange filesystem";
    }
}

void MainWindow::on_DeleteRootFolderToolButton_clicked()
{
    qDebug() << PROJECTROOTDIRPATH;
    if (!QDir(PROJECTROOTDIRPATH).exists()) { qDebug() << "RootDir not present in a filesystem - nothing to remove"; }
    else
    {
        QDir dir(PROJECTROOTDIRPATH);
        if (dir.removeRecursively()) {
            qDebug() << "RootDir was removed, current dir - " << QDir::rootPath();
        }
        model->setRootPath(QDir::rootPath());
        ui->treeView->setRootIndex(model->index(QDir::rootPath()));

    }
}

void MainWindow::on_ClearToolButton_clicked()
{
    QSettings settings("PRZ", "FileManager");
    settings.clear();

    ui->listWidget->clear();
}

void MainWindow::on_DeleteToolButton_clicked()
{
    QListWidgetItem *item = ui->listWidget2->currentItem();

    if (item) {
        QString categoryPath = model->filePath(ui->treeView->rootIndex()) + "/" + item->text();
        qDebug() << categoryPath;

        delete ui->listWidget2->takeItem(ui->listWidget2->row(item));
        QDir dir(categoryPath);
        if (dir.removeRecursively()) {
            qDebug() << "Dir was removed recused way";
        }
    } else {
        qDebug() << "No categories to delete";
    }
}

void MainWindow::createFolder(int type) {
    bool ok;
    QString txt;
    switch (type) {
        case 0: {
            txt = QInputDialog::getText(this,
                                                         "Новая категория",
                                                         "Назовите вашу категорию:",
                                                         QLineEdit::Normal,
                                                         "", &ok);
            break;
        }
        case 1: {
            txt = QInputDialog::getText(this,
                                                         "Новая папка",
                                                         "Назовите вашу папку:",
                                                         QLineEdit::Normal,
                                                         "", &ok);
            break;
        }
    }

    if (model->filePath(ui->treeView->rootIndex()) != PROJECTROOTDIRPATH) {
        QMessageBox::warning(this, " ", "Корневая папка проекта не создана, категории не будут созданы");
        return;
    }

    addSideBarItems(txt, 1);
    model->mkdir(model->index(PROJECTROOTDIRPATH), txt);
}

void MainWindow::createFile() {
    bool ok;
    QString txt = QInputDialog::getText(this,
                                                 "Новая файл",
                                                 "Назовите вашу файл:",
                                                 QLineEdit::Normal,
                                                 "", &ok);
    QString newFilePath = model->filePath(ui->treeView->rootIndex()) + "/" + txt;
    qDebug() << newFilePath;
    QProcess process;
    process.start("touch", QStringList() << newFilePath);
    if (!process.waitForFinished()) {
        qDebug() << "Ошибка при создании файла";
    }

    qDebug() << "touch stdout:" << process.readAllStandardOutput();
    qDebug() << "touch stderr:" << process.readAllStandardError();
}



