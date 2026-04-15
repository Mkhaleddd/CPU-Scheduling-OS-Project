#include "gui.h"
#include "scheduler.h"
#include "GanttStep.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <climits>

std::vector<GanttStep> ganttLog;

// ================= CONSTRUCTOR =================
SchedulerGUI::SchedulerGUI(QWidget* parent)
    : QWidget(parent),
      currentStepIndex(0),
      currentTime(0),
      lastExecutedPid(-1),
      stepStartTime(0),
      isRunning(false)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* inputLayout = new QHBoxLayout();

    burstInput = new QLineEdit(this);
    burstInput->setPlaceholderText("Burst Time");

    priorityInput = new QLineEdit(this);
    priorityInput->setPlaceholderText("Priority");

    arrivalInput = new QLineEdit(this);
    arrivalInput->setPlaceholderText("Arrival Time");

    inputLayout->addWidget(new QLabel("Arrival:"));
    inputLayout->addWidget(arrivalInput);
    inputLayout->addWidget(new QLabel("Burst:"));
    inputLayout->addWidget(burstInput);
    inputLayout->addWidget(new QLabel("Prio:"));
    inputLayout->addWidget(priorityInput);

    algoSelect = new QComboBox(this);
    algoSelect->addItems({
        "FCFS",
        "SJF (Non-Preemptive)",
        "SRTF (Preemptive)",
        "Priority",
        "Priority (Preemptive)",
        "Round Robin"
    });

    modeSelect = new QComboBox(this);
    modeSelect->addItems({"Static", "Live"});

    avgWaitingTimeLabel = new QLabel("Average Waiting Time: 0", this);
    avgTurnaroundTimeLabel = new QLabel("Average Turnaround Time: 0", this);

    table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"PID", "Arrival", "Burst", "Remaining"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    scene = new QGraphicsScene(this);
    ganttView = new QGraphicsView(scene, this);
    ganttView->setMinimumHeight(200);

    QPushButton* addProcessBtn = new QPushButton("Add Process", this);
    QPushButton* startBtn = new QPushButton("Start Simulation", this);

    pauseBtn = new QPushButton("Pause", this);
    resumeBtn = new QPushButton("Resume", this);
    addProcessLiveBtn = new QPushButton("Add Process (Live)", this);
    addProcessLiveBtn->setEnabled(false);

    layout->addLayout(inputLayout);
    layout->addWidget(algoSelect);
    layout->addWidget(modeSelect);

    layout->addWidget(addProcessBtn);
    layout->addWidget(pauseBtn);
    layout->addWidget(resumeBtn);
    layout->addWidget(addProcessLiveBtn);

    layout->addWidget(table);
    layout->addWidget(ganttView);
    layout->addWidget(avgWaitingTimeLabel);
    layout->addWidget(avgTurnaroundTimeLabel);
    layout->addWidget(startBtn);

    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &SchedulerGUI::updateSimulation);
    connect(startBtn, &QPushButton::clicked, this, &SchedulerGUI::startScheduler);
    connect(addProcessBtn, &QPushButton::clicked, this, &SchedulerGUI::addProcess);

    connect(pauseBtn, &QPushButton::clicked, [=]() {
        timer->stop();
        addProcessLiveBtn->setEnabled(true);
    });

    connect(resumeBtn, &QPushButton::clicked, [=]() {
        timer->start(1000);
        addProcessLiveBtn->setEnabled(false);
    });

    connect(addProcessLiveBtn, &QPushButton::clicked, this, &SchedulerGUI::addProcessLive);
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
    table->setItem(row, 0, new QTableWidgetItem(QString::number(p.pid)));
    table->setItem(row, 1, new QTableWidgetItem(QString::number(p.arrivalTime)));
    table->setItem(row, 2, new QTableWidgetItem(QString::number(p.burstTime)));
    table->setItem(row, 3, new QTableWidgetItem(QString::number(p.remainingTime)));
}

// ================= ADD PROCESS LIVE =================
void SchedulerGUI::addProcessLive() {
    Process p;
    p.pid = processes.size() + 1;
    p.arrivalTime = currentTime; // 🔥 المهم
    p.burstTime = 5;
    p.remainingTime = p.burstTime;
    p.priority = 0;

    processes.push_back(p);

    int row = table->rowCount();
    table->insertRow(row);

    table->setItem(row, 0, new QTableWidgetItem(QString::number(p.pid)));
    table->setItem(row, 1, new QTableWidgetItem(QString::number(p.arrivalTime)));
    table->setItem(row, 2, new QTableWidgetItem(QString::number(p.burstTime)));
    table->setItem(row, 3, new QTableWidgetItem(QString::number(p.remainingTime)));
}

// ================= START =================
void SchedulerGUI::startScheduler() {

    if (processes.empty()) return;

    scene->clear();
    ganttLog.clear();
    currentStepIndex = 0;

    QString mode = modeSelect->currentText();
    QString selected = algoSelect->currentText();

    if (mode == "Static") {

        if (selected == "FCFS") FCFS();
        else if (selected == "SJF (Non-Preemptive)") SJF(false);
        else if (selected == "SRTF (Preemptive)") SJF(true);
        else if (selected == "Priority") PriorityScheduling(false);
        else if (selected == "Priority (Preemptive)") PriorityScheduling(true);
        else if (selected == "Round Robin") RoundRobin(2);

        timer->start(300);
    }
    else {

        for (auto &p : processes) {
            p.remainingTime = p.burstTime;
            p.completionTime = 0;
        }

        currentTime = 0;
        lastExecutedPid = -1;
        stepStartTime = 0;
        currentProcessIndex = -1;
        while(!readyQueue.empty()) readyQueue.pop();

        isRunning = true;
        timer->start(1000);
    }
}

