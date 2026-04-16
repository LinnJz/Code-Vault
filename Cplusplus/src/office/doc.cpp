#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#  include <windows.h>  // 用于 MultiByteToWideChar 和 FileTimeToSystemTime
#endif

// 小端读取函数
inline uint16_t readU16(const uint8_t* data, size_t offset)
{
  return static_cast<uint16_t>(data [offset]) |
      (static_cast<uint16_t>(data [offset + 1]) << 8);
}

inline uint32_t readU32(const uint8_t* data, size_t offset)
{
  return static_cast<uint32_t>(data [offset]) |
      (static_cast<uint32_t>(data [offset + 1]) << 8) |
      (static_cast<uint32_t>(data [offset + 2]) << 16) |
      (static_cast<uint32_t>(data [offset + 3]) << 24);
}

// FMTID_SummaryInformation（小端字节序）
const uint8_t FMTID_SUMMARYINFO [] = { 0xE0, 0x85, 0x9F, 0xF2, 0xF9, 0x4F,
                                       0x68, 0x10, 0xAB, 0x91, 0x08, 0x00,
                                       0x2B, 0x27, 0xB3, 0xD9 };

// 属性ID枚举
enum PropertyID
{
  CodePage         = 0x01,
  Title            = 0x02,
  Subject          = 0x03,
  Author           = 0x04,
  Keywords         = 0x05,
  Comments         = 0x06,
  Template         = 0x07,
  LastAuthor       = 0x08,
  RevisionNumber   = 0x09,
  EditTime         = 0x0A,
  LastPrinted      = 0x0B,
  CreateDateTime   = 0x0C,
  LastSaveDateTime = 0x0D,
  PageCount        = 0x0E,
  WordCount        = 0x0F,
  CharCount        = 0x10,
  Thumbnail        = 0x11,
  ApplicationName  = 0x12,
  Security         = 0x13
};

// 将 FILETIME（100ns 间隔，从 1601-01-01 起）转换为可读字符串
std::string FileTimeToString(uint64_t fileTime)
{
#ifdef _WIN32
  FILETIME ft;
  ft.dwLowDateTime  = static_cast<DWORD>(fileTime & 0xFF'FF'FF'FF);
  ft.dwHighDateTime = static_cast<DWORD>(fileTime >> 32);
  SYSTEMTIME st;
  if (FileTimeToSystemTime(&ft, &st)) {
    char buffer [64];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d", st.wYear,
             st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::string(buffer);
  }
  return "Invalid FileTime";
#else
  // 跨平台简单处理：转换为 time_t（从1970-01-01起，需减去11644473600秒）
  const uint64_t EPOCH_DIFFERENCE =
      11644473600LL;  // 秒数：1601-01-01 到 1970-01-01
  time_t     seconds = (fileTime / 10000000) - EPOCH_DIFFERENCE;
  char       buffer [64];
  struct tm* tm_info = gmtime(&seconds);
  if (tm_info) {
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
  }
  return "Invalid FileTime";
#endif
}

// 根据代码页解码单字节字符串（转换为 UTF-8 输出）
std::string DecodeString(const uint8_t* data, size_t len, uint16_t codePage)
{
#ifdef _WIN32
  // 使用 MultiByteToWideChar 转换为 UTF-16，再转换为 UTF-8
  int wlen = MultiByteToWideChar(codePage, 0, reinterpret_cast<const char*>(data),
                                 static_cast<int>(len), NULL, 0);
  if (wlen > 0) {
    std::vector<wchar_t> wbuf(wlen);
    MultiByteToWideChar(codePage, 0, reinterpret_cast<const char*>(data),
                        static_cast<int>(len), wbuf.data(), wlen);
    // 转换为 UTF-8
    int utf8len =
        WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), wlen, NULL, 0, NULL, NULL);
    std::vector<char> utf8buf(utf8len);
    WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), wlen, utf8buf.data(), utf8len,
                        NULL, NULL);
    return std::string(utf8buf.data(), utf8len);
  }
#else
  (void)codePage;  // 避免未使用参数警告
#endif
  // 非 Windows 或转换失败时，直接返回原始字符串（按当前编码处理）
  return std::string(reinterpret_cast<const char*>(data), len);
}

