#ifndef SONG_MONITOR_H
#define SONG_MONITOR_H

#include "fppapi.h"
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "network.h"
#include <nlohmann/json.hpp>

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
    void enqueueScreenData(int song_id, std::string &status_name, int uptime);
    void enqueueApiData(int song_id, int play_count, int reach_count);

private:
    void monitor();

    FppApi &api_;
    Network net;
    std::atomic<bool> stop_flag_;
    std::thread monitor_thread_;
    std::thread retry_thread_;

    std::queue<std::tuple<int, std::string, int>> screen_data_queue_; // Hàng đợi lưu dữ liệu màn hình
    std::mutex queue_mutex_;                                          // Mutex bảo vệ hàng đợi
    std::condition_variable cv_;                                      // Biến điều kiện để thông báo thread
    std::thread screen_thread_;                                       // Thread riêng cho sendDataToApiScreen

    // Hàng đợi cho sendDataToApi
    std::queue<std::tuple<int, int, int>> api_data_queue_;
    std::mutex api_queue_mutex_;
    std::condition_variable api_cv_;
    std::thread api_thread_;

    void sendDataToApiScreenThread();
    void sendDataToApiThread();

    int current_song_;
    std::unordered_map<int, int> song_counts_;
    std::unordered_map<int, int> song_reach_map_; // Map lưu lượt tiếp cận
    std::mutex song_mutex_;

    void sendDataToApi(int song_id, int play_count, int reach_count);
    void sendDataToApiScreen(int song_id, std::string &status_name, int uptime);

    void retrySendingData();

    bool isNetworkAvailable();
    void saveToTemporaryFile(const std::string &filename, const nlohmann::json &data);
    std::vector<nlohmann::json> readFromTemporaryFile(const std::string &filename);
    void clearTemporaryFile(const std::string &filename);
};

#endif // SONG_MONITOR_H
