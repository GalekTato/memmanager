#pragma once
#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QTableWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QHeaderView>

#include "../include/MemoryManager.hpp"
#include "../include/HybridPolicy.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onCreateProcess();
    void onAccessPage();
    void onTerminateProcess();
    void onCapacityChanged(int value);
    void onRefreshTimer();
    void onProcessSelectionChanged();

private:
    void setupUI();
    void setupStyleSheet();
    QGroupBox* makeStatsPanel();
    QGroupBox* makeControlPanel();
    QGroupBox* makeVMPanel();
    QGroupBox* makeProcessPanel();
    QGroupBox* makeLogPanel();

    void refreshAll();
    void refreshStats();
    void refreshVMGrid();
    void refreshProcessList();
    void addLog(const QString& msg, const QString& color = "#89b4fa");
    void rebuildManager();
    QColor getProcessColor(int pid);

    std::unique_ptr<MemoryManager> mm_;
    QTimer* refreshTimer_;

    QProgressBar* occupancyBar_;
    QLabel* freePagesLbl_;
    QLabel* occPagesLbl_;
    QLabel* activeProcsLbl_;

    QTableWidget* vmGrid_;
    QTableWidget* procTable_;
    QTableWidget* localPageTable_;

    QSpinBox* newPidSpin_;
    QSpinBox* reqPagesSpin_;
    QPushButton* createBtn_;

    QSpinBox* accessPidSpin_;
    QSpinBox* logicalPageSpin_;
    QPushButton* accessBtn_;

    QSpinBox* termPidSpin_;
    QPushButton* termBtn_;

    QSpinBox* capacitySpin_;
    QTextEdit* logEdit_;

    QSpinBox* ramSpin_;
};