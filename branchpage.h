#pragma once
#include <QWidget>
#include "util.h"
#include "controller/controller.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QTreeWidget;

// Screen 6: branch manager
class BranchPage : public QWidget {
    Q_OBJECT
public:
    BranchPage(Controller& ctl, Runner run, QWidget* parent = nullptr);
    void refresh();

private:
    QString selectedName() const;
    void updateButtons();
    void switchSelected();
    void createBranch(bool andSwitch);
    void deleteSelected(bool force);

    Controller& ctl_;
    Runner run_;
    QLabel* infoLabel_ = nullptr;
    QTreeWidget* list_ = nullptr;
    QCheckBox* force_ = nullptr;
    QPushButton* switchBtn_ = nullptr;
    QPushButton* newBtn_ = nullptr;
    QPushButton* newSwitchBtn_ = nullptr;
    QPushButton* deleteBtn_ = nullptr;
    QPushButton* forceDeleteBtn_ = nullptr;
    QPushButton* mergeBtn_ = nullptr;
};