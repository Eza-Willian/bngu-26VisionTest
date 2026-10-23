#pragma once
#include <opencv2/opencv.hpp>
#include "vision_types.hpp"

namespace Vision{
    inline Result detectArrow(cv::Mat& frame){
        Result res;
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        std::vector<cv::Mat> channel;
        cv::split(hsv, channel);
        cv::Mat s_channel = channel[1], v_channel = channel[2];

        cv::Mat binary, mask1, mask2;
        cv::threshold(v_channel, mask1, 200, 255, cv::THRESH_BINARY);
        cv::threshold(s_channel, mask2, 100, 255, cv::THRESH_BINARY);
        cv::bitwise_not(mask2, mask2);
        cv::bitwise_and(mask1, mask2, binary);

        static cv::Mat k1 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        static cv::Mat k2 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
        // cv::Mat k3 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
        cv::morphologyEx(binary, binary, cv::MORPH_OPEN, k2);



        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        if(contours.empty()){
            return res;
        }
        cv::drawContours(frame, contours, -1, cv::Scalar(0, 0, 255));

        double maxArea = 0;
        int bestIdx = -1;
        for(size_t i = 0; i < contours.size(); ++i){
            double area = cv::contourArea(contours[i]);
            if(area > maxArea){
                maxArea = area;
                bestIdx = (int)i;
            }
        }
        std::vector<cv::Point> arrowContour = contours[bestIdx];

        cv::Moments m = cv::moments(arrowContour);
        cv::Point2f messCenter(m.m10 / m.m00, m.m01 / m.m00);

        cv::RotatedRect minRect = cv::minAreaRect(arrowContour);
        cv::Point2f rect_point[4];
        minRect.points(rect_point);

        cv::Point2f arrowTip = rect_point[0];
        double maxDistToMess = 0;
        for(int i = 0; i < 4; ++i){
            double dist = cv::norm(rect_point[i] - messCenter);
            if(dist > maxDistToMess){
                maxDistToMess = dist;
                arrowTip = rect_point[i];
            }
        }

        cv::Point2f rotateCenter = minRect.center;
        double dx = arrowTip.x - rotateCenter.x;
        double dy = arrowTip.y - rotateCenter.y;
        double angle_rad = std::atan2(-dx, -dy);
        double angle_deg = angle_rad * 180.0 / CV_PI;
        while(angle_deg < 0){
            angle_deg += 360.0;
        }

        res.x = rotateCenter.x;
        res.y = rotateCenter.y;
        res.angle = angle_deg;
        res.label = "arrow";
        res.valid = true;

        std::string text1 = "Rotate center: ( " + std::to_string(rotateCenter.x) + " , " + std::to_string(rotateCenter.y) + " )";
        std::string text2 = "Arrow Tip: ( " + std::to_string(arrowTip.x) + " , " + std::to_string(arrowTip.y) + " )\n" ;
        std::string text3 = "Arrow direction: " + std::to_string(angle_deg);
        
        cv::putText(frame, text1, cv::Point(20, 140), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 2);
        cv::putText(frame, text2, cv::Point(20, 160), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 2);
        cv::putText(frame, text3, cv::Point(20, 180), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 2);
        cv::circle(frame, cv::Point(rotateCenter), 4, cv::Scalar(0, 0, 255), -1);
        return res;
    }
}
