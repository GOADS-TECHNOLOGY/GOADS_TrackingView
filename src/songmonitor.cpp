#include "songmonitor.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

SongMonitor::SongMonitor(FppApi &api) : api_(api), stop_flag_(false) {}

SongMonitor::~SongMonitor()
{
    stop();
}

void SongMonitor::start()
{
    stop_flag_ = false;
    monitor_thread_ = std::thread(&SongMonitor::monitor, this);
    screen_thread_ = std::thread(&SongMonitor::sendDataToApiScreenThread, this);
    api_thread_ = std::thread(&SongMonitor::sendDataToApiThread, this);
}

void SongMonitor::stop()
{
    stop_flag_ = true;
    cv_.notify_all();     // Thông báo để kết thúc thread
    api_cv_.notify_all(); // Thông báo để kết thúc thread API

    if (monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }
    if (screen_thread_.joinable())
    {
        screen_thread_.join();
    }
    if (api_thread_.joinable())
    {
        api_thread_.join();
    }
}

void SongMonitor::monitor()
{
    int previous_song = -1;
    int song_count = 0;

    while (!stop_flag_)
    {
        try
        {
            // Get current song, status, and uptime
            auto [current_song, status_name, uptime, ip] = api_.getSystemStatusDetails();

            std::cout << "Current song: " << current_song
                      << ", Status: " << status_name
                      << ", Uptime: " << uptime << " seconds\n";

            std::lock_guard<std::mutex> lock(song_mutex_);
            if (current_song != previous_song && (previous_song != -1))
            {
                // Save details of the previous song
                song_counts_[previous_song] = song_count;
                if (song_reach_map_.find(previous_song) == song_reach_map_.end())
                {
                    song_reach_map_[previous_song] = 0;
                }

                enqueueApiData(previous_song, song_counts_[previous_song], song_reach_map_[previous_song]); // Đưa dữ liệu vào hàng đợi API

                enqueueScreenData(current_song, status_name, uptime, ip); // Đưa dữ liệu vào hàng đợi

                // Reset details for the previous song
                song_counts_[previous_song] = 0;
                song_reach_map_[previous_song] = 0;

                // Reset counter
                song_count = 0;
            }

            previous_song = current_song;
            current_song_ = current_song;
            song_count++;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error fetching system status: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Send details of the last song when monitoring stops
    if (previous_song != -1)
    {
        song_counts_[previous_song] = song_count;
        sendDataToApi(previous_song, song_counts_[previous_song], song_reach_map_[previous_song]);
        song_counts_[previous_song] = 0;
        song_reach_map_[previous_song] = 0;
    }
}

void SongMonitor::sendDataToApiScreen(int song_id, std::string &status_name, int uptime, std::string ip)
{
    if (!isNetworkAvailable())
    {
        return;
    }

    CURL *curl;
    CURLcode res;

    // Lấy hostname làm screen_id
    char hostname[1024];
    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        std::cerr << "Failed to get hostname for screen ID" << std::endl;
        return;
    }
    std::string screen_id = hostname;

    // Khởi tạo CURL
    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return;
    }

    // Tạo header cho request
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Authorization: a4311ee0-8159-4eba-a948-1d8da85a484f");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Tạo URL
    std::string url_put = "https://server.goads.com.vn/api/screen/" + screen_id + "/status";

    // Tạo dữ liệu JSON
    nlohmann::json data = {
        {"current_song", song_id},
        {"status_name", status_name},
        {"uptime", uptime},
        {"ip_address", ip}};

    std::string json_data = data.dump();

    // Cài đặt CURL
    curl_easy_setopt(curl, CURLOPT_URL, url_put.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Thực thi yêu cầu
    res = curl_easy_perform(curl);

    // Xử lý lỗi
    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        std::cerr << "Failed URL: " << url_put << std::endl;
        std::cerr << "Payload: " << json_data << std::endl;
    }
    else
    {
        std::cout << "Data sent to API successfully for screen ID: " << screen_id << std::endl;
        std::cout << "Payload: " << json_data << std::endl;
    }

    // Dọn dẹp tài nguyên
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void SongMonitor::sendDataToApi(int song_id, int play_count, int reach_count)
{
    if (!isNetworkAvailable())
    {
        return;
    }
    
    CURL *curl;
    CURLcode res;

    // Lấy hostname làm screen_id
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    std::string screen_id = hostname;

    // Gửi dữ liệu tới API
    curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize CURL\n";
        return;
    }

    // Tạo danh sách header
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Authorization: a4311ee0-8159-4eba-a948-1d8da85a484f");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Dữ liệu JSON
    nlohmann::json data = {
        {"screen", screen_id},
        {"video", song_id},    // Chuyển song_id (chuỗi số) thành số nguyên
        {"reach", reach_count} // Số người nghe
    };

    std::string json_data = data.dump();

    curl_easy_setopt(curl, CURLOPT_URL, "https://server.goads.com.vn/api/reach/create");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Gửi yêu cầu
    res = curl_easy_perform(curl);

    // Xử lý lỗi
    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }
    else
    {
        std::cout << "Data sent to API successfully for song ID: " << song_id << std::endl;
    }

    // Dọn dẹp CURL
    curl_slist_free_all(headers); // Giải phóng tài nguyên headers
    curl_easy_cleanup(curl);
}

