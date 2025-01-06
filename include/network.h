#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class Network
{
private:
   std::string interface; // Tên giao diện mạng, ví dụ: "wlan0"

public:
   // Constructor để khởi tạo giao diện mạng
   explicit Network(const std::string &interfaceName = "wlan0") : interface(interfaceName) {}
   // Hàm lấy địa chỉ MAC
   std::string getHardwareID();
   // Getter và Setter cho tên giao diện
   std::string getInterface();
   void setInterface(const std::string &interfaceName);
   std::string getIPAddress();
};