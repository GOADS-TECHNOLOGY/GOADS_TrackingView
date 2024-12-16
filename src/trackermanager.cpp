#include "trackermanager.h"

TrackerManager::TrackerManager() : tracker_() {}

int TrackerManager::updateTrackers(const std::vector<Object>& detected_objects) {
    std::lock_guard<std::mutex> lock(tracker_mutex_);

    auto output_stracks = tracker_.update(detected_objects);
    int active_trackers_count = 0;

    for (const auto& track : output_stracks) {
        auto tlwh = track.tlwh;
        bool is_valid = tlwh[2] * tlwh[3] > 20 && (tlwh[2] / tlwh[3] <= 1.6);
        if (is_valid) {
            active_trackers_count++;
            unique_ids_.insert(track.track_id);
        }
    }

    return active_trackers_count;
}

std::unordered_set<int> TrackerManager::getUniqueIds() const {
    std::lock_guard<std::mutex> lock(tracker_mutex_);
    return unique_ids_;
}
