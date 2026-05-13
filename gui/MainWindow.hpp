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
#include <QLineEdit>

#include "../include/MemoryManager.hpp"
#include "../include/HybridPolicy.hpp"
#include "../include/Dispatcher.hpp"

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
    void onAddReference();
    void onLoadReferenceString();
    void onClearReferences();
    void onStepReference();
    void onRunReferences();
    void onStep();

private:
    void setupUI();
    void setupStyleSheet();
    QGroupBox* makeStatsPanel();
    QGroupBox* makeControlPanel();
    QGroupBox* makeVMPanel();
    QGroupBox* makeProcessPanel();
    QGroupBox* makeReferencePanel();
    QGroupBox* makeLogPanel();

    void refreshAll();
    void refreshStats();
    void refreshVMGrid();
    void refreshProcessList();
    void refreshReferenceList();
    void addLog(const QString& msg, const QString& color = "#89b4fa");
    void rebuildManager();
    QColor getProcessColor(int pid);
    void showFinalReport();

    std::unique_ptr<MemoryManager> mm_;
    std::unique_ptr<Dispatcher> dispatcher_;
    QTimer* refreshTimer_;

    QProgressBar* occupancyBar_;
    QLabel* freePagesLbl_;
    QLabel* occPagesLbl_;
    QLabel* activeProcsLbl_;

    QTableWidget* vmGrid_;
    QTableWidget* procTable_;
    QTableWidget* localPageTable_;
    QTableWidget* referenceTable_;

    QLineEdit* refStringEdit_;
    QLineEdit* creationRefsEdit_;
    QPushButton* loadStringBtn_;

    QSpinBox* newPidSpin_;
    QSpinBox* reqPagesSpin_;
    QPushButton* createBtn_;

    QSpinBox* accessPidSpin_;
    QSpinBox* logicalPageSpin_;
    QPushButton* accessBtn_;
    QPushButton* addRefBtn_;

    QSpinBox* termPidSpin_;
    QPushButton* termBtn_;

    QSpinBox* capacitySpin_;
    QTextEdit* logEdit_;

    QSpinBox* ramSpin_;

    QPushButton* clearRefsBtn_;
    QPushButton* stepRefBtn_;
    QPushButton* runRefsBtn_;

    QSpinBox* quantumSpin_;
    QSpinBox* clockCycleSpin_;
    QPushButton* stepBtn_;
    QLabel* dispatcherStatusLbl_;
};