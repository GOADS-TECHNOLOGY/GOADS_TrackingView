#include "songmonitor.h"
#include "trackermanager.h"
#include "yolov8.h"
#include <opencv2/opencv.hpp>
#include <iostream>

image_buffer_t convertMatToImageBuffer(const cv::Mat &frame)
{
    image_buffer_t buffer;
    memset(&buffer, 0, sizeof(image_buffer_t));

    buffer.width = frame.cols;
    buffer.height = frame.rows;
    buffer.width_stride = frame.cols;
    buffer.height_stride = frame.rows;
    buffer.format = IMAGE_FORMAT_RGB888;
    buffer.size = frame.total() * frame.elemSize();
    buffer.virt_addr = new unsigned char[buffer.size];
    memcpy(buffer.virt_addr, frame.data, buffer.size);

    return buffer;
}

void freeImageBuffer(image_buffer_t &buffer)
{
    if (buffer.virt_addr != nullptr)
    {
        delete[] buffer.virt_addr;
        buffer.virt_addr = nullptr;
    }
}

int main()
{
    // Initialize FPP API
    FppApi api;

    // Initialize SongMonitor
    SongMonitor song_monitor(api);
    song_monitor.start();

    // Initialize YOLOv8 model
    rknn_app_context_t rknn_app_ctx;
    const char *model_path = "/usr/local/model/yolov8.rknn";
    if (init_yolov8_model(model_path, &rknn_app_ctx) != 0)
    {
        std::cerr << "Failed to initialize YOLOv8 model." << std::endl;
        return -1;
    }

    // Initialize TrackerManager
    TrackerManager tracker_manager;

    // Initialize camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        std::cerr << "Failed to open camera." << std::endl;
        // Default logic when the camera is not available
        while (true)
        {
            int tracker_count = 1; // Default count when no camera is available
            song_monitor.addReachToCurrentSong(tracker_count);

            // Simulate a delay for processing
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // Break the loop if 'q' is pressed
            if (cv::waitKey(1) == 'q')
            {
                break;
            }
        }

        song_monitor.stop();
        release_yolov8_model(&rknn_app_ctx);
        cv::destroyAllWindows();
        return 0;
    }

    while (true)
    {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty())
        {
            std::cerr << "[WARNING] Failed to capture frame from camera. Using default tracker count.\n";
            int default_tracker_count = 1; // Giá trị mặc định khi không có camera
            song_monitor.addReachToCurrentSong(default_tracker_count);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue; // Tiếp tục vòng lặp
        }

        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

        // Perform inference
        image_buffer_t img = convertMatToImageBuffer(frame);
        object_detect_result_list od_results;

        if (inference_yolov8_model(&rknn_app_ctx, &img, &od_results) != 0)
        {
            std::cerr << "Failed to perform inference on YOLOv8 model." << std::endl;
            break;
        }

        // Process detection results
        std::vector<Object> detected_objects;
        for (int i = 0; i < od_results.count; i++)
        {
            const auto &det_result = od_results.results[i];
            if (det_result.prop < 0.5)
                continue;

            detected_objects.push_back({.rect = cv::Rect_<float>(
                                            det_result.box.left, det_result.box.top,
                                            det_result.box.right - det_result.box.left,
                                            det_result.box.bottom - det_result.box.top),
                                        .label = det_result.cls_id,
                                        .prob = det_result.prop});
        }

        // Update tracker
        int tracker_count = tracker_manager.updateTrackers(detected_objects);

        std::cout << "[DEBUG] Active trackers count: " << tracker_count << "\n";

        // Ghi lại lượt tiếp cận cho bài hát hiện tại
        song_monitor.addReachToCurrentSong(tracker_count);

        // free memory
        freeImageBuffer(img);

        // Break the loop if 'q' is pressed
        if (cv::waitKey(1) == 'q')
        {
            break;
        }
    }

    song_monitor.stop();
    release_yolov8_model(&rknn_app_ctx);
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
