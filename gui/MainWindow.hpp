#pragma once
#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QTableWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QFrame>

#include "../include/MemoryManager.hpp"
#include "../include/HybridPolicy.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onAccess();
    void onMarkDirty();
    void onEvict();
    void onFlush();
    void onResetStats();
    void onFramesChanged(int value);
    void onRefreshTimer();

private:
    void setupUI();
    void setupStyleSheet();
    QGroupBox* makeStatsPanel();
    QGroupBox* makeFramePanel();
    QGroupBox* makeClockPanel();
    QGroupBox* makeControlPanel();
    QGroupBox* makeLogPanel();

    void refreshAll();
    void refreshStats();
    void refreshFrames();
    void refreshClock();
    void addLog(const QString& msg, const QString& color = "#00e5ff");
    void rebuildManager();

    std::unique_ptr<MemoryManager> mm_;

    QTimer* refreshTimer_;
    QProgressBar* hitRateBar_;
    QProgressBar* occupancyBar_;
    QLabel* hitsLbl_;
    QLabel* missesLbl_;
    QLabel* evictLbl_;
    QLabel* dirtyWLbl_;
    QLabel* hitRateLbl_;

    QGroupBox* arcGroup_;
    QLabel* arcT1Lbl_;
    QLabel* arcT2Lbl_;
    QLabel* arcB1Lbl_;
    QLabel* arcB2Lbl_;
    QLabel* arcPLbl_;

    QGroupBox* clockGroup_;
    QLabel* clockCyclesLbl_;
    QLabel* clockRefsLbl_;
    QLabel* clockNextLbl_;
    QProgressBar* clockCycleBar_;

    QTableWidget* frameTable_;
    QSpinBox* pageIdSpin_;
    QSpinBox* framesSpin_;
    QPushButton* accessBtn_;
    QPushButton* dirtyBtn_;
    QPushButton* evictBtn_;
    QPushButton* flushBtn_;
    QPushButton* resetBtn_;
    QTextEdit* logEdit_;
};
