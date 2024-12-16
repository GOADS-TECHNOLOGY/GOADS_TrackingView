#ifndef TRACKER_MANAGER_H
#define TRACKER_MANAGER_H

#include "bytetracker.h"
#include <vector>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <unordered_set>

class TrackerManager {
public:
    TrackerManager();

    // Cập nhật các tracker dựa trên các đối tượng phát hiện được
    int updateTrackers(const std::vector<Object>& detected_objects);

    // Lấy danh sách ID duy nhất của các đối tượng đã theo dõi
    std::unordered_set<int> getUniqueIds() const;

private:
    BYTETracker tracker_;
    std::unordered_set<int> unique_ids_; // Lưu trữ các ID duy nhất
    mutable std::mutex tracker_mutex_;   // Đảm bảo thread-safe
};

#endif // TRACKER_MANAGER_H
