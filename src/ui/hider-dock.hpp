#pragma once

#include <QDockWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <memory>
#include "../core/window-manager.hpp"
#include "../core/affinity-controller.hpp"
#include "../core/config-manager.hpp"

class HiderDockWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit HiderDockWidget(QWidget* parent = nullptr);
    ~HiderDockWidget() override;

    void ToggleActiveWindow();

public slots:
    void RefreshWindows();
    void FilterWindows(const QString& query);
    void OnCellClicked(int row, int column);
    void OpenAutoRulesDialog();

private:
    void SetupUi();
    void UpdateRow(int row, const WindowInfo& win);

    QLineEdit* searchInput;
    QPushButton* refreshButton;
    QPushButton* rulesButton;
    QTableWidget* windowTable;
    QTimer* refreshTimer;

    WindowManager winMgr;
    AffinityController affinityCtrl;
    ConfigManager configMgr;
    std::vector<WindowInfo> currentWindows;
};
