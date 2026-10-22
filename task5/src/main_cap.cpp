#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <opencv2/opencv.hpp>

int operation(cv::Mat& frame){
    std::string shape;
    cv::Mat gr, binary, k, mask;
    std::vector<std::vector<cv::Point>> contours;
    int valid = 0;

    cv::cvtColor(frame, gr, cv::COLOR_BGR2GRAY);
    cv::threshold(gr, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    cv::imshow("gr", gr);
    cv::imshow("binary", binary);

    k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, mask, cv::MORPH_OPEN, k);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);
    cv::imshow("mask", mask);

    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for(size_t i = 0; i < contours.size(); ++i){
        double area = cv::contourArea(contours[i]);
        if(area < 100) continue;

        cv::Moments m = cv::moments(contours[i]);
        if(m.m00 == 0) continue;
        double cx = m.m10 / m.m00;
        double cy = m.m01 / m.m00;

        double peri = cv::arcLength(contours[i], true);
        if(peri < 1e-6) continue;
        double circularity = 4.0 * CV_PI * area / (peri * peri);

        std::vector<cv::Point> approx;
        cv::approxPolyDP(contours[i], approx, 0.04 * peri, true);
        int vtx = (int)approx.size();

        if(vtx > 6 || (circularity > 0.85 && vtx > 4)){
            shape = "circle";
        }else if(vtx == 3){
            shape = "triangle";
        }else if(vtx == 4){
            shape = "rectangle";
        }else continue;

        valid++;

        cv::drawContours(frame, contours, (int)i, cv::Scalar(0, 255, 0), 2);
        cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(255, 0, 0), -1);
        std::string coordinates = "(" + std::to_string(cx) + " , " + std::to_string(cy) + ")";
        cv::putText(frame, shape, cv::Point(cx - 20, cy - 15),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
        cv::putText(frame, coordinates, cv::Point(cx + 5, cy - 15),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);

        std::cout << "shape: " << std::setw(10) << shape << 
        " | center: " << "(" << std::fixed << std::setprecision(2) << cx << "," <<
        std :: fixed << std::setprecision(2) << cy << ")" << std::endl;
    }

    if(valid == 0){
        std::cout << "\r" << "Nothing has been recognized!" << std::flush;
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
            std::cout << "Capture stream is over!" << std::endl;
            break;
        }
        if(frame.empty()) continue;
        operation(frame);
        if(cv::waitKey(30) == 27) break;
    }
    cap.release();
    cv::destroyAllWindows();
    return 0;

}