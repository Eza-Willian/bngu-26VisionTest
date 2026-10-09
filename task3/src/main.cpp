#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <string>
#include <vector>

int main(){
    std::string path;
    cv::Mat img, gr, hsv, binary, k1, temp, mask, dst;
    std::vector<std::vector<cv::Point>> contours; 
    cv::Rect r;
    int x, y;

    if(!std::getline(std::cin, path) || path.empty()){
        std::cerr << "Path error!" << std::endl;
        return 0;
    }
    img = cv::imread(path);
    if(img.empty()){
        std::cerr << "Error: The path is not exist!" << std::endl;
        return 0;
    }

    cv::cvtColor(img, gr, cv::COLOR_BGR2GRAY);
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);
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

    dst = img.clone();
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    for(size_t i = 0; i < contours.size(); i++){
        if(cv::contourArea(contours[i]) < 100) continue;
        r = cv::boundingRect(contours[i]);
        cv::rectangle(dst, r, cv::Scalar(255, 255, 0), 4);
        x = r.x + r.width / 2;
        y = r.y + r.height / 2;
        cv::arrowedLine(dst, cv::Point(x, y), cv::Point(x + 50, y), cv::Scalar(255, 0, 255), 2, cv::LINE_8, 0, 0.2);
        cv::arrowedLine(dst, cv::Point(x, y), cv::Point(x, y + 50), cv::Scalar(255, 0, 255), 2, cv::LINE_8, 0, 0.2);

    }
    cv::drawContours(dst, contours, -1, cv::Scalar(0, 255, 255), 3);
    cv::imshow("dst", dst);



    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;

}