#ifndef SONG_MONITOR_H
#define SONG_MONITOR_H

#include "fppapi.h"
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>

class SongMonitor
{
public:
    explicit SongMonitor(FppApi &api);
    ~SongMonitor();

    void start();
    void stop();
    void addReachToCurrentSong(int count); // Thêm lượt tiếp cận
    std::unordered_map<int, int> getSongCounts();
    std::unordered_map<int, int> getSongReaches(); // Lấy lượt tiếp cận

private:
    void monitor();

    FppApi &api_;
    std::atomic<bool> stop_flag_;
    std::thread monitor_thread_;

    int current_song_;
    std::unordered_map<int, int> song_counts_;
    std::unordered_map<int, int> song_reach_map_; // Map lưu lượt tiếp cận
    std::mutex song_mutex_;

    void sendDataToApi(int song_id, int play_count, int reach_count);
    void sendDataToApiScreen(int song_id, std::string &status_name, int uptime);
};

#endif // SONG_MONITOR_H
