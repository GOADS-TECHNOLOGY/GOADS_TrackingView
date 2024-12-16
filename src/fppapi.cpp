#include "fppapi.h"
#include <iostream>
#include <regex>
#include <curl/curl.h>

FppApi::FppApi(const std::string& base_url)
    : base_url_(base_url) {}

std::string FppApi::getSystemStatus() {
    std::string endpoint = "/api/system/status";
    std::string url = base_url_ + endpoint;
    return performGetRequest(url);
}

std::string FppApi::getCurrentSongFileName() {
    std::string statusJson = getSystemStatus();
    std::regex songRegex("\"current_song\":\s*\"(.*?)\"");
    std::smatch match;
    if (std::regex_search(statusJson, match, songRegex) && match.size() > 1) {
        std::string fullPath = match.str(1);
        size_t pos = fullPath.find_last_of("/");
        return (pos != std::string::npos) ? fullPath.substr(pos + 1) : fullPath;
    }
    return "";
}

size_t FppApi::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string FppApi::performGetRequest(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }

    return response;
}