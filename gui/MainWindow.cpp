#include "MainWindow.hpp"
#include <QApplication>
#include <cmath>
#include <QMessageBox>
#include <QDialog>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Administrador de memoria virtual");
    setMinimumSize(1200, 800);
    resize(1366, 768);

    setupStyleSheet();
    setupUI();
    rebuildManager();

    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::onRefreshTimer);
    refreshTimer_->start(500);

    addLog("Administrrador-Inicio", "#a6e3a1");
}

void MainWindow::setupStyleSheet() {
    setStyleSheet(R"(
        QMainWindow, QWidget { background-color: #11111b; color: #cdd6f4; font-family: monospace; font-size: 12px; }
        QGroupBox { border: 1px solid #313244; border-radius: 8px; margin-top: 15px; padding-top: 10px; color: #a6adc8; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; top: -1px; color: #89b4fa; font-weight: bold; }
        QProgressBar { border: 1px solid #313244; border-radius: 4px; background: #1e1e2e; height: 18px; text-align: center; color: #cdd6f4; font-weight: bold; }
        QProgressBar::chunk { border-radius: 3px; background: #89b4fa; }
        QTableWidget { background-color: #1e1e2e; gridline-color: #313244; border: 1px solid #313244; border-radius: 6px; outline: none; }
        QHeaderView::section { background-color: #181825; color: #bac2de; border: none; border-bottom: 1px solid #313244; padding: 6px; font-weight: bold; }
        QTextEdit, QSpinBox, QLineEdit { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; color: #cdd6f4; padding: 6px; }
        QPushButton { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; color: #cdd6f4; padding: 8px 12px; font-weight: bold; }
        QPushButton:hover { background-color: #313244; border-color: #89b4fa; color: #89b4fa; }
        
        /* Estilos específicos para legibilidad */
        #ReferenceTable { font-size: 13px; }
        #LocalPageTable { font-size: 12px; }
    )");
}

void MainWindow::setupUI() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);
    
    auto* titleLbl = new QLabel("ADMINISTRADOR DE MEMORIA VIRTUAL");
    titleLbl->setStyleSheet("color:#89b4fa; font-size:15px; font-weight:bold; letter-spacing:2px;");
    rootLayout->addWidget(titleLbl);

    auto* splitter = new QSplitter(Qt::Horizontal);
    rootLayout->addWidget(splitter, 1);

    auto* leftWidget = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addWidget(makeStatsPanel());
    leftLayout->addWidget(makeControlPanel());
    leftLayout->addWidget(makeLogPanel(), 1);
    splitter->addWidget(leftWidget);

    auto* midWidget = new QWidget;
    auto* midLayout = new QVBoxLayout(midWidget);
    midLayout->addWidget(makeVMPanel());
    splitter->addWidget(midWidget);

    auto* rightWidget = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addWidget(makeProcessPanel());
    rightLayout->addWidget(makeReferencePanel());
    splitter->addWidget(rightWidget);

    splitter->setSizes({350, 600, 350});
}

QGroupBox* MainWindow::makeStatsPanel() {
    auto* grp = new QGroupBox("ESTADO DEL SISTEMA");
    auto* lay = new QVBoxLayout(grp);

    auto* occRow = new QHBoxLayout;
    occupancyBar_ = new QProgressBar; occupancyBar_->setRange(0, 100);
    occRow->addWidget(new QLabel("Ocupación VM")); occRow->addWidget(occupancyBar_, 1);
    lay->addLayout(occRow);

    auto* grid = new QGridLayout;
    freePagesLbl_ = new QLabel("VM Libres: 0"); freePagesLbl_->setStyleSheet("color:#a6e3a1; font-weight:bold;");
    occPagesLbl_  = new QLabel("RAM Usada: 0"); occPagesLbl_->setStyleSheet("color:#f38ba8; font-weight:bold;");
    activeProcsLbl_ = new QLabel("Procesos Activos: 0"); activeProcsLbl_->setStyleSheet("color:#f9e2af; font-weight:bold;");
    
    grid->addWidget(freePagesLbl_, 0, 0); grid->addWidget(occPagesLbl_, 0, 1);
    grid->addWidget(activeProcsLbl_, 1, 0, 1, 2);
    lay->addLayout(grid);

    return grp;
}

QGroupBox* MainWindow::makeControlPanel() {
    auto* grp = new QGroupBox("PROCESOS");
    auto* lay = new QVBoxLayout(grp);

    auto* createLay = new QGridLayout;
    newPidSpin_ = new QSpinBox; newPidSpin_->setRange(1, 9999); newPidSpin_->setPrefix("PID: ");
    reqPagesSpin_ = new QSpinBox; reqPagesSpin_->setRange(1, 1024); reqPagesSpin_->setSuffix(" pgs");
    creationRefsEdit_ = new QLineEdit; creationRefsEdit_->setPlaceholderText("Refs: 0,1,2,3,0,1...");
    createBtn_ = new QPushButton("Crear y Asignar");
    createBtn_->setStyleSheet("color: #a6e3a1; border-color: #a6e3a1;");
    createLay->addWidget(new QLabel("Nuevo:"), 0, 0); createLay->addWidget(newPidSpin_, 0, 1);
    createLay->addWidget(reqPagesSpin_, 0, 2); 
    createLay->addWidget(new QLabel("Refs:"), 1, 0); createLay->addWidget(creationRefsEdit_, 1, 1, 1, 2);
    createLay->addWidget(createBtn_, 2, 0, 1, 3);
    lay->addLayout(createLay);
    lay->addSpacing(10);

    auto* accessLay = new QGridLayout;
    accessPidSpin_ = new QSpinBox; accessPidSpin_->setRange(1, 9999); accessPidSpin_->setPrefix("PID: ");
    logicalPageSpin_ = new QSpinBox; logicalPageSpin_->setRange(0, 1024); logicalPageSpin_->setPrefix("Pg Lógica: ");
    accessBtn_ = new QPushButton("Acceder");
    accessBtn_->setStyleSheet("color: #89b4fa; border-color: #89b4fa;");
    addRefBtn_ = new QPushButton("Encolar Ref");
    addRefBtn_->setStyleSheet("color: #cba6f7; border-color: #cba6f7;");
    
    accessLay->addWidget(new QLabel("Acceso:"), 0, 0); 
    accessLay->addWidget(accessPidSpin_, 0, 1);
    accessLay->addWidget(logicalPageSpin_, 0, 2); 
    accessLay->addWidget(accessBtn_, 1, 0, 1, 2);
    accessLay->addWidget(addRefBtn_, 1, 2, 1, 1);
    lay->addLayout(accessLay);
    lay->addSpacing(10);

    auto* termLay = new QHBoxLayout;
    termPidSpin_ = new QSpinBox; termPidSpin_->setRange(1, 9999); termPidSpin_->setPrefix("PID: ");
    termBtn_ = new QPushButton("Kill");
    termBtn_->setStyleSheet("color: #f38ba8; border-color: #f38ba8;");
    termLay->addWidget(termPidSpin_); termLay->addWidget(termBtn_);
    lay->addLayout(termLay);
    lay->addSpacing(15);

    auto* cfgLay = new QGridLayout;
    capacitySpin_ = new QSpinBox; capacitySpin_->setRange(16, 4096); capacitySpin_->setValue(256);
    ramSpin_ = new QSpinBox; ramSpin_->setRange(4, 4096); ramSpin_->setValue(16);
    quantumSpin_ = new QSpinBox; quantumSpin_->setRange(1, 50); quantumSpin_->setValue(5);
    clockCycleSpin_ = new QSpinBox; clockCycleSpin_->setRange(1, 20); clockCycleSpin_->setValue(3);
    
    cfgLay->addWidget(new QLabel("VM Total:"), 0, 0); cfgLay->addWidget(capacitySpin_, 0, 1);
    cfgLay->addWidget(new QLabel("MF Total:"), 0, 2); cfgLay->addWidget(ramSpin_, 0, 3);
    cfgLay->addWidget(new QLabel("Quantum:"), 1, 0); cfgLay->addWidget(quantumSpin_, 1, 1);
    cfgLay->addWidget(new QLabel("Ciclo Reloj:"), 1, 2); cfgLay->addWidget(clockCycleSpin_, 1, 3);
    lay->addLayout(cfgLay);
    lay->addSpacing(10);

    stepBtn_ = new QPushButton("▶ Paso");
    stepBtn_->setStyleSheet("background-color: #1e1e2e; color: #a6e3a1; border: 1px solid #a6e3a1; font-size: 14px;");
    lay->addWidget(stepBtn_);

    dispatcherStatusLbl_ = new QLabel("t=0 | PID=-1 | Quantum: 0/5");
    dispatcherStatusLbl_->setStyleSheet("color: #89b4fa; font-size: 11px;");
    lay->addWidget(dispatcherStatusLbl_);

    connect(createBtn_, &QPushButton::clicked, this, &MainWindow::onCreateProcess);
    connect(accessBtn_, &QPushButton::clicked, this, &MainWindow::onAccessPage);
    connect(addRefBtn_, &QPushButton::clicked, this, &MainWindow::onAddReference);
    connect(termBtn_, &QPushButton::clicked, this, &MainWindow::onTerminateProcess);
    connect(capacitySpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onCapacityChanged);
    connect(ramSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onCapacityChanged);
    connect(stepBtn_, &QPushButton::clicked, this, &MainWindow::onStep);

    return grp;
}

QGroupBox* MainWindow::makeReferencePanel() {
    auto* grp = new QGroupBox("LISTA DE REFERENCIAS (BATCH)");
    auto* lay = new QVBoxLayout(grp);

    auto* stringLay = new QHBoxLayout;
    refStringEdit_ = new QLineEdit;
    refStringEdit_->setPlaceholderText("Páginas: 0,1,2,0,3...");
    loadStringBtn_ = new QPushButton("Cargar string");
    loadStringBtn_->setStyleSheet("color: #cba6f7; border-color: #cba6f7;");
    stringLay->addWidget(refStringEdit_, 1);
    stringLay->addWidget(loadStringBtn_);
    lay->addLayout(stringLay);

    referenceTable_ = new QTableWidget(0, 2);
    referenceTable_->setObjectName("ReferenceTable");
    referenceTable_->setHorizontalHeaderLabels({"PID", "Pg Lógica"});
    referenceTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    referenceTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    referenceTable_->verticalHeader()->setDefaultSectionSize(30);
    lay->addWidget(referenceTable_, 1);

    auto* btnLay = new QHBoxLayout;
    stepRefBtn_ = new QPushButton("Siguiente");
    stepRefBtn_->setStyleSheet("color: #fab387; border-color: #fab387;");
    runRefsBtn_ = new QPushButton("Ejecutar Todas");
    runRefsBtn_->setStyleSheet("color: #a6e3a1; border-color: #a6e3a1;");
    clearRefsBtn_ = new QPushButton("Limpiar");
    clearRefsBtn_->setStyleSheet("color: #f38ba8; border-color: #f38ba8;");
    
    btnLay->addWidget(stepRefBtn_);
    btnLay->addWidget(runRefsBtn_);
    btnLay->addWidget(clearRefsBtn_);
    lay->addLayout(btnLay);

    connect(stepRefBtn_, &QPushButton::clicked, this, &MainWindow::onStepReference);
    connect(runRefsBtn_, &QPushButton::clicked, this, &MainWindow::onRunReferences);
    connect(clearRefsBtn_, &QPushButton::clicked, this, &MainWindow::onClearReferences);
    connect(loadStringBtn_, &QPushButton::clicked, this, &MainWindow::onLoadReferenceString);

    return grp;
}

QGroupBox* MainWindow::makeVMPanel() {
    auto* grp = new QGroupBox("Memoria Virtual Global");
    auto* lay = new QVBoxLayout(grp);
    vmGrid_ = new QTableWidget;
    vmGrid_->horizontalHeader()->setVisible(false);
    vmGrid_->verticalHeader()->setVisible(false);
    vmGrid_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vmGrid_->setSelectionMode(QAbstractItemView::NoSelection);
    lay->addWidget(vmGrid_);
    return grp;
}

QGroupBox* MainWindow::makeProcessPanel() {
    auto* grp = new QGroupBox("TABLAS DE PÁGINA DE PROCESOS");
    auto* lay = new QVBoxLayout(grp);
    
    procTable_ = new QTableWidget(0, 2);
    procTable_->setHorizontalHeaderLabels({"PID", "Páginas"});
    procTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    procTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    procTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    procTable_->setEditTriggers(QAbstractItemView::NoEditTriggers); 
    lay->addWidget(procTable_, 1);

    localPageTable_ = new QTableWidget(0, 4);
    localPageTable_->setObjectName("LocalPageTable");
    localPageTable_->setHorizontalHeaderLabels({"Pg Lógica", "Pg Virtual", "Ubicación", "Bits de control"});
    
    // Ajuste fino para evitar cortes:
    localPageTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); 
    localPageTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    localPageTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    localPageTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    localPageTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    localPageTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lay->addWidget(new QLabel("Traducción del proceso seleccionado:"));
    lay->addWidget(localPageTable_, 2);

    connect(procTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::onProcessSelectionChanged);

    return grp;
}

QGroupBox* MainWindow::makeLogPanel() {
    auto* grp = new QGroupBox("LOG");
    auto* lay = new QVBoxLayout(grp);
    logEdit_ = new QTextEdit; logEdit_->setReadOnly(true);
    lay->addWidget(logEdit_);
    return grp;
}

void MainWindow::rebuildManager() {
    int vmCap = capacitySpin_->value();
    int ramCap = ramSpin_->value();
    int quantum = quantumSpin_ ? quantumSpin_->value() : 5;
    int clockCycle = clockCycleSpin_ ? clockCycleSpin_->value() : 3;

    if (ramCap > vmCap) {
        ramCap = vmCap;
        ramSpin_->setValue(ramCap);
    }
    auto pol = std::make_unique<HybridPolicy>(ramCap);
    mm_ = std::make_unique<MemoryManager>(vmCap, ramCap, std::move(pol));
    dispatcher_ = std::make_unique<Dispatcher>(*mm_, quantum, clockCycle);

    addLog(QString("SISTEMA: Reiniciado con VM=%1, MF=%2, Q=%3").arg(vmCap).arg(ramCap).arg(quantum), "#89b4fa");
    refreshAll();
}

void MainWindow::onCreateProcess() {
    if (!mm_ || !dispatcher_) return;
    int pid = newPidSpin_->value();
    size_t req = reqPagesSpin_->value();
    QString refStr = creationRefsEdit_->text();
    
    std::vector<size_t> refs;
    QStringList parts = refStr.split(',', Qt::SkipEmptyParts);
    for (const auto& p : parts) {
        bool ok;
        size_t r = p.trimmed().toULong(&ok);
        if (ok) refs.push_back(r);
    }

    if (mm_->admitProcess(pid, req)) {
        Process p(pid, req, refs, dispatcher_->currentTime());
        const auto& mmProcs = mm_->getProcesses();
        auto it = mmProcs.find(pid);
        if (it != mmProcs.end()) {
            for (const auto& [loc, glob] : it->second.getPageTable()) {
                p.mapPage(loc, glob);
            }
        }

        dispatcher_->addProcess(std::move(p));
        addLog(QString("ADMISIÓN: PID %1 aceptado (%2 pags, %3 refs)").arg(pid).arg(req).arg(refs.size()), "#a6e3a1");
        newPidSpin_->setValue(pid + 1); 
        creationRefsEdit_->clear();
    } else {
        addLog(QString("RECHAZO: PID %1 no admitido (Sin memoria o duplicado)").arg(pid), "#f38ba8");
    }
    refreshAll();
}

void MainWindow::onAccessPage() {
    if (!mm_) return;
    int pid = accessPidSpin_->value();
    size_t logicalPg = logicalPageSpin_->value();
    
    int result = mm_->accessProcessPage(pid, logicalPg);
    if (result == 1) {
        addLog(QString("HIT: PID %1 pág. %2 (Estaba en RAM)").arg(pid).arg(logicalPg), "#89b4fa");
    } else if (result == 2) {
        addLog(QString("PAGE FAULT: PID %1 trajo la pág. %2 desde DISCO a RAM (Políticas Activas)").arg(pid).arg(logicalPg), "#f9e2af");
    } else {
        addLog(QString("SEGFAULT: PID %1 acceso fuera de límite").arg(pid), "#f38ba8");
    }
    
    refreshAll();
}

void MainWindow::onTerminateProcess() {
    if (!mm_) return;
    int pid = termPidSpin_->value();
    mm_->terminateProcess(pid);
    addLog(QString("TERMINADO: PID %1 evictado. RAM y Swap liberados.").arg(pid), "#f9e2af");
    refreshAll();
}

void MainWindow::onCapacityChanged(int) { rebuildManager(); }
void MainWindow::onRefreshTimer() { refreshStats(); }

void MainWindow::refreshAll() {
    refreshStats();
    refreshVMGrid();
    refreshProcessList();
    refreshReferenceList();
}

void MainWindow::onAddReference() {
    if (!mm_) return;
    int pid = accessPidSpin_->value();
    size_t logicalPg = logicalPageSpin_->value();
    mm_->addReference(pid, logicalPg);
    refreshReferenceList();
}

void MainWindow::onLoadReferenceString() {
    if (!mm_) return;
    int pid = accessPidSpin_->value();
    QString s = refStringEdit_->text();
    if (s.isEmpty()) return;
    
    mm_->addReferenceString(pid, s.toStdString());
    refreshReferenceList();
    addLog(QString("LISTA: String cargada para PID %1 (Con compresión)").arg(pid), "#cba6f7");
    refStringEdit_->clear();
}

void MainWindow::onClearReferences() {
    if (!mm_) return;
    mm_->clearReferences();
    refreshReferenceList();
    addLog("LISTA: Referencias limpiadas", "#f38ba8");
}

void MainWindow::onStepReference() {
    if (!mm_) return;
    size_t nextIdx = mm_->getNextReferenceIndex();
    const auto& list = mm_->getReferenceList();
    if (nextIdx >= list.size()) {
        addLog("LISTA: No hay más referencias", "#f9e2af");
        return;
    }
    
    auto ref = list[nextIdx];
    auto resultOpt = mm_->executeNextReference();
    int result = resultOpt.has_value() ? resultOpt.value() : 0;
    
    if (result == 1) {
        addLog(QString("BATCH HIT: PID %1 pág. %2").arg(ref.pid).arg(ref.localPage), "#89b4fa");
    } else if (result == 2) {
        addLog(QString("BATCH PAGE FAULT: PID %1 pág. %2").arg(ref.pid).arg(ref.localPage), "#f9e2af");
    } else {
        addLog(QString("BATCH SEGFAULT: PID %1 pág. %2").arg(ref.pid).arg(ref.localPage), "#f38ba8");
    }
    
    refreshAll();
}

void MainWindow::onRunReferences() {
    if (!mm_) return;
    int executed = 0;
    while (mm_->getNextReferenceIndex() < mm_->getReferenceList().size()) {
        mm_->executeNextReference();
        executed++;
    }
    addLog(QString("BATCH: Ejecutadas %1 referencias").arg(executed), "#a6e3a1");
    refreshAll();
}

void MainWindow::refreshReferenceList() {
    if (!mm_) return;
    const auto& list = mm_->getReferenceList();
    size_t nextIdx = mm_->getNextReferenceIndex();
    
    referenceTable_->setRowCount(static_cast<int>(list.size()));
    for (int i = 0; i < static_cast<int>(list.size()); ++i) {
        auto* pidItem = new QTableWidgetItem(QString::number(list[i].pid));
        auto* pgItem = new QTableWidgetItem(QString::number(list[i].localPage));
        
        if (i < static_cast<int>(nextIdx)) {
            pidItem->setForeground(QColor("#585b70"));
            pgItem->setForeground(QColor("#585b70"));
        } else if (i == static_cast<int>(nextIdx)) {
            pidItem->setBackground(QColor("#313244"));
            pidItem->setForeground(QColor("#fab387"));
            pgItem->setBackground(QColor("#313244"));
            pgItem->setForeground(QColor("#fab387"));
        }
        
        referenceTable_->setItem(i, 0, pidItem);
        referenceTable_->setItem(i, 1, pgItem);
    }
}

void MainWindow::refreshStats() {
    if (!mm_) return;
    size_t vmCap = mm_->getVirtualCapacity();
    size_t vmFree = mm_->getFreeVirtualPages();
    size_t vmOcc = vmCap - vmFree;

    occupancyBar_->setValue(static_cast<int>((double)vmOcc / vmCap * 100));
    freePagesLbl_->setText(QString("VM Libres: %1").arg(vmFree));
    
    occPagesLbl_->setText(QString("MF: %1 / %2").arg(mm_->getRamUsed()).arg(mm_->getRamCapacity()));
    activeProcsLbl_->setText(QString("Procesos Activos: %1").arg(mm_->getProcesses().size()));

    if (dispatcher_) {
        dispatcherStatusLbl_->setText(QString("t=%1 | PID=%2 | Quantum: %3/%4")
            .arg(dispatcher_->currentTime())
            .arg(dispatcher_->currentPid())
            .arg(dispatcher_->currentQuantumUsed())
            .arg(dispatcher_->getQuantum()));
    }
}

QColor MainWindow::getProcessColor(int pid) {
    int h = (pid * 137) % 360;
    return QColor::fromHsv(h, 150, 200);
}

void MainWindow::refreshVMGrid() {
    if (!mm_) return;
    const auto& bitmap = mm_->getBitmap();
    size_t cap = bitmap.size();
    
    int cols = std::ceil(std::sqrt(cap));
    int rows = std::ceil((double)cap / cols);

    vmGrid_->setRowCount(rows);
    vmGrid_->setColumnCount(cols);

    std::unordered_map<size_t, int> reverseMap;
    for (const auto& [pid, proc] : mm_->getProcesses()) {
        for (const auto& [loc, glob] : proc.getPageTable()) {
            reverseMap[glob] = pid;
        }
    }

    for (size_t i = 0; i < cap; ++i) {
        int r = i / cols;
        int c = i % cols;
        auto* item = new QTableWidgetItem();
        
        if (!bitmap[i]) {
            item->setBackground(QColor("#1e1e2e")); 
        } else {
            int ownerPid = reverseMap[i];
            QColor color = getProcessColor(ownerPid);
            

            if (!mm_->isPageInRam(i)) {
                color.setAlpha(60); 
                item->setText("SWP");
                item->setForeground(QColor("#a6adc8"));
            } else {
                item->setText(QString::number(ownerPid)); 
                item->setForeground(QColor("#11111b"));
            }
            
            item->setBackground(color);
            item->setTextAlignment(Qt::AlignCenter);
        }
        vmGrid_->setItem(r, c, item);
        vmGrid_->setColumnWidth(c, 35);
        vmGrid_->setRowHeight(r, 35);
    }
}

void MainWindow::refreshProcessList() {
    if (!mm_) return;
    const auto& procs = mm_->getProcesses();
    procTable_->setRowCount(static_cast<int>(procs.size()));
    
    int row = 0;
    for (const auto& [pid, proc] : procs) {
        auto* pidItem = new QTableWidgetItem(QString::number(pid));
        pidItem->setForeground(getProcessColor(pid));
        
        procTable_->setItem(row, 0, pidItem);
        procTable_->setItem(row, 1, new QTableWidgetItem(QString::number(proc.getRequiredPages())));
        row++;
    }
    onProcessSelectionChanged(); 
}

void MainWindow::onProcessSelectionChanged() {
    localPageTable_->setRowCount(0);
    auto items = procTable_->selectedItems();
    if (items.isEmpty() || !mm_) return;

    int pid = items[0]->text().toInt();
    const auto& procs = mm_->getProcesses();
    auto it = procs.find(pid);
    if (it != procs.end()) {
        const auto& pt = it->second.getPageTable();
        localPageTable_->setRowCount(static_cast<int>(pt.size()));
        int row = 0;
        for (const auto& [local, global] : pt) {
            localPageTable_->setItem(row, 0, new QTableWidgetItem(QString::number(local)));
            localPageTable_->setItem(row, 1, new QTableWidgetItem(QString::number(global)));
            
            bool inRam = mm_->isPageInRam(global);
            auto* locItem = new QTableWidgetItem(inRam ? "En RAM" : "Disco (Swap)");
            locItem->setForeground(inRam ? QColor("#a6e3a1") : QColor("#bac2de"));
            localPageTable_->setItem(row, 2, locItem);
            
            QString bitInfo = QString::fromStdString(mm_->policy()->getPageInfo(global));
            auto* bitItem = new QTableWidgetItem(bitInfo);
            bitItem->setForeground(QColor("#f9e2af"));
            localPageTable_->setItem(row, 3, bitItem);
            
            row++;
        }
    }
}
void MainWindow::addLog(const QString& msg, const QString& color) {
    logEdit_->append(QString("<span style='color:%1;'>%2</span>").arg(color, msg.toHtmlEscaped()));
}

void MainWindow::onStep() {
    if (!dispatcher_) return;
    
    auto res = dispatcher_->step();
    if (!res.running) {
        addLog("DISPATCHER: Idle (No hay procesos en cola)", "#585b70");
        return;
    }

    QString resStr = (res.result == 1) ? "HIT" : (res.result == 2 ? "FAULT" : "ERROR");
    QString color = (res.result == 1) ? "#89b4fa" : (res.result == 2 ? "#f9e2af" : "#f38ba8");
    
    addLog(QString("t=%1 | PID=%2 | Acceso a %3 | %4")
        .arg(dispatcher_->currentTime())
        .arg(res.pid)
        .arg(res.ref)
        .arg(resStr), color);

    if (res.result == 2) {
        QMessageBox::warning(this, "⚠️ Page Fault", 
            QString("Fallo de página detectado!\nPID: %1\nPágina: %2\nTiempo: %3")
            .arg(res.pid).arg(res.ref).arg(dispatcher_->currentTime()));
    }

    refreshAll();

    if (dispatcher_->isDone()) {
        showFinalReport();
    }
}

void MainWindow::showFinalReport() {
    if (!dispatcher_) return;
    auto reports = dispatcher_->getReport();
    if (reports.empty()) return;
    
    QDialog dlg(this);
    dlg.setWindowTitle("Reporte Final de Simulación");
    dlg.setMinimumSize(700, 450);
    dlg.setStyleSheet("background-color: #11111b; color: #cdd6f4;");
    auto* lay = new QVBoxLayout(&dlg);
    
    auto* title = new QLabel("RESUMEN DE EJECUCIÓN");
    title->setStyleSheet("color: #89b4fa; font-size: 16px; font-weight: bold; margin: 10px;");
    lay->addWidget(title, 0, Qt::AlignCenter);

    auto* table = new QTableWidget(reports.size() + 1, 5);
    table->setHorizontalHeaderLabels({"PID", "Llegada", "Terminación", "Espera", "Fallos de Página"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    
    int totalPF = 0;
    for (int i = 0; i < (int)reports.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(reports[i].pid)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(reports[i].arrivalTime)));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(reports[i].finishTime)));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(reports[i].waitingTime)));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(reports[i].pageFaults)));
        totalPF += reports[i].pageFaults;
    }
    
    int lastRow = (int)reports.size();
    auto* totalLabelItem = new QTableWidgetItem("TOTAL FALLOS");
    totalLabelItem->setForeground(QColor("#f9e2af"));
    table->setItem(lastRow, 0, totalLabelItem);
    table->setItem(lastRow, 4, new QTableWidgetItem(QString::number(totalPF)));
    
    lay->addWidget(table);
    
    auto* closeBtn = new QPushButton("Cerrar");
    closeBtn->setStyleSheet("background-color: #1e1e2e; color: #a6e3a1; border: 1px solid #a6e3a1; padding: 10px;");
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    lay->addWidget(closeBtn);
    
    dlg.exec();
}