#include "yolov8.h"
#include "bytetracker.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_set>

// Convert OpenCV Mat to image_buffer_t without deep copying data
image_buffer_t convertMatToImageBuffer(const cv::Mat& frame) {
    image_buffer_t buffer = {};
    buffer.width = frame.cols;
    buffer.height = frame.rows;
    buffer.width_stride = frame.cols;
    buffer.height_stride = frame.rows;
    buffer.format = IMAGE_FORMAT_RGB888;
    buffer.size = frame.total() * frame.elemSize();
    buffer.virt_addr = frame.data; // Point directly to Mat data, avoid copying
    return buffer;
}

// Use static map to avoid rebuilding it every time
const char* getClassName(int cls_id) {
    static const std::unordered_map<int, const char*> class_names = {
        {0, "background"},
        {1, "person"},
    };
    auto it = class_names.find(cls_id);
    return it != class_names.end() ? it->second : "unknown";
}

int main() {
    // Initialize YOLOv8 model
    rknn_app_context_t rknn_app_ctx;
    const char* model_path = "../model/yolov8.rknn"; // Provide the path to the YOLOv8 model file
    if (init_yolov8_model(model_path, &rknn_app_ctx) != 0) {
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

    std::unordered_set<int> unique_ids; // Use unordered_set for fast ID tracking
    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to capture frame from camera." << std::endl;
            break;
        }

        // Convert frame to RGB if necessary
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

        // Use frame data directly for inference
        image_buffer_t img = convertMatToImageBuffer(frame);

        // Perform inference
        object_detect_result_list od_results;
        if (inference_yolov8_model(&rknn_app_ctx, &img, &od_results) != 0) {
            std::cerr << "Failed to perform inference on YOLOv8 model." << std::endl;
            break;
        }

        // Process detection results
        std::vector<Object> tracked_objects;
        for (int i = 0; i < od_results.count; i++) {
            const auto& det_result = od_results.results[i];
            if (det_result.prop < 0.5) continue; // Ignore low-confidence detections

            tracked_objects.push_back({
                .rect = cv::Rect_<float>(
                    det_result.box.left, det_result.box.top,
                    det_result.box.right - det_result.box.left,
                    det_result.box.bottom - det_result.box.top
                ),
                .label = det_result.cls_id,
                .prob = det_result.prop
            });
        }

        // Update tracker
        auto output_stracks = tracker.update(tracked_objects);
        int active_trackers_count = 0;

        for (const auto& track : output_stracks) {
            auto tlwh = track.tlwh;
            bool is_valid = tlwh[2] * tlwh[3] > 20 && (tlwh[2] / tlwh[3] <= 1.6);
            if (is_valid) {
                active_trackers_count++;
                unique_ids.insert(track.track_id);
                std::cout << "[TRACK ID] " << track.track_id
                          << " @ " << track.score << "\n";
            }
        }

        // Debugging
        if (active_trackers_count != 0) {
            std::cout << "[DEBUG] Active trackers count: " << active_trackers_count << "\n";
        }
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
