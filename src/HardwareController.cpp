#include <QDebug>
#include <QThread>

#include <cmath>

#ifdef HAVE_WIRINGPI
#include <wiringPi.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/*
#ifdef HAVE_LCCV
// #include <linux/i2c-dev.h>
// #include <sys/ioctl.h>
#endif
*/

#include "HardwareController.hpp"
#include "FluorescenceAnalyzer.hpp"
#include "DataManager.hpp"

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

#ifdef HAVE_WIRINGPI
HardwareController* HardwareController::s_instance = nullptr;
std::chrono::steady_clock::time_point HardwareController::s_lastIsrTime{};
volatile int HardwareController::m_successInterruptCallbacks = 0;
volatile int HardwareController::m_debouncedInterruptCallbacks = 0;
#endif

/**
 * Constructor : Initialize ledPin and other PWM parameters
 * @param <int> ledPin BCM pin of PWM0 (use `gpio readall`)
 *
 */
HardwareController::HardwareController(uint32_t camId, int isrPin, int ledPin, QObject* parent)
    : QObject(parent)
    , m_camId(camId)
    , m_isrPin(isrPin)
    , m_ledPin(ledPin)
    , m_ledClockDivisor(640)
    , m_ledPwmRange(101)    // To accomodate for 0-100 integer value range from the slider
    , m_currentIntensity(0)
    , m_pcrCycle(0)
    , m_maxPcrCycle(0)
    , m_isInitialized(false)
    , m_backgroundCaptured(false)
{

#ifdef HAVE_WIRINGPI
    m_cam = new lccv::PiCamera(m_camId);

    m_cam->options->video_width=FluorescenceAnalyzer::WIDTH;
    m_cam->options->video_height=FluorescenceAnalyzer::HEIGHT;
    m_cam->options->framerate=5;
    m_cam->options->verbose=true;

    //m_cam->options->shutter=120000.0f;

    m_cam->options->setWhiteBalance(WhiteBalance_Modes::WB_CUSTOM);
    m_cam->options->awb_gain_r = 0.7f; 
    m_cam->options->awb_gain_b = 0.6f;
    m_cam->options->contrast = 0.8f;

    m_backGreen = cv::Mat::zeros(FluorescenceAnalyzer::HEIGHT, FluorescenceAnalyzer::WIDTH, CV_32FC1);

    s_instance = this;
#endif

#ifdef USE_TIMER
    // Using sensor timer to displace DMF communication methods (for debugging)
    m_sensorTimer = new QTimer(this);
    m_sensorTimer->setInterval(90000); 
    connect(m_sensorTimer, &QTimer::timeout, this, &HardwareController::onSensorTimer);
#endif

    begin();
}

/**
 * Destructor : Closes I2C file and turns LED down upon program exit
 *
 */
HardwareController::~HardwareController()
{
#ifdef HAVE_WIRINGPI
    if (m_cam != nullptr)
    {
        delete m_cam;
    }

    if (m_isInitialized) 
    {
        writeLedPwm(0);
        wiringPiISRStop(m_isrPin);
    }

    s_instance = nullptr;
#endif

#ifdef USE_TIMER
    if (m_sensorTimer) {
        m_sensorTimer->stop();
    }
#endif
}

#ifdef HAVE_WIRINGPI
void HardwareController::isrCallback()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - s_lastIsrTime;
    
    if (elapsed < ISR_DEBOUNCE_MS) {
        ++m_debouncedInterruptCallbacks;
        qDebug() << "HardwareController: Interrupt from DMF debounced:" << m_successInterruptCallbacks << ". Moving on";
        return;
    }
    
    if (s_instance) {
        // Update last call time
        s_lastIsrTime = now;
        ++m_successInterruptCallbacks;

        qDebug() << "HardwareController: Interrupt from DMF detected:" << m_successInterruptCallbacks << ". Performing acquisition...";
        QMetaObject::invokeMethod(s_instance, [=]() {
            s_instance->performSensorReading(false);
        }, Qt::QueuedConnection);
    }

}

/**
 * Public Slot : Calls all hardware initializer methods
 * @return <bool> true if all initializers successfully executed, false otherwise
 *
 */
