#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <queue>
#include <iostream>
#include <vector>

#include <stdarg.h>
#define display_print Serial1.printf
/*
GPS related data
*/
char *at_cmd = "ATE0\r\n";

const char *gps_enable_query = "AT+CGPS?\r\n";
const char *gps_enable = "AT+CGPS=1\r\n";
const char *gps_enabled_response = "+CGPS: 1,1\r\n\r\nOK\r\n";
const char *gps_enabled_needs = "+CGPS: 0,1\r\n\r\nOK\r\n";

const char *gnss_enable_query = "AT+CGNSSINFO?\r\n";
const char *gnss_needs_enable = "+CGNSSINFO: 0\r\n\r\nOK\r\n";
const char *gnss_auto_enable = "AT+CGNSSINFO=1\r\n";
const char *gnss_enabled = "+CGNSSINFO: 1\r\n\r\nOK\r\n";
const char *gnss_info_header = "+CGNSSINFO:";

const char *at_ok = "OK\r\n";
const char *at_error = "ERROR\r\n";
#define GSM_RX 16
#define GSM_TX 17

typedef struct
{
    int mode;
    int sat_GPS;
    int sat_GLONASS;
    int sat_BEIDOU;
    float latitude;
    char indicator_ns;
    float longitude;
    char indicator_ew;
    int date;
    float utc_time;
    float alt;
    float speed;
    float course;
    float PDOP;
    float HDOP;
    float VDOP;
} GNSS_DATA;

typedef struct
{
    GNSS_DATA gps_data;
    int day;
    int month;
    int year;
    int seconds;
    int minutes;
    int hours;
} GNSS_ENVELOPE;

int gps_check_state = 0;
const int pwrkey = 14;
GNSS_ENVELOPE data;

/*
Display related data
*/

int display_check_state = 0;
uint8_t brightness_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t page_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t hour_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t minutes_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t seconds_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t day_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t month_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t year_data[] = {0x5a, 0xa5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};

/*
alarm related data
*/
int alarm_state_check[2] = {0, 0};
int alarm_hour[2] = {6, 8};
int alarm_minute[2] = {30, 30};
int snooze_timeout = 5;

int user_defined_hour = 0;
int user_defined_minute = 0;
bool snooze_clicked = false;
bool stop_alarm_clicked = false;
/*
common data
*/
#define LENGTH_OF_ARRAY(ARR) (sizeof(ARR) / sizeof(ARR[0]))
bool send_display = false;

char *check_string = "+CGNSSINFO: 10,10,10,10,2049.457465,N,12445.882711,E,230524,190031.0,7.9,0.0,,0.0,0.0,0.0";

static char display_date[5], display_month[5];
static int display_day, display_hour, display_minute, display_seconds, display_year;

static long int tz_constant[] = {0, 19800, -25200, 32400}; // utc ist pst jst

static char tz_string[][20] = {
    "UTC+0    UTC ",
    "UTC+5:30 IST ",
    "UTC-7:00 PST ",
    "UTC+9:00 JST "};
static int time_zone = 0;
static int current_page = 0;

using namespace std;

std::vector<uint8_t> dwinVector{};
std::queue<std::vector<uint8_t>> data_queue;

long int queueTimer = 0;
void process_display(uint8_t *data_buffer, uint8_t data_size);
typedef struct __attribute__((__packed__)) dwin_data_slider
{
    uint8_t header[2];      // 5a a5
    uint8_t size;           // 06
    uint8_t operation_type; // write or read
    uint8_t vp_address[2];  // vp address
    uint8_t data_type;      // 01 - slider
    uint8_t data_frame[2];  // 00 10
} dwin_data_slider;         // Fixed the typedef declaration

#define ACK_ADDRESS 0x4f4b

typedef struct __attribute__((__packed__)) dwin_data_ack
{
    uint8_t header[2];      // 5a a5
    uint8_t size;           // 06
    uint8_t operation_type; // write or read
    uint8_t vp_address[2];  // vp address
} dwin_data_ack;            // Fixed the typedef declaration

#define DISPLAY_HEADER 0x5a
TaskHandle_t taskMpHandle = NULL;

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, GSM_RX, GSM_TX);
    Serial1.begin(115200, SERIAL_8N1, 25, 33);
    // pinMode(pwrkey, OUTPUT);
    xTaskCreatePinnedToCore(
        serial_task,      /* Task function. */
        "gsm",            /* name of task. */
        10000,            /* Stack size of task */
        NULL,             /* parameter of the task */
        tskIDLE_PRIORITY, /* priority of the task */
        &taskMpHandle,    /* Task handle to keep track of created task */
        0);               /* pin task to core 0 */
}

void serial_task(void *args)
{
    while (1)
    {
        check_incoming_serial_message();
        process_data_queue();
    }
}
void loop()
{
    monitor_gps();
}

