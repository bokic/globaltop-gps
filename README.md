# globaltop-gps - Library for parsing GlobalTop(FGPMMOPA6H) GPS data stream.

## Project description
globaltop-gps is a parsing library that parses GPS data stream. Code is written in portable C suitable for embedded systems.

## Data stream example
```
$GPRMC,003300.799,V,,,,,0.00,0.00,060180,,,N*45
$GPVTG,0.00,T,,M,0.00,N,0.00,K,N*32
$GPGGA,003301.799,,,,,0,0,,,M,,M,,*4E
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,003301.799,V,,,,,0.00,0.00,060180,,,N*44
$GPVTG,0.00,T,,M,0.00,N,0.00,K,N*32
$GPGGA,003302.799,,,,,0,0,,,M,,M,,*4D
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,003302.799,V,,,,,0.00,0.00,060180,,,N*47
$GPVTG,0.00,T,,M,0.00,N,0.00,K,N*32
$GPGGA,003303.799,,,,,0,0,,,M,,M,,*4C
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPGSV,1,1,00*79
$GPRMC,003303.799,V,,,,,0.00,0.00,060180,,,N*46
$GPVTG,0.00,T,,M,0.00,N,0.00,K,N*32
$GPGGA,003304.799,,,,,0,0,,,M,,M,,*4B
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,003304.799,V,,,,,0.00,0.00,060180,,,N*41
$GPVTG,0.00,T,,M,0.00,N,0.00,K,N*32
$GPGGA,003305.799,,,,,0,0,,,M,,M,,*4A
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,003305.799,V,,,,,0.00,0.00,060180,,,N*40
$GPVTG,0.00,T,,M,0.00,N,0.00,K,N*32
$GPGGA,003306.799,,,,,0,0,,,M,,M,,*49
$GPGSA,A,1,,,,,,,,,,,,,,,*1E
$GPRMC,003306.799,V,,,,,0.00,0.00,06
```

## Callback functions
``` C
    void gps_callback_GPRMC(float utc, char status, float latitude, char ns_indicator, float longitude, char ew_indicator, float ground_speed, float ground_course, int date, float magnetic_variation, char magnetic_variation_direction, char mode);
    void gps_callback_GPGGA(float utc, float latitude, char ns_indicator, float longitude, char ew_indicator, char position_fix_indicator, int satelites_used, float hdop, float altitude, char altitude_units, float geoidal_separation, char geoidal_units, float age_of_diff_correction);
    void gps_callback_GPVTG(float course1, char reference1, float course2, char reference2, float speed1, char unit1, float speed2, char unit2, char mode);
    void gps_callback_GPGSA(char mode1, char mode2, char channels[12], float pdop, float hdop, float vdop);
    void gps_callback_GPGSV(uint8_t number_of_messages, uint8_t message_number, uint8_t satelitte_id[4], uint8_t elevation[4], uint16_t azimuth[4], uint16_t snr[4]);
```

## Build
``` bash
cd <root of the project>
mkdir build
cmake -Bbuild -G Ninja
ninja -Cbuild
```
