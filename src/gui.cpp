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
#include <QBrush>
#include <QPen>

SchedulerGUI::SchedulerGUI(QWidget* parent) : QWidget(parent), currentStepIndex(0) {
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
    prioLabel = new QLabel("Prio:", this);
    inputLayout->addWidget(prioLabel);
    inputLayout->addWidget(priorityInput);
    

    algoSelect = new QComboBox(this);
    algoSelect->addItems({"FCFS", "SJF (Non-Preemptive)", "SRTF (Preemptive)", "Priority", "Round Robin"});

    avgWaitingTimeLabel = new QLabel("Average Waiting Time: 0", this);
    avgTurnaroundTimeLabel = new QLabel("Average Turnaround Time: 0", this);

    table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"PID", "Arrival", "Burst", "Priority"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    scene = new QGraphicsScene(this);
    ganttView = new QGraphicsView(scene, this);
    ganttView->setMinimumHeight(200);

    QPushButton* addProcessBtn = new QPushButton("Add Process", this);
    QPushButton* startBtn = new QPushButton("Start Simulation", this);
    //for delete button
    deleteBtn = new QPushButton("Clear All", this);
    connect(deleteBtn, &QPushButton::clicked, this, &SchedulerGUI::resetAll);

     // Adding everything to layout
    layout->addLayout(inputLayout);
    layout->addWidget(new QLabel("Choose Algorithm:"));
    layout->addWidget(algoSelect);
    layout->addWidget(addProcessBtn);
    layout->addWidget(table);
    layout->addWidget(ganttView);
    layout->addWidget(avgWaitingTimeLabel);
    layout->addWidget(avgTurnaroundTimeLabel);
    layout->addWidget(startBtn);
    layout->addWidget(deleteBtn);//new

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SchedulerGUI::updateSimulation);
    connect(startBtn, &QPushButton::clicked, this, &SchedulerGUI::startScheduler);
    connect(addProcessBtn, &QPushButton::clicked, this, &SchedulerGUI::addProcess);
    connect(algoSelect, &QComboBox::currentTextChanged, this, &SchedulerGUI::handleAlgoChange);
   //hide priority input if not needed
    priorityInput->setVisible(false);
    prioLabel->setVisible(false);
    table->setColumnHidden(3, true);
}

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
    table->setItem(row, 3, new QTableWidgetItem(QString::number(p.priority)));
}




void SchedulerGUI::startScheduler() {
    if (processes.empty()) return;
    scene->clear();
    currentStepIndex = 0;

    QString selected = algoSelect->currentText();
    if (selected == "FCFS") FCFS();
    else if (selected == "SJF (Non-Preemptive)") SJF(false);
    else if (selected == "SRTF (Preemptive)") SJF(true);
    else if (selected == "Priority") PriorityScheduling(false);
    else if (selected == "Round Robin") RoundRobin(2); // Using 2 as default quantum

    timer->start(300); 
}

void SchedulerGUI::updateSimulation() {
    if (static_cast<size_t>(currentStepIndex) >= ganttLog.size()) {
        timer->stop();
        double totalWait = 0, totalTAT = 0;
        for(const auto& p : processes) {
            totalWait += p.waitingTime;
            totalTAT += p.turnaroundTime;
        }
        avgWaitingTimeLabel->setText(QString("Average Waiting Time: %1").arg(totalWait / processes.size()));
        avgTurnaroundTimeLabel->setText(QString("Average Turnaround Time: %1").arg(totalTAT / processes.size()));
        return;
    }

    GanttStep step = ganttLog[currentStepIndex];
    int scale = 30;
    int height = 60;
    int x = step.startTime * scale;
    int width = (step.endTime - step.startTime) * scale;

    scene->addRect(x, 0, width, height, QPen(Qt::black), QBrush(Qt::cyan));
   
    // Draw timeline ONLY ONCE (first step)
if (currentStepIndex == 0) {
    int scale = 30;
    int y = 70;

    for (const auto& s : ganttLog) {
        int posX = s.startTime * scale;

        QGraphicsTextItem* timeText = scene->addText(QString::number(s.startTime));
        timeText->setPos(posX, y);
    }

    // Draw LAST end time
    int lastEnd = ganttLog.back().endTime;
    QGraphicsTextItem* endText = scene->addText(QString::number(lastEnd));
    endText->setPos(lastEnd * scale, y);
}

    QGraphicsTextItem* text = scene->addText(QString("P%1").arg(step.pid));
    text->setPos(x + (width / 2) - 10, height / 3);
   
    currentStepIndex++;

}
  
    
// Clear all data and reset the GUI to its initial state
void SchedulerGUI::resetAll() {
        processes.clear();
        ganttLog.clear();
        table->setRowCount(0);
        scene->clear();
        currentStepIndex = 0;
        timer->stop(); 
        avgWaitingTimeLabel->setText("Average Waiting Time: 0");
        avgTurnaroundTimeLabel->setText("Average Turnaround Time: 0");
    }

// only show priority input when Priority scheduling is selected
void SchedulerGUI::handleAlgoChange(const QString &algo) {
    bool isPriority = algo.contains("Priority");
    priorityInput->setVisible(isPriority);
    prioLabel->setVisible(isPriority);
    table->setColumnHidden(3, !isPriority);
}