#ifndef __I2S_AUDIO_PLAYER_H__
#define __I2S_AUDIO_PLAYER_H__

#include <Arduino.h>
#include <LittleFS.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "define.h"

inline bool playAudio(const char* path) {
    if (!LittleFS.exists(path)) {
        Serial.printf("[AudioPlayer] Missing file %s\n", path);
        return false;
    }

    I2SStream i2s;
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = PIN_I2S_BCLK;
    cfg.pin_ws = PIN_I2S_LRC;
    cfg.pin_data = PIN_I2S_DOUT;
    cfg.sample_rate = 24000;
    cfg.channels = 1;
    i2s.begin(cfg);

    MP3DecoderHelix decoder;
    EncodedAudioStream mp3Out(&i2s, &decoder);
    mp3Out.begin();

    File f = LittleFS.open(path, "r");
    uint8_t buf[512];
    while (f && f.available()) {
        size_t n = f.read(buf, sizeof(buf));
        if (n > 0) mp3Out.write(buf, n);
    }
    f.close();

    delay(250);
    mp3Out.end();
    i2s.end();
    return true;
}

#endif // __I2S_AUDIO_PLAYER_H__
