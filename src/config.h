// macros and defines

#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))

#define SUBSCRIBE_TOPIC "devices/" DEVICE_ID "/messages/devicebound/#"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0

#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define UNIX_TIME_NOV_13_2017 1510592825
#define PST_TIME_ZONE 1
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1
#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

#define DATA_BUFFER_SIZE 128

#define TELEMETRY_FREQUENCY_MILLISECS 30000

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static char mqtt_publish_topic[128];

int64_t next_time {0};
time_t current_time;
int message_count {1};
const u_int16_t buffer_size = 2048;
