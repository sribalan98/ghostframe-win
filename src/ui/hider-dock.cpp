#include "hider-dock.hpp"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QListWidget>
#include <QDialog>
#include <QDialogButtonBox>

HiderDockWidget::HiderDockWidget(QWidget* parent)
    : QDockWidget(tr("Window Hider (Capture Cloaker)"), parent),
      searchInput(nullptr),
      refreshButton(nullptr),
      rulesButton(nullptr),
      windowTable(nullptr),
      refreshTimer(nullptr) {

    SetupUi();

    // Auto-refresh timer every 5 seconds
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &HiderDockWidget::RefreshWindows);
    refreshTimer->start(5000);

    // Initial load
    RefreshWindows();
}

HiderDockWidget::~HiderDockWidget() {
    if (refreshTimer) {
        refreshTimer->stop();
    }
}

void HiderDockWidget::SetupUi() {
    QWidget* container = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // Controls Header (Search + Refresh + Rules)
    QHBoxLayout* headerLayout = new QHBoxLayout();

    searchInput = new QLineEdit(container);
    searchInput->setPlaceholderText(tr("Search process or window..."));
    searchInput->setClearButtonEnabled(true);
    connect(searchInput, &QLineEdit::textChanged, this, &HiderDockWidget::FilterWindows);
    headerLayout->addWidget(searchInput, 1);

    refreshButton = new QPushButton(tr("Refresh"), container);
    connect(refreshButton, &QPushButton::clicked, this, &HiderDockWidget::RefreshWindows);
    headerLayout->addWidget(refreshButton);

    rulesButton = new QPushButton(tr("Auto-Rules"), container);
    connect(rulesButton, &QPushButton::clicked, this, &HiderDockWidget::OpenAutoRulesDialog);
    headerLayout->addWidget(rulesButton);

    mainLayout->addLayout(headerLayout);

    // Table Widget
    windowTable = new QTableWidget(container);
    windowTable->setColumnCount(4);
    windowTable->setHorizontalHeaderLabels({
        tr("Status"),
        tr("Process"),
        tr("Window Title"),
        tr("Action")
    });

    windowTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    windowTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    windowTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    windowTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    windowTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    windowTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    windowTable->verticalHeader()->setVisible(false);

    connect(windowTable, &QTableWidget::cellClicked, this, &HiderDockWidget::OnCellClicked);

    mainLayout->addWidget(windowTable, 1);

    setWidget(container);
}

void HiderDockWidget::RefreshWindows() {
    currentWindows = winMgr.GetActiveWindows();

    // Auto-hide processes matching rules
    for (auto& win : currentWindows) {
        if (!win.isExcludedFromCapture && configMgr.ShouldAutoExclude(win.processName)) {
            if (affinityCtrl.SetWindowExcluded(win.hwnd, true)) {
                win.isExcludedFromCapture = true;
            }
        }
    }

    FilterWindows(searchInput ? searchInput->text() : QString());
}

void HiderDockWidget::FilterWindows(const QString& query) {
    QString q = query.trimmed().toLower();

    windowTable->setRowCount(0);

    for (const auto& win : currentWindows) {
        QString procName = QString::fromStdWString(win.processName);
        QString title = QString::fromStdWString(win.windowTitle);

        if (!q.isEmpty() && !procName.toLower().contains(q) && !title.toLower().contains(q)) {
            continue;
        }

        int row = windowTable->rowCount();
        windowTable->insertRow(row);

        // Column 0: Status tag
        QTableWidgetItem* statusItem = new QTableWidgetItem(
            win.isExcludedFromCapture ? tr("HIDDEN") : tr("CAPTURED")
        );
        statusItem->setTextAlignment(Qt::AlignCenter);
        if (win.isExcludedFromCapture) {
            statusItem->setForeground(QBrush(QColor(255, 85, 85))); // Red/orange for hidden
        } else {
            statusItem->setForeground(QBrush(QColor(85, 255, 127))); // Green for captured
        }
        windowTable->setItem(row, 0, statusItem);

        // Column 1: Process Name
        QTableWidgetItem* procItem = new QTableWidgetItem(procName);
        windowTable->setItem(row, 1, procItem);

        // Column 2: Title
        QTableWidgetItem* titleItem = new QTableWidgetItem(title);
        windowTable->setItem(row, 2, titleItem);

        // Column 3: Action Button
        QPushButton* actionBtn = new QPushButton(
            win.isExcludedFromCapture ? tr("Unhide") : tr("Hide"),
            windowTable
        );

        HWND targetHwnd = win.hwnd;
        bool targetExcluded = !win.isExcludedFromCapture;

        connect(actionBtn, &QPushButton::clicked, this, [this, targetHwnd, targetExcluded]() {
            affinityCtrl.SetWindowExcluded(targetHwnd, targetExcluded);
            RefreshWindows();
        });

        windowTable->setCellWidget(row, 3, actionBtn);
    }
}

void HiderDockWidget::OnCellClicked(int row, int column) {
    // If user clicks status or row, toggle action button
    if (column < 3) {
        QWidget* w = windowTable->cellWidget(row, 3);
        QPushButton* btn = qobject_cast<QPushButton*>(w);
        if (btn) {
            btn->animateClick();
        }
    }
}

void HiderDockWidget::ToggleActiveWindow() {
    HWND fg = GetForegroundWindow();
    if (!fg || !WindowManager::IsCandidateWindow(fg)) return;

    bool isCurrentlyExcluded = WindowManager::IsExcluded(fg);
    affinityCtrl.SetWindowExcluded(fg, !isCurrentlyExcluded);
    RefreshWindows();
}

void HiderDockWidget::OpenAutoRulesDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Auto-Hide Rules"));
    dlg.resize(360, 300);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);

    QLabel* label = new QLabel(tr("Processes below are automatically hidden from capture when opened:"), &dlg);
    label->setWordWrap(true);
    layout->addWidget(label);

    QListWidget* listWidget = new QListWidget(&dlg);
    auto rules = configMgr.GetAutoExcludeList();
    for (const auto& r : rules) {
        listWidget->addItem(QString::fromStdWString(r));
    }
    layout->addWidget(listWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton(tr("Add Process..."), &dlg);
    QPushButton* removeBtn = new QPushButton(tr("Remove Selected"), &dlg);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, [&]() {
        bool ok = false;
        QString text = QInputDialog::getText(&dlg, tr("Add Rule"), tr("Process Name (e.g. discord.exe):"), QLineEdit::Normal, "", &ok);
        if (ok && !text.trimmed().isEmpty()) {
            configMgr.AddAutoExcludeProcess(text.trimmed().toStdWString());
            listWidget->addItem(text.trimmed().toLower());
            RefreshWindows();
        }
    });

    connect(removeBtn, &QPushButton::clicked, [&]() {
        auto items = listWidget->selectedItems();
        for (auto* item : items) {
            configMgr.RemoveAutoExcludeProcess(item->text().toStdWString());
            delete listWidget->takeItem(listWidget->row(item));
        }
        RefreshWindows();
    });

    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
    layout->addWidget(box);

    dlg.exec();
}
