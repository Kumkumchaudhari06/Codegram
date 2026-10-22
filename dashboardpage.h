#pragma once
#include <QWidget>
#include "util.h"
#include "controller/controller.h"

class QFrame;
class QLabel;
class QStackedWidget;
class QTreeWidget;

// Screen 1: Dashboard, repository overview
class DashboardPage : public QWidget {
    Q_OBJECT
public:
    DashboardPage(Controller& ctl, Runner run, QWidget* parent = nullptr);
    void refresh();   // reads the Controller again

private:
    QFrame* makeCard(const QString& title, QLabel** value);

    Controller& ctl_;
    Runner run_;
    QStackedWidget* stack_ = nullptr;
    QLabel* pathLabel_ = nullptr;
    QLabel* mergeLabel_ = nullptr;
    QLabel* lastCommitLabel_ = nullptr;
    QLabel* branchValue_ = nullptr;
    QLabel* commitsValue_ = nullptr;
    QLabel* branchesValue_ = nullptr;
    QLabel* filesValue_ = nullptr;
    QLabel* stagedValue_ = nullptr;
    QLabel* unstagedValue_ = nullptr;
    QLabel* untrackedValue_ = nullptr;
    QLabel* conflictsValue_ = nullptr;
    QTreeWidget* recent_ = nullptr;
};