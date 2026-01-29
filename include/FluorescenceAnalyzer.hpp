#ifndef FLUORESCENCEANALYZER_H
#define FLUORESCENCEANALYZER_H

#include <QObject>
#include <QVariantMap>
#ifdef HAVE_LCCV
#include <opencv2/opencv.hpp>
#endif

class FluorescenceAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit FluorescenceAnalyzer(QObject *parent = nullptr);
    
    static constexpr int HEIGHT = 600;
    static constexpr int WIDTH = 800;

#ifdef HAVE_LCCV
    static cv::Mat normalizeRange(const cv::Mat &input);
    static cv::Mat extractGreenChannel(const cv::Mat &input);
    static cv::Mat subtractBackground(const cv:: Mat &imageG, const cv:: Mat &backgroundG);
    static double pixelAverageAnalysis(const cv::Mat &gray, int section_rows = 3, int section_cols = 4);
#endif
};

#endif // FLUORESCENCEANALYZER_H
