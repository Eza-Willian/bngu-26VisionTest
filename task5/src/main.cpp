#include  <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <opencv2/opencv.hpp>

int main(){
    std::string path, shape;
    cv::Mat src, gr, binary, k, mask, dst;
    std::vector<std::vector<cv::Point>> contours;
    int valid = 0;

    std::cout << "Input your picture path: " << std::flush;
    if(!std::getline(std::cin, path) || path.empty()){
        std::cerr << "Error: Path is not exist!" << std::endl;
        return 0;
    }

    src = cv::imread(path);
    if(src.empty()){
        std::cerr << "Error: Your picture has broken or not existed!" << std::endl;
        return 0;
    }
    cv::imshow("src", src);

    cv::cvtColor(src, gr, cv::COLOR_BGR2GRAY);
    cv::threshold(gr, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    cv::imshow("gr", gr);
    cv::imshow("binary", binary);

    k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, mask, cv::MORPH_OPEN, k);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);
    cv::imshow("mask", mask);

    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    dst = src.clone();
    for(size_t i = 0; i < contours.size(); ++i){
        double area = cv::contourArea(contours[i]);
        if(area < 100) continue;

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
            shape = "circle";
        }else if(vtx == 3){
            shape = "triangle";
        }else if(vtx == 4){
            shape = "rectangle";
        }else continue;

        valid++;
        std::cout << "shape: " << std::setw(10) << shape << 
        " | center: " << "(" << std::fixed << std::setprecision(2) << cx << "," <<
        std :: fixed << std::setprecision(2) << cy << ")" << std::endl;
    }

    if(valid == 0){
        std::cout << "Nothing has been recognized!" << std::endl;
    }

    cv::imshow("dst", dst); 
    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;

}