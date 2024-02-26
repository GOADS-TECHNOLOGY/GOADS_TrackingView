#include "yolov8.h"
#include "bytetracker.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <zmq.hpp>
#include <chrono>
#include <thread>

image_buffer_t convertMatToImageBuffer(const cv::Mat& frame) {
    image_buffer_t buffer;
    memset(&buffer, 0, sizeof(image_buffer_t));

    // Fill in image width, height, and other relevant information
    buffer.width = frame.cols;
    buffer.height = frame.rows;
    buffer.width_stride = frame.cols;
    buffer.height_stride = frame.rows;
    buffer.format = IMAGE_FORMAT_RGB888; // Assuming the input frame is in BGR format
    buffer.size = frame.total() * frame.elemSize();
    buffer.virt_addr = new unsigned char[buffer.size];

    // Copy pixel data from the OpenCV frame to the image_buffer_t structure
    memcpy(buffer.virt_addr, frame.data, buffer.size);

    return buffer;
}

void freeImageBuffer(image_buffer_t& buffer) {
    if (buffer.virt_addr != nullptr) {
        delete[] buffer.virt_addr;
        buffer.virt_addr = nullptr;
    }
}

const char* getClassName(int cls_id) {
    static std::unordered_map<int, const char*> class_names = {
        {0, "background"},
        {1, "person"},
    };

    auto it = class_names.find(cls_id);
    if (it != class_names.end()) {
        return it->second;
    } else {
        return "unknown"; // or handle unknown class id differently
    }
}

int main() {
    // Initialize YOLOv8 model
    rknn_app_context_t rknn_app_ctx;
    const char* model_path = "../model/yolov8.rknn"; // Provide the path to the YOLOv8 model file
    int ret = init_yolov8_model(model_path, &rknn_app_ctx);
    if (ret != 0) {
        std::cerr << "Failed to initialize YOLOv8 model." << std::endl;
        return -1;
    }

    // Initialize Tracker
    BYTETracker tracker;

    // Initialize camera
    cv::VideoCapture cap(0); // Open default camera
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera." << std::endl;
        release_yolov8_model(&rknn_app_ctx);
        return -1;
    }
    
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);
    publisher.bind("tcp://*:140799");

    std::set<int> unique_ids;
    // Loop to continuously perform object detection
    auto last_send_time = std::chrono::high_resolution_clock::now();
    while (true) {
        // Capture frame from camera
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to capture frame from camera." << std::endl;
            break;
        }

        // Convert OpenCV frame to image_buffer_t
        image_buffer_t img = convertMatToImageBuffer(frame);
        // Implement function convertMatToImageBuffer to convert frame to img

        // Perform inference on YOLOv8 model
        object_detect_result_list od_results;
        ret = inference_yolov8_model(&rknn_app_ctx, &img, &od_results);
        if (ret != 0) {
            std::cerr << "Failed to perform inference on YOLOv8 model." << std::endl;
            break;
        }

        vector<Object> tracked_objects;
        for (int i = 0; i < od_results.count; i++) {
            object_detect_result* det_result = &(od_results.results[i]);

            // Convert detection result to an Object struct
            Object obj;
            obj.rect = cv::Rect_<float>(det_result->box.left, det_result->box.top,
                                        det_result->box.right - det_result->box.left, 
                                        det_result->box.bottom - det_result->box.top); // Convert to cv::Rect
            obj.label = det_result->cls_id; // Use cls_id as the label
            obj.prob = det_result->prop; // Use prop as the probability

            // Add the object to the vector
            tracked_objects.push_back(obj);
        }

        // Assuming `tracker` is a ByteTrack instance you've initialized
        vector<STrack> output_stracks = tracker.update(tracked_objects);
        
        int active_trackers_count = 0;
        for (int i = 0; i < output_stracks.size(); i++)
		{
			vector<float> tlwh = output_stracks[i].tlwh;
			bool vertical = tlwh[2] / tlwh[3] > 1.6;
			if (tlwh[2] * tlwh[3] > 20 && !vertical)
			{
                active_trackers_count++; 
                unique_ids.insert(output_stracks[i].track_id);
                printf("[TRACK ID] %d - %s @ %.3f\n", output_stracks[i].track_id, "person", output_stracks[i].score);
			}
		}

        // free memory
        freeImageBuffer(img);
        
        if(active_trackers_count != 0)
        {
            printf("[DEBUG] Active trackers count: %d\n", active_trackers_count);
        }
        
        // auto now = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<double> elapsed = now - last_send_time;
        // if (elapsed.count() >= 15.0) { // Check if 15 seconds have passed
        //     std::ostringstream oss;
        //     oss << unique_ids.size();
        //     std::string count_str = oss.str();
        //     zmq::message_t message(count_str.size());
        //     memcpy(message.data(), count_str.data(), count_str.size());
        //     publisher.send(message);
            
        //     printf("[INFO] Publish people count: %lu\n", unique_ids.size());
            
        //     unique_ids.clear(); // Clear unique memory
        //     last_send_time = now; // Reset the timer
        // }

        std::ostringstream oss;
        //Draw data
        //oss << unique_ids.size();
        //Activate data
        oss << active_trackers_count;
        std::string count_str = oss.str();
        zmq::message_t message(count_str.size());
        memcpy(message.data(), count_str.data(), count_str.size());
        publisher.send(message, zmq::send_flags::none);
        printf("[INFO] Publish people count: %lu\n", unique_ids.size());
        
        // Break the loop if 'q' is pressed
        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    // Release resources
    release_yolov8_model(&rknn_app_ctx);
    cap.release();
    cv::destroyAllWindows();

    return 0;
}