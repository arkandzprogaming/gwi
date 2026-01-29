#pragma once

#include <QDebug>
#include <QObject>
#include <QTimer>
#include <QSharedPointer>

#include "DataManager.hpp"

class SliderHandler : public QObject
{
    Q_OBJECT

    QTimer* m_updateTimer;
    int m_currentValue;
    QSharedPointer<DataManager> m_dataManager;

public:
    SliderHandler(QSharedPointer<DataManager> dataManager, QObject* parent = nullptr);
    void setInitialSliderValue(int value);

public Q_SLOTS:
    int getCurrentValue();
    void changeSliderValue(int currValue);
    void onUpdateTimer();

Q_SIGNALS:
    void ledIntensityRequested(int currValue);
};
