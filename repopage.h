#pragma once
#include <QWidget>
#include "util.h"
#include "controller/controller.h"

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

// Screen 2: Repository manager and file browser
class RepoPage : public QWidget {
    Q_OBJECT
public:
    RepoPage(Controller& ctl, Runner run, QWidget* parent = nullptr);
    void refresh();

private:
    void showPreview(QTreeWidgetItem* item);
    void ignoreSelected();
    void chooseFolder(bool createNew);

    Controller& ctl_;
    Runner run_;
    QLabel* pathLabel_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QComboBox* filter_ = nullptr;
    QPushButton* closeBtn_ = nullptr;
    QPushButton* ignoreBtn_ = nullptr;
};