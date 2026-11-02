# The Reminder — Edge ESP32

Firmware cho thiết bị nhắc lịch **ESP32** hoạt động theo cơ chế **Deep Sleep** tiết kiệm năng lượng, đồng bộ lịch trình và tải trước file âm thanh MP3 từ **Cloudflare Worker Gateway**, lưu trữ ngoại tuyến trên **LittleFS**, và phát qua mạch khuếch đại kỹ thuật số **I2S MAX98357A**.

---

## Kiến trúc & Cấu trúc thư mục

```text
edge-esp32/
├── src/
│   ├── main.cpp             # Luồng điều khiển chính (Wake-up, Poll, Play, Deep Sleep)
│   ├── define.h             # Định nghĩa cấu trúc dữ liệu, chân GPIO và hằng số
│   ├── rtc_manager.h        # Quản lý biến RTC_DATA_ATTR, đồng bộ giờ và tính thời gian Deep Sleep
│   ├── gateway_client.h     # HTTP/HTTPS Client gọi /poll và tải /audio về LittleFS
│   ├── storage_manager.h    # Quản lý đọc/ghi file an toàn (.tmp -> rename) trên LittleFS
│   └── i2s_audio_player.h   # Giải mã MP3 (Helix) và xuất âm thanh qua giao tiếp I2S
├── scripts/
│   └── load_env.py          # Pre-build script nạp biến từ file .env vào C++ macros
├── platformio.ini           # Cấu hình PlatformIO và thư viện phụ thuộc
└── .env.example             # Mẫu biến môi trường (WiFi, Gateway URL, Token)
```

---

## Phần cứng & Sơ đồ đấu nối (Pinout)

### 1. Mạch khuếch đại âm thanh I2S (MAX98357A)

| Chân MAX98357A | Chân ESP32 | Mô tả |
| :--- | :--- | :--- |
| **BCLK** | `GPIO 17` (`PIN_I2S_BCLK`) | I2S Bit Clock |
| **LRC / WS** | `GPIO 18` (`PIN_I2S_LRC`) | I2S Left/Right Word Select |
| **DIN** | `GPIO 16` (`PIN_I2S_DOUT`) | I2S Digital Audio Data |
| **VIN** | `5V` (hoặc `3V3`) | Nguồn cấp (khuyên dùng `5V` để đạt công suất loa tốt nhất) |
| **GND** | `GND` | Mass chung |

### 2. Nút nhấn điều khiển (`EXT0 Wake-up`)

| Thành phần | Chân ESP32 | Chức năng |
| :--- | :--- | :--- |
| **Nút nhấn (Active LOW)** | `GPIO 0` (`PIN_BTN`) | Đánh thức ESP32 từ Deep Sleep:<br>• **Nhấn nhả nhanh (< 600ms):** Phát sự kiện tiếp theo (`/next.mp3`).<br>• **Nhấn giữ (≥ 600ms):** Phát tổng hợp lịch cả ngày (`/today.mp3`). |

*(Có thể thay đổi cấu hình chân tại [`src/define.h`](src/define.h)).*

---

## Cơ chế hoạt động

1. **Khởi động lần đầu / Đến hạn Poll (`isPollDue`):**
   - Kết nối Wi-Fi, gọi `GET /poll?version=<rtc_version>`.
   - Đồng bộ đồng hồ hệ thống (`settimeofday`) theo `server_time` từ Gateway (không cần NTP riêng).
   - Nếu lịch thay đổi (`changed: true`), cập nhật danh sách sự kiện vào bộ nhớ `RTC_DATA_ATTR` và tải sẵn 2 file `/today.mp3`, `/next.mp3` lưu vào bộ nhớ Flash (`LittleFS`).
2. **Đến giờ nhắc sự kiện (`isEventDue`):**
   - Đánh thức bằng Timer đúng mốc `remind_ts`.
   - Phát ngay lập tức file `/next.mp3` từ `LittleFS` (không phụ thuộc mạng lúc phát).
   - Tăng con trỏ sự kiện (`advanceEvent`) và tải trước `/next.mp3` của sự kiện kế tiếp.
3. **Nhấn nút (`ESP_SLEEP_WAKEUP_EXT0`):**
   - Phát ngay `/next.mp3` (nhấn ngắn) hoặc `/today.mp3` (nhấn giữ ≥ 600ms) trực tiếp từ `LittleFS` mà không cần bật Wi-Fi (trừ khi file chưa tồn tại).
4. **Ngủ sâu (`enterDeepSleep`):**
   - Ngắt Wi-Fi hoàn toàn, tính toán thời gian ngủ tới mốc gần nhất giữa `remind_ts` tiếp theo và `rtcNextPollTs_`, sau đó đưa chip vào chế độ Deep Sleep.

---

## Cài đặt & Nạp Firmware

### 1. Yêu cầu môi trường
- [PlatformIO](https://platformio.org/) (VS Code Extension hoặc PlatformIO Core CLI).

### 2. Cấu hình biến môi trường (`.env`)
Tạo file `.env` tại thư mục `edge-esp32/`:
```bash
cp .env.example .env
```
Cập nhật thông số kết nối thực tế:
```env
WIFI_SSID="Ten_WiFi_Cua_Ban"
WIFI_PASS="Mat_Khau_WiFi"
GATEWAY_BASE="https://the-reminder-gateway.your-domain.com"
EDGE_TOKEN="your-secret-edge-token"
```
*(Script [`scripts/load_env.py`](scripts/load_env.py) sẽ tự động đọc `.env` lúc biên dịch và truyền vào các macro `WIFI_SSID`, `WIFI_PASS`, `GATEWAY_BASE`, `EDGE_TOKEN`).*

### 3. Biên dịch và nạp code
```bash
# Biên dịch firmware
pio run

# Nạp firmware vào ESP32
pio run -t upload

# Theo dõi Serial Monitor (115200 baud)
pio device monitor
```