void monitor_gps()
{
    static long int timer_expired = 0;
    static int saved_state = 0;
    switch (gps_check_state)
    {
    case 0:
    {
        debug_print("sending at message\n");
        Serial2.printf("%s", at_cmd);
        timer_expired = millis();
        gps_check_state = 1;
    }
    break;
    case 1:
    {
        if (millis() - timer_expired >= 500)
        {
            timer_expired = millis();
            gps_check_state = 0;
        }
        else
        {
            uint8_t data_bytes = Serial2.available();
            if (data_bytes)
            {
                debug_print("1- Size received %d\n", data_bytes);
                timer_expired = millis();
                // char received_message[data_bytes];
                String received_message = Serial2.readString();
                debug_print("%s", received_message);
                if (memcmp(received_message.c_str(), at_ok, data_bytes) == 0)
                {
                    debug_print("Device is responding. Now we will enable GPS\n");
                    gps_check_state = 2;
                }
            }
        }
    }
    break;
    case 2:
    {
        debug_print("%s", gps_enable_query);
        Serial2.printf("%s", gps_enable_query);
        timer_expired = millis();
        gps_check_state = 3;
    }
    break;
    case 3:
    {
        if (millis() - timer_expired >= 5000)
        {
            debug_print("Timeout after sending gps enable message\n");
            timer_expired = millis();
            gps_check_state = 0;
        }
        else
        {
            int data_bytes = Serial2.available();
            if (data_bytes)
            {
                timer_expired = millis();

                String gps_check_enable = Serial2.readString();
                debug_print("3 - %s", gps_check_enable.c_str());
                if (memcmp(gps_check_enable.c_str(), gps_enabled_response, data_bytes) == 0)
                {
                    debug_print("GPS is enabled. Now we will set GNSS to respon every 1s\n");
                    gps_check_state = 4;
                }
                else if (memcmp(gps_check_enable.c_str(), gps_enabled_needs, data_bytes) == 0)
                {
                    debug_print("[3]lets send gps enable message\n");
                    Serial2.write("AT+CGPS=1\r\n", LENGTH_OF_ARRAY("AT+CGPS=1\r\n"));
                    saved_state = 2;
                    gps_check_state = 7;
                }
                else if (memcmp(gps_check_enable.c_str(), at_error, data_bytes) == 0)
                {
                    debug_print("GPS enable error. retry\n");
                    gps_check_state = 2;
                }
            }
        }
    }
    break;
    case 4:
    {
        debug_print("%s", gnss_auto_enable);
        Serial2.printf("%s", gnss_auto_enable);
        timer_expired = millis();
        gps_check_state = 5;
    }
    break;
    case 5:
    {
        if (millis() - timer_expired >= 5000)
        {
            debug_print("Timeout after sending gnss enable message\n");
            timer_expired = millis();
            gps_check_state = 0;
        }
        else
        {
            int data_bytes = Serial2.available();
            if (data_bytes)
            {
                timer_expired = millis();

                String gps_check_enable = Serial2.readString();
                debug_print("5 - %s", gps_check_enable.c_str());
                if (strcmp(gps_check_enable.c_str(), at_ok) == 0)
                {
                    debug_print("gnss is enabled. Now we will RECEIVE GNSS  every 1s\n");
                    gps_check_state = 6;
                }
                else if (strcmp(gps_check_enable.c_str(), at_error) == 0)
                {
                    debug_print("gnss enable error. retry\n");
                    gps_check_state = 4;
                }
                else if (memcmp(gps_check_enable.c_str(), gnss_info_header, data_bytes) == 0)
                {
                    debug_print("5-valid gnss info received. lets process it\n");
                    process_gnss((char *)gps_check_enable.c_str());
                    gps_check_state = 6;
                }
            }
        }
    }
    break;
    case 6:
    {
        if (millis() - timer_expired >= 1500)
        {
            debug_print("--Timeout--\n");
            timer_expired = millis();
            // gps_check_state = 0;
        }
        else
        {
            int data_bytes = Serial2.available();
            if (data_bytes)
            {
                String gnss_data = Serial2.readString();
                debug_print("6-%s", gnss_data.c_str());
                timer_expired = millis();

                if (memcmp(gnss_data.c_str(), gnss_info_header, data_bytes) == 0)
                {
                    debug_print("7-valid gnss info received. lets process it\n");
                    process_gnss((char *)gnss_data.c_str());
                    send_display = true;
                }
            }
        }
    }
    break;
    case 7:
    {
        if (millis() - timer_expired >= 500)
        {
            timer_expired = millis();
            gps_check_state = saved_state;
        }
    }
    break;
    default:
    {
        gps_check_state = 0;
    }
    break;
    }
}

