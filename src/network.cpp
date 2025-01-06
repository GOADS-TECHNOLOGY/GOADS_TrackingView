#include "network.h"

std::string Network::getHardwareID()
{
   std::string macAddress;
   std::ifstream file("/sys/class/net/" + interface + "/address");

   if (!file.is_open())
   {
      throw std::runtime_error("Không thể mở file. Giao diện không tồn tại hoặc không có quyền truy cập.");
   }

   std::getline(file, macAddress);
   file.close();

   if (macAddress.empty())
   {
      throw std::runtime_error("Không thể đọc địa chỉ MAC từ giao diện " + interface);
   }

   return macAddress;
}

// Getter và Setter cho tên giao diện
std::string Network::getInterface()
{
   return interface;
}

void Network::setInterface(const std::string &interfaceName)
{
   interface = interfaceName;
}

std::string Network::getIPAddress()
{
   struct ifaddrs *ifAddrStruct = nullptr;
   struct ifaddrs *ifa = nullptr;
   void *tmpAddrPtr = nullptr;
   std::string ipAddress;

   // Lấy danh sách các giao diện mạng
   if (getifaddrs(&ifAddrStruct) == -1)
   {
      return "";
   }

   // Duyệt qua danh sách các giao diện mạng
   for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
   {
      if (!ifa->ifa_addr)
         continue; // Bỏ qua nếu không có địa chỉ

      // Kiểm tra giao diện có tên khớp với interface không
      if (interface == ifa->ifa_name && ifa->ifa_addr->sa_family == AF_INET)
      {
         // Lấy địa chỉ IPv4
         tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
         char addressBuffer[INET_ADDRSTRLEN];
         inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
         ipAddress = addressBuffer;
         break;
      }
   }

   freeifaddrs(ifAddrStruct);

   if (ipAddress.empty())
   {
      return "";
   }

   return ipAddress.empty() ? "" : ipAddress;
}