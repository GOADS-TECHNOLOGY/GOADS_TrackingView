#include "fppapi.h"
#include <iostream>
#include <regex>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

FppApi::FppApi(const std::string &base_url)
    : base_url_(base_url) {}

std::string FppApi::getSystemStatus()
{
    std::string endpoint = "/api/system/status";
    std::string url = base_url_ + endpoint;
    return performGetRequest(url);
}

std::tuple<int, std::string, int, std::string> FppApi::getSystemStatusDetails()
{
    std::string statusJson = getSystemStatus();

    try
    {
        // Phân tích JSON
        nlohmann::json jsonData = nlohmann::json::parse(statusJson);

        // Lấy current_song
        std::string currentSongPath = jsonData.value("current_song", "");
        int currentSong = 0;

        if (!currentSongPath.empty())
        {
            size_t pos = currentSongPath.find_last_of('/');
            std::string fileName = (pos != std::string::npos) ? currentSongPath.substr(pos + 1) : currentSongPath;

            // Loại bỏ phần mở rộng ".mp4"
            size_t extPos = fileName.find_last_of('.');
            if (extPos != std::string::npos && fileName.substr(extPos) == ".mp4")
            {
                fileName = fileName.substr(0, extPos);
            }

            // Chuyển đổi tên file sang số nguyên
            try
            {
                currentSong = std::stoi(fileName);
            }
            catch (const std::exception &)
            {
                currentSong = 0; // Nếu không chuyển đổi được, đặt giá trị mặc định
            }
        }

        // Lấy status_name
        std::string statusName = jsonData.value("status_name", "idle");

        // Lấy uptime
        int uptime = jsonData.value("uptimeTotalSeconds", 0);

        // Lấy IPs từ advancedView
        std::string ipAddress = ""; // Giá trị mặc định
        if (jsonData.contains("advancedView") && jsonData["advancedView"].contains("IPs"))
        {
            auto ips = jsonData["advancedView"]["IPs"];
            if (ips.is_array() && !ips.empty())
            {
                ipAddress = ips.dump(); // Chuyển cả mảng IPs thành chuỗi JSON
            }
        }

        return {currentSong, statusName, uptime, ipAddress};
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return {0, "idle", 0, ""}; // Trả về giá trị mặc định khi gặp lỗi
    }
}

size_t FppApi::writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    std::string *str = static_cast<std::string *>(userp);
    str->append(static_cast<char *>(contents), totalSize);
    return totalSize;
}

std::string FppApi::performGetRequest(const std::string &url)
{
    CURL *curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }

    return response;
}