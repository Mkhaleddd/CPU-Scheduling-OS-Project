#pragma once
#include <QWidget>
#include <QTimer>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLineEdit>    
#include <QComboBox>    

class SchedulerGUI : public QWidget {
    Q_OBJECT
public:
    explicit SchedulerGUI(QWidget* parent = nullptr);

private slots:
    void startScheduler();
    void updateSimulation();
    void addProcess();
    void resetAll();
    void handleAlgoChange(const QString &algo);

private:
    
    QLineEdit* arrivalInput;

    QPushButton* deleteBtn;
    QLabel* prioLabel;
    QLineEdit* burstInput;
    QLineEdit* priorityInput;
    QComboBox* algoSelect;


    QLabel* avgWaitingTimeLabel;
    QLabel* avgTurnaroundTimeLabel;


    QTableWidget* table;
    QGraphicsView* ganttView;
    QGraphicsScene* scene;

    
    QTimer* timer;
    int currentStepIndex; 
};