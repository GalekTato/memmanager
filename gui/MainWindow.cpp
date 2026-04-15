#include "MainWindow.hpp"
#include <QApplication>
#include <QHeaderView>
#include <QFont>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Administrador de memoria");
    setMinimumSize(1100, 720);
    resize(1280, 800);

    setupStyleSheet();
    setupUI();
    rebuildManager();

    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::onRefreshTimer);
    refreshTimer_->start(300);

    addLog("Gestor iniciado — frames: 8", "#00e5ff");
}

void MainWindow::setupStyleSheet() {

    setStyleSheet(R"(
        QMainWindow, QWidget { background-color: #11111b; color: #cdd6f4; font-family: monospace; font-size: 12px; }
        QGroupBox { border: 1px solid #313244; border-radius: 8px; margin-top: 12px; padding-top: 8px; color: #a6adc8; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; top: -1px; color: #89b4fa; font-weight: bold; }
        QProgressBar { border: 1px solid #313244; border-radius: 4px; background: #1e1e2e; height: 16px; text-align: center; color: #cdd6f4; }
        QProgressBar::chunk { border-radius: 3px; background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #89b4fa, stop:1 #b4befe); }
        QTableWidget { background-color: #11111b; gridline-color: #313244; border: 1px solid #313244; border-radius: 6px; }
        QHeaderView::section { background-color: #1e1e2e; color: #bac2de; border: none; border-bottom: 1px solid #313244; padding: 6px; }
        QTextEdit, QSpinBox { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; color: #cdd6f4; padding: 5px; }
        QPushButton { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; color: #cdd6f4; padding: 6px; }
        QPushButton:hover  { background-color: #313244; border-color: #89b4fa; color: #89b4fa; }
    )");
}

void MainWindow::setupUI() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);

    // Título más técnico y limpio
    auto* titleLbl = new QLabel("GESTOR DE MEMORIA");
    titleLbl->setStyleSheet("color:#89b4fa; font-size:15px; font-weight:bold; letter-spacing:2px;");
    rootLayout->addWidget(titleLbl);

    auto* splitter = new QSplitter(Qt::Horizontal);
    rootLayout->addWidget(splitter, 1);

    auto* leftWidget = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addWidget(makeStatsPanel());
    leftLayout->addWidget(makeClockPanel());
    leftLayout->addWidget(makeControlPanel());
    leftLayout->addStretch();
    splitter->addWidget(leftWidget);

    auto* rightWidget = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addWidget(makeFramePanel(), 3);
    rightLayout->addWidget(makeLogPanel(), 2);
    splitter->addWidget(rightWidget);
    splitter->setSizes({420, 680});
}

QGroupBox* MainWindow::makeStatsPanel() {
    auto* grp = new QGroupBox("ESTADÍSTICAS GLOBALES");
    auto* lay = new QVBoxLayout(grp);

    auto* hrRow = new QHBoxLayout;
    hitRateBar_ = new QProgressBar; hitRateBar_->setRange(0, 100);
    hitRateLbl_ = new QLabel("0.0%"); hitRateLbl_->setStyleSheet("color:#a6e3a1; font-weight:bold;");
    hrRow->addWidget(new QLabel("Hit Rate")); hrRow->addWidget(hitRateBar_, 1); hrRow->addWidget(hitRateLbl_);
    lay->addLayout(hrRow);

    auto* occRow = new QHBoxLayout;
    occupancyBar_ = new QProgressBar; occupancyBar_->setRange(0, 100);
    occupancyBar_->setStyleSheet("QProgressBar::chunk { background: #a6e3a1; }");
    occRow->addWidget(new QLabel("Ocupación")); occRow->addWidget(occupancyBar_, 1);
    lay->addLayout(occRow);

    // Contadores limpios sin emojis
    auto* grid = new QGridLayout;
    hitsLbl_   = new QLabel("Hits: 0");      hitsLbl_->setStyleSheet("color:#a6e3a1; font-weight:bold;");
    missesLbl_ = new QLabel("Misses: 0");    missesLbl_->setStyleSheet("color:#f38ba8; font-weight:bold;");
    evictLbl_  = new QLabel("Evictions: 0"); evictLbl_->setStyleSheet("color:#f9e2af; font-weight:bold;");
    dirtyWLbl_ = new QLabel("DirtyW: 0");    dirtyWLbl_->setStyleSheet("color:#cba6f7; font-weight:bold;");
    grid->addWidget(hitsLbl_, 0, 0); grid->addWidget(missesLbl_, 0, 1);
    grid->addWidget(evictLbl_, 1, 0); grid->addWidget(dirtyWLbl_, 1, 1);
    lay->addLayout(grid);

    arcGroup_ = new QGroupBox("Listas Adaptativas");
    auto* arcLay = new QGridLayout(arcGroup_);
    arcT1Lbl_ = new QLabel("T1 Recientes: 0");  arcT1Lbl_->setStyleSheet("color:#a6e3a1; font-weight:bold;");
    arcT2Lbl_ = new QLabel("T2 Frecuentes: 0"); arcT2Lbl_->setStyleSheet("color:#89b4fa; font-weight:bold;");
    arcB1Lbl_ = new QLabel("B1 Ghost: 0");
    arcB2Lbl_ = new QLabel("B2 Ghost: 0");
    arcPLbl_  = new QLabel("Target p: 0");      arcPLbl_->setStyleSheet("color:#f9e2af; font-weight:bold;");
    arcLay->addWidget(arcT1Lbl_, 0, 0); arcLay->addWidget(arcT2Lbl_, 0, 1);
    arcLay->addWidget(arcB1Lbl_, 1, 0); arcLay->addWidget(arcB2Lbl_, 1, 1);
    arcLay->addWidget(arcPLbl_, 2, 0);
    lay->addWidget(arcGroup_);

    return grp;
}

QGroupBox* MainWindow::makeClockPanel() {
    clockGroup_ = new QGroupBox("Reloj Maestro");
    clockGroup_->setStyleSheet("QGroupBox { border-color: #f9e2af; } QGroupBox::title { color:#f9e2af; }");
    auto* lay = new QGridLayout(clockGroup_);
    clockCyclesLbl_ = new QLabel("Ciclos ejecutados: 0"); clockCyclesLbl_->setStyleSheet("color:#f9e2af; font-weight:bold;");
    clockRefsLbl_   = new QLabel("Referencias totales: 0");
    clockNextLbl_   = new QLabel("Próximo ciclo en: —");
    clockCycleBar_  = new QProgressBar; clockCycleBar_->setRange(0, 3);
    clockCycleBar_->setStyleSheet("QProgressBar::chunk { background: #f9e2af; }");
    lay->addWidget(clockCyclesLbl_, 0, 0); lay->addWidget(clockRefsLbl_, 0, 1);
    lay->addWidget(clockNextLbl_, 1, 0); lay->addWidget(clockCycleBar_, 1, 1);
    return clockGroup_;
}

QGroupBox* MainWindow::makeFramePanel() {
    auto* grp = new QGroupBox("MEMORIA RAM (Frames Físicos)");
    auto* lay = new QVBoxLayout(grp);
    frameTable_ = new QTableWidget(0, 5);
    frameTable_->setHorizontalHeaderLabels({"Frame", "Página", "Estado Dirty", "Bit R", "Frecuencia"});
    frameTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    frameTable_->verticalHeader()->setVisible(false);
    lay->addWidget(frameTable_);
    return grp;
}

QGroupBox* MainWindow::makeControlPanel() {
    auto* grp = new QGroupBox("CONTROLES");
    auto* lay = new QVBoxLayout(grp);
    auto* cfgRow = new QHBoxLayout;

    pageIdSpin_ = new QSpinBox; pageIdSpin_->setRange(0, 9999); pageIdSpin_->setValue(1); pageIdSpin_->setPrefix("P");

    // Aquí está el aumento del límite de frames a 256
    framesSpin_ = new QSpinBox; framesSpin_->setRange(2, 256); framesSpin_->setValue(8);

    cfgRow->addWidget(new QLabel("ID:")); cfgRow->addWidget(pageIdSpin_);
    cfgRow->addWidget(new QLabel("Frames:")); cfgRow->addWidget(framesSpin_);
    lay->addLayout(cfgRow);

    // Botones limpios, sin emojis
    auto* btnRow = new QHBoxLayout;
    accessBtn_ = new QPushButton("Acceder");
    dirtyBtn_  = new QPushButton("Marcar Dirty");
    evictBtn_  = new QPushButton("Evictar");
    flushBtn_  = new QPushButton("Flush");
    resetBtn_  = new QPushButton("Resetear Stats");

    btnRow->addWidget(accessBtn_); btnRow->addWidget(dirtyBtn_); btnRow->addWidget(evictBtn_);
    btnRow->addWidget(flushBtn_); btnRow->addWidget(resetBtn_);
    lay->addLayout(btnRow);

    connect(accessBtn_, &QPushButton::clicked, this, &MainWindow::onAccess);
    connect(dirtyBtn_, &QPushButton::clicked, this, &MainWindow::onMarkDirty);
    connect(evictBtn_, &QPushButton::clicked, this, &MainWindow::onEvict);
    connect(flushBtn_, &QPushButton::clicked, this, &MainWindow::onFlush);
    connect(resetBtn_, &QPushButton::clicked, this, &MainWindow::onResetStats);
    connect(framesSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onFramesChanged);

    return grp;
}

QGroupBox* MainWindow::makeLogPanel() {
    auto* grp = new QGroupBox("REGISTRO DE EVENTOS");
    auto* lay = new QVBoxLayout(grp);
    logEdit_ = new QTextEdit; logEdit_->setReadOnly(true);
    lay->addWidget(logEdit_);
    return grp;
}

void MainWindow::rebuildManager() {
    int frames = framesSpin_->value();
    auto pol = std::make_unique<HybridPolicy>(frames);
    mm_ = std::make_unique<MemoryManager>(frames, std::move(pol));
    mm_->setPageInHook([](PageID pid){ return Page(pid); });
    mm_->setPageOutHook([](const Page&){});
    refreshAll();
}

void MainWindow::onAccess() {
    if (!mm_) return;
    PageID pid = static_cast<PageID>(pageIdSpin_->value());
    uint64_t hitsBefore = mm_->stats().hits.load();
    if (mm_->access(pid)) {
        bool hit = mm_->stats().hits.load() > hitsBefore;
        addLog(QString("ACCESS P%1 -> %2").arg(pid).arg(hit ? "HIT" : "MISS"), hit ? "#a6e3a1" : "#f38ba8");
    }
    refreshAll();
}

void MainWindow::onMarkDirty() { if (mm_) { mm_->markDirty(pageIdSpin_->value()); refreshFrames(); } }
void MainWindow::onEvict() { if (mm_) { mm_->evict(pageIdSpin_->value()); refreshAll(); } }
void MainWindow::onFlush() { if (mm_) { mm_->flush(pageIdSpin_->value()); refreshAll(); } }
void MainWindow::onResetStats() { if (mm_) { mm_->resetStats(); refreshAll(); } }
void MainWindow::onFramesChanged(int) { rebuildManager(); }
void MainWindow::onRefreshTimer() { refreshStats(); refreshClock(); }

void MainWindow::refreshAll() { refreshStats(); refreshFrames(); refreshClock(); }

void MainWindow::refreshStats() {
    if (!mm_) return;
    auto& s = mm_->stats();
    hitRateBar_->setValue(static_cast<int>(s.hitRate()));
    hitRateLbl_->setText(QString("%1%").arg(s.hitRate(), 0, 'f', 1));
    occupancyBar_->setValue(static_cast<int>(mm_->capacity() ? (double)mm_->used() / mm_->capacity() * 100 : 0));

    hitsLbl_->setText(QString("Hits: %1").arg(s.hits.load()));
    missesLbl_->setText(QString("Misses: %1").arg(s.misses.load()));
    evictLbl_->setText(QString("Evictions: %1").arg(s.evictions.load()));
    dirtyWLbl_->setText(QString("DirtyW: %1").arg(s.dirtyW.load()));

    if (auto* hyb = dynamic_cast<HybridPolicy*>(mm_->policy())) {
        arcT1Lbl_->setText(QString("T1 Recientes: %1").arg(hyb->t1Size()));
        arcT2Lbl_->setText(QString("T2 Frecuentes: %1").arg(hyb->t2Size()));
        arcB1Lbl_->setText(QString("B1 Ghost: %1").arg(hyb->b1Size()));
        arcB2Lbl_->setText(QString("B2 Ghost: %1").arg(hyb->b2Size()));
        arcPLbl_->setText(QString("Target p: %1").arg(hyb->pTarget()));
    }
}

void MainWindow::refreshFrames() {
    if (!mm_) return;
    auto snap = mm_->snapshot();
    frameTable_->setRowCount(static_cast<int>(mm_->capacity()));
    auto* hyb = dynamic_cast<HybridPolicy*>(mm_->policy());

    for (int r = 0; r < (int)mm_->capacity(); ++r) {
        for (int c = 0; c < 5; ++c) frameTable_->setItem(r, c, new QTableWidgetItem("—"));
        frameTable_->setItem(r, 0, new QTableWidgetItem(QString("F%1").arg(r)));
    }

    for (auto& [fid, pg] : snap) {
        int row = static_cast<int>(fid);
        bool rBit = hyb ? hyb->getRbit(pg.id) : pg.referenced;

        frameTable_->setItem(row, 1, new QTableWidgetItem(QString("P%1").arg(pg.id)));

        // Textos formales en lugar de emojis para Dirty y Bit R
        frameTable_->setItem(row, 2, new QTableWidgetItem(pg.dirty ? "[ MODIFICADO ]" : "Limpio"));
        frameTable_->item(row, 2)->setForeground(pg.dirty ? QColor("#f9e2af") : QColor("#bac2de"));

        frameTable_->setItem(row, 3, new QTableWidgetItem(rBit ? "R=1" : "R=0"));
        frameTable_->item(row, 3)->setForeground(rBit ? QColor("#a6e3a1") : QColor("#f38ba8"));

        frameTable_->setItem(row, 4, new QTableWidgetItem(QString::number(pg.frequency)));
    }
}

void MainWindow::refreshClock() {
    if (!mm_) return;
    if (auto* hyb = dynamic_cast<HybridPolicy*>(mm_->policy())) {
        clockCyclesLbl_->setText(QString("Ciclos ejecutados: %1").arg(hyb->cycles()));
        clockRefsLbl_->setText(QString("Referencias totales: %1").arg(hyb->refs()));
        size_t refsNext = hyb->refsUntilNextCycle();
        clockCycleBar_->setValue(static_cast<int>(3 - refsNext));
        clockNextLbl_->setText(refsNext == 0 ? "¡Ciclo en progreso!" : QString("Próximo ciclo en %1 ref").arg(refsNext));
    }
}

void MainWindow::addLog(const QString& msg, const QString& color) {
    logEdit_->append(QString("<span style='color:%1;'>%2</span>").arg(color, msg.toHtmlEscaped()));
}
