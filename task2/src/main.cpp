#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <iomanip>

int main(){
    std::string mode;
    std::cout << "Please choose your mode: " << std::flush;
    std::getline(std::cin, mode);
    if(mode == "pic" || mode == "Pic"){
        cv::Mat src;
        cv::TickMeter tm;
        std::string src_path, output;
        int count = 1;
        while(true){
            tm.start();
            if(!std::getline(std::cin, src_path)) break;
            if(src_path.empty()) break;
            src = cv::imread(src_path);
            if(src.empty()){
                std::cout << "Read failed, retry. Press Enter to exit." << std::endl;
                continue;
            }

            std::string ext = ".jpg";
            if(src_path.find_last_of('.') != std::string::npos){
                ext = src_path.substr(src_path.find_last_of('.'));
            }
            output = "src_pic_" + std::to_string(count) + ext;
            cv::imshow(output, src);
            cv::imwrite(output, src);
            tm.stop();
            cv::waitKey(0);
            cv::destroyAllWindows();

            ++count;
        }
        std::cout << std::fixed << std::setprecision(2) <<tm.getTimeSec() << "s" <<std::endl;
        return 0;
    }else if(mode == "video" || mode == "Video"){
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

    }else if(mode == "capture" || mode == "Capture"){
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
    }else{
        std::cerr << "Input error!" << std::endl;
    }

    return 0;
}
