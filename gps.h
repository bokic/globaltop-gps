#ifndef GPS_H
#define GPS_H

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*gps_callback_GPRMC_t)(double utc, char status, double latitude, char ns_indicator, double longitude, char ew_indicator, double ground_speed, double ground_course, int date, double magnetic_variation, char magnetic_variation_direction, char mode);
    typedef void (*gps_callback_GPGGA_t)(double utc, double latitude, char ns_indicator, double longitude, char ew_indicator, char position_fix_indicator, int satelites_used, double hdop, double altitude, char altitude_units, double geoidal_separation, char geoidal_units, double age_of_diff_correction);
    typedef void (*gps_callback_GPVTG_t)(double course1, char reference1, double course2, char reference2, double speed1, char unit1, double speed2, char unit2, char mode);
    typedef void (*gps_callback_GPGSA_t)(char mode1, char mode2, char channels[12], double pdop, double hdop, double vdop);
    typedef void (*gps_callback_GPGSV_t)(uint8_t number_of_messages, uint8_t message_number, uint8_t satelitte_id[4], uint8_t elevation[4], uint16_t azimuth[4], uint16_t snr[4]);

    #define GPS_BUFFER_SIZE 1024
    #define GPS_COMMAND_SIZE 256

    typedef struct {
        char gps_buffer[GPS_BUFFER_SIZE];
        char gps_command_buffer[GPS_COMMAND_SIZE];
        int gps_buffer_append_pointer;
        int gps_buffer_processing_pointer;

        gps_callback_GPRMC_t gps_callback_GPRMC;
        gps_callback_GPGGA_t gps_callback_GPGGA;
        gps_callback_GPVTG_t gps_callback_GPVTG;
        gps_callback_GPGSA_t gps_callback_GPGSA;
        gps_callback_GPGSV_t gps_callback_GPGSV;
    } gps_t;

    void gps_init(gps_t *gps);
    void gps_set_callbacks(gps_t *gps, gps_callback_GPRMC_t gprmc, gps_callback_GPGGA_t gpgga, gps_callback_GPVTG_t gpvtg, gps_callback_GPGSA_t gpgsa, gps_callback_GPGSV_t gpgsv);
    void gps_add_data(gps_t *gps, const char buffer[], int len);
    void gps_process_data(gps_t *gps);

#ifdef __cplusplus
}
#endif

#endif // GPS_H
