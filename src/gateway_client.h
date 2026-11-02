#ifndef __GATEWAY_CLIENT_H__
#define __GATEWAY_CLIENT_H__

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "define.h"
#include "storage_manager.h"

class GatewayClient {
private:
    const char* baseUrl_;
    const char* edgeToken_;
    StorageManager& storage_;

    bool beginRequest(WiFiClientSecure& client, HTTPClient& http, const String& url) {
        client.setInsecure();
        if (!http.begin(client, url)) {
            return false;
        }
        http.addHeader("Authorization", String("Bearer ") + edgeToken_);
        return true;
    }

    static uint32_t extractJsonNumber(const String& json, const char* key, uint32_t fallback = 0) {
        String pattern = String("\"") + key + "\":";
        int idx = json.indexOf(pattern);
        if (idx < 0) return fallback;
        idx += pattern.length();
        while (idx < (int)json.length() && json[idx] == ' ') idx++;
        return (uint32_t)strtoul(json.c_str() + idx, nullptr, 10);
    }

    static String extractJsonString(const String& json, const char* key) {
        String pattern = String("\"") + key + "\":\"";
        int idx = json.indexOf(pattern);
        if (idx < 0) return "";
        idx += pattern.length();
        int endIdx = json.indexOf("\"", idx);
        return endIdx > idx ? json.substring(idx, endIdx) : "";
    }

    bool downloadToFile(const String& url, const char* destPath) {
        WiFiClientSecure client;
        HTTPClient http;
        http.useHTTP10(true);
        if (!beginRequest(client, http, url)) {
            return false;
        }

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            Serial.printf("[GatewayClient] Audio GET failed (%d)\n", code);
            http.end();
            return false;
        }

        bool ok = storage_.saveFromHttp(http, destPath);
        http.end();
        return ok;
    }

public:
    GatewayClient(const char* baseUrl, const char* edgeToken, StorageManager& storage)
        : baseUrl_(baseUrl), edgeToken_(edgeToken), storage_(storage) {}

    bool poll(const char* currentVersion, PollResult& out, CompactEvent* eventsBuf, int maxEvents) {
        WiFiClientSecure client;
        HTTPClient http;
        String url = String(baseUrl_) + "/poll?version=" + (currentVersion ? currentVersion : "");
        if (!beginRequest(client, http, url)) {
            return false;
        }

        int code = http.GET();
        if (code != HTTP_CODE_OK) {
            Serial.printf("[GatewayClient] Poll GET failed (%d)\n", code);
            http.end();
            return false;
        }

        String body = http.getString();
        http.end();

        out.server_time = extractJsonNumber(body, "server_time", 0);
        out.poll_after_sec = extractJsonNumber(body, "poll_after_sec", 1800);
        out.changed = body.indexOf("\"changed\":true") >= 0;

        String ver = extractJsonString(body, "version");
        strncpy(out.version, ver.c_str(), sizeof(out.version) - 1);
        out.version[sizeof(out.version) - 1] = '\0';

        if (!out.changed) {
            return true;
        }

        out.current_idx = (int)extractJsonNumber(body, "current_idx", 0);
        out.event_count = 0;

        int evPos = body.indexOf("\"events\":[");
        if (evPos >= 0 && eventsBuf != nullptr && maxEvents > 0) {
            int cursor = evPos;
            while (out.event_count < maxEvents) {
                int rIdx = body.indexOf("\"remind_ts\":", cursor);
                if (rIdx < 0) break;
                int sIdx = body.indexOf("\"start_ts\":", rIdx);
                if (sIdx < 0) break;

                eventsBuf[out.event_count].remind_ts =
                    (uint32_t)strtoul(body.c_str() + rIdx + 12, nullptr, 10);
                eventsBuf[out.event_count].start_ts =
                    (uint32_t)strtoul(body.c_str() + sIdx + 11, nullptr, 10);
                out.event_count++;
                cursor = sIdx + 11;
            }
        }

        return true;
    }

    bool downloadTodayAudio(const char* version, const char* destPath) {
        String url = String(baseUrl_) + "/audio?target=today&version=" + (version ? version : "");
        return downloadToFile(url, destPath);
    }

    bool downloadNextAudio(const char* version, int idx, const char* destPath) {
        String url = String(baseUrl_) + "/audio?target=next&version=" + (version ? version : "") + "&idx=" + idx;
        return downloadToFile(url, destPath);
    }
};

#endif // __GATEWAY_CLIENT_H__
