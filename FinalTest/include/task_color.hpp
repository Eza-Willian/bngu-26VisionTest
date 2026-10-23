#pragma once
#include <opencv2/opencv.hpp>
#include "vision_types.hpp"

namespace Vision{
    inline Result detectColor(cv::Mat& frame, std::string& targetColor){
        Result res;
        cv::Mat hsv, mask, mask2;
        cv::Scalar lower, upper;
        bool isRed = false;
        std::vector<std::vector<cv::Point>> contours;
        cv::Rect r;
        int x, y;
        std::vector<cv::Point> centers;
        int valid = 0;

        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        if(targetColor == "red" || targetColor == "Red" || targetColor == "RED"){
            lower = cv::Scalar (0, 100, 100);
            upper = cv::Scalar (10, 255, 255);
            isRed = true;
        }else if(targetColor == "blue" || targetColor == "Blue" || targetColor == "BLUE"){
            lower = cv::Scalar(100, 100, 100);
            upper = cv::Scalar(130, 255, 255);
        }else if(targetColor == "green" || targetColor == "Green" || targetColor == "GREEN"){
            lower = cv::Scalar(35, 100, 100);
            upper = cv::Scalar(85, 255, 255);
        }else if(targetColor == "black" || targetColor == "Black" || targetColor == "BLACK"){
            lower = cv::Scalar(0, 0, 0);
            upper = cv::Scalar(180, 255, 50);
        }else{
            std::cerr << "Error: Not support this targetColor!" << std::endl;
            std::cout << valid << std::endl;
            return res;
        }

        cv::inRange(hsv, lower, upper, mask);

        if(isRed){
            cv::inRange(hsv, cv::Scalar(170, 100, 100), cv::Scalar(180, 255, 255), mask2);
            cv::bitwise_or(mask, mask2, mask);
        }

        static cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, k);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);

        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        double maxArea = 0.0;
        for(size_t i = 0; i < contours.size(); ++i){
            double area = cv::contourArea(contours[i]);
            r = cv::boundingRect(contours[i]);

            if(area > 100 && area > maxArea){
                cv::rectangle(frame, r, cv::Scalar(0, 255, 100), 2);

                x = r.x + r.width / 2;
                y = r.y + r.height / 2;
                centers.push_back(cv::Point(x, y));

                res.x = x;
                res.y = y;
                res.label = targetColor;
                res.valid = true;

            cv::circle(frame, cv::Point(x, y), 5, cv::Scalar(0, 255, 255), -1);
            std::string txt = "(" + std::to_string(x) + "," + std::to_string(y) + ")";
            cv::putText(frame, txt, cv::Point(r.x, y - 7),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
            }
        }
        return res;
    } 
}
