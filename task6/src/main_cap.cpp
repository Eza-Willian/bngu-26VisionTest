#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <opencv2/opencv.hpp>

bool Isomorphic(const cv::Mat& input, cv::Mat& output) {
    // 输入为空，直接返回失败
    if (input.empty()) return false;

    std::vector<cv::Point> nzPts;
    cv::findNonZero(input, nzPts);
    if (nzPts.empty()) return false;   // ★ 把失败信号交给调用方

    cv::Rect tight = cv::boundingRect(nzPts);
    cv::Mat cropped = input(tight).clone();

    int sz = std::max(cropped.rows, cropped.cols);
    int pad = std::max(sz / 5, 1);
    sz += pad * 2;

    cv::Mat canvas = cv::Mat::zeros(sz, sz, CV_8UC1);
    int y0 = (sz - cropped.rows) / 2;
    int x0 = (sz - cropped.cols) / 2;
    cropped.copyTo(canvas(cv::Rect(x0, y0, cropped.cols, cropped.rows)));

    cv::resize(canvas, output, cv::Size(20, 20), 0, 0, cv::INTER_NEAREST);

    return true;
}

int operation(cv::Mat& frame){
    cv::Mat gr, k, binary, mask;
    std::vector<cv::Mat> templates;
    std::vector<int> labels;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    for(int i = 0; i <= 9; i++){
        std::string nPath = "source/" + std::to_string(i) + ".png";
        cv::Mat nPic = cv::imread(nPath, cv::IMREAD_GRAYSCALE);

        if(nPic.empty()){
            std::cerr << "Warning: " << nPath << " is not exist!";
            continue;
        }

        cv::Mat temp;
        cv::threshold(nPic, temp, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        double wr = (double)cv::countNonZero(temp) / (temp.rows * temp.cols);
        if (wr > 0.5) cv::bitwise_not(temp, temp);

        if(!Isomorphic(temp, temp)) continue;

        templates.push_back(temp);
        labels.push_back(i);
    }

    cv::cvtColor(frame, gr, cv::COLOR_BGR2GRAY);
    cv::imshow("gr", gr);

    cv::threshold(gr, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    cv::imshow("binary", binary);

    k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
    cv::morphologyEx(binary, mask, cv::MORPH_OPEN, k);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);
    cv::imshow("mask", mask);

    cv::findContours(mask, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    for(size_t i = 0; i < contours.size(); ++i){
        double area = cv::contourArea(contours[i]);
        if(area <= 500 || area >= 50000) continue;

        int parent = hierarchy[i][3];
        if(parent == -1) continue;

        int grandparent = hierarchy[parent][3];
        if(grandparent == -1) continue;

        int greatgrandparent = hierarchy[grandparent][3];
        if(greatgrandparent != -1) continue;

        cv::Rect r = cv::boundingRect(contours[i]);
        cv::Mat Roi = mask(r);

        if(!Isomorphic(Roi, Roi)) continue;
        cv::imshow("Roi", Roi);
        // cv::imwrite("Roi.png", Roi);

        double maxConfidence = -1.0;
        int bestLable = -1;

        for(size_t j = 0; j < templates.size(); ++j){
            cv::Mat result;
            cv::matchTemplate(Roi, templates[j], result, cv::TM_CCOEFF_NORMED);

            double minVal, maxVal;
            cv::minMaxLoc(result, &minVal, &maxVal);

            /*std::cout << "  Template " << labels[j] << " -> " 
            << std::fixed << std::setprecision(3) << maxVal << std::endl;*/

            if(maxVal > maxConfidence){
                maxConfidence = maxVal;
                bestLable = labels[j];
            }
        }
        cv::Moments m = cv::moments(contours[i]);
        double cx = m.m10 / m.m00;
        double cy = m.m01 / m.m00;

        if(maxConfidence < 0.5){
            std::cout << "Fail to match!" << std::endl;
            std::cout << "Confidence is " << 
            std::fixed << std::setprecision(2) << maxConfidence << std::endl;
        }else{
            std::string text = std::to_string(bestLable) + " " 
                               + "(" + std::to_string(cx) + " , " + std::to_string(cy) + ") "
                               + std::to_string(maxConfidence);
            cv::circle(frame, cv::Point(cx, cy), 3, cv::Scalar(0, 0, 255), -1);
            cv::putText(frame, text, cv::Point(cx, cy - 3),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 0, 0), 2);
            std::cout << "Number is " << bestLable << 
            " | Center is (" << std::fixed << std::setprecision(2) << cx << 
            ", " << std::fixed << std::setprecision(2) << cy << ")" <<
            " | Confidence is " << std::fixed << std::setprecision(2) << 
            maxConfidence << std::endl;
        }

    }
    cv::imshow("frame", frame);
    return 0;

}

int main(){
    cv::VideoCapture cap(0);
    cv::Mat frame;

    cap >> frame;
    if(!cap.isOpened() || frame.empty()){
        std::cerr << "Error: Fail to open the capture!" << std::endl;
        return 0;
    }
    operation(frame);

    while(1){
        if(!cap.read(frame)){
            std::cout << "Video stream is over!" << std::endl;
            break;
        }
        if(frame.empty()) break;

        operation(frame);
        if(cv::waitKey(30) == 27) break;
    }
    cap.release();
    cv::destroyAllWindows();
    return 0;

}