#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <iomanip>

int pic(){
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
}
