#include "repopage.h"
#include "theme.h"

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <map>

namespace {

QColor colorForState(const QString& state) {
    if (state == "modified" || state == "staged+modified") return QColor(Theme::warn);
    if (state == "staged") return QColor(Theme::good);
    if (state == "untracked") return QColor(Theme::info);
    if (state == "deleted" || state == "deleted (staged)" || state == "conflict") return QColor(Theme::bad);
    return QColor(Theme::muted);   // clean
}

} // namespace

RepoPage::RepoPage(Controller& ctl, Runner run, QWidget* parent) : QWidget(parent), ctl_(ctl), run_(run) {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    QLabel* title = new QLabel("Repository");
    title->setObjectName("title");
    root->addWidget(title);

    // ---- repository manager: new / open / close ----
    QHBoxLayout* top = new QHBoxLayout;
    pathLabel_ = new QLabel;
    pathLabel_->setObjectName("muted");
    top->addWidget(pathLabel_, 1);
    QPushButton* newBtn = new QPushButton("New repository");
    QPushButton* openBtn = new QPushButton("Open repository");
    closeBtn_ = new QPushButton("Close");
    top->addWidget(newBtn);
    top->addWidget(openBtn);
    top->addWidget(closeBtn_);
    root->addLayout(top);

    // ---- file browser: tree on the left, preview on the right ----
    QSplitter* split = new QSplitter(Qt::Horizontal);
    tree_ = new QTreeWidget;
    tree_->setHeaderLabels({"Name", "State"});
    tree_->setColumnWidth(0, 300);
    preview_ = new QPlainTextEdit;
    preview_->setReadOnly(true);
    preview_->setLineWrapMode(QPlainTextEdit::NoWrap);
    preview_->setFont(QFont(Theme::mono));
    split->addWidget(tree_);
    split->addWidget(preview_);
    split->setSizes({440, 600});
    root->addWidget(split, 1);

    QHBoxLayout* bottom = new QHBoxLayout;
    filter_ = new QComboBox;
    filter_->addItem("All files");
    filter_->addItem("Changed files only");
    ignoreBtn_ = new QPushButton("Ignore selected");
    QPushButton* refreshBtn = new QPushButton("Refresh");
    bottom->addWidget(filter_);
    bottom->addStretch();
    bottom->addWidget(ignoreBtn_);
    bottom->addWidget(refreshBtn);
    root->addLayout(bottom);

    connect(newBtn, &QPushButton::clicked, this, [this] { chooseFolder(true); });
    connect(openBtn, &QPushButton::clicked, this, [this] { chooseFolder(false); });
    connect(closeBtn_, &QPushButton::clicked, this, [this] { run_("close"); });          // command: close
    connect(ignoreBtn_, &QPushButton::clicked, this, [this] { ignoreSelected(); });      // command: ignore
    connect(refreshBtn, &QPushButton::clicked, this, [this] { refresh(); });
    connect(filter_, &QComboBox::currentIndexChanged, this, [this] { refresh(); });
    connect(tree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current) { showPreview(current); });

    refresh();
}

void RepoPage::chooseFolder(bool createNew) {
    const QString dir = QFileDialog::getExistingDirectory(
        this, createNew ? tr("Folder for the new repository") : tr("Open a repository folder"));
    if (dir.isEmpty()) return;
    run_((createNew ? "init " : "open ") + quoted(dir));   // commands: init / open
}

void RepoPage::refresh() {
    tree_->clear();
    preview_->clear();
    const bool open = ctl_.isOpen();
    closeBtn_->setEnabled(open);
    ignoreBtn_->setEnabled(open);
    filter_->setEnabled(open);
    if (!open) {
        pathLabel_->setText("No repository open");
        return;
    }
    pathLabel_->setText(qs(ctl_.root()));

    const bool changedOnly = filter_->currentIndex() == 1;
    std::map<QString, QTreeWidgetItem*> folders;   // "src/utils/" -> its tree item

    for (const FileEntry& f : ctl_.files()) {
        const QString state = qs(f.state);
        if (changedOnly && state == "clean") continue;

        const QStringList parts = qs(f.path).split(QLatin1Char('/'));
        QTreeWidgetItem* parent = nullptr;
        QString key;
        for (int i = 0; i + 1 < parts.size(); ++i) {   // make the folder items
            key += parts[i] + "/";
            auto it = folders.find(key);
            if (it == folders.end()) {
                QTreeWidgetItem* folder = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
                folder->setText(0, parts[i]);
                folder->setData(0, Qt::UserRole, key);   // folder: the path with a trailing slash
                it = folders.insert(std::make_pair(key, folder)).first;
            }
            parent = it->second;
        }
        QTreeWidgetItem* leaf = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
        leaf->setText(0, parts.last());
        leaf->setText(1, state);
        leaf->setForeground(1, QBrush(colorForState(state)));
        leaf->setData(0, Qt::UserRole, qs(f.path));
        leaf->setData(0, Qt::UserRole + 1, true);        // marks a file (not a folder)
    }
    tree_->expandAll();
}

void RepoPage::showPreview(QTreeWidgetItem* item) {
    preview_->clear();
    if (!item || !item->data(0, Qt::UserRole + 1).toBool()) return;   // nothing or a folder

    const std::string path = item->data(0, Qt::UserRole).toString().toStdString();
    std::string content;
    const Result r = ctl_.fileContent("", path, content);              // same as: cat <file>
    if (!r.ok) {
        preview_->setPlainText(qs(r.message));
        return;
    }
    if (content.find('\0') != std::string::npos) {
        preview_->setPlainText("(binary file)");
        return;
    }
    const size_t limit = 200000;
    const bool cut = content.size() > limit;
    if (cut) content.resize(limit);
    QString text = QString::fromUtf8(content.data(), static_cast<int>(content.size()));
    if (cut) text += "\n\n... (the file is longer, showing the first 200 KB)";
    preview_->setPlainText(text);
}

void RepoPage::ignoreSelected() {
    QTreeWidgetItem* item = tree_->currentItem();
    if (!item) return;
    const QString pattern = item->data(0, Qt::UserRole).toString();   // file path, or "folder/"
    if (pattern.isEmpty()) return;
    run_("ignore " + quoted(pattern));
}