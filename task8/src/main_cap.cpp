#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>

const float tagSize = 12.0f;
const float half = tagSize / 2.0f;

int operation(cv::Mat& frame, const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs, 
              std::vector<cv::Point3f> singleTagObject, const cv::Ptr<cv::aruco::Dictionary>& dictionary,
              const cv::Ptr<cv::aruco::DetectorParameters>& pramas){
    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners, rejected;
    cv::aruco::detectMarkers(frame, dictionary, corners, ids, pramas, rejected);

    if(ids.empty()){
        cv::putText(frame, "No Tag Detected", cv::Point(20, 40), 
                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,0,255), 2);
        return 0;
    }else{
        cv::aruco::drawDetectedMarkers(frame, corners, ids);
        for(size_t i = 0; i < ids.size(); ++i){
            bool isComplete = true;
            for(const auto& pt : corners[i]){
                if(pt.x <= 0 || pt.y <= 0 || pt.x >= frame.cols || pt.y >= frame.rows){
                    isComplete = false;
                    break;
                }
            }
            if(!isComplete) continue;
    
            cv::Mat rvec, tvec;
            cv::solvePnP(singleTagObject, corners[i], cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
            cv::drawFrameAxes(frame, cameraMatrix, distCoeffs, rvec, tvec, tagSize * 0.8f);
            
            cv::Mat R;
            cv::Rodrigues(rvec, R);
            double sy = std::sqrt(R.at<double>(0, 0) * R.at<double>(0, 0) + 
            R.at<double>(1, 0) * R.at<double>(1, 0));
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

            cv::Mat targetInBoard = (cv::Mat_<double>(3, 1) << 0, 0, 0);
            cv::Mat targetInCamera = R * targetInBoard + tvec;

            std::vector<cv::Point2f> projectedPoints;
            cv::projectPoints(singleTagObject, rvec, tvec, cameraMatrix, distCoeffs, projectedPoints);

            cv::circle(frame, projectedPoints[0], 3, cv::Scalar(0, 0, 255), -1);
            std::string text = "Yaw: " + std::to_string((int)(yaw * RAD2DEG));
            cv::putText(frame, text, cv::Point(corners[i][0].x, corners[i][0].y - 10),
                        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);

            double totalErr = 0.0;
            for(size_t j = 0; j < 4; ++j){
                totalErr += cv::norm(corners[i][j] - projectedPoints[j]);
            }
            double meanReproErr = totalErr / 4.0;
            float pixelWidth = (tagSize * cameraMatrix.at<double>(0, 0)) / targetInCamera.at<double>(2);
            float errorThreshold = pixelWidth * 0.05;

            if(meanReproErr > errorThreshold){
            cv::putText(frame, "Error High", cv::Point(corners[i][0].x, corners[i][0].y + 20), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
            }
        }
    }
    return 0;
}

int main(){
    cv::Mat cameraMatrix, distCoeffs;
    cv::FileStorage fs("source/mono_calib.yaml", cv::FileStorage::READ);
    if(!fs.isOpened()){
        std::cerr << "Error: Fail to open the file!" << std::endl;
        return 0;
    }

    fs["camera_matrix"] >> cameraMatrix;
    fs["distortion_coefficients"] >> distCoeffs;
    fs.release();

    std::vector<cv::Point3f> singleTagObject = {
        cv::Point3f(- half, - half, 0),
        cv::Point3f(half, - half, 0),
        cv::Point3f(half, half, 0),
        cv::Point3f(- half, half, 0)
    };

    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_APRILTAG_36h11);
    cv::Ptr<cv::aruco::DetectorParameters> pramas = cv::aruco::DetectorParameters::create();
    pramas -> cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;

    cv::VideoCapture cap(0);
    cv::Mat frame;

    cap >> frame;
    if(!cap.isOpened() || frame.empty()){
        std::cerr << "Error: Fail to open the capture!" << std::endl;
        return 0;
    }
    operation(frame, cameraMatrix, distCoeffs, singleTagObject, dictionary, pramas);

    while(1){
        if(!cap.read(frame)){
            std::cout << "Capture stream is over" << std::endl;
            break;
        }
        if(frame.empty()) break;
        operation(frame, cameraMatrix, distCoeffs, singleTagObject, dictionary, pramas);
        cv::imshow("frame", frame);
        if(cv::waitKey(30) == 27) break;
    }

    
    cap.release();
    cv::destroyAllWindows();
    return 0;

}