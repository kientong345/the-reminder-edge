#ifndef __STORAGE_MANAGER_H__
#define __STORAGE_MANAGER_H__

#include <Arduino.h>
#include <HTTPClient.h>
#include <LittleFS.h>

class StorageManager {
public:
    bool begin() {
        return LittleFS.begin(true);
    }

    bool exists(const char* path) {
        return LittleFS.exists(path);
    }

    File openRead(const char* path) {
        return LittleFS.open(path, "r");
    }

    bool saveFromHttp(HTTPClient& http, const char* destPath) {
        int totalLen = http.getSize();
        const char* tmpPath = "/audio.tmp";

        if (LittleFS.exists(tmpPath)) {
            LittleFS.remove(tmpPath);
        }

        File f = LittleFS.open(tmpPath, "w", true);
        if (!f) {
            Serial.println("[Storage] Failed to create /audio.tmp");
            return false;
        }

        int written = http.writeToStream(&f);
        f.close();

        if (written <= 0 || (totalLen > 0 && written != totalLen)) {
            Serial.printf("[Storage] Stream incomplete (%d/%d bytes)\n", written, totalLen);
            LittleFS.remove(tmpPath);
            return false;
        }

        if (LittleFS.exists(destPath)) {
            LittleFS.remove(destPath);
        }
        bool ok = LittleFS.rename(tmpPath, destPath);
        Serial.printf("[Storage] Saved %s (%d bytes)\n", destPath, written);
        return ok;
    }
};

#endif // __STORAGE_MANAGER_H__
