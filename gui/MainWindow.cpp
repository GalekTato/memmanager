#include "MainWindow.hpp"
#include <QApplication>
#include <cmath>

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
        QGroupBox { border: 1px solid #313244; border-radius: 8px; margin-top: 12px; padding-top: 8px; color: #a6adc8; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; top: -1px; color: #89b4fa; font-weight: bold; }
        QProgressBar { border: 1px solid #313244; border-radius: 4px; background: #1e1e2e; height: 16px; text-align: center; color: #cdd6f4; }
        QProgressBar::chunk { border-radius: 3px; background: #89b4fa; }
        QTableWidget { background-color: #1e1e2e; gridline-color: #313244; border: 1px solid #313244; border-radius: 6px; }
        QHeaderView::section { background-color: #181825; color: #bac2de; border: none; border-bottom: 1px solid #313244; padding: 4px; }
        QTextEdit, QSpinBox { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; color: #cdd6f4; padding: 5px; }
        QPushButton { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; color: #cdd6f4; padding: 6px; font-weight: bold; }
        QPushButton:hover  { background-color: #313244; border-color: #89b4fa; color: #89b4fa; }
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
    createBtn_ = new QPushButton("Crear y Asignar");
    createBtn_->setStyleSheet("color: #a6e3a1; border-color: #a6e3a1;");
    createLay->addWidget(new QLabel("Nuevo:"), 0, 0); createLay->addWidget(newPidSpin_, 0, 1);
    createLay->addWidget(reqPagesSpin_, 0, 2); createLay->addWidget(createBtn_, 1, 0, 1, 3);
    lay->addLayout(createLay);
    lay->addSpacing(10);

    auto* accessLay = new QGridLayout;
    accessPidSpin_ = new QSpinBox; accessPidSpin_->setRange(1, 9999); accessPidSpin_->setPrefix("PID: ");
    logicalPageSpin_ = new QSpinBox; logicalPageSpin_->setRange(0, 1024); logicalPageSpin_->setPrefix("Pg Lógica: ");
    accessBtn_ = new QPushButton("Acceder a Memoria");
    accessBtn_->setStyleSheet("color: #89b4fa; border-color: #89b4fa;");
    accessLay->addWidget(new QLabel("Acceso:"), 0, 0); accessLay->addWidget(accessPidSpin_, 0, 1);
    accessLay->addWidget(logicalPageSpin_, 0, 2); accessLay->addWidget(accessBtn_, 1, 0, 1, 3);
    lay->addLayout(accessLay);
    lay->addSpacing(10);

    auto* termLay = new QHBoxLayout;
    termPidSpin_ = new QSpinBox; termPidSpin_->setRange(1, 9999); termPidSpin_->setPrefix("PID: ");
    termBtn_ = new QPushButton("Kill");
    termBtn_->setStyleSheet("color: #f38ba8; border-color: #f38ba8;");
    termLay->addWidget(termPidSpin_); termLay->addWidget(termBtn_);
    lay->addLayout(termLay);
    lay->addSpacing(15);

    auto* cfgLay = new QHBoxLayout;
    capacitySpin_ = new QSpinBox; capacitySpin_->setRange(16, 4096); capacitySpin_->setValue(256);
    ramSpin_ = new QSpinBox; ramSpin_->setRange(4, 4096); ramSpin_->setValue(16);
    cfgLay->addWidget(new QLabel("VM Total:")); cfgLay->addWidget(capacitySpin_, 1);
    cfgLay->addWidget(new QLabel("MF Total:")); cfgLay->addWidget(ramSpin_, 1);
    lay->addLayout(cfgLay);

    connect(createBtn_, &QPushButton::clicked, this, &MainWindow::onCreateProcess);
    connect(accessBtn_, &QPushButton::clicked, this, &MainWindow::onAccessPage);
    connect(termBtn_, &QPushButton::clicked, this, &MainWindow::onTerminateProcess);
    connect(capacitySpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onCapacityChanged);
    connect(capacitySpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onCapacityChanged);
    connect(ramSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onCapacityChanged);

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
    localPageTable_->setHorizontalHeaderLabels({"Pg Lógica", "Pg Virtual", "Ubicación", "Bits de control"});
    localPageTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
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
        if (ramCap > vmCap) {
            ramCap = vmCap;
            ramSpin_->setValue(ramCap);
        }
    auto pol = std::make_unique<HybridPolicy>(ramCap);
    mm_ = std::make_unique<MemoryManager>(vmCap, ramCap, std::move(pol));
    refreshAll();
}

void MainWindow::onCreateProcess() {
    if (!mm_) return;
    int pid = newPidSpin_->value();
    size_t req = reqPagesSpin_->value();
    
    if (mm_->admitProcess(pid, req)) {
        addLog(QString("ADMISIÓN: PID %1 aceptado (%2 pags apartadas en VM/Swap)").arg(pid).arg(req), "#a6e3a1");
        newPidSpin_->setValue(pid + 1); 
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
}

void MainWindow::refreshStats() {
    if (!mm_) return;
    size_t vmCap = mm_->getVirtualCapacity();
    size_t vmFree = mm_->getFreeVirtualPages();
    size_t vmOcc = vmCap - vmFree;

    occupancyBar_->setValue(static_cast<int>((double)vmOcc / vmCap * 100));
    freePagesLbl_->setText(QString("VM Libres: %1").arg(vmFree));
    
    // Mostramos también la RAM real usada
    occPagesLbl_->setText(QString("MF: %1 / %2").arg(mm_->getRamUsed()).arg(mm_->getRamCapacity()));
    activeProcsLbl_->setText(QString("Procesos Activos: %1").arg(mm_->getProcesses().size()));
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