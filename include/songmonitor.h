#ifndef SONG_MONITOR_H
#define SONG_MONITOR_H

#include "fppapi.h"
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>

class SongMonitor {
public:
    explicit SongMonitor(FppApi& api);
    ~SongMonitor();

    void start();
    void stop();
    void addReachToCurrentSong(int count); // Thêm lượt tiếp cận
    std::unordered_map<std::string, int> getSongCounts();
    std::unordered_map<std::string, int> getSongReaches(); // Lấy lượt tiếp cận

private:
    void monitor();

    FppApi& api_;
    std::atomic<bool> stop_flag_;
    std::thread monitor_thread_;

    std::string current_song_;
    std::unordered_map<std::string, int> song_counts_;
    std::unordered_map<std::string, int> song_reach_map_; // Map lưu lượt tiếp cận
    std::mutex song_mutex_;
};

#endif // SONG_MONITOR_H