bool HardwareController::begin()
{
#ifndef USE_TIMER
    qDebug() << "HardwareController: Initializing in ISR mode...";
#else
    qDebug() << "HardwareController: Initializing in Timer mode...";
#endif
    if (!beginWiringPi()) {
        return false;
    }
    
    if (!beginLedPwm()) {
        return false;
    }
    
    if (!beginSensor()) {
        return false;
    }
    
    m_isInitialized = true;
    Q_EMIT hardwareInitialized(true);
    qDebug() << "HardwareController: Initialization complete";
    
    return true;
}

/**
 * Private Method : Initializes wiringPi
 * @return <bool> true if initializing successfully executed, false otherwise
 *
 */
bool HardwareController::beginWiringPi()
{
    if (wiringPiSetupPinType(WPI_PIN_BCM) == -1) {
        qFatal("HardwareController: Failed to initialize wiringPi");
    }

#ifndef USE_TIMER
    if (wiringPiISR(m_isrPin, INT_EDGE_RISING, &HardwareController::isrCallback) < 0) {
        qFatal("HardwareController: Failed to setup ISR on pin %d", m_isrPin);
    }
    qDebug() << "HardwareController: ISR configured on GPIO" << m_isrPin;
    qDebug() << "HardwareController: m_successInterruptCallbacks initialized at" << m_successInterruptCallbacks;
#else
    qDebug() << "HardwareController: Using timer mode, ISR not configured";
#endif
    return true;
}

/**
 * Private Method : Initializes LED and PWM
 * @return <bool> true if initializing successfully executed
 *
 */
