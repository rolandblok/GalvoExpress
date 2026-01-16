
#include <LittleFS.h>

#include "mystore.hpp"

const String STORAGE_PATH = "/pattern.txt";

bool save_pattern(const vector<patern_point>& data) {
    // Implementation to save data to storage
    File file = LittleFS.open(STORAGE_PATH, "w");
    if (!file) {
        Serial.println("# Failed to open file for writing");
        return false;
    }
    // write number of points
    file.println(String(data.size()));
    for (const auto& point : data) {
        file.printf("%d,%d,%d\n", point.x, point.y, point.laser_on ? 1 : 0);
    }
    file.close();
    Serial.println("# Pattern saved");
    return true;
}

bool load_pattern(vector<patern_point>& data) {
    // Implementation to load data from storage
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
    int line_count = file.readStringUntil('\n').toInt();
    String line;
    int lines_read = 0;
    while (file.available()) {
        lines_read++;
        line = file.readStringUntil('\n');
        if (line.isEmpty()) break; // End of pattern
        
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        int x = line.substring(0, comma1).toInt();
        int y = line.substring(comma1 + 1, comma2).toInt();
        bool laser_on = line.substring(comma2 + 1).toInt() != 0;
        data.push_back({x, y, laser_on});
    }

    file.close();
    if (lines_read != line_count) {
        Serial.println("# Pattern not found");
        return false;
    }
    Serial.println("Pattern loaded");
    return true;
}

void delete_pattern() {
    // Implementation to delete pattern from storage
    if (LittleFS.exists(STORAGE_PATH)) {
        LittleFS.remove(STORAGE_PATH);
        Serial.println("# Pattern deleted");
    } else {
        Serial.println("# No pattern to delete");
    }
}