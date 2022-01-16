
#include "WiFi.h"
#include "secret.h"
#include "esp_sntp.h"
#include "time.h"
#define GPIO_LED 21
#define GPIO_BUTTONL 5
#define GPIO_BUTTONR 4

static QueueHandle_t xqueue;
static QueueHandle_t queue;
static const int reset_press = -998;

const char * ntpServer = "nl.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;


void WiFi_connect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printf(".\r\n");
  }
  printf(" Verbonden met WiFi\r\n");
}

char * printLocalTime() {
  //https://docs.espressif.com/projects/esp-idf/en/latest/esp32/apireference/system/system_time.html
  time_t now;
  char * time_buf = (char * ) malloc(64 * sizeof(char));
  struct tm timeinfo;
  time( & now);
  localtime_r( & now, & timeinfo);
  strftime(time_buf, (64 * sizeof(char)), "%c", & timeinfo);
  //ESP_LOGI(TAG, "\r\nde datum/time is: %s\r\n", time_buf);
  return time_buf;
}

static void set_time(void * para)
{
  // init time protocol sync
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();
  //vTaskSuspend(NULL);
  vTaskDelete(NULL);
}

static void debounce_task(void * argp) {
  unsigned button_gpio = * (unsigned * ) argp;
  uint32_t level, state = 0;
  uint32_t mask = 0x7FFFFFFF;
  int event, last = -999;

 for (;;) {
 level = !digitalRead(button_gpio);
 state = (state << 1) | level;
   if ((state & mask) == mask)
    event = button_gpio; // Press
   else
    event = -button_gpio; // Release
if (event != last) {
if (xQueueSendToBack(queue, & event, 0) == pdPASS) {
last = event;
} else if (event < 0) {
       
  do {
xQueueReset(queue); // Empty queue
} while (xQueueSendToBack(queue, & reset_press, 0) != pdPASS);
last = event;}
  }
    taskYIELD();
  }
}
//
// Hydraulic Press Task (LED)
//
static void press_task(void * argp) {
  static
  const uint32_t enable = (1 << GPIO_BUTTONL) |
    (1 << GPIO_BUTTONR);
  BaseType_t s;
  int event;
  uint32_t state = 0;
  // Make sure press is OFF
  digitalWrite(GPIO_LED, LOW);
  for (;;) {
   s = xQueueReceive(
   queue, &
   event,
   portMAX_DELAY
    );
    assert(s == pdPASS);
   if (event == reset_press) {
   digitalWrite(GPIO_LED, LOW);
   state = 0;
   printf("RESET!!\n");
    continue;
  }
 if (event >= 0) {
  // Button press
  state |= 1 << event;
 } else {
  // Button release
  state &= ~(1 << -event);
 }
 if (state == enable) {
 // Activate press when both
// Left and Right buttons are
// pressed.
   digitalWrite(GPIO_LED, HIGH);

   char trash[64];
   printf("pressed\r\n");

 if (xQueueSendToBack(xqueue, printLocalTime(), 0) != pdPASS) {
     xQueueReceive(xqueue, & trash, 0);
     printf("trash data: %s\r\n", trash);
     xQueueSendToBack(xqueue, printLocalTime(), 0);
      }
  } else {
      // Deactivate press
      digitalWrite(GPIO_LED, LOW);
    }
  }
}

//
// Initialization:
//
void setup() {
  int app_cpu = xPortGetCoreID();
  static int left = GPIO_BUTTONL;
  static int right = GPIO_BUTTONR;
  TaskHandle_t h;
  BaseType_t rc;

  //task and basetype
  TaskHandle_t tijd;
  BaseType_t settime;
  WiFi_connect();
 
  xqueue = xQueueCreate(5, 64 * sizeof(char));
  delay(1000); // Allow USB to connect
  queue = xQueueCreate(2, sizeof(int));
  assert(queue);
  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_BUTTONL, INPUT_PULLUP);
  pinMode(GPIO_BUTTONR, INPUT_PULLUP);

  

 rc = xTaskCreatePinnedToCore(
    debounce_task,

    "debounceL",
    2048, // Stack size
    &left, // Left button gpio
    2, // Priority
    &h, // Task handle
    app_cpu // CPU
  );
  assert(rc == pdPASS);
  assert(h);
//set time task
  settime = xTaskCreatePinnedToCore(
    set_time,
    "Set time",
    2048,
    &tijd,
    3,
    &tijd,
    app_cpu);
  assert(settime == pdPASS);
  assert(tijd);
  
 rc = xTaskCreatePinnedToCore(
    debounce_task,
    "debounceR",
    2048, // Stack size
    &right, // Right button gpio
    2, // Priority
    &h, // Task handle
    app_cpu // CPU
  );
  assert(rc == pdPASS);
  assert(h);

 rc = xTaskCreatePinnedToCore(
    press_task,
    "led",
    2048, // Stack size
    nullptr, // Not used
    2, // Priority
    &h, // Task handle
    app_cpu // CPU
  );
  assert(rc == pdPASS);
  assert(h);

  vTaskStartScheduler();
}

// Not used:
void loop() {
  vTaskDelete(nullptr);
}
