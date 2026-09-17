#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

#define WIFI_CONNECTED_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1
#define EVENT_QUEUE_LENGTH 12

// Den publika CA-delen genereras på lärarlaptopen och bäddas in i firmware.
// Linkersymbolerna skapas av EMBED_TXTFILES i main/CMakeLists.txt.
extern const uint8_t laptop_ca_crt_start[] asm("_binary_laptop_ca_crt_start");

// Beskriver en periodisk sensor och kopplar ihop dess timer med dess task.
// Samma taskfunktion kan därför återanvändas för flera sensorer.
typedef struct {
    const char *sensor_id;
    const char *unit;
    double base_value;
    uint32_t period_ms;
    TaskHandle_t task_handle;
    atomic_uint sequence;
} sensor_source_t;

// Detta är meddelandet som producenterna skickar genom FreeRTOS-kön.
// Fasta teckenbuffertar gör att inga pekare till lokala strängar lämnar tasken.
typedef struct {
    char sensor_id[24];
    char unit[8];
    double value;
    int64_t created_us;
} sensor_event_t;

static const char *TAG = "day8_mqtts";
static EventGroupHandle_t connection_events;
static QueueHandle_t sensor_queue;
static esp_mqtt_client_handle_t mqtt_client;

// Flera tasks och callbacks använder räknarna. Atomära operationer hindrar
// att en uppdatering försvinner om två körflöden skriver samtidigt.
static atomic_uint created_count;
static atomic_uint published_count;
static atomic_uint offline_dropped_count;
static atomic_uint queue_dropped_count;
static atomic_uint reconnect_count;
static atomic_bool has_connected_once;

static sensor_source_t temperature_source = {
    .sensor_id = "temperature",
    .unit = "C",
    .base_value = 20.0,
    .period_ms = 5000,
};

static sensor_source_t humidity_source = {
    .sensor_id = "humidity",
    .unit = "%",
    .base_value = 40.0,
    .period_ms = 7000,
};

// Timercallbacken körs i ESP Timers systemtask. Den ska avslutas snabbt och
// gör därför inget sensor- eller nätverksarbete, utan väcker bara rätt task.
static void sensor_timer_callback(void *argument)
{
    sensor_source_t *source = argument;
    xTaskNotifyGive(source->task_handle);
}

static void sensor_task(void *argument)
{
    sensor_source_t *source = argument;

    while (true) {
        // Tasken sover utan aktiv CPU-tid tills dess timer skickar en notifiering.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        unsigned int sequence = atomic_fetch_add(&source->sequence, 1) + 1;

        sensor_event_t event = {
            .value = source->base_value + (double)(sequence % 10) / 10.0,
            .created_us = esp_timer_get_time(),
        };
        snprintf(event.sensor_id, sizeof(event.sensor_id), "%s", source->sensor_id);
        snprintf(event.unit, sizeof(event.unit), "%s", source->unit);
        atomic_fetch_add(&created_count, 1);

        // Noll ticks betyder att producenten aldrig blockeras av en full kö.
        // I stället registreras dataförlusten i queue_dropped_count.
        if (xQueueSend(sensor_queue, &event, 0) != pdTRUE) {
            atomic_fetch_add(&queue_dropped_count, 1);
            ESP_LOGW(TAG, "QUEUE_DROP producer=%s", source->sensor_id);
        } else {
            ESP_LOGI(
                TAG,
                "SENSOR task=%s sequence=%u queued=%u",
                pcTaskGetName(NULL),
                sequence,
                (unsigned int)uxQueueMessagesWaiting(sensor_queue));
        }
    }
}

