#pragma once
#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QTableWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>

#include "../include/MemoryManager.hpp"
#include "../include/LRUPolicy.hpp"
#include "../include/ClockPolicy.hpp"
#include "../include/ARCPolicy.hpp"

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
    void onPolicyChanged(int index);
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
    void refreshLog();
    void addLog(const QString& msg, const QString& color = "#00e5ff");

    void rebuildManager();

    // ── Core ──────────────────────────────────────────────────────────────────
    std::unique_ptr<MemoryManager> mm_;
    std::vector<QString>           eventLog_;
    static constexpr int           MAX_LOG = 100;

    // ── Widgets ───────────────────────────────────────────────────────────────
    QTimer*       refreshTimer_;

    // Stats
    QProgressBar* hitRateBar_;
    QProgressBar* occupancyBar_;
    QLabel*       hitsLbl_;
    QLabel*       missesLbl_;
    QLabel*       evictLbl_;
    QLabel*       dirtyWLbl_;
    QLabel*       hitRateLbl_;

    // ARC panel
    QGroupBox*    arcGroup_;
    QLabel*       arcT1Lbl_;
    QLabel*       arcT2Lbl_;
    QLabel*       arcB1Lbl_;
    QLabel*       arcB2Lbl_;
    QLabel*       arcPLbl_;

    // Clock panel
    QGroupBox*    clockGroup_;
    QLabel*       clockCyclesLbl_;
    QLabel*       clockRefsLbl_;
    QLabel*       clockHandLbl_;
    QLabel*       clockNextLbl_;
    QProgressBar* clockCycleBar_;

    // Frame table
    QTableWidget* frameTable_;

    // Controls
    QSpinBox*     pageIdSpin_;
    QComboBox*    policyCbo_;
    QSpinBox*     framesSpin_;
    QPushButton*  accessBtn_;
    QPushButton*  dirtyBtn_;
    QPushButton*  evictBtn_;
    QPushButton*  flushBtn_;
    QPushButton*  resetBtn_;

    // Log
    QTextEdit*    logEdit_;
};
