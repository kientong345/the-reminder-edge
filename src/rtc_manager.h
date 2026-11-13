#ifndef __RTC_MANAGER_H__
#define __RTC_MANAGER_H__

#include <Arduino.h>
#include <esp_sleep.h>
#include <sys/time.h>
#include <time.h>
#include "define.h"

struct RtcState {
    char version[12];
    int event_count;
    int current_idx;
    uint32_t next_poll_ts;
    CompactEvent events[MAX_EVENTS];
};

RTC_DATA_ATTR static RtcState rtc = {};

inline void syncTime(uint32_t serverTimeSec) {
    if (serverTimeSec == 0) return;
    struct timeval tv = { .tv_sec = (time_t)serverTimeSec, .tv_usec = 0 };
    settimeofday(&tv, nullptr);
}

inline void applyPollResult(const PollResult& res) {
    syncTime(res.server_time);
    rtc.next_poll_ts = res.server_time + res.poll_after_sec;

    if (res.changed) {
        strncpy(rtc.version, res.version, sizeof(rtc.version) - 1);
        rtc.version[sizeof(rtc.version) - 1] = '\0';
        rtc.current_idx = res.current_idx;
        rtc.event_count = res.event_count;
    }
}

inline bool isEventDue() {
    uint32_t now = (uint32_t)time(nullptr);
    return now >= 1000000000UL &&
           rtc.current_idx < rtc.event_count &&
           (now + 5) >= rtc.events[rtc.current_idx].remind_ts;
}

inline void advanceEvent() {
    uint32_t now = (uint32_t)time(nullptr);
    while (rtc.current_idx < rtc.event_count &&
           rtc.events[rtc.current_idx].remind_ts <= (now + 5)) {
        rtc.current_idx++;
    }
}

inline bool isPollDue() {
    uint32_t now = (uint32_t)time(nullptr);
    return rtc.version[0] == '\0' || now < 1000000000UL || now >= rtc.next_poll_ts;
}

inline void enterDeepSleep(gpio_num_t wakePin) {
    uint32_t now = (uint32_t)time(nullptr);
    uint32_t sleepSec = 1800;

    if (now >= 1000000000UL) {
        uint32_t nextWake = rtc.next_poll_ts > now ? rtc.next_poll_ts : now + 1800;
        if (rtc.current_idx < rtc.event_count) {
            uint32_t remTs = rtc.events[rtc.current_idx].remind_ts;
            if (remTs > now && remTs < nextWake) {
                nextWake = remTs;
            }
        }
        sleepSec = nextWake > now ? (nextWake - now) : 5;
    }

    if (sleepSec < 5) sleepSec = 5;
    Serial.printf("[Rtc] Deep sleep for %u sec (idx=%d/%d)\n",
                  sleepSec, rtc.current_idx, rtc.event_count);

    esp_sleep_enable_ext0_wakeup(wakePin, 1);
    esp_sleep_enable_timer_wakeup((uint64_t)sleepSec * 1000000ULL);
    esp_deep_sleep_start();
}

#endif // __RTC_MANAGER_H__
