#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>

int main(){
    cv::Mat cameraMatrix, distCoeffs;
    cv::FileStorage fs("source/opencv_camera_calibration.yaml", cv::FileStorage::READ);
    if(!fs.isOpened()){
        std::cerr << "Error: Fail to open the file!" << std::endl;
        return 0;
    }

    fs["camera_matrix"] >> cameraMatrix;
    fs["distortion_coefficients"] >> distCoeffs;
    fs.release();

    const float tagSize = 12.0f;
    const float spacing = 20.0f;
    const int expectTagCount = 4;

    const float half = tagSize / 2.0f;
    std::vector<cv::Point3f> singleTagObject = {
        cv::Point3f(- half, - half, 0),
        cv::Point3f(half, - half, 0),
        cv::Point3f(half, half, 0),
        cv::Point3f(- half, half, 0)
    };

    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_APRILTAG_36h11);
    cv::Ptr<cv::aruco::DetectorParameters> pramas = cv::aruco::DetectorParameters::create();
    pramas -> cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

    std::string path;
    std::cout << "Input your path:" << std::flush;
    std::getline(std::cin, path);
    cv::Mat src = cv::imread(path);
    if(src.empty()){
        std::cerr << "Error: Your picture is broken or empty!" << std::endl;
        return 0;
    }

    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners, rejected;
    cv::aruco::detectMarkers(src, dictionary, corners, ids, pramas, rejected);

    cv::Mat output = src.clone();

    std::cout << "Find " << ids.size() << " AprilTags." << std::endl;
    if(ids.empty()){
        std::cout << "No detect tag" << std::endl;
    }else{
        bool isComplete = true;
        for(const auto& c : corners){
            for(const auto& pt : c){
                if(pt.x <= 0 || pt.y <= 0 || pt.x >= src.cols || pt.y >= src.rows){
                    isComplete = false;
                    break;
                }
            }
            if(!isComplete) break;
        }
        cv::aruco::drawDetectedMarkers(output, corners, ids);

        if(isComplete){
            std::cout << "Board is visible." << std::endl;

            double maxArea = 0;
            int bestId = 0;

            for(size_t i = 0; i < corners.size(); ++i){
                double area = cv::contourArea(corners[i]);
                if(area > maxArea){
                    maxArea = area;
                    bestId = (int)i;
                }
            }

            cv::Mat rvec, tvec;
            cv::solvePnP(singleTagObject, corners[bestId], cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
            cv::drawFrameAxes(output, cameraMatrix, distCoeffs, rvec, tvec, tagSize * 1.0f);
            
            cv::Mat R;
            cv::Rodrigues(rvec, R);
            double sy = std::sqrt(R.at<double>(0, 0) * R.at<double>(0, 0) + 
            R.at<double>(0, 1) * R.at<double>(0, 1));
            bool singular = sy < 1e-6;
            double roll, pitch,yaw;

            if(!singular){
                roll = std::atan2(R.at<double>(2, 1), R.at<double>(2, 2));
                yaw = std::atan2(R.at<double>(1, 0), R.at<double>(0, 0));
                pitch = std::atan2(-R.at<double>(2, 0), sy);
            }else{
                pitch = std::atan2(-R.at<double>(2, 0), sy);
                roll = std::atan2(-R.at<double>(1, 2), -R.at<double>(1, 1));
                yaw = 0;
            }
            const double RAD2DEG = 180.0 / CV_PI;
            std::cout << "Board pose: " << std::endl;
            std::cout << std::setw(8) << "Pitch = " << std::setprecision(2) << pitch * RAD2DEG << std::endl;
            std::cout << std::setw(8) << "Yaw = " << std::setprecision(2) << yaw * RAD2DEG << std::endl;
            std::cout << std::setw(8) << "Roll = " << std::setprecision(2) << roll * RAD2DEG << std::endl;

            cv::Mat targetInBoard = (cv::Mat_<double>(3, 1) << 0, 0, 0);
            cv::Mat targetInCamera = R * targetInBoard + tvec;
            std::cout << "Target in camera: " 
                      << targetInCamera.at<double>(0) << ","
                      << targetInCamera.at<double>(1) << ","
                      << targetInCamera.at<double>(2) << std::endl;

            std::vector<cv::Point2f> projectedPoints;
            cv::projectPoints(singleTagObject, rvec, tvec, cameraMatrix, distCoeffs, projectedPoints);

            double totalErr = 0.0;
            for(size_t i = 0; i < 4; ++i){
                totalErr += cv::norm(corners[bestId][i] - projectedPoints[i]);
            }
            double meanReproErr = totalErr / 4.0;
            float pixelWidth = (tagSize * cameraMatrix.at<double>(0, 0)) / targetInCamera.at<double>(2);
            float errorThreshold = pixelWidth * 0.05;

            std::cout << "Reproject Error: " << std::fixed << std::setprecision(2) << meanReproErr 
                      << " (error threshold: " << std::fixed << std::setprecision(2) <<  errorThreshold << " )" << std::endl;
            if(meanReproErr < errorThreshold){
                std::cout << "The error is under 5%" << std::endl;
            } 
        }else{
            std::cout << "Board is not complete!" << std::endl;

        }
    }
    
    cv::imshow("output", output);
    cv::waitKey(0);
    cv::destroyAllWindows();
    return 0;

}