void process_gnss(char *data_str)
{
    // Extract the values from the string and store them in the struct
    sscanf(data_str, "\r\n+CGNSSINFO: %d,%d,%d,%d,%f,%c,%f,%c,%d,%f,%f,%f,%f,%f,%f,%f",
           &data.gps_data.mode, &data.gps_data.sat_GPS, &data.gps_data.sat_GLONASS, &data.gps_data.sat_BEIDOU,
           &data.gps_data.latitude, &data.gps_data.indicator_ns, &data.gps_data.longitude, &data.gps_data.indicator_ew,
           &data.gps_data.date, &data.gps_data.utc_time, &data.gps_data.alt, &data.gps_data.speed, &data.gps_data.course,
           &data.gps_data.PDOP, &data.gps_data.HDOP, &data.gps_data.VDOP);

    data.year = data.gps_data.date % 100;
    data.day = data.gps_data.date / 10000;
    data.month = (data.gps_data.date - data.day * 10000) / 100;

    data.seconds = int(data.gps_data.utc_time) % 100;
    data.hours = data.gps_data.utc_time / 10000;
    data.minutes = (data.gps_data.utc_time - data.hours * 10000) / 100;

    send_display = true;
    datetime_gps();
}

void datetime_gps()
{
    time_t t;
    static struct tm utc_parsed = {.tm_sec = data.seconds, .tm_min = data.minutes, .tm_hour = data.hours, .tm_mday = data.day, .tm_mon = (data.month - 1), .tm_year = (data.year + 100)};
    t = mktime(&utc_parsed) + tz_constant[time_zone];
    struct tm *local_parsed = gmtime(&t);
    sscanf(asctime(local_parsed), "%s %s %02d %02d:%02d:%02d %04d", display_date, display_month, &display_day, &display_hour, &display_minute, &display_seconds, &display_year);
    // printf("%s %s %02d %02d:%02d:%02d %04d\n", display_date, display_month, display_day, display_hour, display_minute, display_seconds, display_year);
    send_message();
}

void set_alarm(int i)// process state machine for two alarms
{
    static int timer_expired = 0;
    switch (alarm_state_check[i])
    {
    case 0:
    {
        if (alarm_hour[i] == data.hours && alarm_minute[i] == data.minutes)
        {
            // time for alarm.
            // enable popup
            // enable music
            alarm_state_check[i] = 1;
            timer_expired = millis();
        }
    }
    break;
    case 1:
    {
        if (millis() - timer_expired > 1000)
        {
            alarm_state_check[i] = 0;
        }
        else
        {
            if (snooze_clicked)
            {
                snooze_clicked = false;
                alarm_minute[i] = alarm_minute[i] + snooze_timeout;//needs to verify the logic
                alarm_state_check[i] = 0;
            }
            else if (stop_alarm_clicked)
            {
                alarm_state_check[i] = 0;
            }
        }
    }
    break;
    default:
        break;
    }
}

void print_data()
{
    debug_print("+CGNSSINFO: %d, %d, %d, %d, %f, %c, %f, %c, %d, %.1f, %.1f, %.1f, %.1f,%.1f,%.1f,%.1f",
                data.gps_data.mode, data.gps_data.sat_GPS, data.gps_data.sat_GLONASS, data.gps_data.sat_BEIDOU,
                data.gps_data.latitude, data.gps_data.indicator_ns, data.gps_data.longitude, data.gps_data.indicator_ew,
                data.gps_data.date, data.gps_data.utc_time, data.gps_data.alt, data.gps_data.speed, data.gps_data.course,
                data.gps_data.PDOP, data.gps_data.HDOP, data.gps_data.VDOP);
}

void check_incoming_serial_message()
{
    int serial_data_size = Serial1.available();
    if (serial_data_size)
    {
        debug_print("serial1 %d\n", serial_data_size);
        uint8_t display_data[serial_data_size];
        Serial1.read(display_data, serial_data_size);
        dwinVector.insert(dwinVector.end(), display_data, display_data + serial_data_size);
        data_queue.push(dwinVector);
        dwinVector.clear();
    }
}

void process_data_queue()
{
    if (!data_queue.empty())
    {
        uint8_t _temp_size = data_queue.front().size();
        uint8_t _temp_buf[_temp_size];
        memcpy(_temp_buf, data_queue.front().data(), _temp_size);
        process_display(_temp_buf, _temp_size);
        data_queue.pop();
    }
}

