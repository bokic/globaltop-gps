#include "gps.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define GPS_BUFFER_SIZE 1024
#define GPS_COMMAND_SIZE 256

static char gps_buffer[GPS_BUFFER_SIZE];
static char gps_command_buffer[GPS_COMMAND_SIZE];
static int gps_buffer_append_pointer = 0;
static int gps_buffer_processing_pointer = 0;

static gps_callback_GPRMC_t gps_callback_GPRMC = NULL;
static gps_callback_GPGGA_t gps_callback_GPGGA = NULL;
static gps_callback_GPVTG_t gps_callback_GPVTG = NULL;
static gps_callback_GPGSA_t gps_callback_GPGSA = NULL;
static gps_callback_GPGSV_t gps_callback_GPGSV = NULL;

/* Private functions */

bool gps_is_packet_valid(const char *packet)
{
    uint8_t current_checksum = 0;
    char checksumStr[3] = {0, };
    int c = 0;

    for(c = 0; ; c++)
    {
        if (packet[c] == 0) return false;
        if (c == 0) continue;
        if (packet[c] == '*') break;

        current_checksum ^= packet[c];
    }

    if (packet[c + 1] == '\0' || packet[c + 2] == '\0')
    {
        return false;
    }

    checksumStr[0] = packet[c + 1];
    checksumStr[1] = packet[c + 2];
    checksumStr[2] = 0;

    return strtol(checksumStr, NULL, 16) == current_checksum;
}

char *_strtok(char *str, const char search[])
{
    static char *g_str = NULL;

    if (str)
    {
        g_str = str;
    }

    if (g_str)
    {
        char *found = strpbrk(g_str, search);

        if (found)
        {
            *found = 0;
            char *ret = g_str;
            g_str = found + 1;

            return ret;
        }
        else
        {
            char *ret = g_str;
            g_str = NULL;
            if (strlen(ret) > 0)
            {
                return ret;
            }
        }
    }

    return NULL;
}

int gps_buffer_indexOf(const char str[])
{
    int slen = (int)strlen(str);

    if (slen > 0)
    {
        int occupied = gps_buffer_append_pointer - gps_buffer_processing_pointer;
        if (occupied < 0)
        {
            occupied += GPS_BUFFER_SIZE;
        }

        int limit = occupied - slen;
        if (limit < 0)
        {
            return -1;
        }

        for(int i = 0; i <= limit; i++)
        {
            bool found = true;

            for(int c = 0; c < slen; c++)
            {
                if (gps_buffer[(gps_buffer_processing_pointer + i + c) % GPS_BUFFER_SIZE] != str[c])
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                return (gps_buffer_processing_pointer + i) % GPS_BUFFER_SIZE;
            }
        }
    }

    return -1;
}

void gps_buffer_copy(int length)
{
    memset(gps_command_buffer, 0, sizeof(gps_command_buffer));

    if (length > 0)
    {
        if (length >= GPS_COMMAND_SIZE)
        {
            length = GPS_COMMAND_SIZE - 1;
        }

        for (int i = 0; i < length; i++)
        {
            gps_command_buffer[i] = gps_buffer[(gps_buffer_processing_pointer + i) % GPS_BUFFER_SIZE];
        }
    }
}

/* Public functions */

void gps_set_callbacks(gps_callback_GPRMC_t gprmc, gps_callback_GPGGA_t gpgga, gps_callback_GPVTG_t gpvtg, gps_callback_GPGSA_t gpgsa, gps_callback_GPGSV_t gpgsv)
{
    gps_callback_GPRMC = gprmc;
    gps_callback_GPGGA = gpgga;
    gps_callback_GPVTG = gpvtg;
    gps_callback_GPGSA = gpgsa;
    gps_callback_GPGSV = gpgsv;
}

void gps_add_data(const char buffer[], int len)
{
    if (len > 0)
    {
        if (gps_buffer_append_pointer + len >= GPS_BUFFER_SIZE)
        {
            int part1 = GPS_BUFFER_SIZE - gps_buffer_append_pointer;
            int part2 = len - part1;

            if (part1 > 0)
            {
                memcpy(gps_buffer + gps_buffer_append_pointer, buffer, (unsigned long)part1);
            }

            if (part2 > 0)
            {
                memcpy(gps_buffer, buffer + part1, (unsigned long)part2);
            }

            gps_buffer_append_pointer = part2;
        }
        else
        {
            memcpy(gps_buffer + gps_buffer_append_pointer, buffer, (unsigned long)len);

            gps_buffer_append_pointer = (gps_buffer_append_pointer + len) % GPS_BUFFER_SIZE;
        }
    }
}

