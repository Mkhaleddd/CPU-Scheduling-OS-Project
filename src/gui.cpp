#include "gui.h"
#include "scheduler.h"
#include "GanttStep.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGraphicsTextItem>

// ================= CONSTRUCTOR =================
SchedulerGUI::SchedulerGUI(QWidget* parent)
    : QWidget(parent),
    currentTime(0),
    lastExecutedPid(-1),
    stepStartTime(0)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* inputLayout = new QHBoxLayout();

    arrivalInput = new QLineEdit(this);
    arrivalInput->setPlaceholderText("Arrival");

    burstInput = new QLineEdit(this);
    burstInput->setPlaceholderText("Burst");

    priorityInput = new QLineEdit(this);
    priorityInput->setPlaceholderText("Priority");

    inputLayout->addWidget(arrivalInput);
    inputLayout->addWidget(burstInput);
    inputLayout->addWidget(priorityInput);

    modeSelect = new QComboBox(this);
    modeSelect->addItems({"Static", "Live"});

    algoSelect = new QComboBox(this);
    algoSelect->addItems({"FCFS", "Priority"});

    // ⏱ Time label
    timeLabel = new QLabel("Time: 0", this);

    // Table
    table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"PID","Arrival","Burst","Remaining"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Gantt
    scene = new QGraphicsScene(this);
    ganttView = new QGraphicsView(scene, this);
    ganttView->setMinimumHeight(220);

    // Buttons
    QPushButton* addBtn = new QPushButton("Add Process", this);
    QPushButton* startBtn = new QPushButton("Start", this);

    deleteBtn = new QPushButton("Delete Selected", this);
    pauseBtn = new QPushButton("Pause", this);
    resumeBtn = new QPushButton("Resume", this);
    addProcessLiveBtn = new QPushButton("Add Process (Live)", this);

    addProcessLiveBtn->setEnabled(false);

    layout->addLayout(inputLayout);
    layout->addWidget(modeSelect);
    layout->addWidget(algoSelect);
    layout->addWidget(timeLabel);
    layout->addWidget(addBtn);
    layout->addWidget(deleteBtn);
    layout->addWidget(startBtn);
    layout->addWidget(pauseBtn);
    layout->addWidget(resumeBtn);
    layout->addWidget(addProcessLiveBtn);
    layout->addWidget(table);
    layout->addWidget(ganttView);

    timer = new QTimer(this);

    // ================= CONNECTIONS =================
    connect(addBtn, &QPushButton::clicked, this, &SchedulerGUI::addProcess);
    connect(startBtn, &QPushButton::clicked, this, &SchedulerGUI::startScheduler);
    connect(timer, &QTimer::timeout, this, &SchedulerGUI::updateSimulation);

    // Delete
    connect(deleteBtn, &QPushButton::clicked, [=]() {

        if (timer->isActive()) return;

        int row = table->currentRow();
        if (row < 0) return;

        processes.erase(processes.begin() + row);
        table->removeRow(row);

        // Fix PIDs
        for (int i = 0; i < processes.size(); i++) {
            processes[i].pid = i + 1;
            table->item(i,0)->setText(QString::number(processes[i].pid));
        }
    });

    // Pause
    connect(pauseBtn, &QPushButton::clicked, [=]() {
        timer->stop();
        addProcessLiveBtn->setEnabled(true);
    });

    // Resume
    connect(resumeBtn, &QPushButton::clicked, [=]() {
        timer->start(1000);
        addProcessLiveBtn->setEnabled(false);
    });

    // Live add
    connect(addProcessLiveBtn, &QPushButton::clicked, this, &SchedulerGUI::addProcessLive);

    // Priority enable/disable
    connect(algoSelect, &QComboBox::currentTextChanged, [=](QString algo){
        if (algo.contains("Priority")) {
            priorityInput->setEnabled(true);
            priorityInput->setStyleSheet("");
        } else {
            priorityInput->setEnabled(false);
            priorityInput->setStyleSheet("background-color: lightgray;");
            priorityInput->clear();
        }
    });
}

// ================= ADD PROCESS =================
void SchedulerGUI::addProcess() {
    Process p;
    p.pid = processes.size() + 1;
    p.arrivalTime = arrivalInput->text().toInt();
    p.burstTime = burstInput->text().toInt();
    p.remainingTime = p.burstTime;
    p.priority = priorityInput->text().toInt();

    processes.push_back(p);

    int row = table->rowCount();
    table->insertRow(row);

    table->setItem(row,0,new QTableWidgetItem(QString::number(p.pid)));
    table->setItem(row,1,new QTableWidgetItem(QString::number(p.arrivalTime)));
    table->setItem(row,2,new QTableWidgetItem(QString::number(p.burstTime)));
    table->setItem(row,3,new QTableWidgetItem(QString::number(p.remainingTime)));
}

