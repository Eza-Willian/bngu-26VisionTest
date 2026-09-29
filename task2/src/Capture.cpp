#include <iostream>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <string>

int capture(){
    cv::VideoCapture cap(0);
    cv::Mat frame;
    cv::TickMeter tm;
    double freq = cv::getTickFrequency();
    double t0 = (double)cv::getTickCount();

    tm.start();
    cap >> frame;
    if(!cap.isOpened() || frame.empty()){
        std::cerr << "Error: There is something wrong in opening capture!" << std::endl;
        return 0;
    }

    long long frameCount = 1;
    double timestamp = 0.0;

    int width = frame.cols;
    int height = frame.rows;
    double fps = 30;
    std::string output = "Cap_video.mp4";
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');

    cv::VideoWriter writer(output, fourcc, fps, cv::Size (width, height));
    writer.write(frame);
    timestamp = ((double)cv::getTickCount() - t0) / freq * 1000.0;
    tm.stop();

    std::cout << "\r"
    << "Frame number: " << std::setw(5) << frameCount
    << "| Timestamp: " << std::fixed << std::setprecision(2) << timestamp << "ms"
    << "| Operation time" << std::fixed << std::setprecision(2) << tm.getTimeMilli() << "ms"
    <<std::flush;
    
    ++frameCount;

    while(true){
        tm.reset();
        tm.start();
        if(!cap.read(frame)){
            std::cout << "Stream ended!" << std::endl;
            break;
        }
        if(frame.empty()) continue;

        timestamp = ((double)cv::getTickCount() - t0) / freq * 1000.0;
        tm.stop();

        std::cout << "\r"
        << "Frame number: " << std::setw(5) << frameCount
        << "| Timestamp: " << std::fixed << std::setprecision(2) << timestamp << "ms"
        << "| Operation time: " << std::fixed << std::setprecision(2) << tm.getTimeMilli() << "ms"
        << std::flush;

        cv::imshow("Frame", frame);
        writer.write(frame);
        ++frameCount;

        if(cv::waitKey(30) == 27) break;
    }

    cap.release();
    writer.release();
    cv::destroyAllWindows();
    return 0;
}