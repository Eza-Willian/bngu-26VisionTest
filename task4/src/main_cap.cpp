#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

int operation(cv::Mat frame, const std::string color){
    cv::Mat hsv, mask, mask2, k;
    cv::Scalar lower, upper;
    bool isRed = false;
    std::vector<std::vector<cv::Point>> contours;
    cv::Rect r;
    int x, y;
    std::vector<cv::Point> centers;
    int valid = 0;

    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::imshow("hsv", hsv);

    if(color == "red" || color == "Red" || color == "RED"){
        lower = cv::Scalar (0, 100, 100);
        upper = cv::Scalar (10, 255, 255);
        isRed = true;
    }else if(color == "blue" || color == "Blue" || color == "BLUE"){
        lower = cv::Scalar(100, 100, 100);
        upper = cv::Scalar(130, 255, 255);
    }else if(color == "green" || color == "Green" || color == "GREEN"){
        lower = cv::Scalar(35, 100, 100);
        upper = cv::Scalar(85, 255, 255);
    }else if(color == "black" || color == "Black" || color == "BLACK"){
        lower = cv::Scalar(0, 0, 0);
        upper = cv::Scalar(180, 255, 50);
    }else{
        std::cerr << "Error: Not support this color!" << std::endl;
        std::cout << valid << std::endl;
        return 0;
    }

    cv::inRange(hsv, lower, upper, mask);

    if(isRed){
        cv::inRange(hsv, cv::Scalar(170, 100, 100), cv::Scalar(180, 255, 255), mask2);
        cv::bitwise_or(mask, mask2, mask);
    }

    k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, k);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);
    cv::imshow("mask", mask);

    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    for(size_t i = 0; i < contours.size(); ++i){
        if(cv::contourArea(contours[i]) < 100) continue;
        r = cv::boundingRect(contours[i]);
        cv::rectangle(frame, r, cv::Scalar(0, 255, 100), 2);

        x = r.x + r.width / 2;
        y = r.y + r.height / 2;
        centers.push_back(cv::Point(x, y));

        cv::circle(frame, cv::Point(x, y), 5, cv::Scalar(0, 255, 255), -1);
        std::string txt = "(" + std::to_string(x) + "," + std::to_string(y) + ")";
        cv::putText(frame, txt, cv::Point(r.x, y - 7),
        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
    }
    if(!centers.empty()) valid = 1;

    std::cout << "\r" << "valid = " << valid << std::flush;
    cv::imshow("frame", frame);
    return 0;

}

int main(){
    cv::VideoCapture cap(0);
    cv::Mat frame;
    std::string color;

    cap >> frame;
    if(!cap.isOpened() || frame.empty()){
        std::cerr << "Error: Fail to open capture!" << std::endl;
        return 0;
    }
    std::cout << "Choose one color: " << std::flush;
    std::cin >> color;
    operation(frame, color);

    while(1){
        if(!cap.read(frame)){
            std::cout << "Stream is over" << std::endl;
            break;
        }

        if(frame.empty()) continue;
        operation(frame, color);
        if(cv::waitKey(30) == 27) break;
    }
    cap.release();
    cv::destroyAllWindows();
    return 0;

}