static void publish_task(void *argument)
{
    (void)argument;
    sensor_event_t event;
    char topic[96];
    char payload[192];

    while (true) {
        // portMAX_DELAY blockerar konsumenten när kön är tom.
        if (xQueueReceive(sensor_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        // Nätverkscallbackarna äger anslutningstillståndet. Publiceringstasken
        // läser bara tillståndet och tillämpar demots uttalade offlinepolicy.
        EventBits_t bits = xEventGroupGetBits(connection_events);
        if ((bits & MQTT_CONNECTED_BIT) == 0) {
            atomic_fetch_add(&offline_dropped_count, 1);
            ESP_LOGW(TAG, "OFFLINE_DROP sensor=%s", event.sensor_id);
            continue;
        }

        snprintf(topic, sizeof(topic), "%s/%s", CONFIG_DEMO_TOPIC_PREFIX, event.sensor_id);
        snprintf(
            payload,
            sizeof(payload),
            "{\"sensorId\":\"%s\",\"value\":%.1f,\"unit\":\"%s\",\"uptimeUs\":%lld}",
            event.sensor_id,
            event.value,
            event.unit,
            (long long)event.created_us);

        // QoS 0 används för ett enkelt telemetriflöde. Ett lyckat returvärde
        // betyder att klienten accepterade anropet, inte en applikationskvittens.
        int message_id = esp_mqtt_client_publish(
            mqtt_client,
            topic,
            payload,
            0,
            0,
            0);

        if (message_id >= 0) {
            atomic_fetch_add(&published_count, 1);
            ESP_LOGI(TAG, "PUBLISH id=%d topic=%s payload=%s", message_id, topic, payload);
        } else {
            ESP_LOGE(TAG, "PUBLISH_FAILED topic=%s", topic);
        }
    }
}

static void statistics_task(void *argument)
{
    (void)argument;
    while (true) {
        // Statistik separeras från producenter och konsument så att loggningens
        // intervall inte styr när mätningar skapas eller publiceras.
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(
            TAG,
            "COUNTERS created=%u published=%u offline_dropped=%u queue_dropped=%u reconnects=%u queued=%u",
            atomic_load(&created_count),
            atomic_load(&published_count),
            atomic_load(&offline_dropped_count),
            atomic_load(&queue_dropped_count),
            atomic_load(&reconnect_count),
            (unsigned int)uxQueueMessagesWaiting(sensor_queue));
    }
}

static void mqtt_event_handler(
    void *handler_arguments,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    (void)handler_arguments;
    (void)event_base;
    (void)event_data;

    // ESP-MQTT anropar denna callback för klientevent. Callbacken uppdaterar
    // endast delat tillstånd och räknare; sensordata skapas i sensortaskarna.
    if (event_id == MQTT_EVENT_CONNECTED) {
        // Första anslutningen är inte en reconnect. atomic_exchange returnerar
        // det tidigare värdet och gör därför skillnaden explicit.
        if (atomic_exchange(&has_connected_once, true)) {
            atomic_fetch_add(&reconnect_count, 1);
        }
        xEventGroupSetBits(connection_events, MQTT_CONNECTED_BIT);
        ESP_LOGI(TAG, "MQTT_CONNECTED laptop broker verified with embedded lab CA");
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        xEventGroupClearBits(connection_events, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "MQTT_DISCONNECTED automatic reconnect remains enabled");
    } else if (event_id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "MQTT_ERROR inspect TLS and transport details");
    }
}

static void wifi_event_handler(
    void *handler_arguments,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    (void)handler_arguments;

    // Wi-Fi- och IP-drivrutinerna publicerar event till ESP-IDF:s eventloop.
    // Denna callback översätter dem till applikationens event bits.
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *disconnected = event_data;
        xEventGroupClearBits(connection_events, WIFI_CONNECTED_BIT | MQTT_CONNECTED_BIT);
        ESP_LOGW(
            TAG,
            "WIFI_DISCONNECTED reason=%u reconnecting",
            disconnected != NULL ? disconnected->reason : 0);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(connection_events, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "WIFI_CONNECTED IPv4 acquired");
    }
}

