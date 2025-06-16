#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_AddToolButton_clicked();

    void createMainCategoiesDirs(const QString&, const QString&);

    void on_DeleteRootFolderToolButton_clicked();

    void on_ClearToolButton_clicked();

    void on_DeleteToolButton_clicked();

    void showListContextMenu(const QPoint &pos);
    void showTreeContextMenu(const QPoint &pos);
    void createFolder(int);
    void createFile();

private:
    Ui::MainWindow *ui;
    QFileSystemModel* model;
    QListWidgetItem *currentItem;
    void addSideBarItems(const QString& name, const QString& path);
    void addSideBarItems(const QString& name, int type);
    void saveAllCategories();
    void loadCategories();
    void closeEvent(QCloseEvent *event) override;
    void mountAndDisplaySmb(const QString&, const QString&);
};
#endif // MAINWINDOW_H
