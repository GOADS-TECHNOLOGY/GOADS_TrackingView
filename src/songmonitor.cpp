#include "songmonitor.h"
#include <iostream>
#include <chrono>
#include <thread>

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
                song_counts_[previous_song] = song_count;
                if (song_reach_map_.find(previous_song) == song_reach_map_.end()) {
                    song_reach_map_[previous_song] = 0;
                }
                std::cout << "Song: " << previous_song
                          << " played " << song_count << " times, reached "
                          << song_reach_map_[previous_song] << " people.\n";
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

    if (!previous_song.empty()) {
        song_counts_[previous_song] = song_count;
    }
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
