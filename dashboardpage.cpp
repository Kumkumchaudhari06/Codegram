#include "dashboardpage.h"
#include "theme.h"
#include "core/commitmanager.h"

#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

QFrame* DashboardPage::makeCard(const QString& title, QLabel** value) {
    QFrame* card = new QFrame;
    card->setObjectName("card");
    QVBoxLayout* l = new QVBoxLayout(card);
    l->setContentsMargins(16, 12, 16, 12);
    l->setSpacing(2);
    QLabel* t = new QLabel(title);
    t->setObjectName("cardTitle");
    *value = new QLabel("-");
    (*value)->setObjectName("cardValue");
    l->addWidget(t);
    l->addWidget(*value);
    return card;
}

DashboardPage::DashboardPage(Controller& ctl, Runner run, QWidget* parent)
    : QWidget(parent), ctl_(ctl), run_(run) {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    QLabel* title = new QLabel("Dashboard");
    title->setObjectName("title");
    root->addWidget(title);

    stack_ = new QStackedWidget;
    root->addWidget(stack_, 1);

    // ---- page 0: no repository open ----
    QWidget* empty = new QWidget;
    QVBoxLayout* emptyLayout = new QVBoxLayout(empty);
    emptyLayout->addStretch();
    QLabel* message = new QLabel("No repository is open.\nCreate a new one, or open an existing folder.");
    message->setObjectName("muted");
    message->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(message);
    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addStretch();
    QPushButton* newBtn = new QPushButton("New repository");
    newBtn->setObjectName("primary");
    QPushButton* openBtn = new QPushButton("Open repository");
    buttons->addWidget(newBtn);
    buttons->addWidget(openBtn);
    buttons->addStretch();
    emptyLayout->addLayout(buttons);
    emptyLayout->addStretch();
    stack_->addWidget(empty);

    connect(newBtn, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(this, "Folder for the new repository");
        if (!dir.isEmpty()) run_("init " + quoted(dir));       // command: init
    });
    connect(openBtn, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(this, "Open a repository folder");
        if (!dir.isEmpty()) run_("open " + quoted(dir));       // command: open
    });

    // ---- page 1: the overview ----
    QWidget* content = new QWidget;
    QVBoxLayout* cl = new QVBoxLayout(content);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(12);

    pathLabel_ = new QLabel;
    pathLabel_->setObjectName("muted");
    cl->addWidget(pathLabel_);
    mergeLabel_ = new QLabel("Merge in progress: resolve the conflicts, then commit.");
    mergeLabel_->setStyleSheet("color: " + Theme::warn + "; font-weight: bold;");
    cl->addWidget(mergeLabel_);

    QGridLayout* cards = new QGridLayout;
    cards->setSpacing(12);
    cards->addWidget(makeCard("Branch", &branchValue_), 0, 0);
    cards->addWidget(makeCard("Commits", &commitsValue_), 0, 1);
    cards->addWidget(makeCard("Branches", &branchesValue_), 0, 2);
    cards->addWidget(makeCard("Tracked files", &filesValue_), 0, 3);
    cards->addWidget(makeCard("Staged", &stagedValue_), 1, 0);
    cards->addWidget(makeCard("Not staged", &unstagedValue_), 1, 1);
    cards->addWidget(makeCard("Untracked", &untrackedValue_), 1, 2);
    cards->addWidget(makeCard("Conflicts", &conflictsValue_), 1, 3);
    cl->addLayout(cards);

    lastCommitLabel_ = new QLabel;
    cl->addWidget(lastCommitLabel_);

    QLabel* recentTitle = new QLabel("Recent commits");
    recentTitle->setObjectName("muted");
    cl->addWidget(recentTitle);
    recent_ = new QTreeWidget;
    recent_->setHeaderLabels({"Commit", "Date", "Message"});
    recent_->setRootIsDecorated(false);
    recent_->setColumnWidth(0, 90);
    recent_->setColumnWidth(1, 170);
    cl->addWidget(recent_, 1);

    stack_->addWidget(content);
    refresh();
}

void DashboardPage::refresh() {
    Dashboard d;
    if (!ctl_.isOpen() || !ctl_.dashboard(d).ok) {
        stack_->setCurrentIndex(0);
        return;
    }
    stack_->setCurrentIndex(1);

    pathLabel_->setText(qs(d.root) + "      user: " + qs(d.user));
    mergeLabel_->setVisible(d.merging);
    branchValue_->setText(d.detached ? QStringLiteral("detached") : qs(d.branch));
    commitsValue_->setText(QString::number(d.commitCount));
    branchesValue_->setText(QString::number(d.branchCount));
    filesValue_->setText(QString::number(d.trackedFiles));
    stagedValue_->setText(QString::number(d.staged));
    unstagedValue_->setText(QString::number(d.unstaged));
    untrackedValue_->setText(QString::number(d.untracked));
    conflictsValue_->setText(QString::number(d.conflicts));
    conflictsValue_->setStyleSheet(d.conflicts > 0 ? "color: " + Theme::bad + ";" : QString());

    lastCommitLabel_->setText(d.headShort.empty() ? QStringLiteral("Last commit: none yet")
                                                  : "Last commit: " + qs(d.headShort) + "  " + qs(d.headSubject));

    recent_->clear();
    for (const Commit& c : d.recent) {
        QTreeWidgetItem* item = new QTreeWidgetItem(recent_);
        item->setText(0, qs(CommitManager::shortHash(c.hash)));
        item->setText(1, qs(CommitManager::formatTime(c.timestamp)));
        item->setText(2, qs(CommitManager::firstLine(c.message)));
    }
}