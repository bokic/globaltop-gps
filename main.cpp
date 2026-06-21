#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <iostream>

#include <QSerialPortInfo>
#include <QSerialPort>

#include "gps.h"


using namespace std;

void callback_GPRMC(uint32_t utc, char status, int32_t latitude, char ns_indicator, int32_t longitude, char ew_indicator, uint16_t ground_speed, uint16_t ground_course, uint32_t date, uint16_t magnetic_variation, char magnetic_variation_direction, char mode)
{
    return;
}

void callback_GPGGA(uint32_t utc, int32_t latitude, char ns_indicator, int32_t longitude, char ew_indicator, char position_fix_indicator, uint8_t satelites_used, uint16_t hdop, int16_t altitude, char altitude_units, int16_t geoidal_separation, char geoidal_units, uint16_t age_of_diff_correction)
{
    return;
}

void callback_GPVTG(uint16_t course1, char reference1, uint16_t course2, char reference2, uint16_t speed1, char unit1, uint16_t speed2, char unit2, char mode)
{
    return;
}

void callback_GPGSA(char mode1, char mode2, char channels[12], uint16_t pdop, uint16_t hdop, uint16_t vdop)
{
    return;
}

void callback_GPGSV(uint8_t number_of_messages, uint8_t message_number, uint8_t satelitte_id[4], uint8_t elevation[4], uint16_t azimuth[4], uint16_t snr[4])
{
    return;
}

int main()
{
    QString portname;
    gps_t gps;

    gps_init(&gps);
    gps_set_callbacks(&gps, callback_GPRMC, callback_GPGGA, callback_GPVTG, callback_GPGSA, callback_GPGSV);

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    portname = ports.at(0).portName();

    QSerialPort port(portname);

    port.setBaudRate(QSerialPort::Baud9600);

    cout << "Opening port: " << portname.toUtf8().constData() << endl;
    if (!port.open(QIODevice::ReadWrite))
    {
        cerr << "Open FAILED! Error: " << port.errorString().toUtf8().constData() << endl;
        return 1;
    }

    int c = 0;

    while(port.isOpen())
    {
        if (port.waitForReadyRead(100))
        {
            auto buf = port.readAll();

            if (buf.length() > 0)
            {
                gps_add_data(&gps, buf.constData(), buf.length());

                gps_process_data(&gps);
            }
        }

        if (c > 50)
            break;

        c++;

        usleep(50000);
    }

    return 0;
}
