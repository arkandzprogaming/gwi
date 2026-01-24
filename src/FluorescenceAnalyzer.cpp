#include "FluorescenceAnalyzer.hpp"
#include <algorithm> // For std::max, std::min
#include <limits>    // For numeric_limits

FluorescenceAnalyzer::FluorescenceAnalyzer(QObject *parent) : QObject(parent)
{
}

cv::Mat FluorescenceAnalyzer::extractGreenChannel(const cv::Mat &input)
{
    cv::Mat greenChannel;
    if (input.empty()) return cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC1);
    cv::extractChannel(input, greenChannel, 1);

    return greenChannel;
}

cv::Mat FluorescenceAnalyzer::subtractBackground(const cv::Mat &imageG, const cv::Mat &backgroundG)
{
    if (imageG.empty()) return imageG;
    
    cv::Mat bg = backgroundG;
    if (bg.empty()) {
        bg = cv::Mat::zeros(imageG.rows, imageG.cols, CV_8UC1);
    }

    cv::Mat result;
    cv::subtract(imageG, bg, result);
    return result;
}

double FluorescenceAnalyzer::pixelAverageAnalysis(const cv::Mat &gray, int section_rows, int section_cols)
{
    double max_section_avg = -1.0;

    // -- Safety checks
    if (gray.empty() || section_rows <= 0 || section_cols <= 0) {
        return 0.0;
    }

    // -- Divide image into sections
    int height = gray.rows;
    int width = gray.cols;
    
    int row_size = height / section_rows;
    int col_size = width / section_cols;

    // -- Loop through sections
    for (int i = 0; i < section_rows; ++i) {
        for (int j = 0; j < section_cols; ++j) {
            
            // Calculate boundaries
            int start_row = i * row_size;
            // Handle the last section taking the remainder of pixels
            // Logic: if i == section_rows - 1, end_row = height
            int end_row = (i == section_rows - 1) ? height : (i + 1) * row_size;

            int start_col = j * col_size;
            // Logic: if j == section_cols - 1, end_col = width
            int end_col = (j == section_cols - 1) ? width : (j + 1) * col_size;

            // Define Region of Interest (ROI)
            cv::Rect roi(start_col, start_row, end_col - start_col, end_row - start_row);
            
            // Extract section using ROI (no deep copy, very fast)
            cv::Mat section = gray(roi);

            // Calculate average for this section
            cv::Scalar sectionMean = cv::mean(section);
            double section_avg = sectionMean[0];

            // Update maximum section average
            if (section_avg > max_section_avg) {
                max_section_avg = section_avg;
            }
        }
    }

    return max_section_avg;
}
