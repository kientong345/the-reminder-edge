#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "define.h"
#include "i2s_audio_player.h"
#include "rtc_manager.h"
#include "gateway_client.h"

GatewayClient gateway(GATEWAY_BASE, EDGE_TOKEN);

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
    if (!gateway.poll(rtc.version, res, rtc.events, MAX_EVENTS)) {
        return;
    }

    applyPollResult(res);
    if (res.changed) {
        Serial.printf("[Main] Schedule updated (version=%s, events=%d, idx=%d)\n",
                      rtc.version, rtc.event_count, rtc.current_idx);
    }
    if (res.changed || !LittleFS.exists("/today.mp3")) {
        gateway.downloadAudio("today", rtc.version, "/today.mp3");
    }
    if (res.changed || !LittleFS.exists("/next.mp3")) {
        gateway.downloadAudio("next", rtc.version, "/next.mp3", rtc.current_idx);
    }
}

void handleButtonWakeup() {
    uint32_t t0 = millis();
    while (digitalRead(PIN_BTN) == HIGH && (millis() - t0) < HOLD_MS) {
        delay(10);
    }

    const char* path = (millis() - t0 >= HOLD_MS) ? "/today.mp3" : "/next.mp3";
    if (!LittleFS.exists(path)) {
        syncWithGateway();
        disconnectWiFi();
    }

    playAudio(path);

    while (digitalRead(PIN_BTN) == HIGH) {
        delay(10);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN, INPUT_PULLDOWN);
    LittleFS.begin(true);

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        handleButtonWakeup();
        enterDeepSleep(PIN_BTN);
        return;
    }

    if (isEventDue()) {
        playAudio("/next.mp3");
        advanceEvent();
        if (connectWiFi()) {
            gateway.downloadAudio("next", rtc.version, "/next.mp3", rtc.current_idx);
        }
    }

    if (isPollDue()) {
        syncWithGateway();
    }

    disconnectWiFi();
    enterDeepSleep(PIN_BTN);
}

void loop() {
}
