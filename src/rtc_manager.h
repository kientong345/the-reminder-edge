#ifndef __RTC_MANAGER_H__
#define __RTC_MANAGER_H__

#include <Arduino.h>
#include <esp_sleep.h>
#include <sys/time.h>
#include "define.h"

#ifndef MAX_EVENTS
#define MAX_EVENTS 100
#endif

RTC_DATA_ATTR static char rtcVersion_[12]    = "";
RTC_DATA_ATTR static int rtcEventCount_      = 0;
RTC_DATA_ATTR static int rtcCurrentIdx_      = 0;
RTC_DATA_ATTR static CompactEvent rtcEvents_[MAX_EVENTS];
RTC_DATA_ATTR static uint32_t rtcNextPollTs_ = 0;

class RtcManager {
public:
    esp_sleep_wakeup_cause_t wakeupCause() const {
        return esp_sleep_get_wakeup_cause();
    }

    uint32_t nowSec() const {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return (uint32_t)tv.tv_sec;
    }

    void syncTime(uint32_t serverTimeSec) {
        if (serverTimeSec == 0) return;
        struct timeval tv = { .tv_sec = (time_t)serverTimeSec, .tv_usec = 0 };
        settimeofday(&tv, nullptr);
    }

    const char* version() const { return rtcVersion_; }
    int currentIdx() const { return rtcCurrentIdx_; }
    int eventCount() const { return rtcEventCount_; }
    CompactEvent* eventsBuffer() { return rtcEvents_; }
    int maxEvents() const { return MAX_EVENTS; }

    void applyPollResult(const PollResult& res) {
        syncTime(res.server_time);
        rtcNextPollTs_ = res.server_time + res.poll_after_sec;

        if (res.changed) {
            strncpy(rtcVersion_, res.version, sizeof(rtcVersion_) - 1);
            rtcVersion_[sizeof(rtcVersion_) - 1] = '\0';
            rtcCurrentIdx_ = res.current_idx;
            rtcEventCount_ = res.event_count;
        }
    }

    bool isEventDue() const {
        uint32_t now = nowSec();
        return now >= 1000000000UL &&
               rtcCurrentIdx_ < rtcEventCount_ &&
               (now + 5) >= rtcEvents_[rtcCurrentIdx_].remind_ts;
    }

    void advanceEvent() {
        uint32_t now = nowSec();
        while (rtcCurrentIdx_ < rtcEventCount_ &&
               rtcEvents_[rtcCurrentIdx_].remind_ts <= (now + 5)) {
            rtcCurrentIdx_++;
        }
    }

    bool isPollDue() const {
        uint32_t now = nowSec();
        return rtcVersion_[0] == '\0' || now < 1000000000UL || now >= rtcNextPollTs_;
    }

    void enterDeepSleep(gpio_num_t wakePin) {
        uint32_t now = nowSec();
        uint32_t sleepSec = 1800;

        if (now >= 1000000000UL) {
            uint32_t nextWake = rtcNextPollTs_ > now ? rtcNextPollTs_ : now + 1800;
            if (rtcCurrentIdx_ < rtcEventCount_) {
                uint32_t remTs = rtcEvents_[rtcCurrentIdx_].remind_ts;
                if (remTs > now && remTs < nextWake) {
                    nextWake = remTs;
                }
            }
            sleepSec = nextWake > now ? (nextWake - now) : 5;
        }

        if (sleepSec < 5) sleepSec = 5;
        Serial.printf("[RtcManager] Deep sleep for %u sec (idx=%d/%d)\n",
                      sleepSec, rtcCurrentIdx_, rtcEventCount_);

        esp_sleep_enable_ext0_wakeup(wakePin, 0);
        esp_sleep_enable_timer_wakeup((uint64_t)sleepSec * 1000000ULL);
        esp_deep_sleep_start();
    }
};

#endif // __RTC_MANAGER_H__
