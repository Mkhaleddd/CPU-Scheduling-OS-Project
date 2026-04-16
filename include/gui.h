#pragma once

#include <QWidget>
#include <QTimer>
#include <QTableWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class SchedulerGUI : public QWidget {
    Q_OBJECT

public:
    explicit SchedulerGUI(QWidget* parent = nullptr);

private slots:
    void addProcess();
    void addProcessLive();
    void startScheduler();
    void updateSimulation();

private:
    // Inputs
    QLineEdit* arrivalInput;
    QLineEdit* burstInput;
    QLineEdit* priorityInput;

    // Mode + Algo
    QComboBox* modeSelect;
    QComboBox* algoSelect;

    // Table + Gantt
    QTableWidget* table;
    QGraphicsView* ganttView;
    QGraphicsScene* scene;

    // Buttons
    QPushButton* pauseBtn;
    QPushButton* resumeBtn;
    QPushButton* addProcessLiveBtn;

    // Timer
    QTimer* timer;

    // Labels
    QLabel* timeLabel;
    QPushButton* deleteBtn;

    // Simulation
    int currentTime;
    int lastExecutedPid;
    int stepStartTime;

    // Core
    void simulateStep();
    void drawGanttRect(int pid, int start, int end);
    void drawLiveGantt();
    void drawFullGantt();
    void drawTimeline();
    bool allProcessesFinished();
};