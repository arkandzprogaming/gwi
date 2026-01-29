#pragma once

#include <QObject>
#include <QTimer>

#ifdef HAVE_LCCV
#include <lccv.hpp>
#include <opencv2/opencv.hpp>
#endif

class HardwareController : public QObject {

    Q_OBJECT

#ifdef HAVE_LCCV
    // -- OpenCV and LCCV
    cv::Mat m_backGreen;
#endif

    // -- PWM-LED control
    const int m_ledPin;
    int m_ledClockDivisor;      // Defaulted to 32 --> 640
    int m_ledPwmRange;          // Defaulted to 1024 --> 100
    int m_currentIntensity;

    // -- BH1750 sensor control
    //int m_i2cAdapter;
    //int m_i2cFd;
    //uint8_t m_sensorAddr;

    // -- 5MP CMOS sensor control
#ifdef HAVE_LCCV
    lccv::PiCamera* m_cam;
#endif
    uint32_t m_camId;
    int m_pcrCycle;
    
    // -- Timer to displace DMF interrupt comm
    int m_maxPcrCycle;  // Get from ButtonHandler's QSharedPointer
#ifdef USE_TIMER
    QTimer* m_sensorTimer;
#endif

    // -- ISR
    const int m_isrPin;
#ifdef HAVE_WIRINGPI
    static volatile int m_successInterruptCallbacks;
    static volatile int m_debouncedInterruptCallbacks;
    static HardwareController* s_instance;
    static void isrCallback();

    // -- Debounce timing
    static std::chrono::steady_clock::time_point s_lastIsrTime;
    static constexpr std::chrono::milliseconds ISR_DEBOUNCE_MS{1000};
#endif

    // -- Hardware Boolean
    bool m_isInitialized;
    bool m_backgroundCaptured;

#ifdef HAVE_WIRINGPI
    /**
     * Hardware methods adopted from
     * https://github.com/arkandzprogaming/pcr-instrument-mproc.git
     *  
     */
    bool beginWiringPi();
    bool beginLedPwm();
    bool beginSensor();
    
    // -- LED control method
    void writeLedPwm(int intensity);

    // -- Sensor iteraction methods
    bool writeToSensor(cv::Mat &img);
#endif
        
public:
    explicit HardwareController(uint32_t camId = 0, int isrPin = 17, int ledPin = 18, QObject* parent = nullptr);
    ~HardwareController();

    // -- BH1750 sensor operation modes
    /*
    enum SensorMode : uint8_t {
        CONTINUOUSLY_H_RES_MODE = 0x10,
        CONTINUOUSLY_H_RES_MODE_2 = 0x11,
        CONTINUOUSLY_L_RES_MODE = 0x13,
        ONETIME_H_RES_MODE = 0x20,
        ONETIME_H_RES_MODE_2 = 0x21,
        ONETIME_L_RES_MODE = 0x23
    };
    */

public Q_SLOTS:
    bool begin();
    void setLedIntensity(int intensity);
    void performSensorReading(bool isBackground = false);
#ifdef USE_TIMER
    void startSensorReading(int maxCycle);
#endif
    void stopSensorReading();

private Q_SLOTS:
#ifdef HAVE_WIRINGPI
    double readMatrixFromSensor(cv::Mat &img, cv::Mat &backGreen);
#endif
#ifdef USE_TIMER
    void onSensorTimer();
#endif

Q_SIGNALS:
    void hardwareInitialized(bool success);
    void ledIntensityChanged(int intensity);
    void sensorDataReady(double msaMaxValue);
#ifdef HAVE_WIRINGPI
    void errorOccurred(const QString& error);
#endif
};