// ================= UPDATE =================
void SchedulerGUI::updateSimulation() {

    QString mode = modeSelect->currentText();

    if (mode == "Static") {

        if (currentStepIndex >= ganttLog.size()) {
            timer->stop();
            return;
        }

        GanttStep step = ganttLog[currentStepIndex];

        int scale = 30;
        int x = step.startTime * scale;
        int width = (step.endTime - step.startTime) * scale;

        QColor color = QColor::fromHsv((step.pid * 50) % 360, 255, 200);

        scene->addRect(x, 0, width, 60, QPen(Qt::black), QBrush(color));
        QGraphicsTextItem* text = scene->addText(QString("P%1").arg(step.pid));
        text->setPos(x + width / 2, 20);

        currentStepIndex++;
    }
    else {

        if (!isRunning) return;

        simulateStep();

        for (int i = 0; i < processes.size(); i++) {
            if (table->item(i,3))
                table->item(i,3)->setText(QString::number(processes[i].remainingTime));
        }

        scene->clear();
        for (auto &step : ganttLog) {

            int scale = 30;
            int x = step.startTime * scale;
            int width = (step.endTime - step.startTime) * scale;

            QColor color = QColor::fromHsv((step.pid * 50) % 360, 255, 200);

            scene->addRect(x, 0, width, 60, QPen(Qt::black), QBrush(color));
            QGraphicsTextItem* t = scene->addText(QString("P%1").arg(step.pid));
            t->setPos(x + width / 2, 20);
        }
    }
}

// ================= SIMULATE (FULL FIX) =================
void SchedulerGUI::simulateStep() {

    QString algo = algoSelect->currentText();
    int idx = -1;

    // FCFS
    if (algo == "FCFS") {
        for (int i = 0; i < processes.size(); i++) {
            if (processes[i].arrivalTime <= currentTime &&
                processes[i].remainingTime > 0) {
                idx = i;
                break;
            }
        }
    }

    // SRTF
    else if (algo == "SRTF (Preemptive)") {
        int minRemaining = INT_MAX;

        for (int i = 0; i < processes.size(); i++) {
            if (processes[i].arrivalTime <= currentTime &&
                processes[i].remainingTime > 0 &&
                processes[i].remainingTime < minRemaining) {

                minRemaining = processes[i].remainingTime;
                idx = i;
            }
        }
    }

    // SJF NON PREEMPTIVE
    else if (algo == "SJF (Non-Preemptive)") {

        if (currentProcessIndex == -1) {
            int minBurst = INT_MAX;

            for (int i = 0; i < processes.size(); i++) {
                if (processes[i].arrivalTime <= currentTime &&
                    processes[i].remainingTime > 0 &&
                    processes[i].burstTime < minBurst) {

                    minBurst = processes[i].burstTime;
                    currentProcessIndex = i;
                }
            }
        }

        idx = currentProcessIndex;

        if (idx != -1 && processes[idx].remainingTime == 0)
            currentProcessIndex = -1;
    }

    // PRIORITY NON PREEMPTIVE
    else if (algo == "Priority") {

        if (currentProcessIndex == -1) {
            int best = INT_MAX;

            for (int i = 0; i < processes.size(); i++) {
                if (processes[i].arrivalTime <= currentTime &&
                    processes[i].remainingTime > 0 &&
                    processes[i].priority < best) {

                    best = processes[i].priority;
                    currentProcessIndex = i;
                }
            }
        }

        idx = currentProcessIndex;

        if (idx != -1 && processes[idx].remainingTime == 0)
            currentProcessIndex = -1;
    }

    // PRIORITY PREEMPTIVE
    else if (algo == "Priority (Preemptive)") {

        int best = INT_MAX;

        for (int i = 0; i < processes.size(); i++) {
            if (processes[i].arrivalTime <= currentTime &&
                processes[i].remainingTime > 0 &&
                processes[i].priority < best) {

                best = processes[i].priority;
                idx = i;
            }
        }
    }

    // ROUND ROBIN
    else if (algo == "Round Robin") {

        for (int i = 0; i < processes.size(); i++) {
            if (processes[i].arrivalTime == currentTime) {
                readyQueue.push(i);
            }
        }

        if (!readyQueue.empty()) {

            idx = readyQueue.front();
            readyQueue.pop();

            int exec = std::min(quantum, processes[idx].remainingTime);

            if (processes[idx].pid != lastExecutedPid) {
                if (lastExecutedPid != -1)
                    ganttLog.push_back({lastExecutedPid, stepStartTime, currentTime});

                lastExecutedPid = processes[idx].pid;
                stepStartTime = currentTime;
            }

            processes[idx].remainingTime -= exec;
            currentTime += exec;

            if (processes[idx].remainingTime > 0)
                readyQueue.push(idx);
            else
                processes[idx].completionTime = currentTime;

            return;
        }
    }

    // NO PROCESS
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