void SongMonitor::addReachToCurrentSong(int count)
{
    std::lock_guard<std::mutex> lock(song_mutex_);
    if (current_song_ != -1)
    {
        song_reach_map_[current_song_] += count;
    }
}

std::unordered_map<int, int> SongMonitor::getSongCounts()
{
    std::lock_guard<std::mutex> lock(song_mutex_);
    return song_counts_;
}

std::unordered_map<int, int> SongMonitor::getSongReaches()
{
    std::lock_guard<std::mutex> lock(song_mutex_);
    return song_reach_map_;
}

// Thêm hàm để đưa dữ liệu vào hàng đợi
void SongMonitor::enqueueScreenData(int song_id, std::string &status_name, int uptime, std::string ip)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        screen_data_queue_.emplace(song_id, status_name, uptime, ip);
    }
    cv_.notify_one(); // Thông báo cho thread có dữ liệu mới
}

// Hàm xử lý thread riêng
void SongMonitor::sendDataToApiScreenThread()
{
    while (!stop_flag_)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this]
                 { return !screen_data_queue_.empty() || stop_flag_; });

        if (stop_flag_)
            break;

        // Lấy dữ liệu từ hàng đợi
        auto [song_id, status_name, uptime, ip] = screen_data_queue_.front();
        screen_data_queue_.pop();
        lock.unlock(); // Mở khóa mutex trước khi gửi dữ liệu

        sendDataToApiScreen(song_id, status_name, uptime, ip);
    }
}

void SongMonitor::enqueueApiData(int song_id, int play_count, int reach_count)
{
    {
        std::lock_guard<std::mutex> lock(api_queue_mutex_);
        api_data_queue_.emplace(song_id, play_count, reach_count);
    }
    api_cv_.notify_one(); // Thông báo cho thread xử lý API
}

void SongMonitor::sendDataToApiThread()
{
    while (!stop_flag_)
    {
        std::unique_lock<std::mutex> lock(api_queue_mutex_);
        api_cv_.wait(lock, [this]
                     { return !api_data_queue_.empty() || stop_flag_; });

        if (stop_flag_)
            break;

        // Lấy dữ liệu từ hàng đợi
        auto [song_id, play_count, reach_count] = api_data_queue_.front();
        api_data_queue_.pop();
        lock.unlock(); // Mở khóa mutex trước khi gửi dữ liệu

        sendDataToApi(song_id, play_count, reach_count);
    }
}

bool SongMonitor::isNetworkAvailable()
{
    return system("ping -c 1 google.com > /dev/null 2>&1") == 0;
}