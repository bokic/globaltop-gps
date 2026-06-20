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

void callback_GPRMC(double utc, char status, double latitude, char ns_indicator, double longitude, char ew_indicator, double ground_speed, double ground_course, int date, double magnetic_variation, char magnetic_variation_direction, char mode)
{
    return;
}

void callback_GPGGA(double utc, double latitude, char ns_indicator, double longitude, char ew_indicator, char position_fix_indicator, int satelites_used, double hdop, double altitude, char altitude_units, double geoidal_separation, char geoidal_units, double age_of_diff_correction)
{
    return;
}

void callback_GPVTG(double course1, char reference1, double course2, char reference2, double speed1, char unit1, double speed2, char unit2, char mode)
{
    return;
}

void callback_GPGSA(char mode1, char mode2, char channels[12], double pdop, double hdop, double vdop)
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