void process_display(uint8_t *data_buffer, uint8_t data_size)
{
    uint16_t DWIN_HEADER = 0x5aa5;
    const uint16_t TZ_CHANGE = 0x301c;
    if (data_size == sizeof(dwin_data_ack))
    {
        dwin_data_ack *dwin_ack = NULL;
        dwin_ack = (dwin_data_ack *)data_buffer;
        uint16_t header = dwin_ack->header[1] | dwin_ack->header[0] << 8;
        uint16_t vp_address = dwin_ack->vp_address[1] | dwin_ack->vp_address[0] << 8;
        if (header == DWIN_HEADER && vp_address == ACK_ADDRESS)
        {
            // ack message received
        }
    }
    else if (data_size == sizeof(dwin_data_slider))
    {
        dwin_data_slider *dwin_pwm_data = NULL;
        dwin_pwm_data = (dwin_data_slider *)data_buffer;
        uint16_t header = dwin_pwm_data->header[1] | dwin_pwm_data->header[0] << 8;
        if (header == DWIN_HEADER)
        {
            uint16_t vp_address = dwin_pwm_data->vp_address[1] | dwin_pwm_data->vp_address[0] << 8;
            uint16_t data_frame = dwin_pwm_data->data_frame[1] | dwin_pwm_data->data_frame[0] << 8;
            switch (vp_address)
            {
            case TZ_CHANGE:
            {
                Serial.printf("Change tz to -> %d %s\n", data_frame, tz_constant[data_frame]);
                time_zone = data_frame;
            }
            break;
            default:
                break;
            }
        }
    }
}

void debug_print(const char *fmt, ...)
{
    char buff[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff, sizeof(buff), fmt, args);
    Serial.write(buff, strlen(buff));
    va_end(args);
}

void send_message()
{
    uint8_t hours_message[] = {0x5a, 0xa5, 0x05, 0x82, 0x21, 0x02, 0x00, (uint8_t)display_hour};
    uint8_t minutes_message[] = {0x5a, 0xa5, 0x05, 0x82, 0x21, 0x06, 0x00, (uint8_t)display_minute};
    uint8_t seconds_message[] = {0x5a, 0xa5, 0x05, 0x82, 0x21, 0x0a, 0x00, (uint8_t)display_seconds};
    // uint8_t page_change[] = {0x5a, 0xa5, 0x07, 0x82, 0x00, 0x84, 0x5a, 0x01, 0x00, 0x00};
    uint8_t gnss_message[] = {0x5a, 0xa5, 0x05, 0x82, 0x21, 0x0f, 0x00, (uint8_t)data.gps_data.sat_GPS};
    uint8_t gal_message[] = {0x5a, 0xa5, 0x05, 0x82, 0x21, 0x14, 0x00, (uint8_t)data.gps_data.sat_GLONASS};
    uint8_t beidou_message[] = {0x5a, 0xa5, 0x05, 0x82, 0x21, 0x18, 0x00, (uint8_t)data.gps_data.sat_BEIDOU};
    Serial1.write(hours_message, LENGTH_OF_ARRAY(hours_message));
    delay(5);
    Serial1.write(minutes_message, LENGTH_OF_ARRAY(minutes_message));
    delay(5);
    Serial1.write(seconds_message, LENGTH_OF_ARRAY(seconds_message));
    delay(5);
    Serial1.write(gnss_message, LENGTH_OF_ARRAY(gnss_message));
    delay(5);
    Serial1.write(gal_message, LENGTH_OF_ARRAY(gal_message));
    delay(5);
    Serial1.write(beidou_message, LENGTH_OF_ARRAY(beidou_message));
    delay(5);
    process_hour(data.hours);
    char day_details[50];
    snprintf(day_details, sizeof(day_details), "%s, %d %s %d", display_date, display_day, display_month, display_year);
    process_string(0x30ae, day_details);
}

void process_string(uint16_t vp_address, String text_message)
{
    uint8_t addr_l = (vp_address & 0xff);
    uint8_t addr_h = (vp_address >> 8 & 0xff);
    Serial1.write(0x5a);
    Serial1.write(0xa5);
    Serial1.write((uint8_t)strlen(text_message.c_str()) + 0x02);
    Serial1.write(addr_h);
    Serial1.write(addr_l);
    Serial1.printf("%s", text_message.c_str());
}

void process_hour(uint8_t hrs)
{
    uint8_t day_icon = 0;

    if (hrs >= 12 && hrs < 15)
    {
        day_icon = 5;
    }
    else if (hrs >= 15 && hrs <= 18)
    {
        day_icon = 4;
    }
    else if (hrs >= 19 && hrs <= 23 || hrs >= 0 && hrs <= 5)
    {
        day_icon = 2;
    }
    else if (hrs >= 6 && hrs <= 8)
    {
        day_icon = 1;
    }
    else if (hrs >= 9 && hrs < 11)
    {
        day_icon = 0;
    }
    Serial.printf("hrs = %x day icon = %x", hrs, day_icon);
    uint8_t icon_message[] = {0x5a, 0xa5, 0x05, 0x82, 0x30, 0x00, 0x00, day_icon};
    Serial1.write(icon_message, LENGTH_OF_ARRAY(icon_message));
}
