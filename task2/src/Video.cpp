#include <iostream>
#include <opencv2/opencv.hpp>
#include <iomanip>

int video(){
    cv::TickMeter tm;
    cv::VideoCapture cap;
    std::string video_path;

    std::cout << "Please input your video path, press ESC to quit." << std::flush;
    std::getline(std::cin, video_path);
    tm.start();
    if(video_path.empty()) return 0;
    cap.open(video_path);
    if(!cap.isOpened()){
        std::cerr << "Error at opening the capture" << std::endl;
        return -1;
    }

    cv::Mat frame;
    long long frameCount = 1;
    
    if(!cap.read(frame) || frame.empty()){
        std::cerr << "Cannot read frame. Please try again." << std::endl;
        return -1;
    }

    double timestamp = cap.get(cv::CAP_PROP_POS_MSEC);

    int width = frame.cols;
    int height = frame.rows;
    double fps = cap.get(cv::CAP_PROP_FPS);

    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    std::string output ="output.mp4";

    cv::VideoWriter writer(output, fourcc, fps, cv::Size (width, height));
    if(!writer.isOpened()){
        std::cerr << "Cannot open VideoWriter!" << std::endl;
        return -1;
    }

    writer.write(frame);
    tm.stop();

    std::cout << "\r" 
    << "Frame number" << std::setw(5) << frameCount 
    << "| Timestamp" << std::fixed << std::setprecision(2) << timestamp << "ms"
    << "| Operation time" << std::fixed << std::setprecision(2) << tm.getTimeSec() << "s"
    << std::flush;

    ++frameCount;

    while(true){
        tm.reset();
        tm.start();
        if(!cap.read(frame)){
            std::cout << "\nStream ended." << std::endl;
            break;
        }
        if(frame.empty()) continue;

        double timestamp = cap.get(cv::CAP_PROP_POS_MSEC);

        tm.stop();

        std::cout << "\r" 
        << "Frame number" << std::setw(5) << frameCount 
        << "| Timestamp" << std::fixed << std::setprecision(2) << timestamp << "ms"
        << "| Operation time" << std::fixed << std::setprecision(2) << tm.getTimeSec() << "s"
        << std::flush;

        cv::imshow("Frame", frame);
        // cv::imwrite("Frame_" + std::to_string(frameCount), frame);
        writer.write(frame);
        ++frameCount;

        if(cv::waitKey(30) == 27) break;
    }

    writer.release();
    cap.release();
    cv::destroyAllWindows();
    return 0;

}