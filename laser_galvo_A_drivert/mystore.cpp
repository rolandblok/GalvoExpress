#include <LittleFS.h>
#include "mystore.hpp"

const String STORAGE_PATH = "/pattern.bin";

// Pack x(12), y(12), laser(1) into 32-bit integer
// Format: [7 unused bits][1 laser bit][12 y bits][12 x bits]
uint32_t pack_point(int x, int y, bool laser_on) {
    uint32_t packed = 0;
    packed |= (x & 0xFFF);           // 12 bits for x (bits 0-11)
    packed |= ((y & 0xFFF) << 12);   // 12 bits for y (bits 12-23)  
    packed |= (laser_on ? 1U : 0U) << 24; // 1 bit for laser (bit 24)
    return packed;
}

void unpack_point(uint32_t packed, int& x, int& y, bool& laser_on) {
    x = packed & 0xFFF;              // Extract bits 0-11
    y = (packed >> 12) & 0xFFF;      // Extract bits 12-23
    laser_on = (packed >> 24) & 1;   // Extract bit 24
}

void mystore_init() {
  // Initialize LittleFS
  //   LittleFS.format();
  if (!LittleFS.begin()) {
    Serial.println("# Failed to mount LittleFS");
  } 
  FSInfo info;
  LittleFS.info(info);
  Serial.printf("# FSInfo Total: %u bytes\n", info.totalBytes);
  Serial.printf("# FSInfo Used : %u bytes\n", info.usedBytes);
}

bool mystore_save_pattern(const std::vector<patern_point>& data) {
    Serial.println("# Saving pattern (packed 25-bit), length: " + String(data.size()));
    
    if (data.empty()) {
        Serial.println("# Cannot save empty pattern");
        return false;
    }
    
    if (LittleFS.exists(STORAGE_PATH)) {
        LittleFS.remove(STORAGE_PATH);
        Serial.println("# Old Pattern deleted");
    }
    
    File file = LittleFS.open(STORAGE_PATH, "w");
    if (!file) {
        Serial.println("# Failed to open file for writing");
        return false;
    }
    
    // Write point count
    uint32_t point_count = data.size();
    file.write((uint8_t*)&point_count, sizeof(point_count));
    
    // Pack and write points
    for (const auto& point : data) {
        // Validate ranges
        if (point.x < 0 || point.x > 4095 || point.y < 0 || point.y > 4095) {
            Serial.println("# Point out of range: x=" + String(point.x) + ", y=" + String(point.y));
            file.close();
            return false;
        }
        
        uint32_t packed = pack_point(point.x, point.y, point.laser_on);
        file.write((uint8_t*)&packed, sizeof(packed));
    }
    
    file.close();
    Serial.println("# Pattern saved (packed) with length: " + String(data.size()));
    FSInfo info;
    LittleFS.info(info);
    Serial.printf("# FSInfo Total: %u bytes\n", info.totalBytes);
    Serial.printf("# FSInfo Used : %u bytes\n", info.usedBytes);

    return true;
}

bool mystore_load_pattern(std::vector<patern_point>& data) {
    if (!LittleFS.exists(STORAGE_PATH)) {
        Serial.println("# Storage file does not exist");
        return false;
    }
    
    File file = LittleFS.open(STORAGE_PATH, "r");
    if (!file) {
        Serial.println("# Failed to open file for reading");
        return false;
    }
    
    data.clear();
    
    // Read point count
    uint32_t point_count = 0;
    if (file.readBytes((char*)&point_count, sizeof(point_count)) != sizeof(point_count)) {
        Serial.println("# Failed to read point count");
        file.close();
        return false;
    }
    
    Serial.println("# Loading pattern (packed), expected length: " + String(point_count));
    
    if (point_count == 0 || point_count > 100000) {
        Serial.println("# Invalid point count");
        file.close();
        return false;
    }
    
    data.reserve(point_count);
    
    // Read and unpack points
    for (uint32_t i = 0; i < point_count; i++) {
        uint32_t packed;
        if (file.readBytes((char*)&packed, sizeof(packed)) != sizeof(packed)) {
            Serial.println("# Failed to read point " + String(i));
            file.close();
            data.clear();
            return false;
        }
        
        patern_point point;
        unpack_point(packed, point.x, point.y, point.laser_on);
        data.push_back(point);
    }
    
    file.close();
    Serial.println("# Pattern loaded (packed) with length: " + String(data.size()));
    return true;
}

void mystore_delete_pattern() {
    if (LittleFS.exists(STORAGE_PATH)) {
        LittleFS.remove(STORAGE_PATH);
        Serial.println("# Pattern deleted");
    } else {
        Serial.println("# No pattern to delete");
    }
}