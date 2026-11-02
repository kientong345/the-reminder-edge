#include <Arduino.h>
#include <WiFi.h>
#include "define.h"
#include "storage_manager.h"
#include "i2s_audio_player.h"
#include "rtc_manager.h"
#include "gateway_client.h"

StorageManager storage;
I2SAudioPlayer player(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
RtcManager rtc;
GatewayClient gateway(GATEWAY_BASE, EDGE_TOKEN, storage);

bool connectWiFi(uint32_t timeoutMs = 15000) {
    if (WiFi.status() == WL_CONNECTED) return true;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
}

void disconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
}

void syncWithGateway() {
    if (!connectWiFi()) {
        Serial.println("[Main] WiFi connect failed");
        return;
    }

    PollResult res = {};
    if (!gateway.poll(rtc.version(), res, rtc.eventsBuffer(), rtc.maxEvents())) {
        return;
    }

    rtc.applyPollResult(res);
    if (res.changed) {
        Serial.printf("[Main] Schedule updated (version=%s, events=%d, idx=%d)\n",
                      rtc.version(), rtc.eventCount(), rtc.currentIdx());
    }
    if (res.changed || !storage.exists("/today.mp3")) {
        gateway.downloadTodayAudio(rtc.version(), "/today.mp3");
    }
    if (res.changed || !storage.exists("/next.mp3")) {
        gateway.downloadNextAudio(rtc.version(), rtc.currentIdx(), "/next.mp3");
    }
}

void handleButtonWakeup() {
    uint32_t t0 = millis();
    while (digitalRead(PIN_BTN) == LOW && (millis() - t0) < HOLD_MS) {
        delay(10);
    }

    const char* path = (millis() - t0 >= HOLD_MS) ? "/today.mp3" : "/next.mp3";
    if (!storage.exists(path)) {
        syncWithGateway();
        disconnectWiFi();
    }

    player.playFile(storage, path);

    while (digitalRead(PIN_BTN) == LOW) {
        delay(10);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN, INPUT_PULLUP);
    storage.begin();

    if (rtc.wakeupCause() == ESP_SLEEP_WAKEUP_EXT0) {
        handleButtonWakeup();
        rtc.enterDeepSleep(PIN_BTN);
        return;
    }

    if (rtc.isEventDue()) {
        player.playFile(storage, "/next.mp3");
        rtc.advanceEvent();
        if (connectWiFi()) {
            gateway.downloadNextAudio(rtc.version(), rtc.currentIdx(), "/next.mp3");
        }
    }

    if (rtc.isPollDue()) {
        syncWithGateway();
    }

    disconnectWiFi();
    rtc.enterDeepSleep(PIN_BTN);
}

void loop() {
}
