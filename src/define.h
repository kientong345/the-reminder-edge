#ifndef __DEFINE_H__
#define __DEFINE_H__

#include <Arduino.h>

#ifndef WIFI_SSID
#define WIFI_SSID       ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS       ""
#endif

#ifndef GATEWAY_BASE
#define GATEWAY_BASE    "http://localhost:3000"
#endif

#ifndef EDGE_TOKEN
#define EDGE_TOKEN      ""
#endif

#define PIN_BTN         GPIO_NUM_0
#define PIN_I2S_BCLK    17
#define PIN_I2S_LRC     18
#define PIN_I2S_DOUT    16

#define HOLD_MS         3000
#define MAX_EVENTS      100

struct CompactEvent {
    uint32_t start_ts;
    uint32_t remind_ts;
};

struct PollResult {
    bool changed;
    char version[12];
    uint32_t server_time;
    uint32_t poll_after_sec;
    int current_idx;
    int event_count;
};

#endif // __DEFINE_H__
