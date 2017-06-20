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

void callback_GPRMC(float utc, char status, float latitude, char ns_indicator, float longitude, char ew_indicator, float ground_speed, float ground_course, int date, float magnetic_variation, char magnetic_variation_direction, char mode)
{
    return;
}

void callback_GPGGA(float utc, float latitude, char ns_indicator, float longitude, char ew_indicator, char position_fix_indicator, int satelites_used, float hdop, float altitude, char altitude_units, float geoidal_separation, char geoidal_units, float age_of_diff_correction)
{
    return;
}

void callback_GPVTG(float course1, char reference1, float course2, char reference2, float speed1, char unit1, float speed2, char unit2, char mode)
{
    return;
}

void callback_GPGSA(char mode1, char mode2, char channels[12], float pdop, float hdop, float vdop)
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

    gps_set_callbacks(callback_GPRMC, callback_GPGGA, callback_GPVTG, callback_GPGSA, callback_GPGSV);

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
                gps_add_data(buf.constData(), buf.length());

                gps_process_data();
            }
        }

        if (c > 50)
            break;

        c++;

        usleep(50000);
    }

    return 0;
}
