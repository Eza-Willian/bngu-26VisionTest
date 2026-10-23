#pragma once
#include <opencv2/opencv.hpp>
#include "vision_types.hpp"

namespace Vision{
    inline Result detectShape(cv::Mat& frame){
        Result res;
        cv::Mat gr, binary, mask;
        std::vector<std::vector<cv::Point>> contours;

        cv::cvtColor(frame, gr, cv::COLOR_BGR2GRAY);
        cv::threshold(gr, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        static cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(binary, mask, cv::MORPH_OPEN, k);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);

        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        double maxArea = 0.0;
        for(size_t i = 0; i < contours.size(); ++i){
            double area = cv::contourArea(contours[i]);
            if(area < 100 || area <= maxArea) continue;

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
                res.label = "circle";
            }else if(vtx == 3){
                res.label = "triangle";
            }else if(vtx == 4){
                res.label = "rectangle";
            }else continue;

            res.valid = true;
            res.x = cx;
            res.y = cy;

            cv::drawContours(frame, contours, (int)i, cv::Scalar(0, 255, 0), 2);
            cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(255, 0, 0), -1);
            std::string coordinates = "(" + std::to_string(cx) + " , " + std::to_string(cy) + ")";
            cv::putText(frame, res.label, cv::Point(cx - 20, cy - 15),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
            cv::putText(frame, coordinates, cv::Point(cx + 5, cy - 15),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);

        }
        return res;
    }
}