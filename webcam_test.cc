#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Initialize a VideoCapture object to open the camera
    cv::VideoCapture cap(0); // 0: default camera, or provide the ID of another camera (e.g., 1, 2, ...)
    
    // Check if the camera is opened successfully
    if (!cap.isOpened()) {
        std::cerr << "Failed to open the camera." << std::endl;
        return -1;
    }
    
    std::cout << "Camera opened successfully." << std::endl;
    
    // Loop to continuously capture images from the camera
    while (true) {
        // Declare a variable to store the captured frame
        cv::Mat frame;
        
        // Capture a frame from the camera
        cap >> frame;
        
        // Check if the frame is captured successfully
        if (frame.empty()) {
            std::cerr << "Failed to capture frame from camera." << std::endl;
            break;
        }
        
        // Log to console
        std::cout << "Frame captured from camera." << std::endl;
        
        // Wait for a short amount of time to display the frame (in milliseconds)
        // Exit the loop if the 'q' key is pressed
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }
    
    // Release camera resources and close the window
    cap.release();
    cv::destroyAllWindows();
    
    std::cout << "Camera closed." << std::endl;
    
    return 0;
}
