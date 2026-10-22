#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <string>
#include <vector>

int operation(cv::Mat& frame){
    std::string path;
    cv::Mat gr, hsv, binary, k1, temp, mask;
    std::vector<std::vector<cv::Point>> contours; 
    cv::Rect r;
    int x, y;

    cv::cvtColor(frame, gr, cv::COLOR_BGR2GRAY);
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::imshow("gr", gr);
    cv::imshow("hsv", hsv); 

    cv::threshold(gr, binary, 150, 255, cv::THRESH_BINARY);
    cv::imshow("ex_binary", binary);
    cv::bitwise_not(binary, binary);
    cv::imshow("binary", binary);

    k1 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, temp, cv::MORPH_OPEN, k1);
    cv::morphologyEx(temp, mask, cv::MORPH_CLOSE, k1);
    cv::imshow("mask", mask);

    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    for(size_t i = 0; i < contours.size(); i++){
        if(cv::contourArea(contours[i]) < 100) continue;
        r = cv::boundingRect(contours[i]);
        cv::rectangle(frame, r, cv::Scalar(255, 255, 0), 4);
        x = r.x + r.width / 2;
        y = r.y + r.height / 2;
        cv::arrowedLine(frame, cv::Point(x, y), cv::Point(x + 50, y), cv::Scalar(255, 0, 255), 2, cv::LINE_8, 0, 0.2);
        cv::arrowedLine(frame, cv::Point(x, y), cv::Point(x, y + 50), cv::Scalar(255, 0, 255), 2, cv::LINE_8, 0, 0.2);

    }
    cv::drawContours(frame, contours, -1, cv::Scalar(0, 255, 255), 3);
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
            std::cout << "Stream is over" << std::endl;
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