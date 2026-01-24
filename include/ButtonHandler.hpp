#pragma once

#include <QObject>
#include <QString>
#include <QSharedPointer>

#include "DataManager.hpp"
#include "HardwareController.hpp"

class ButtonHandler : public QObject
{
    Q_OBJECT
    QSharedPointer<DataManager> m_dataManager;
    QSharedPointer<HardwareController> m_hardwareController;

public:
    ButtonHandler(QSharedPointer<DataManager> dm, QSharedPointer<HardwareController> hwc, QObject* parent = nullptr);
    void handleRunStart();
    void handleRunStop();

Q_SIGNALS:
    void exitApp();

public Q_SLOTS:
    void handleButtonClick(const QString &buttonName);
    void saveDataClick();
};
