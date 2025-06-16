#ifndef CUSTOMWIDGETITEM_H
#define CUSTOMWIDGETITEM_H

#include <QString>
#include <QWidget>
#include <QListWidgetItem>

class CustomWidgetItem : public QListWidgetItem {
public:
    CustomWidgetItem(const QString& name, const QString& path) : MName(name), MPath(path) {
        this->setText(name);
    };
    ~CustomWidgetItem() {};
    QString& getPath() {return MPath;};
    QString& getName() {return MName;};

private:
    QString MName;
    QString MPath;
    QWidget* parent;
};

#endif // CUSTOMWIDGETITEM_H
