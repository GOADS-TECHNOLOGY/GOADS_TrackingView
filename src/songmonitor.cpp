#include "songmonitor.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h> 
#include <curl/curl.h>
#include <nlohmann/json.hpp>

SongMonitor::SongMonitor(FppApi& api) : api_(api), stop_flag_(false) {}

SongMonitor::~SongMonitor() {
    stop();
}

void SongMonitor::start() {
    stop_flag_ = false;
    monitor_thread_ = std::thread(&SongMonitor::monitor, this);
}

void SongMonitor::stop() {
    stop_flag_ = true;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

void SongMonitor::monitor() {
    std::string previous_song = "";
    int song_count = 0;

    while (!stop_flag_) {
        try {
            std::string new_song = api_.getCurrentSongFileName();

            std::cout << "Current song: " << new_song << std::endl;

            std::lock_guard<std::mutex> lock(song_mutex_);
            if (new_song != previous_song && !previous_song.empty()) {
                // Lưu lại thông tin bài hát trước đó
                song_counts_[previous_song] = song_count;
                if (song_reach_map_.find(previous_song) == song_reach_map_.end()) {
                    song_reach_map_[previous_song] = 0;
                }

                sendDataToApi(previous_song, song_counts_[previous_song], song_reach_map_[previous_song]);
                
                std::cout << "Song: " << previous_song
                          << " played " << song_count << " times, reached "
                          << song_reach_map_[previous_song] << " people.\n";

                // Reset thông tin bài hát trước đó
                song_counts_[previous_song] = 0;
                song_reach_map_[previous_song] = 0;

                // Đặt lại bộ đếm
                song_count = 0;
            }
            previous_song = new_song;
            current_song_ = new_song;
            song_count++;
        } catch (const std::exception& e) {
            std::cerr << "Error fetching current_song: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Gửi dữ liệu và reset bài hát cuối cùng khi dừng giám sát
    if (!previous_song.empty()) {
        song_counts_[previous_song] = song_count;
        sendDataToApi(previous_song, song_counts_[previous_song], song_reach_map_[previous_song]);
        song_counts_[previous_song] = 0;
        song_reach_map_[previous_song] = 0;
    }
}

void SongMonitor::sendDataToApi(const std::string& song_id, int play_count, int reach_count) {
    CURL* curl;
    CURLcode res;

    // Lấy hostname làm screen_id
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    std::string screen_id = hostname;

    // Gửi dữ liệu tới API
    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL\n";
        return;
    }

    // Tạo danh sách header
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Authorization: 9dc5ec8c-2f2a-487b-9337-0aa585f69f9e");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Dữ liệu JSON
    nlohmann::json data = {
        {"screen_id", screen_id},
        {"video_id", std::stoi(song_id)}, // Chuyển song_id (chuỗi số) thành số nguyên
        {"value", reach_count}           // Số người nghe
    };

    std::string json_data = data.dump();

    curl_easy_setopt(curl, CURLOPT_URL, "http://118.69.191.129:8000/api/reach/create");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Gửi yêu cầu
    res = curl_easy_perform(curl);

    // Xử lý lỗi
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    } else {
        std::cout << "Data sent to API successfully for song ID: " << song_id << std::endl;
    }

    // Dọn dẹp CURL
    curl_slist_free_all(headers); // Giải phóng tài nguyên headers
    curl_easy_cleanup(curl);
}

void SongMonitor::addReachToCurrentSong(int count) {
    std::cout << "Count add " << count << std::endl;
    std::lock_guard<std::mutex> lock(song_mutex_);
    if (!current_song_.empty()) {
        song_reach_map_[current_song_] += count;
    }
}

std::unordered_map<std::string, int> SongMonitor::getSongCounts() {
    std::lock_guard<std::mutex> lock(song_mutex_);
    return song_counts_;
}

std::unordered_map<std::string, int> SongMonitor::getSongReaches() {
    std::lock_guard<std::mutex> lock(song_mutex_);
    return song_reach_map_;
}