static void initialise_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    // Standardeventloopen levererar bland annat WIFI_EVENT och IP_EVENT till
    // de handlers som registreras nedan.
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event_handler,
        NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi_event_handler,
        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_DEMO_WIFI_SSID,
            .password = CONFIG_DEMO_WIFI_PASSWORD,
            .failure_retry_cnt = 5,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    // Uppstarten fortsätter först när DHCP har gett stationen en IPv4-adress.
    // Vid senare avbrott sköter eventhandlern nya anslutningsförsök.
    xEventGroupWaitBits(
        connection_events,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY);
}

static void create_application_task(
    TaskFunction_t task_function,
    const char *name,
    uint32_t stack_size,
    void *argument,
    UBaseType_t priority,
    TaskHandle_t *task_handle)
{
    // Hjälpfunktionen samlar felkontroll och startlogg för alla kurskodens tasks.
    BaseType_t result = xTaskCreate(
        task_function,
        name,
        stack_size,
        argument,
        priority,
        task_handle);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task=%s", name);
        abort();
    }
    ESP_LOGI(TAG, "TASK_CREATED name=%s priority=%u", name, (unsigned int)priority);
}

static void start_sensor_timer(
    esp_timer_handle_t *timer,
    sensor_source_t *source,
    const char *name)
{
    // Båda timrarna delar callback. Pekaren i .arg avgör vilken sensortask
    // som notifieras och vilket intervall som används.
    const esp_timer_create_args_t timer_arguments = {
        .callback = sensor_timer_callback,
        .arg = source,
        .dispatch_method = ESP_TIMER_TASK,
        .name = name,
        .skip_unhandled_events = true,
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_arguments, timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(*timer, source->period_ms * 1000ULL));
    ESP_LOGI(TAG, "TIMER_STARTED name=%s period_ms=%lu", name, (unsigned long)source->period_ms);
}

void app_main(void)
{
    // NVS behövs av Wi-Fi. Vid inkompatibelt eller fullt NVS startas lagringen
    // om enligt ESP-IDF:s vanliga initieringsmönster.
    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);

    // Eventgruppen delar anslutningstillstånd; kön flyttar ägarskapet av
    // kompletta sensorevent från producenterna till publiceringstasken.
    connection_events = xEventGroupCreate();
    sensor_queue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(sensor_event_t));
    if (connection_events == NULL || sensor_queue == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RTOS objects");
        return;
    }

    initialise_wifi();

    // mqtts:// tillsammans med labbets CA-certifikat gör att klienten verifierar
    // laptopbrokerns certifikat. URI:ns IP-adress måste finnas i certifikatets SAN.
    const esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = CONFIG_DEMO_MQTT_BROKER_URI,
        .broker.verification.certificate = (const char *)laptop_ca_crt_start,
        .credentials.username = CONFIG_DEMO_MQTT_USERNAME,
        .credentials.authentication.password = CONFIG_DEMO_MQTT_PASSWORD,
        .network.reconnect_timeout_ms = 3000,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_config);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));

    // Skapa tasks innan timrarna startas. Då är task_handle giltigt redan
    // vid den första timercallbacken.
    create_application_task(
        sensor_task,
        "temperature_task",
        3072,
        &temperature_source,
        4,
        &temperature_source.task_handle);
    create_application_task(
        sensor_task,
        "humidity_task",
        3072,
        &humidity_source,
        4,
        &humidity_source.task_handle);
    create_application_task(publish_task, "publish_task", 4096, NULL, 5, NULL);
    create_application_task(statistics_task, "statistics_task", 3072, NULL, 3, NULL);

    // Timrarna är statiska eftersom de ska leva kvar efter att app_main returnerar.
    static esp_timer_handle_t temperature_timer;
    static esp_timer_handle_t humidity_timer;
    start_sensor_timer(&temperature_timer, &temperature_source, "temperature_timer");
    start_sensor_timer(&humidity_timer, &humidity_source, "humidity_timer");

    ESP_LOGI(TAG, "Two timers and four application tasks started");
}
