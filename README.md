# globaltop-gps - Library for parsing GPS / GNSS data stream

## Project description
`globaltop-gps` is a lightweight, portable parser library written in C, suitable for embedded systems and other environments. It parses standard NMEA 0183 sentences.

The parser matches sentence types by skipping the 2-character Talker ID, meaning it supports multiple constellations (e.g. `$GP` for GPS, `$GL` for GLONASS, `$GN` for GNSS, etc.) for the following sentence types:
- **RMC** (Recommended Minimum Navigation Information)
- **GGA** (Global Positioning System Fix Data)
- **VTG** (Track Made Good and Ground Speed)
- **GSA** (GPS DOP and Active Satellites)
- **GSV** (GPS Satellites in View)

The library uses **integer-only fixed-point math** instead of floating-point math to optimize performance and reduce size on MCUs without hardware FPU (e.g. Cortex-M3).

## Data stream example
```
$GPRMC,003300.799,V,,,,,0.00,0.00,060180,,,N*45
$GPVTG,0.00,T,,M,0.00,N,0.00,K,N*32
$GPGGA,003301.799,,,,,0,0,,,M,,M,,*4E
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,003301.799,V,,,,,0.00,0.00,060180,,,N*44
$GPVTG,0.00,T,,M,0.00,N,0.00,K*32
```

## Callback functions
Implement these callback signatures in your application to receive parsed data. Floating-point values from the NMEA stream are parsed directly into fixed-point integers corresponding to their native decimal precision:

* **Time (UTC)**: Format `hhmmsssss` (scaled by $10^3$, e.g. `232828000` for `23:28:28.000`).
* **Coordinates (Lat/Lon)**: Raw minute degrees (scaled by $10^4$, e.g. `41596173` for `4159.6173`).
* **Speed / Course / HDOP / PDOP / VDOP**: Scaled by $10^2$ (value $\times 100$).
* **Altitude / Geoidal Separation**: Scaled by $10^1$ (decimeters, value $\times 10$).

```c
void gps_callback_GPRMC(uint32_t utc, char status, int32_t latitude, char ns_indicator, int32_t longitude, char ew_indicator, uint16_t ground_speed, uint16_t ground_course, uint32_t date, uint16_t magnetic_variation, char magnetic_variation_direction, char mode);
void gps_callback_GPGGA(uint32_t utc, int32_t latitude, char ns_indicator, int32_t longitude, char ew_indicator, char position_fix_indicator, uint8_t satelites_used, uint16_t hdop, int16_t altitude, char altitude_units, int16_t geoidal_separation, char geoidal_units, uint16_t age_of_diff_correction);
void gps_callback_GPVTG(uint16_t course1, char reference1, uint16_t course2, char reference2, uint16_t speed1, char unit1, uint16_t speed2, char unit2, char mode);
void gps_callback_GPGSA(char mode1, char mode2, char channels[12], uint16_t pdop, uint16_t hdop, uint16_t vdop);
void gps_callback_GPGSV(uint8_t number_of_messages, uint8_t message_number, uint8_t satelitte_id[4], uint8_t elevation[4], uint16_t azimuth[4], uint16_t snr[4]);
```

## Usage Example
Here is a basic example of how to initialize and use the library:

```c
#include "gps.h"
#include <stdio.h>
#include <string.h>

// 1. Define callbacks
void my_gprmc_callback(uint32_t utc, char status, int32_t latitude, char ns_indicator,
                       int32_t longitude, char ew_indicator, uint16_t ground_speed,
                       uint16_t ground_course, uint32_t date, uint16_t magnetic_variation,
                       char magnetic_variation_direction, char mode) {
    (void)ground_course; (void)date; (void)magnetic_variation; (void)magnetic_variation_direction; (void)mode;
    if (status == 'A') {
        printf("UTC: %02lu:%02lu:%02lu.%03lu | Lat: %ld.%04ld %c | Lon: %ld.%04ld %c | Speed: %u.%02u knots\n",
               utc / 10000000, (utc % 10000000) / 100000, (utc % 100000) / 1000, utc % 1000,
               latitude / 10000, latitude % 10000, ns_indicator,
               longitude / 10000, longitude % 10000, ew_indicator,
               ground_speed / 100, ground_speed % 100);
    } else {
        printf("UTC: %02lu:%02lu:%02lu.%03lu | Status: Void (No Fix)\n",
               utc / 10000000, (utc % 10000000) / 100000, (utc % 100000) / 1000, utc % 1000);
    }
}

int main() {
    // 2. Initialize the gps_t parser instance
    gps_t gps;
    gps_init(&gps);

    // 3. Set callback functions (pass NULL for sentences you don't need)
    gps_set_callbacks(&gps, my_gprmc_callback, NULL, NULL, NULL, NULL);

    // 4. Feed stream data to the parser buffer
    const char *incoming_stream = "$GPRMC,003300.799,A,4807.038,N,01131.000,E,10.5,180.2,200626,,,A*6C\r\n";
    gps_add_data(&gps, incoming_stream, strlen(incoming_stream));

    // 5. Process the buffer to trigger callbacks
    gps_process_data(&gps);

    return 0;
}
```

## Build from main branch
The library includes a simple CMake build configuration:
```bash
git clone https://github.com/bokic/globaltop-gps.git
cd globaltop-gps
cmake -B build
cmake --build build
```
