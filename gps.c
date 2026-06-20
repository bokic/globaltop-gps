#include "gps.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

char *_strtok_r(char *str, const char search[], char **saveptr)
{
    char *g_str = str ? str : *saveptr;

    if (g_str)
    {
        char *found = strpbrk(g_str, search);

        if (found)
        {
            *found = 0;
            char *ret = g_str;
            *saveptr = found + 1;

            return ret;
        }
        else
        {
            char *ret = g_str;
            *saveptr = NULL;
            if (strlen(ret) > 0)
            {
                return ret;
            }
        }
    }

    return NULL;
}

int gps_buffer_indexOf(const gps_t *gps, const char str[])
{
    int slen = (int)strlen(str);

    if (slen > 0)
    {
        int occupied = gps->gps_buffer_append_pointer - gps->gps_buffer_processing_pointer;
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
                if (gps->gps_buffer[(gps->gps_buffer_processing_pointer + i + c) % GPS_BUFFER_SIZE] != str[c])
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                return (gps->gps_buffer_processing_pointer + i) % GPS_BUFFER_SIZE;
            }
        }
    }

    return -1;
}

void gps_buffer_copy(gps_t *gps, int length)
{
    memset(gps->gps_command_buffer, 0, sizeof(gps->gps_command_buffer));

    if (length > 0)
    {
        if (length >= GPS_COMMAND_SIZE)
        {
            length = GPS_COMMAND_SIZE - 1;
        }

        for (int i = 0; i < length; i++)
        {
            gps->gps_command_buffer[i] = gps->gps_buffer[(gps->gps_buffer_processing_pointer + i) % GPS_BUFFER_SIZE];
        }
    }
}

/* Public functions */

void gps_init(gps_t *gps)
{
    if (gps)
    {
        memset(gps, 0, sizeof(gps_t));
    }
}

void gps_set_callbacks(gps_t *gps, gps_callback_GPRMC_t gprmc, gps_callback_GPGGA_t gpgga, gps_callback_GPVTG_t gpvtg, gps_callback_GPGSA_t gpgsa, gps_callback_GPGSV_t gpgsv)
{
    if (gps)
    {
        gps->gps_callback_GPRMC = gprmc;
        gps->gps_callback_GPGGA = gpgga;
        gps->gps_callback_GPVTG = gpvtg;
        gps->gps_callback_GPGSA = gpgsa;
        gps->gps_callback_GPGSV = gpgsv;
    }
}

void gps_add_data(gps_t *gps, const char buffer[], int len)
{
    if (gps && len > 0)
    {
        if (gps->gps_buffer_append_pointer + len >= GPS_BUFFER_SIZE)
        {
            int part1 = GPS_BUFFER_SIZE - gps->gps_buffer_append_pointer;
            int part2 = len - part1;

            if (part1 > 0)
            {
                memcpy(gps->gps_buffer + gps->gps_buffer_append_pointer, buffer, (unsigned long)part1);
            }

            if (part2 > 0)
            {
                memcpy(gps->gps_buffer, buffer + part1, (unsigned long)part2);
            }

            gps->gps_buffer_append_pointer = part2;
        }
        else
        {
            memcpy(gps->gps_buffer + gps->gps_buffer_append_pointer, buffer, (unsigned long)len);

            gps->gps_buffer_append_pointer = (gps->gps_buffer_append_pointer + len) % GPS_BUFFER_SIZE;
        }
    }
}

