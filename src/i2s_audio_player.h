#ifndef __I2S_AUDIO_PLAYER_H__
#define __I2S_AUDIO_PLAYER_H__

#include <Arduino.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "storage_manager.h"

class I2SAudioPlayer {
private:
    int bclkPin_;
    int wsPin_;
    int dataPin_;
    I2SStream i2s_;
    EncodedAudioStream mp3Out_;

public:
    I2SAudioPlayer(int bclkPin, int wsPin, int dataPin)
        : bclkPin_(bclkPin),
          wsPin_(wsPin),
          dataPin_(dataPin),
          mp3Out_(&i2s_, new MP3DecoderHelix()) {}

    bool playFile(StorageManager& storage, const char* path) {
        if (!storage.exists(path)) {
            Serial.printf("[AudioPlayer] Missing file %s\n", path);
            return false;
        }

        auto cfg = i2s_.defaultConfig(TX_MODE);
        cfg.pin_bck = bclkPin_;
        cfg.pin_ws = wsPin_;
        cfg.pin_data = dataPin_;
        cfg.sample_rate = 24000;
        cfg.channels = 1;
        i2s_.begin(cfg);
        mp3Out_.begin();

        File f = storage.openRead(path);
        uint8_t buf[512];
        while (f && f.available()) {
            size_t n = f.read(buf, sizeof(buf));
            if (n > 0) mp3Out_.write(buf, n);
        }
        f.close();

        delay(250);
        mp3Out_.end();
        i2s_.end();
        return true;
    }
};

#endif // __I2S_AUDIO_PLAYER_H__