bool HardwareController::beginLedPwm()
{
    pinMode(m_ledPin, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(m_ledPwmRange);
    pwmSetClock(m_ledClockDivisor);
    
    writeLedPwm(0);
    
    qDebug() << "HardwareController: LED initialized on pin" << m_ledPin;
    return true;
}

/**
 * Private Method : Initializes CMOS sensor using LCCV in video mode
 * @return <bool> true if initializing successfully executed, false otherwise
 *
 */
bool HardwareController::beginSensor()
{
    /*
    if (ioctl(m_i2cFd, I2C_SLAVE, m_sensorAddr) < 0) {
        qDebug() << "HardwareController: Failed to connect to sensor at address" << Qt::hex << m_sensorAddr;
        close(m_i2cFd);
        m_i2cFd = -1;
        return false;
    }
    */

    qDebug() << "HardwareController: CMOS sensor starting...";
    if (!m_cam->startVideo())
    {
        qFatal("HardwareController: Failed to open and start CMOS sensor. ID: %d. Abort", m_camId);
    }
    
    sleep_for(milliseconds(2000));
    qDebug() << "HardwareController: CMOS sensor started";
    return true;
}
#else
bool HardwareController::begin()
{
    qDebug() << "HardwareController: Initializing...";

    m_isInitialized = true;
    Q_EMIT hardwareInitialized(true);
    qDebug() << "HardwareController: Initialization complete";

    return true;
}
#endif

#ifdef HAVE_WIRINGPI
void HardwareController::setLedIntensity(int intensity)
{
    if (!m_isInitialized) {
        qDebug() << "HardwareController: Hardware not initialized";
        return;
    }
    
    writeLedPwm(intensity);
    m_currentIntensity = intensity;
    
    Q_EMIT ledIntensityChanged(intensity);    // Currently unused
    qDebug() << "HardwareController: LED intensity set to" << m_currentIntensity << "%";
}

void HardwareController::writeLedPwm(int intensity)
{
    pwmWrite(m_ledPin, intensity);
}
#else
void HardwareController::setLedIntensity(int intensity)
{
    m_currentIntensity = intensity;
}
#endif

#ifdef USE_TIMER
void HardwareController::startSensorReading(int maxCycle)
{
    if (m_isInitialized) {
        if (maxCycle >= 0) {
            m_maxPcrCycle = maxCycle;
            m_pcrCycle = 0;
        } else {
            qFatal("HardwareController: m_maxPcrCycle must not be %d. Abort", maxCycle);
        }
        m_sensorTimer->start();
        qDebug() << "HardwareController: Started sensor reading";
    }
}
#endif

void HardwareController::stopSensorReading()
{
#ifndef USE_TIMER
    m_successInterruptCallbacks = 0;
    m_debouncedInterruptCallbacks = 0;
    qDebug() << "HardwareController: Stop button pressed in ISR mode. m_successInterruptCallbacks reset to" << m_successInterruptCallbacks;
#else
    m_sensorTimer->stop();
    qDebug() << "HardwareController: Stopped sensor reading";
#endif
}

#ifdef USE_TIMER
void HardwareController::onSensorTimer()
{
    if (m_pcrCycle++ < m_maxPcrCycle) {
            performSensorReading();
            qDebug() << "HardwareController: pcrCycle:" << m_pcrCycle;
    }
    else {
        qDebug() << "HardwareController: Acquisition amount met DataManager::m_maxCycle spec";
        stopSensorReading();
    }
}
#endif

#ifdef HAVE_WIRINGPI
/**
 * Public Method : Uses LCCV to capture a video frame immediately after a hardware interrupt is detected from DMF
 * @param <bool> isBackground true if background captured during setup, false otherwise
 * @return <void>
 *
 */
void HardwareController::performSensorReading(bool isBackground)
{
    if (!m_isInitialized) {
        return;
    }
    
    // -- Capture background frame using LCCV (during Setup)
    if (isBackground)
    {
        m_backgroundCaptured = true;

        cv::Mat backgroundFrame;
        if (!writeToSensor(backgroundFrame)) {
            Q_EMIT errorOccurred("Failed to write to sensor (isBackground)");
            return;
        }

        m_backGreen = FluorescenceAnalyzer::extractGreenChannel(backgroundFrame);
    } 
    else
    {
        if (!m_backgroundCaptured) qDebug() << "HardwareController: Warning: Background not captured during setup. Using zeros instead";
        // -- Capture frame using LCCV and OpenCV
        cv::Mat frame;
        if (!writeToSensor(frame)) {
            Q_EMIT errorOccurred("Failed to write to sensor");
            return;
        }
    
        // -- Read sensor data using OpenCV
        double msaMaxValue = readMatrixFromSensor(frame, m_backGreen);
        if (msaMaxValue >= 0) {
            Q_EMIT sensorDataReady(msaMaxValue);
        } else {
            qFatal("HardwareController: Illegal: msaMaxValue from OpenCV is in the negatives. Abort");
            return;
        }
    }
}

/**
 * Private Method : Uses LCCV to capture a video frame and processes it in an 8-bit cv::Mat BGR matrix. Can be used either to capture fluorescence or background image.
 * @param <cv::Mat> img BGR Matrix of 8-bit captured by LCCV's video frame capture
 * @return <bool> true If frame successfully captured, false otherwise
 *
 */
bool HardwareController::writeToSensor(cv::Mat &img)
{
    if (!m_cam->getVideoFrame(img, 1000)) {
        qDebug() << "HardwareController: Failed to write to sensor";
        return false;
    }
    return true;
}

/**
 * Private Method : Performs quantification of input BGR matrix using the MSA algorithm (OpenCV)
 * @param <cv::Mat> img BGR Matrix of 8-bit captured by LCCV
 * @param <cv::Mat> backGreen G Matrix of 8-bit captured by LCCV; processed by OpenCV
 * @return <float> msaMaxValue Maximum value detected by MSA using OpenCV
 *
 */
double HardwareController::readMatrixFromSensor(cv::Mat &img, cv::Mat &backGreen)
{
    /*
    unsigned char data[2];
    
    if (read(m_i2cFd, data, 2) != 2) {
        qDebug() << "HardwareController: Failed to read from sensor";
        return -1.0f;
    }
    
    uint16_t raw = (data[0] << 8) | data[1];
    float lux = raw / 1.2f;
    */
    
    // -- Extract G-channel
    cv::Mat green = FluorescenceAnalyzer::extractGreenChannel(img);

    // -- Subtract background values
    cv::Mat processed = FluorescenceAnalyzer::subtractBackground(green, backGreen);

    // -- Analyze
    double msaMaxValue = FluorescenceAnalyzer::pixelAverageAnalysis(processed, 6, 8);

    qDebug() << "HardwareController: ROI avg. value detected by MSA:" << msaMaxValue << "/ 1";
    return msaMaxValue;
}
#else
void HardwareController::performSensorReading(bool isBackground)
{
    if (!m_isInitialized) {
        return;
    }
    
    double msaMaxValue;
    if (m_pcrCycle <= 24) msaMaxValue = pow(2, m_pcrCycle) * 0.00001;
    else msaMaxValue = 187 - pow(0.5, (m_pcrCycle - 28));
    if (msaMaxValue >= 0) {
        Q_EMIT sensorDataReady(msaMaxValue);
    }
}
#endif