void gps_process_data()
{
    char *pch = NULL;

    // Ignore all leading characters that do not start with '$GP'
    int start = gps_buffer_indexOf("$GP");

    //printf("start %i\n", start); fflush(stdout);

    if (start >= 0)
    {
        gps_buffer_processing_pointer = start;

        int end = gps_buffer_indexOf("\r\n");

        //printf("end %i\n", end); fflush(stdout);

        if (end >= 0)
        {
            gps_buffer_copy(end > start? end - start + 2: end + GPS_BUFFER_SIZE - start + 2);

            gps_buffer_processing_pointer = (end + 2) % GPS_BUFFER_SIZE;

            if (!gps_is_packet_valid(gps_command_buffer))
                return;

            if ((gps_callback_GPRMC)&&(strncmp(gps_command_buffer, "$GPRMC,", 7) == 0)) // Time, date, position, course, speed(RMC—Recommended Minimum Navigation Information)
            {
                float utc = 0.0;
                char status = 0;
                float latitude = 0.0;
                char ns_indicator = 0;
                float longitude = 0.0;
                char ew_indicator = 0;
                float ground_speed = 0.0; // in knots
                float ground_course = 0.0; // in degrees
                int date = 0;
                float magnetic_variation = 0.0;
                char magnetic_variation_direction = 0;
                char mode = 0;
                int checksum = 0;

                pch = _strtok(gps_command_buffer, ",");

                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) utc = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) status = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) latitude = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) ns_indicator = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) longitude = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) ew_indicator = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) ground_speed = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) ground_course = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) date = atoi(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) magnetic_variation = (float)atof(pch);
                pch = _strtok(NULL, ",*"); if ((pch)&&(strlen(pch) > 0)) magnetic_variation_direction = pch[0];
                pch = _strtok(NULL, "*"); if ((pch)&&(strlen(pch) > 0)) mode = pch[0];
                pch = _strtok(NULL, "\r"); if ((pch)&&(strlen(pch) > 0)) checksum = (int)strtol(pch, NULL, 16);

                gps_callback_GPRMC(utc, status, latitude, ns_indicator, longitude, ew_indicator, ground_speed, ground_course, date, magnetic_variation, magnetic_variation_direction, mode);
            }
            else if ((gps_callback_GPVTG)&&(strncmp(gps_command_buffer, "$GPVTG,", 7) == 0)) // Course and speed relative to ground
            {
                // VTG—Course and speed information relative to the ground

                float course1 = 0.0; // in degrees
                char reference1 = 0;
                float course2 = 0.0; // in degrees
                char reference2 = 0;
                float speed1 = 0.0;
                char unit1 = 0;
                float speed2 = 0.0;
                char unit2 = 0;
                char mode = 0;
                int checksum = 0;

                pch = _strtok(gps_command_buffer, ",");

                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) course1 = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) reference1 = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) course2 = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) reference2 = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) speed1 = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) unit1 = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) speed2 = (float)atof(pch);
                pch = _strtok(NULL, ",*"); if ((pch)&&(strlen(pch) > 0)) unit2 = pch[0];
                pch = _strtok(NULL, "*"); if ((pch)&&(strlen(pch) > 0)) mode = pch[0];
                pch = _strtok(NULL, "\r"); if ((pch)&&(strlen(pch) > 0)) checksum = (int)strtol(pch, NULL, 16);

                gps_callback_GPVTG(course1, reference1, course2, reference2, speed1, unit1, speed2, unit2, mode);
            }
            else if ((gps_callback_GPGGA)&&(strncmp(gps_command_buffer, "$GPGGA,", 7) == 0)) // Time, position, fix type
            {
                // GGA—Global Positioning System Fixed Data. Time, Position and fix related data

                float utc = 0.0;                  // UTC time: hhmmss.sss
                float latitude = 0.0;             // Latitude: ddmm.mmmm
                char ns_indicator = 0;            // N=north, S=south
                float longitude = 0.0;            // Longitude: ddmm.mmmm
                char ew_indicator = 0;            // E=east, W=weast
                char position_fix_indicator = 0;  // 0=fix not available, 1=GPS fix, 2=Differential GPS fix
                int satelites_used = 0;           // Number of satellites used
                float hdop = 0.0;                 // horizontal delution of precision
                float altitude = 0.0;             // Antenna altitude above/below mean-sea-level
                char altitude_units = 0;          // Units of altitude(m=meters)
                float geoidal_separation = 0.0;   // Geoidal separation
                char geoidal_units = 0;           // Units of geoidal separation(m=meters)
                float age_of_diff_correction = 0; // How old diff correction is old in seconds
                int checksum = 0;

                pch = _strtok(gps_command_buffer, ",");

                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) utc = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) latitude = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) ns_indicator = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) longitude = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) ew_indicator = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) position_fix_indicator = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) satelites_used = (int)atoi(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) hdop = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) altitude = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) altitude_units = pch[0];
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) geoidal_separation = (float)atof(pch);
                pch = _strtok(NULL, ",*"); if ((pch)&&(strlen(pch) > 0)) geoidal_units = pch[0];
                pch = _strtok(NULL, ",*"); if ((pch)&&(strlen(pch) > 0)) age_of_diff_correction = (float)atof(pch);
                pch = _strtok(NULL, "\r"); if ((pch)&&(strlen(pch) > 0)) checksum = (int)strtol(pch, NULL, 16);

                gps_callback_GPGGA(utc, latitude, ns_indicator, longitude, ew_indicator, position_fix_indicator, satelites_used, hdop, altitude, altitude_units, geoidal_separation, geoidal_units, age_of_diff_correction);
            }
            else if ((gps_callback_GPGSA)&&(strncmp(gps_command_buffer, "$GPGSA,", 7) == 0)) // Opration mode, active satelites, DOP values
            {
                // GSA—GNSS DOP and Active Satellites

                char mode1 = 0;           // M = Manual(force 2D or 3D), A = Automatic
                char mode2 = 0;           // 1 = Fix not available, 2 = 2D(< 4 SVs used), 3 3D(>= 4 SVs used)
                char channels[12] ={0, }; // Satelite id for each channel
                float pdop = 0.0;         // Position dilution of precision
                float hdop = 0.0;         // Horizontal dilution of precision
                float vdop = 0.0;         // Vertical dilution of precision
                int checksum = 0;

                pch = _strtok(gps_command_buffer, ",");

                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) mode1 = (char)atoi(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) mode2 = (char)atoi(pch);
                for (int ch = 0; ch < 12; ch++)
                    { pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) channels[ch] = (char)atoi(pch); }
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) pdop = (float)atof(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) hdop = (float)atof(pch);
                pch = _strtok(NULL, ",*"); if ((pch)&&(strlen(pch) > 0)) vdop = (float)atof(pch);
                pch = _strtok(NULL, "\r"); if ((pch)&&(strlen(pch) > 0)) checksum = (int)strtol(pch, NULL, 16);

                gps_callback_GPGSA(mode1, mode2, channels, pdop, hdop, vdop);
            }
            else if ((gps_callback_GPGSV)&&(strncmp(gps_command_buffer, "$GPGSV,", 7) == 0)) // Number of GPS satelites in vew, satelite IDs, elevation, azimuth, SNR values
            {
                // GSV—GNSS Satellites in View

                uint8_t number_of_messages = 0;  //
                uint8_t message_number = 0;      //
                uint8_t satelitte_id[4] = {0, }; //
                uint8_t elevation[4] = {0, };    //
                uint16_t azimuth[4] = {0, };     //
                uint16_t snr[4] = {0, };         //

                pch = _strtok(gps_command_buffer, ",");

                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) number_of_messages = (char)atoi(pch);
                pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) message_number = (char)atoi(pch);

                for(int c = 0; c < 4; c++)
                {
                    pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) satelitte_id[c] = (char)atoi(pch);
                    pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) elevation[c] = (char)atoi(pch);
                    pch = _strtok(NULL, ","); if ((pch)&&(strlen(pch) > 0)) azimuth[c] = (char)atoi(pch);
                    pch = _strtok(NULL, ",*"); if ((pch)&&(strlen(pch) > 0)) snr[c] = (char)atoi(pch);
                }

                gps_callback_GPGSV(number_of_messages, message_number, satelitte_id, elevation, azimuth, snr);
            }
        }
    }
}