// ================= ADD LIVE =================
void SchedulerGUI::addProcessLive() {

    if (timer->isActive()) return;

    Process p;
    p.pid = processes.size() + 1;
    p.arrivalTime = currentTime;

    p.burstTime = burstInput->text().toInt();
    p.remainingTime = p.burstTime;
    p.priority = priorityInput->text().toInt();

    processes.push_back(p);

    int row = table->rowCount();
    table->insertRow(row);

    table->setItem(row,0,new QTableWidgetItem(QString::number(p.pid)));
    table->setItem(row,1,new QTableWidgetItem(QString::number(p.arrivalTime)));
    table->setItem(row,2,new QTableWidgetItem(QString::number(p.burstTime)));
    table->setItem(row,3,new QTableWidgetItem(QString::number(p.remainingTime)));
}

// ================= START =================
void SchedulerGUI::startScheduler() {
    QString algo = algoSelect->currentText();
    if (algo == "FCFS") FCFS();
    else if (algo == "SJF") SJF(false);
    else if (algo == "SRTF") SJF(true);
    else if (algo == "Priority") PriorityScheduling(false);
    else if (algo == "Priority (Preemptive)") PriorityScheduling(true);
    else if (algo == "Round Robin") RoundRobin(2);
    if (processes.empty()) return;

    scene->clear();
    ganttLog.clear();

    if (modeSelect->currentText() == "Static") {
        FCFS();
        drawFullGantt();
    } else {
        currentTime = 0;
        lastExecutedPid = -1;
        stepStartTime = 0;
        timer->start(1000);
    }
}

// ================= UPDATE =================
void SchedulerGUI::updateSimulation() {

    if (modeSelect->currentText() == "Static")
        return;

    if (allProcessesFinished()) {

        if (lastExecutedPid != -1)
            ganttLog.push_back({lastExecutedPid, stepStartTime, currentTime});

        timer->stop();
        drawLiveGantt();
        return;
    }

    simulateStep();

    for (size_t i = 0; i < processes.size(); i++) {
        table->item(i,3)->setText(QString::number(processes[i].remainingTime));

        if (processes[i].remainingTime == 0)
            table->item(i,3)->setBackground(Qt::green);
    }

    timeLabel->setText("Time: " + QString::number(currentTime));

    drawLiveGantt();
}

// ================= SIMULATION =================
void SchedulerGUI::simulateStep() {

    int idx = -1;

    for (size_t i = 0; i < processes.size(); i++) {
        if (processes[i].arrivalTime <= currentTime &&
            processes[i].remainingTime > 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        currentTime++;
        return;
    }

    Process &p = processes[idx];

    if (p.pid != lastExecutedPid) {
        if (lastExecutedPid != -1)
            ganttLog.push_back({lastExecutedPid, stepStartTime, currentTime});

        lastExecutedPid = p.pid;
        stepStartTime = currentTime;
    }

    p.remainingTime--;
    currentTime++;

    if (p.remainingTime == 0)
        p.completionTime = currentTime;
}

// ================= DRAW RECT =================
void SchedulerGUI::drawGanttRect(int pid, int start, int end) {

    int scale = 40;
    int x = start * scale;
    int width = (end - start) * scale;

    QColor color = QColor::fromHsv((pid * 70) % 360, 200, 255);

    scene->addRect(x, 0, width, 60, QPen(Qt::black), QBrush(color));

    QGraphicsTextItem* text = scene->addText(QString("P%1").arg(pid));
    text->setPos(x + width/3, 20);
}

// ================= DRAW =================
void SchedulerGUI::drawLiveGantt() {
    scene->clear();

    for (auto &s : ganttLog)
        drawGanttRect(s.pid, s.startTime, s.endTime);

    drawTimeline();
}

void SchedulerGUI::drawFullGantt() {
    for (auto &s : ganttLog)
        drawGanttRect(s.pid, s.startTime, s.endTime);

    drawTimeline();
}

// ================= TIMELINE =================
void SchedulerGUI::drawTimeline() {

    int scale = 40;

    for (int t = 0; t <= currentTime; t++) {
        int x = t * scale;

        QGraphicsTextItem* text = scene->addText(QString::number(t));
        text->setPos(x, 70);
    }
}

// ================= CHECK =================
bool SchedulerGUI::allProcessesFinished() {
    for (auto &p : processes) {
        if (p.remainingTime > 0)
            return false;
    }
    return true;
}