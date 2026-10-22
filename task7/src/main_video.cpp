#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <opencv2/opencv.hpp>

const int boardRows = 6;
const int boardCols = 9;
const float squareSize = 10.0f;
cv::Size patternSize(boardCols, boardRows);
int main(){
    
    std::cout << "calibration board size is " << boardCols << "x" 
    << boardRows << " points, square size is " << squareSize << "cm." << std::endl;

    std::string video;
    std::cout << "Please input your calibration video path: " << std::flush;
    std::getline(std::cin, video);

    cv::Mat frame;
    cv::VideoCapture cap(video);

    if(!cap.isOpened()){
        std::cerr << "Error: Video is not exist!" << std::endl;
        return 0;
    }

    if(!cap.read(frame) || frame.empty()){
        std::cerr << "Error: The video is broken!" << std::endl;
        return 0;
    }


    std::vector<cv::Point3f> objp;
    for(int i = 0; i < boardRows; ++i){
        for(int j = 0; j < boardCols; ++j){
            objp.emplace_back(j * squareSize, i * squareSize, 0.0f);
        }
    }

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imgPoints;
    cv::Size imageSize;
    int successPic = 0;
    int count = 0;
    imageSize = frame.size();

    while(1){
        
        cv::Mat gr;
        cv::cvtColor(frame, gr, cv::COLOR_BGR2GRAY);

        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCornersSB(gr, patternSize, corners, 
        cv::CALIB_CB_NORMALIZE_IMAGE);

        if(found){
            cv::cornerSubPix(gr, corners, cv::Size(11, 11), cv::Size(-1, -1),
                             cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.001));

            imgPoints.push_back(corners);
            objectPoints.push_back(objp);
            successPic++;
            
            cv::drawChessboardCorners(frame, patternSize, corners, found);
            cv::imshow("corners", frame);
            cv::waitKey(30);
        }

    }

    cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat distCoeffs = cv::Mat::zeros(1, 5, CV_64F);
    std::vector<cv::Mat> rvecs, tvecs;
    double rms = cv::calibrateCamera
    (objectPoints, imgPoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs);

    double totalErr = 0.0;
    int totalpts = 0;
    std::vector<double> preViewErr;

    for(size_t i = 0; i < objectPoints.size(); ++i){
        std::vector<cv::Point2f> proj;
        cv::projectPoints(objectPoints[i], rvecs[i], tvecs[i],
                        cameraMatrix, distCoeffs, proj);
        
        double err = cv::norm(imgPoints[i], proj, cv::NORM_L2);
        err *= err;
        totalErr += err;
        totalpts += (int)objectPoints[i].size();
        preViewErr.push_back(std::sqrt(err / objectPoints[i].size()));

    }
    double meanErr = std::sqrt(totalErr / totalpts); 

    std::cout << "Calibration board size is " << boardCols << " * " << boardRows << ", "
    << "square length is " << squareSize << "cm" << std::endl;
    std::cout << "Image resolution is " << imageSize.width << " * " << imageSize.height << std::endl;
    std::cout << "K: " << cameraMatrix << std::endl;
    std::cout << "[k1, k2, p1, p2, p3]: " << distCoeffs << std::endl;
    std::cout << "RMS = " << std::fixed << std::setprecision(4) << rms << " pixel." << std::endl;
    std::cout << "Average reproject error is " << std::fixed << std::setprecision(4) << " pixel." << std::endl;

    for(size_t i = 0; i < preViewErr.size(); ++i){
        std::cout << "NO." << (i + 1) << " picture's reprojection error is " 
        << std::fixed << std::setprecision(4) << preViewErr[i] << " pixel" << std::endl;
    }

    std::cout << "Error is " << meanErr << std::endl;

    if(meanErr < 0.5){
        std::cout << "[comment] A (Error < 0.5)" << std::endl;
    }else if(meanErr <= 1.0){
        std::cout << "[comment] B (0.5 <= Error <= 1)" << std::endl;
    }else{
        std::cout << "[comment] C (error > 1)" << std::endl;
    }

    cv::FileStorage fs("opencv_camera_calibration2.yaml", cv::FileStorage::WRITE);
    fs << "image_width" << imageSize.width;
    fs << "image_height" << imageSize.height;
    fs << "camera_matrix" << cameraMatrix;
    fs << "distorition_coefficients" << distCoeffs;
    fs << "rms_error" << rms;
    fs << "mean_reprojection_error" << meanErr;
    fs << "square_size" << (double)squareSize;
    fs << "square_cols" << boardCols;
    fs << "square_rows" << boardRows;
    fs.release();
    std::cout << "Camera calibration has done!" << std::endl;

    cv::waitKey(300);
    cv::destroyAllWindows();
    return 0;

}