#include "branchpage.h"
#include "theme.h"
#include "core/commitmanager.h"

#include <QCheckBox>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

BranchPage::BranchPage(Controller& ctl, Runner run, QWidget* parent) : QWidget(parent), ctl_(ctl), run_(run) {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    QHBoxLayout* head = new QHBoxLayout;
    QLabel* title = new QLabel("Branches");
    title->setObjectName("title");
    infoLabel_ = new QLabel;
    infoLabel_->setObjectName("muted");
    head->addWidget(title);
    head->addSpacing(16);
    head->addWidget(infoLabel_, 1);
    root->addLayout(head);

    list_ = new QTreeWidget;
    list_->setHeaderLabels({"", "Branch", "Commit", "Last commit message"});
    list_->setRootIsDecorated(false);
    list_->setColumnWidth(0, 30);
    list_->setColumnWidth(1, 220);
    list_->setColumnWidth(2, 90);
    root->addWidget(list_, 1);

    QHBoxLayout* row1 = new QHBoxLayout;
    switchBtn_ = new QPushButton("Switch to selected");
    switchBtn_->setObjectName("primary");
    force_ = new QCheckBox("Discard local changes (force)");
    mergeBtn_ = new QPushButton("Merge selected into current");
    row1->addWidget(switchBtn_);
    row1->addWidget(force_);
    row1->addStretch();
    row1->addWidget(mergeBtn_);
    root->addLayout(row1);

    QHBoxLayout* row2 = new QHBoxLayout;
    newBtn_ = new QPushButton("New branch...");
    newSwitchBtn_ = new QPushButton("New branch and switch...");
    deleteBtn_ = new QPushButton("Delete");
    forceDeleteBtn_ = new QPushButton("Force delete");
    row2->addWidget(newBtn_);
    row2->addWidget(newSwitchBtn_);
    row2->addStretch();
    row2->addWidget(deleteBtn_);
    row2->addWidget(forceDeleteBtn_);
    root->addLayout(row2);

    connect(list_, &QTreeWidget::currentItemChanged, this, [this] { updateButtons(); });
    connect(list_, &QTreeWidget::itemDoubleClicked, this, [this] { switchSelected(); });
    connect(switchBtn_, &QPushButton::clicked, this, [this] { switchSelected(); });
    connect(newBtn_, &QPushButton::clicked, this, [this] { createBranch(false); });
    connect(newSwitchBtn_, &QPushButton::clicked, this, [this] { createBranch(true); });
    connect(deleteBtn_, &QPushButton::clicked, this, [this] { deleteSelected(false); });
    connect(forceDeleteBtn_, &QPushButton::clicked, this, [this] { deleteSelected(true); });
    connect(mergeBtn_, &QPushButton::clicked, this, [this] {
        const QString name = selectedName();
        if (!name.isEmpty()) run_("merge " + quoted(name));     // command: merge <branch>
    });
    refresh();
}

QString BranchPage::selectedName() const {
    QTreeWidgetItem* item = list_->currentItem();
    return item ? item->data(1, Qt::UserRole).toString() : QString();
}

void BranchPage::refresh() {
    const QString keep = selectedName();
    list_->clear();
    if (!ctl_.isOpen()) {
        infoLabel_->setText("No repository open");
        updateButtons();
        return;
    }
    QString current;
    for (const BranchInfo& b : ctl_.branches()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(list_);
        item->setText(0, b.current ? "●" : "");
        item->setText(1, qs(b.name));
        item->setText(2, qs(CommitManager::shortHash(b.commit)));
        item->setText(3, qs(b.subject));
        item->setData(1, Qt::UserRole, qs(b.name));
        if (b.current) {
            QFont f = item->font(1);
            f.setBold(true);
            item->setFont(1, f);
            current = qs(b.name);
        }
        if (qs(b.name) == keep) list_->setCurrentItem(item);
    }
    infoLabel_->setText(current.isEmpty() ? QStringLiteral("HEAD is detached") : "Current branch: " + current);
    updateButtons();
}

void BranchPage::updateButtons() {
    const bool open = ctl_.isOpen();
    const QString name = selectedName();
    bool isCurrent = false;
    if (QTreeWidgetItem* it = list_->currentItem()) isCurrent = !it->text(0).isEmpty();
    const bool has = open && !name.isEmpty();
    switchBtn_->setEnabled(has && !isCurrent);
    force_->setEnabled(open);
    mergeBtn_->setEnabled(has && !isCurrent);
    newBtn_->setEnabled(open);
    newSwitchBtn_->setEnabled(open);
    deleteBtn_->setEnabled(has && !isCurrent);
    forceDeleteBtn_->setEnabled(has && !isCurrent);
}

void BranchPage::switchSelected() {
    const QString name = selectedName();
    if (name.isEmpty()) return;
    const QString cmd = force_->isChecked() ? "checkout -f " : "checkout ";
    run_(cmd + quoted(name));                                   // command: checkout [-f] <branch>
}

void BranchPage::createBranch(bool andSwitch) {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New branch", "Branch name:", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok || name.isEmpty()) return;
    const QString cmd = andSwitch ? "checkout -b " : "branch ";
    run_(cmd + quoted(name));                                   // command: branch <name> / checkout -b <name>
}

void BranchPage::deleteSelected(bool force) {
    const QString name = selectedName();
    if (name.isEmpty()) return;
    const QString text = force ? "Delete '" + name + "' even if it has unmerged commits?"
                               : "Delete branch '" + name + "'?";
    if (QMessageBox::question(this, "Delete branch", text) != QMessageBox::Yes) return;
    const QString cmd = force ? "branch -D " : "branch -d ";
    run_(cmd + quoted(name));                                   // command: branch -d / -D <name>
}