void gps_process_data(gps_t *gps)
{
    if (!gps) return;

    char *pch = NULL;
    char *saveptr = NULL;

    // Ignore all leading characters that do not start with '$'
    int start = gps_buffer_indexOf(gps, "$");

    if (start >= 0)
    {
        gps->gps_buffer_processing_pointer = start;

        int end = gps_buffer_indexOf(gps, "\r\n");

        if (end >= 0)
        {
            gps_buffer_copy(gps, end > start? end - start + 2: end + GPS_BUFFER_SIZE - start + 2);

            gps->gps_buffer_processing_pointer = (end + 2) % GPS_BUFFER_SIZE;

            if (!gps_is_packet_valid(gps->gps_command_buffer))
                return;

            if (strlen(gps->gps_command_buffer) >= 7)
            {
                // Match sentence type by skipping the 2-character Talker ID (e.g. $GP, $GN, $GL)
                if ((gps->gps_callback_GPRMC) && (strncmp(gps->gps_command_buffer + 3, "RMC,", 4) == 0))
                {
                    double utc = 0.0;
                    char status = 0;
                    double latitude = 0.0;
                    char ns_indicator = 0;
                    double longitude = 0.0;
                    char ew_indicator = 0;
                    double ground_speed = 0.0;
                    double ground_course = 0.0;
                    int date = 0;
                    double magnetic_variation = 0.0;
                    char magnetic_variation_direction = 0;
                    char mode = 0;

                    pch = _strtok_r(gps->gps_command_buffer, ",", &saveptr);

                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) utc = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) status = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) latitude = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) ns_indicator = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) longitude = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) ew_indicator = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) ground_speed = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) ground_course = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) date = atoi(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) magnetic_variation = atof(pch);
                    pch = _strtok_r(NULL, ",*", &saveptr); if ((pch)&&(strlen(pch) > 0)) magnetic_variation_direction = pch[0];
                    pch = _strtok_r(NULL, "*", &saveptr); if ((pch)&&(strlen(pch) > 0)) mode = pch[0];

                    gps->gps_callback_GPRMC(utc, status, latitude, ns_indicator, longitude, ew_indicator, ground_speed, ground_course, date, magnetic_variation, magnetic_variation_direction, mode);
                }
                else if ((gps->gps_callback_GPVTG) && (strncmp(gps->gps_command_buffer + 3, "VTG,", 4) == 0))
                {
                    double course1 = 0.0;
                    char reference1 = 0;
                    double course2 = 0.0;
                    char reference2 = 0;
                    double speed1 = 0.0;
                    char unit1 = 0;
                    double speed2 = 0.0;
                    char unit2 = 0;
                    char mode = 0;

                    pch = _strtok_r(gps->gps_command_buffer, ",", &saveptr);

                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) course1 = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) reference1 = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) course2 = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) reference2 = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) speed1 = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) unit1 = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) speed2 = atof(pch);
                    pch = _strtok_r(NULL, ",*", &saveptr); if ((pch)&&(strlen(pch) > 0)) unit2 = pch[0];
                    pch = _strtok_r(NULL, "*", &saveptr); if ((pch)&&(strlen(pch) > 0)) mode = pch[0];

                    gps->gps_callback_GPVTG(course1, reference1, course2, reference2, speed1, unit1, speed2, unit2, mode);
                }
                else if ((gps->gps_callback_GPGGA) && (strncmp(gps->gps_command_buffer + 3, "GGA,", 4) == 0))
                {
                    double utc = 0.0;
                    double latitude = 0.0;
                    char ns_indicator = 0;
                    double longitude = 0.0;
                    char ew_indicator = 0;
                    char position_fix_indicator = 0;
                    int satelites_used = 0;
                    double hdop = 0.0;
                    double altitude = 0.0;
                    char altitude_units = 0;
                    double geoidal_separation = 0.0;
                    char geoidal_units = 0;
                    double age_of_diff_correction = 0.0;

                    pch = _strtok_r(gps->gps_command_buffer, ",", &saveptr);

                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) utc = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) latitude = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) ns_indicator = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) longitude = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) ew_indicator = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) position_fix_indicator = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) satelites_used = atoi(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) hdop = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) altitude = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) altitude_units = pch[0];
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) geoidal_separation = atof(pch);
                    pch = _strtok_r(NULL, ",*", &saveptr); if ((pch)&&(strlen(pch) > 0)) geoidal_units = pch[0];
                    pch = _strtok_r(NULL, ",*", &saveptr); if ((pch)&&(strlen(pch) > 0)) age_of_diff_correction = atof(pch);

                    gps->gps_callback_GPGGA(utc, latitude, ns_indicator, longitude, ew_indicator, position_fix_indicator, satelites_used, hdop, altitude, altitude_units, geoidal_separation, geoidal_units, age_of_diff_correction);
                }
                else if ((gps->gps_callback_GPGSA) && (strncmp(gps->gps_command_buffer + 3, "GSA,", 4) == 0))
                {
                    char mode1 = 0;
                    char mode2 = 0;
                    char channels[12] = {0, };
                    double pdop = 0.0;
                    double hdop = 0.0;
                    double vdop = 0.0;

                    pch = _strtok_r(gps->gps_command_buffer, ",", &saveptr);

                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) mode1 = (char)atoi(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) mode2 = (char)atoi(pch);
                    for (int ch = 0; ch < 12; ch++)
                        { pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) channels[ch] = (char)atoi(pch); }
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) pdop = atof(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) hdop = atof(pch);
                    pch = _strtok_r(NULL, ",*", &saveptr); if ((pch)&&(strlen(pch) > 0)) vdop = atof(pch);

                    gps->gps_callback_GPGSA(mode1, mode2, channels, pdop, hdop, vdop);
                }
                else if ((gps->gps_callback_GPGSV) && (strncmp(gps->gps_command_buffer + 3, "GSV,", 4) == 0))
                {
                    uint8_t number_of_messages = 0;
                    uint8_t message_number = 0;
                    uint8_t satelitte_id[4] = {0, };
                    uint8_t elevation[4] = {0, };
                    uint16_t azimuth[4] = {0, };
                    uint16_t snr[4] = {0, };

                    pch = _strtok_r(gps->gps_command_buffer, ",", &saveptr);

                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) number_of_messages = (char)atoi(pch);
                    pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) message_number = (char)atoi(pch);

                    for(int c = 0; c < 4; c++)
                    {
                        pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) satelitte_id[c] = (char)atoi(pch);
                        pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) elevation[c] = (char)atoi(pch);
                        pch = _strtok_r(NULL, ",", &saveptr); if ((pch)&&(strlen(pch) > 0)) azimuth[c] = (char)atoi(pch);
                        pch = _strtok_r(NULL, ",*", &saveptr); if ((pch)&&(strlen(pch) > 0)) snr[c] = (char)atoi(pch);
                    }

                    gps->gps_callback_GPGSV(number_of_messages, message_number, satelitte_id, elevation, azimuth, snr);
                }
            }
        }
    }
}
