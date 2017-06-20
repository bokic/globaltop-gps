#ifndef GPS_H
#define GPS_H

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*gps_callback_GPRMC_t)(float utc, char status, float latitude, char ns_indicator, float longitude, char ew_indicator, float ground_speed, float ground_course, int date, float magnetic_variation, char magnetic_variation_direction, char mode);
    typedef void (*gps_callback_GPGGA_t)(float utc, float latitude, char ns_indicator, float longitude, char ew_indicator, char position_fix_indicator, int satelites_used, float hdop, float altitude, char altitude_units, float geoidal_separation, char geoidal_units, float age_of_diff_correction);
    typedef void (*gps_callback_GPVTG_t)(float course1, char reference1, float course2, char reference2, float speed1, char unit1, float speed2, char unit2, char mode);
    typedef void (*gps_callback_GPGSA_t)(char mode1, char mode2, char channels[12], float pdop, float hdop, float vdop);
    typedef void (*gps_callback_GPGSV_t)(uint8_t number_of_messages, uint8_t message_number, uint8_t satelitte_id[4], uint8_t elevation[4], uint16_t azimuth[4], uint16_t snr[4]);

    void gps_set_callbacks(gps_callback_GPRMC_t gprmc, gps_callback_GPGGA_t gpgga, gps_callback_GPVTG_t gpvtg, gps_callback_GPGSA_t gpgsa, gps_callback_GPGSV_t gpgsv);
    void gps_add_data(const char buffer[], int len);
    void gps_process_data();

#ifdef __cplusplus
}
#endif

#endif // GPS_H