// 解析 SummaryInformation 流
bool ParseSummaryInformation(const std::vector<uint8_t>& data)
{
  size_t pos       = 0;
  size_t totalSize = data.size();

  // 1. 全局头 (28字节 + N * 20字节)
  if (totalSize < 28) {
    std::cerr << "Data too small for global header." << std::endl;
    return false;
  }

  // 字节顺序
  uint16_t byteOrder = readU16(data.data(), pos);
  if (byteOrder != 0xFF'FE) {
    std::cerr << "Invalid byte order: expected 0xFFFE, got 0x" << std::hex
              << byteOrder << std::dec << std::endl;
    return false;
  }
  pos += 2;

  // 跳过版本、操作系统信息、CLSID 等（共 2+1+1+2+16 = 22 字节）
  pos += 2;   // 版本
  pos += 1;   // 操作系统主版本
  pos += 1;   // 操作系统次版本
  pos += 2;   // 操作系统类型（应为 0x0002）
  pos += 16;  // CLSID（通常为 16 字节，尽管文档说 10 字节，但按实际规范处理）

  uint32_t numPropertySets = readU32(data.data(), pos);
  pos += 4;

  std::cout << "Number of property sets: " << numPropertySets << std::endl;

  // 遍历所有属性组
  for (uint32_t i = 0; i < numPropertySets; ++i) {
    if (pos + 20 > totalSize) {
      std::cerr << "Unexpected end of data while reading property set header."
                << std::endl;
      break;
    }

    // 读取 FMTID 和偏移
    const uint8_t* fmtid = &data [pos];
    pos += 16;
    uint32_t sectionOffset = readU32(data.data(), pos);
    pos += 4;

    // 只处理 SummaryInformation 组
    if (memcmp(fmtid, FMTID_SUMMARYINFO, 16) != 0) continue;

    std::cout << "Found SummaryInformation section at offset 0x" << std::hex
              << sectionOffset << std::dec << std::endl;

    // 2. 定位到属性组头部
    if (sectionOffset + 8 > totalSize) {
      std::cerr << "Section offset out of range." << std::endl;
      continue;
    }
    size_t   sectionPos  = sectionOffset;
    uint32_t sectionSize = readU32(data.data(), sectionPos);
    sectionPos += 4;
    uint32_t numProperties = readU32(data.data(), sectionPos);
    sectionPos += 4;

    std::cout << "  Section size: " << sectionSize
              << ", properties: " << numProperties << std::endl;

    // 临时存储所有属性描述符，以便先读取代码页
    struct PropDesc
    {
      uint32_t id;
      uint32_t offset;
    };

    std::vector<PropDesc> props;
    size_t                descPos = sectionPos;  // 当前指向第一个属性描述符

    for (uint32_t j = 0; j < numProperties; ++j) {
      if (descPos + 8 > totalSize) break;
      uint32_t propID        = readU32(data.data(), descPos);
      uint32_t contentOffset = readU32(data.data(), descPos + 4);
      props.push_back({ propID, contentOffset });
      descPos += 8;
    }

    // 默认代码页（简体中文），可在解析过程中更新
    uint16_t codePage = 936;

    // 第一遍：查找代码页属性（ID=0x01）
    for (const auto& p : props) {
      if (p.id == CodePage) {
        size_t contentPos = sectionOffset + p.offset;
        if (contentPos + 4 <= totalSize) {
          uint32_t type = readU32(data.data(), contentPos);
          if (type == 0x02) {  // VT_I2
            codePage = readU16(data.data(), contentPos + 4);
            std::cout << "  CodePage: " << codePage << std::endl;
          }
        }
        break;  // 代码页通常只有一个，找到即可
      }
    }

    // 第二遍：解析所有属性
    for (const auto& p : props) {
      size_t contentPos = sectionOffset + p.offset;
      if (contentPos + 4 > totalSize) {
        std::cout << "    PropID 0x" << std::hex << p.id << std::dec
                  << ": content offset out of range." << std::endl;
        continue;
      }

      uint32_t type = readU32(data.data(), contentPos);
      contentPos += 4;

      std::cout << "    PropID 0x" << std::hex << p.id << std::dec << " (Type 0x"
                << std::hex << type << std::dec << ")";

      switch (type) {
      case 0x02 : {  // VT_I2
        if (contentPos + 2 > totalSize) break;
        uint16_t val = readU16(data.data(), contentPos);
        std::cout << " (UInt16): " << val << std::endl;
        break;
      }
      case 0x03 : {  // VT_I4
        if (contentPos + 4 > totalSize) break;
        uint32_t val = readU32(data.data(), contentPos);
        std::cout << " (UInt32): " << val << std::endl;
        break;
      }
      case 0x0B : {  // VT_BOOL
        if (contentPos + 2 > totalSize) break;
        uint16_t val = readU16(data.data(), contentPos);
        bool     b   = (val != 0);
        std::cout << " (Bool): " << (b ? "true" : "false") << std::endl;
        break;
      }
      case 0x1E : {  // VT_LPSTR (单字节字符串)
        if (contentPos + 4 > totalSize) break;
        uint32_t strLen = readU32(data.data(), contentPos);
        contentPos += 4;
        if (contentPos + strLen > totalSize) break;
        // 去除末尾的 '\0'
        std::string str = DecodeString(&data [contentPos], strLen - 1, codePage);
        std::cout << " (String): " << str << std::endl;
        break;
      }
      case 0x40 : {  // VT_FILETIME
        if (contentPos + 8 > totalSize) break;
        uint64_t fileTime = readU32(data.data(), contentPos) |
            (static_cast<uint64_t>(readU32(data.data(), contentPos + 4)) << 32);
        std::string timeStr = FileTimeToString(fileTime);
        std::cout << " (FileTime): " << timeStr << std::endl;
        break;
      }
      default : {
        // 未知类型：打印原始数据前8字节帮助调试
        std::cout << " (Unknown type, raw bytes): ";
        for (int k = 0; k < 8 && contentPos + k < totalSize; ++k) {
          std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(data [contentPos + k]) << " ";
        }
        std::cout << std::dec << std::endl;
        break;
      }
      }
    }
  }
  return true;
}

//// 使用示例：从文件读取流数据并解析
//int main(int argc, char* argv [])
//{
//  const char* filename;
//  if (argc < 2) {
//    std::cout << "Usage: " << argv [0] << " <SummaryInformation_stream_file>"
//              << std::endl;
//    std::cout << "Using default test file: [5]SummaryInformation" << std::endl;
//    filename = R"(C:\Users\Yourname\Desktop\[5]SummaryInformation)";
//  } else {
//    filename = argv [1];
//  }
//
//  std::ifstream file(filename, std::ios::binary | std::ios::ate);
//  if (!file) {
//    std::cerr << "Cannot open file: " << filename << std::endl;
//    return 1;
//  }
//
//  std::streamsize size = file.tellg();
//  file.seekg(0, std::ios::beg);
//
//  std::vector<uint8_t> buffer(size);
//  if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
//    std::cerr << "Read failed." << std::endl;
//    return 1;
//  }
//
//  ParseSummaryInformation(buffer);
//
//  return 0;
//}