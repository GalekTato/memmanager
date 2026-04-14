#include "MainWindow.hpp"
#include <QApplication>
#include <QHeaderView>
#include <QFont>
#include <QIcon>
#include <QStatusBar>
#include <sstream>

// ─── Constructor ──────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Memory Manager — C++ Buffer Pool");
    setMinimumSize(1100, 720);
    resize(1280, 800);

    setupStyleSheet();
    setupUI();

    // Build initial manager (8 frames, ARC)
    rebuildManager();

    // Auto-refresh every 300ms
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::onRefreshTimer);
    refreshTimer_->start(300);

    addLog("Gestor iniciado — política: ARC, frames: 8", "#00e5ff");
}

// ─── Style ────────────────────────────────────────────────────────────────────
void MainWindow::setupStyleSheet() {
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #0d1117;
            color: #c9d1d9;
            font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace;
            font-size: 12px;
        }
        QGroupBox {
            border: 1px solid #30363d;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 8px;
            font-size: 11px;
            color: #8b949e;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            top: -1px;
            color: #58a6ff;
            font-size: 11px;
            font-weight: bold;
            letter-spacing: 1px;
        }
        QLabel { color: #c9d1d9; }
        QProgressBar {
            border: 1px solid #30363d;
            border-radius: 4px;
            background: #161b22;
            height: 16px;
            text-align: center;
            color: #c9d1d9;
            font-size: 11px;
        }
        QProgressBar::chunk {
            border-radius: 3px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #1f6feb, stop:1 #58a6ff);
        }
        QTableWidget {
            background-color: #0d1117;
            gridline-color: #21262d;
            border: 1px solid #30363d;
            border-radius: 6px;
            selection-background-color: #1f6feb33;
        }
        QTableWidget::item { padding: 4px 8px; border: none; }
        QTableWidget::item:selected { background: #1f6feb33; color: #58a6ff; }
        QHeaderView::section {
            background-color: #161b22;
            color: #8b949e;
            border: none;
            border-bottom: 1px solid #30363d;
            padding: 6px 8px;
            font-size: 11px;
            font-weight: bold;
            letter-spacing: 1px;
        }
        QTextEdit {
            background-color: #0d1117;
            border: 1px solid #30363d;
            border-radius: 6px;
            color: #8b949e;
            font-size: 11px;
            padding: 4px;
        }
        QSpinBox, QComboBox, QLineEdit {
            background-color: #161b22;
            border: 1px solid #30363d;
            border-radius: 6px;
            color: #c9d1d9;
            padding: 5px 10px;
            font-size: 12px;
            min-height: 28px;
        }
        QSpinBox:focus, QComboBox:focus {
            border-color: #58a6ff;
        }
        QComboBox::drop-down { border: none; width: 20px; }
        QComboBox QAbstractItemView {
            background: #161b22;
            border: 1px solid #30363d;
            selection-background-color: #1f6feb;
            color: #c9d1d9;
        }
        QPushButton {
            background-color: #21262d;
            border: 1px solid #30363d;
            border-radius: 6px;
            color: #c9d1d9;
            padding: 6px 16px;
            font-size: 12px;
            min-height: 30px;
        }
        QPushButton:hover  { background-color: #30363d; border-color: #58a6ff; color: #58a6ff; }
        QPushButton:pressed{ background-color: #1f6feb; border-color: #1f6feb; color: #ffffff; }
        QPushButton#accessBtn { background-color: #0d2a4a; border-color: #1f6feb; color: #58a6ff; }
        QPushButton#accessBtn:hover { background-color: #1f6feb; color: #ffffff; }
        QPushButton#dirtyBtn  { background-color: #2a1f00; border-color: #d29922; color: #d29922; }
        QPushButton#dirtyBtn:hover  { background-color: #d29922; color: #000; }
        QPushButton#evictBtn  { background-color: #2d0f0f; border-color: #da3633; color: #da3633; }
        QPushButton#evictBtn:hover  { background-color: #da3633; color: #fff; }
        QPushButton#flushBtn  { background-color: #0d2a1a; border-color: #2ea043; color: #2ea043; }
        QPushButton#flushBtn:hover  { background-color: #2ea043; color: #fff; }
        QPushButton#resetBtn  { background-color: #1a0d2a; border-color: #8957e5; color: #8957e5; }
        QPushButton#resetBtn:hover  { background-color: #8957e5; color: #fff; }
        QSplitter::handle { background: #21262d; width: 2px; height: 2px; }
        QScrollBar:vertical {
            background: #0d1117; width: 8px; border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #30363d; border-radius: 4px; min-height: 20px;
        }
        QStatusBar { background: #161b22; color: #8b949e; border-top: 1px solid #30363d; }
    )");
}

// ─── UI Layout ────────────────────────────────────────────────────────────────
void MainWindow::setupUI() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(12, 12, 12, 8);
    rootLayout->setSpacing(8);

    // ── Title bar ─────────────────────────────────────────────────────────────
    auto* titleLbl = new QLabel("◈  MEMORY MANAGER  ·  C++ Buffer Pool");
    titleLbl->setStyleSheet("color:#58a6ff; font-size:15px; font-weight:bold; letter-spacing:2px;");
    rootLayout->addWidget(titleLbl);

    // ── Main splitter: left | right ───────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    rootLayout->addWidget(splitter, 1);

    // ── LEFT column ───────────────────────────────────────────────────────────
    auto* leftWidget = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0,0,6,0);
    leftLayout->setSpacing(8);

    leftLayout->addWidget(makeStatsPanel());
    leftLayout->addWidget(makeClockPanel());
    leftLayout->addWidget(makeControlPanel());
    leftLayout->addStretch();
    splitter->addWidget(leftWidget);

    // ── RIGHT column ──────────────────────────────────────────────────────────
    auto* rightWidget = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(6,0,0,0);
    rightLayout->setSpacing(8);

    rightLayout->addWidget(makeFramePanel(), 3);
    rightLayout->addWidget(makeLogPanel(), 2);
    splitter->addWidget(rightWidget);

    splitter->setSizes({420, 680});

    statusBar()->showMessage("Listo");
}

// ─── Stats Panel ──────────────────────────────────────────────────────────────
QGroupBox* MainWindow::makeStatsPanel() {
    auto* grp = new QGroupBox("ESTADÍSTICAS");
    auto* lay = new QVBoxLayout(grp);
    lay->setSpacing(6);

    // Hit Rate
    auto* hrRow = new QHBoxLayout;
    auto* hrLbl = new QLabel("Hit Rate");
    hrLbl->setFixedWidth(80);
    hrLbl->setStyleSheet("color:#8b949e;");
    hitRateBar_ = new QProgressBar;
    hitRateBar_->setRange(0, 100);
    hitRateBar_->setValue(0);
    hitRateLbl_ = new QLabel("0.0%");
    hitRateLbl_->setFixedWidth(48);
    hitRateLbl_->setStyleSheet("color:#2ea043; font-weight:bold;");
    hrRow->addWidget(hrLbl);
    hrRow->addWidget(hitRateBar_, 1);
    hrRow->addWidget(hitRateLbl_);
    lay->addLayout(hrRow);

    // Occupancy
    auto* occRow = new QHBoxLayout;
    auto* occLbl = new QLabel("Ocupación");
    occLbl->setFixedWidth(80);
    occLbl->setStyleSheet("color:#8b949e;");
    occupancyBar_ = new QProgressBar;
    occupancyBar_->setRange(0, 100);
    occupancyBar_->setValue(0);
    occupancyBar_->setStyleSheet(
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #2ea043, stop:1 #56d364); }");
    auto* occValLbl = new QLabel("0/0");
    occValLbl->setFixedWidth(48);
    occValLbl->setStyleSheet("color:#56d364; font-weight:bold;");
    occRow->addWidget(occLbl);
    occRow->addWidget(occupancyBar_, 1);
    occRow->addWidget(occValLbl);
    lay->addLayout(occRow);

    // Store occupancy label for refresh
    connect(occupancyBar_, &QProgressBar::valueChanged, [=](int v){
        if (mm_) occValLbl->setText(QString("%1/%2").arg(mm_->used()).arg(mm_->capacity()));
        (void)v;
    });

    // Counters grid
    auto* grid = new QGridLayout;
    grid->setSpacing(8);
    auto mkCounter = [&](const QString& label, const QString& color) -> QLabel* {
        auto* lbl = new QLabel(label + ": 0");
        lbl->setStyleSheet(QString("color:%1; font-weight:bold; font-size:13px;").arg(color));
        return lbl;
    };
    hitsLbl_   = mkCounter("✓ Hits",   "#2ea043");
    missesLbl_ = mkCounter("✗ Miss",   "#da3633");
    evictLbl_  = mkCounter("⇡ Evict",  "#d29922");
    dirtyWLbl_ = mkCounter("✎ DirtyW", "#8957e5");
    grid->addWidget(hitsLbl_,   0, 0);
    grid->addWidget(missesLbl_, 0, 1);
    grid->addWidget(evictLbl_,  1, 0);
    grid->addWidget(dirtyWLbl_, 1, 1);
    lay->addLayout(grid);

    // ARC sub-panel
    arcGroup_ = new QGroupBox("ARC — Listas internas");
    arcGroup_->setStyleSheet("QGroupBox { border-color: #8957e5; } QGroupBox::title { color:#8957e5; }");
    auto* arcLay = new QGridLayout(arcGroup_);
    arcLay->setSpacing(6);
    auto mkArc = [&](const QString& label, const QString& color) -> QLabel* {
        auto* l = new QLabel(label + ": 0");
        l->setStyleSheet(QString("color:%1; font-weight:bold;").arg(color));
        return l;
    };
    arcT1Lbl_ = mkArc("T1 recientes", "#2ea043");
    arcT2Lbl_ = mkArc("T2 frecuentes","#58a6ff");
    arcB1Lbl_ = mkArc("B1 ghost",     "#8b949e");
    arcB2Lbl_ = mkArc("B2 ghost",     "#8b949e");
    arcPLbl_  = mkArc("p (target)",   "#d29922");
    arcLay->addWidget(arcT1Lbl_, 0, 0);
    arcLay->addWidget(arcT2Lbl_, 0, 1);
    arcLay->addWidget(arcB1Lbl_, 1, 0);
    arcLay->addWidget(arcB2Lbl_, 1, 1);
    arcLay->addWidget(arcPLbl_,  2, 0);
    lay->addWidget(arcGroup_);

    return grp;
}

// ─── Clock Panel ──────────────────────────────────────────────────────────────
QGroupBox* MainWindow::makeClockPanel() {
    clockGroup_ = new QGroupBox("RELOJ — Ciclo cada 3 referencias");
    clockGroup_->setStyleSheet(
        "QGroupBox { border-color: #d29922; }"
        "QGroupBox::title { color:#d29922; }");
    auto* lay = new QGridLayout(clockGroup_);
    lay->setSpacing(6);

    auto mkClk = [&](const QString& label, const QString& color) -> QLabel* {
        auto* l = new QLabel(label + ": —");
        l->setStyleSheet(QString("color:%1; font-weight:bold;").arg(color));
        return l;
    };
    clockCyclesLbl_ = mkClk("Ciclos ejecutados", "#d29922");
    clockRefsLbl_   = mkClk("Referencias totales","#c9d1d9");
    clockHandLbl_   = mkClk("Puntero apunta a",  "#58a6ff");
    clockNextLbl_   = new QLabel("Próximo ciclo en: —");
    clockNextLbl_->setStyleSheet("color:#8b949e;");

    // Progress bar for refs toward next cycle
    clockCycleBar_ = new QProgressBar;
    clockCycleBar_->setRange(0, 3);
    clockCycleBar_->setValue(0);
    clockCycleBar_->setFormat("ref %v / 3");
    clockCycleBar_->setStyleSheet(
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #d29922, stop:1 #f0c040); }");

    lay->addWidget(clockCyclesLbl_, 0, 0);
    lay->addWidget(clockRefsLbl_,   0, 1);
    lay->addWidget(clockHandLbl_,   1, 0);
    lay->addWidget(clockNextLbl_,   1, 1);
    lay->addWidget(new QLabel("Progreso al próximo ciclo:"), 2, 0);
    lay->addWidget(clockCycleBar_,  2, 1);

    return clockGroup_;
}

// ─── Frame Panel ──────────────────────────────────────────────────────────────
QGroupBox* MainWindow::makeFramePanel() {
    auto* grp = new QGroupBox("FRAME POOL");
    auto* lay = new QVBoxLayout(grp);

    frameTable_ = new QTableWidget(0, 6);
    frameTable_->setHorizontalHeaderLabels({"Frame","Página","Dirty","Bit R","Freq","Estado"});
    frameTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    frameTable_->verticalHeader()->setVisible(false);
    frameTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    frameTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    frameTable_->setAlternatingRowColors(false);
    frameTable_->setShowGrid(true);
    frameTable_->verticalHeader()->setDefaultSectionSize(28);

    lay->addWidget(frameTable_);
    return grp;
}

// ─── Control Panel ────────────────────────────────────────────────────────────
QGroupBox* MainWindow::makeControlPanel() {
    auto* grp = new QGroupBox("CONTROLES");
    auto* lay = new QVBoxLayout(grp);
    lay->setSpacing(10);

    // Page ID + policy + frames row
    auto* cfgRow = new QHBoxLayout;
    auto* pidLbl = new QLabel("Página ID:");
    pidLbl->setStyleSheet("color:#8b949e;");
    pageIdSpin_ = new QSpinBox;
    pageIdSpin_->setRange(0, 9999);
    pageIdSpin_->setValue(1);
    pageIdSpin_->setPrefix("P");

    auto* polLbl = new QLabel("Política:");
    polLbl->setStyleSheet("color:#8b949e;");
    policyCbo_ = new QComboBox;
    policyCbo_->addItems({"ARC", "LRU", "Clock"});

    auto* frmLbl = new QLabel("Frames:");
    frmLbl->setStyleSheet("color:#8b949e;");
    framesSpin_ = new QSpinBox;
    framesSpin_->setRange(2, 64);
    framesSpin_->setValue(8);

    cfgRow->addWidget(pidLbl);
    cfgRow->addWidget(pageIdSpin_);
    cfgRow->addSpacing(12);
    cfgRow->addWidget(polLbl);
    cfgRow->addWidget(policyCbo_);
    cfgRow->addSpacing(12);
    cfgRow->addWidget(frmLbl);
    cfgRow->addWidget(framesSpin_);
    lay->addLayout(cfgRow);

    // Action buttons
    auto* btnRow = new QHBoxLayout;
    accessBtn_ = new QPushButton("▶  Acceder");  accessBtn_->setObjectName("accessBtn");
    dirtyBtn_  = new QPushButton("✎  Dirty");    dirtyBtn_->setObjectName("dirtyBtn");
    evictBtn_  = new QPushButton("✕  Evictar");  evictBtn_->setObjectName("evictBtn");
    flushBtn_  = new QPushButton("⇡  Flush");    flushBtn_->setObjectName("flushBtn");
    resetBtn_  = new QPushButton("↺  Reset");    resetBtn_->setObjectName("resetBtn");
    btnRow->addWidget(accessBtn_);
    btnRow->addWidget(dirtyBtn_);
    btnRow->addWidget(evictBtn_);
    btnRow->addWidget(flushBtn_);
    btnRow->addWidget(resetBtn_);
    lay->addLayout(btnRow);

    // Connections
    connect(accessBtn_, &QPushButton::clicked, this, &MainWindow::onAccess);
    connect(dirtyBtn_,  &QPushButton::clicked, this, &MainWindow::onMarkDirty);
    connect(evictBtn_,  &QPushButton::clicked, this, &MainWindow::onEvict);
    connect(flushBtn_,  &QPushButton::clicked, this, &MainWindow::onFlush);
    connect(resetBtn_,  &QPushButton::clicked, this, &MainWindow::onResetStats);
    connect(policyCbo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPolicyChanged);
    connect(framesSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFramesChanged);

    return grp;
}

// ─── Log Panel ────────────────────────────────────────────────────────────────
QGroupBox* MainWindow::makeLogPanel() {
    auto* grp = new QGroupBox("REGISTRO DE EVENTOS");
    auto* lay = new QVBoxLayout(grp);
    logEdit_ = new QTextEdit;
    logEdit_->setReadOnly(true);
    logEdit_->document()->setMaximumBlockCount(200);
    lay->addWidget(logEdit_);
    return grp;
}

// ─── rebuildManager ───────────────────────────────────────────────────────────
void MainWindow::rebuildManager() {
    int frames = framesSpin_->value();
    int polIdx = policyCbo_->currentIndex();

    std::unique_ptr<ReplacementPolicy> pol;
    if      (polIdx == 0) pol = std::make_unique<ARCPolicy>(frames);
    else if (polIdx == 1) pol = std::make_unique<LRUPolicy>();
    else                  pol = std::make_unique<ClockPolicy>();

    mm_ = std::make_unique<MemoryManager>(frames, std::move(pol));
    mm_->setPageInHook ([](PageID pid){ return Page(pid); });
    mm_->setPageOutHook([](const Page&){});

    // Update Clock/ARC panel visibility
    bool isClock = (policyCbo_->currentText() == "Clock");
    bool isARC   = (policyCbo_->currentText() == "ARC");
    clockGroup_->setVisible(isClock);
    arcGroup_->setVisible(isARC);

    refreshStats();
    refreshFrames();
    statusBar()->showMessage(QString("Gestor reconstruido — %1 frames, política %2")
        .arg(frames).arg(policyCbo_->currentText()));
}

// ─── Slots ────────────────────────────────────────────────────────────────────
void MainWindow::onAccess() {
    if (!mm_) return;
    PageID pid = static_cast<PageID>(pageIdSpin_->value());
    uint64_t hitsBefore = mm_->stats().hits.load();
    auto* p = mm_->access(pid);
    if (p) {
        bool wasHit = mm_->stats().hits.load() > hitsBefore;
        addLog(QString("ACCESS  P%1  →  %2")
            .arg(pid).arg(wasHit ? "HIT" : "MISS"),
            wasHit ? "#2ea043" : "#da3633");
    } else {
        addLog(QString("ACCESS  P%1  →  error").arg(pid), "#da3633");
    }
    refreshAll();
}


void MainWindow::onMarkDirty() {
    if (!mm_) return;
    PageID pid = static_cast<PageID>(pageIdSpin_->value());
    bool ok = mm_->markDirty(pid);
    addLog(ok ? QString("DIRTY   P%1  →  marcada").arg(pid)
              : QString("DIRTY   P%1  →  no está en pool").arg(pid),
           ok ? "#d29922" : "#da3633");
    refreshFrames();
}

void MainWindow::onEvict() {
    if (!mm_) return;
    PageID pid = static_cast<PageID>(pageIdSpin_->value());
    bool ok = mm_->evict(pid);
    addLog(ok ? QString("EVICT   P%1  →  eviccionada").arg(pid)
              : QString("EVICT   P%1  →  no está en pool").arg(pid),
           ok ? "#da3633" : "#8b949e");
    refreshAll();
}

void MainWindow::onFlush() {
    if (!mm_) return;
    PageID pid = static_cast<PageID>(pageIdSpin_->value());
    bool ok = mm_->flush(pid);
    addLog(ok ? QString("FLUSH   P%1  →  escrita a disco").arg(pid)
              : QString("FLUSH   P%1  →  no está en pool").arg(pid),
           ok ? "#2ea043" : "#8b949e");
    refreshAll();
}

void MainWindow::onResetStats() {
    if (!mm_) return;
    mm_->resetStats();
    addLog("RESET   →  estadísticas reiniciadas", "#8957e5");
    refreshStats();
}

void MainWindow::onPolicyChanged(int) {
    if (!mm_) return;
    QString pol = policyCbo_->currentText();
    int frames = framesSpin_->value();
    std::unique_ptr<ReplacementPolicy> p;
    if      (pol == "ARC")   p = std::make_unique<ARCPolicy>(frames);
    else if (pol == "LRU")   p = std::make_unique<LRUPolicy>();
    else                     p = std::make_unique<ClockPolicy>();
    mm_->setPolicy(std::move(p));

    clockGroup_->setVisible(pol == "Clock");
    arcGroup_->setVisible(pol == "ARC");

    addLog(QString("POLICY  →  cambiada a %1").arg(pol), "#8957e5");
    refreshAll();
}

void MainWindow::onFramesChanged(int) {
    rebuildManager();
    addLog(QString("CONFIG  →  frames: %1, política: %2")
        .arg(framesSpin_->value()).arg(policyCbo_->currentText()), "#58a6ff");
}

void MainWindow::onRefreshTimer() {
    refreshStats();
    refreshClock();
}

// ─── Refresh helpers ──────────────────────────────────────────────────────────
void MainWindow::refreshAll() {
    refreshStats();
    refreshFrames();
    refreshClock();
}

void MainWindow::refreshStats() {
    if (!mm_) return;
    auto& s  = mm_->stats();
    double hr  = s.hitRate();
    double occ = mm_->capacity() ? (double)mm_->used() / mm_->capacity() * 100 : 0;

    hitRateBar_->setValue(static_cast<int>(hr));
    hitRateLbl_->setText(QString("%1%").arg(hr, 0, 'f', 1));
    occupancyBar_->setValue(static_cast<int>(occ));

    hitsLbl_  ->setText(QString("✓ Hits: %1")  .arg(s.hits.load()));
    missesLbl_->setText(QString("✗ Miss: %1")  .arg(s.misses.load()));
    evictLbl_ ->setText(QString("⇡ Evict: %1") .arg(s.evictions.load()));
    dirtyWLbl_->setText(QString("✎ DirtyW: %1").arg(s.dirtyW.load()));

    if (mm_->policyName() == "ARC") {
        auto* arc = dynamic_cast<ARCPolicy*>(mm_->policy());
        if (arc) {
            arcT1Lbl_->setText(QString("T1 recientes: %1") .arg(arc->t1Size()));
            arcT2Lbl_->setText(QString("T2 frecuentes: %1").arg(arc->t2Size()));
            arcB1Lbl_->setText(QString("B1 ghost: %1")     .arg(arc->b1Size()));
            arcB2Lbl_->setText(QString("B2 ghost: %1")     .arg(arc->b2Size()));
            arcPLbl_ ->setText(QString("p (target): %1")   .arg(arc->pTarget()));
        }
    }
}

void MainWindow::refreshFrames() {
    if (!mm_) return;
    auto snap = mm_->snapshot();
    auto ord  = mm_->policyOrder();

    std::unordered_map<PageID, size_t> rank;
    for (size_t i = 0; i < ord.size(); ++i) rank[ord[i]] = i;

    ClockPolicy* clk = nullptr;
    if (mm_->policyName() == "Clock")
        clk = dynamic_cast<ClockPolicy*>(mm_->policy());

    frameTable_->setRowCount(static_cast<int>(mm_->capacity()));

    // Fill empty rows first
    for (int r = 0; r < (int)mm_->capacity(); ++r) {
        for (int c = 0; c < 6; ++c) {
            auto* item = new QTableWidgetItem("—");
            item->setForeground(QColor("#30363d"));
            item->setTextAlignment(Qt::AlignCenter);
            frameTable_->setItem(r, c, item);
        }
        auto* fItem = new QTableWidgetItem(QString("F%1").arg(r));
        fItem->setForeground(QColor("#30363d"));
        fItem->setTextAlignment(Qt::AlignCenter);
        frameTable_->setItem(r, 0, fItem);
    }

    for (auto& [fid, pg] : snap) {
        int row = static_cast<int>(fid);
        bool isHand = false;
        bool rBit   = pg.referenced;

        if (clk) {
            rBit = clk->getRbit(pg.id);
            const auto& ring = clk->ring();
            if (!ring.empty())
                isHand = (ring[clk->hand() % ring.size()].pid == pg.id);
        }

        size_t rnk = rank.count(pg.id) ? rank[pg.id] : 99;

        // Frame
        auto mkItem = [](const QString& txt, const QColor& col, bool bold=false) {
            auto* it = new QTableWidgetItem(txt);
            it->setForeground(col);
            it->setTextAlignment(Qt::AlignCenter);
            if (bold) { QFont f = it->font(); f.setBold(true); it->setFont(f); }
            return it;
        };

        QString frameLabel = isHand
            ? QString("► F%1").arg(fid)
            : QString("  F%1").arg(fid);
        QColor frameColor  = isHand ? QColor("#d29922") : QColor("#8b949e");
        frameTable_->setItem(row, 0, mkItem(frameLabel, frameColor, isHand));

        // Page
        QColor pgColor = pg.dirty ? QColor("#d29922") : QColor("#58a6ff");
        frameTable_->setItem(row, 1, mkItem(QString("P%1").arg(pg.id), pgColor, true));

        // Dirty
        if (pg.dirty)
            frameTable_->setItem(row, 2, mkItem("✎ Sí", QColor("#d29922"), true));
        else
            frameTable_->setItem(row, 2, mkItem("No",   QColor("#30363d")));

        // Bit R
        if (rBit)
            frameTable_->setItem(row, 3, mkItem("R=1  ●", QColor("#2ea043"), true));
        else
            frameTable_->setItem(row, 3, mkItem("R=0  ○", QColor("#da3633")));

        // Freq
        frameTable_->setItem(row, 4, mkItem(QString::number(pg.frequency), QColor("#8b949e")));

        // Estado / Rango
        QString estado;
        if (mm_->policyName() == "Clock") {
            estado = isHand ? "◄ puntero" : (rBit ? "segunda oportunidad" : "víctima candidata");
        } else if (mm_->policyName() == "ARC") {
            estado = QString("rank %1").arg(rnk);
        } else {
            estado = QString("rank %1").arg(rnk);
        }
        QColor estadoColor = isHand ? QColor("#d29922")
                           : (!rBit && clk) ? QColor("#da3633")
                           : QColor("#8b949e");
        frameTable_->setItem(row, 5, mkItem(estado, estadoColor));

        // Row background highlight for hand
        if (isHand) {
            for (int c = 0; c < 6; ++c) {
                if (frameTable_->item(row, c))
                    frameTable_->item(row, c)->setBackground(QColor("#2a1f00"));
            }
        }
    }
}

void MainWindow::refreshClock() {
    if (!mm_ || mm_->policyName() != "Clock") return;
    auto* clk = dynamic_cast<ClockPolicy*>(mm_->policy());
    if (!clk) return;

    clockCyclesLbl_->setText(QString("Ciclos ejecutados: %1").arg(clk->cycles()));
    clockRefsLbl_  ->setText(QString("Referencias totales: %1").arg(clk->refs()));

    const auto& ring = clk->ring();
    if (!ring.empty()) {
        PageID handPid = ring[clk->hand() % ring.size()].pid;
        clockHandLbl_->setText(QString("Puntero apunta a: P%1 (R=%2)")
            .arg(handPid).arg(clk->getRbit(handPid) ? "1" : "0"));
    } else {
        clockHandLbl_->setText("Puntero apunta a: —");
    }

    size_t refsNext = clk->refsUntilNextCycle();
    size_t refsUsed = 3 - refsNext;
    clockCycleBar_->setValue(static_cast<int>(refsUsed));
    clockNextLbl_->setText(refsNext == 0
        ? "¡Ciclo en progreso!"
        : QString("Próximo ciclo en %1 referencia%2")
            .arg(refsNext).arg(refsNext == 1 ? "" : "s"));
}

void MainWindow::refreshLog() {}

void MainWindow::addLog(const QString& msg, const QString& color) {
    logEdit_->append(
        QString("<span style='color:%1;font-family:monospace;'>%2</span>")
            .arg(color, msg.toHtmlEscaped()));
    QTextCursor cursor = logEdit_->textCursor();
    cursor.movePosition(QTextCursor::End);
    logEdit_->setTextCursor(cursor);
}
