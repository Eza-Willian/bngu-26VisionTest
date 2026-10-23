#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <opencv2/opencv.hpp>

#include "vision_types.hpp"
#include "task_color.hpp"
#include "task_digit.hpp"
#include "task_shape.hpp"
#include "task_arrow.hpp"
#include "control.h"

const int TASK_MODE = 1;
const std::string TARGET_COLOR = "red";

static double normalization(double angle){
    if(std::isinf(angle)) angle = 0.0;
    angle = std::fmod(angle + 180.0, 360.0);
    if(angle < 0.0) angle += 360.0;
    return angle - 180.0;
}

int main(){
    try {
        if(!control::open("config/serial.yml")){
            std::cerr << "[Warning] Config not found. Running visual only." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Warning] Serial init failed: " << e.what() << ". Running visual only." << std::endl;
    }

    cv::VideoCapture cap(0);
    if(!cap.isOpened()){
        std::cerr << "[Error] Fail to open the capture!" << std::endl;
        return -1;
    }

    cv::Mat frame;
    control::Frame frame_data;
    unsigned int seq = 0;
    int period_ms = control::periodMs();
    auto next_time = std::chrono::steady_clock::now();

    std::cout << "Press ESC if you want to quit" << std::endl;

    while(1){
        if(!cap.read(frame) || frame.empty()) break;

        Vision::Result arrowRes = Vision::detectArrow(frame);

        Vision::Result targetRes;
        if(TASK_MODE == 1) {
            std::string color = TARGET_COLOR;
            targetRes = Vision::detectColor(frame, color);
        }else if(TASK_MODE == 2){
            targetRes = Vision::detectShape(frame);
        }else if(TASK_MODE == 3){
            targetRes = Vision::detectDigit(frame);
        }

        frame_data.seq = ++seq;
        if(targetRes.valid && arrowRes.valid){
            frame_data.valid = true;
            frame_data.hasAngle = true;
            frame_data.x = targetRes.x;
            frame_data.y = targetRes.y;
            frame_data.label = targetRes.label;
            
            double dx_target = targetRes.x - frame.cols / 2;
            double dy_target = targetRes.y - frame.rows / 2;

            double targetAngle = std::atan2(-dx_target, -dy_target) * 180.0 / CV_PI;
            while(targetAngle < 0) targetAngle += 360.0;

            double errorAngle = normalization(targetAngle - arrowRes.angle);

            if(std::abs(errorAngle) < 1.0){
                errorAngle = 0.0;
            }

            frame_data.angle = errorAngle;

            cv::putText(frame, "Target: " + targetRes.label, cv::Point(20, 40),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2);
            cv::putText(frame, "Target Angle: " + std::to_string((int)targetAngle),
                        cv::Point(20, 70), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2);
            cv::putText(frame, "Error: " + std::to_string((int)errorAngle) + " deg",
                        cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,255), 2);
            
            control::TurnAngle(frame_data, frame_data.angle);
        }else{
            frame_data.valid = false;
            frame_data.hasAngle = false;
            frame_data.label = "lost";

            cv::putText(frame, "Target Lost", cv::Point(20, 40),
                        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0,0,255), 2);

            control::Send(frame_data);
        }
        cv::imshow("Loop", frame);

        next_time += std::chrono::milliseconds(period_ms);
        std::this_thread::sleep_until(next_time);

        if(cv::waitKey(1) == 27) break;
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
    
}