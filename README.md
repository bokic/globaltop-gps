# globaltop-gps - Library for parsing GPS / GNSS data stream

## Project description
`globaltop-gps` is a lightweight, portable parser library written in C, suitable for embedded systems and other environments. It parses standard NMEA 0183 sentences.

The parser matches sentence types by skipping the 2-character Talker ID, meaning it supports multiple constellations (e.g. `$GP` for GPS, `$GL` for GLONASS, `$GN` for GNSS, etc.) for the following sentence types:
- **RMC** (Recommended Minimum Navigation Information)
- **GGA** (Global Positioning System Fix Data)
- **VTG** (Track Made Good and Ground Speed)
- **GSA** (GPS DOP and Active Satellites)
- **GSV** (GPS Satellites in View)

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
Implement these callback signatures in your application to receive parsed data. Note that floating-point values are parsed as `double` to prevent precision loss.

```c
void gps_callback_GPRMC(double utc, char status, double latitude, char ns_indicator, double longitude, char ew_indicator, double ground_speed, double ground_course, int date, double magnetic_variation, char magnetic_variation_direction, char mode);
void gps_callback_GPGGA(double utc, double latitude, char ns_indicator, double longitude, char ew_indicator, char position_fix_indicator, int satelites_used, double hdop, double altitude, char altitude_units, double geoidal_separation, char geoidal_units, double age_of_diff_correction);
void gps_callback_GPVTG(double course1, char reference1, double course2, char reference2, double speed1, char unit1, double speed2, char unit2, char mode);
void gps_callback_GPGSA(char mode1, char mode2, char channels[12], double pdop, double hdop, double vdop);
void gps_callback_GPGSV(uint8_t number_of_messages, uint8_t message_number, uint8_t satelitte_id[4], uint8_t elevation[4], uint16_t azimuth[4], uint16_t snr[4]);
```

## Usage Example
Here is a basic example of how to initialize and use the library:

```c
#include "gps.h"
#include <stdio.h>
#include <string.h>

// 1. Define callbacks
void my_gprmc_callback(double utc, char status, double latitude, char ns_indicator,
                       double longitude, char ew_indicator, double ground_speed,
                       double ground_course, int date, double magnetic_variation,
                       char magnetic_variation_direction, char mode) {
    if (status == 'A') {
        printf("UTC: %.2f | Lat: %.6f %c | Lon: %.6f %c | Speed: %.2f knots\n",
               utc, latitude, ns_indicator, longitude, ew_indicator, ground_speed);
    } else {
        printf("UTC: %.2f | Status: Void (No Fix)\n", utc);
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
