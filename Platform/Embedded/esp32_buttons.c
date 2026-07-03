/*
 * esp32_buttons.c ESP32-S3 GPIO Button Input Driver
 * 
 * Implements the InputDriver interface (input.h) using 4 GPIO buttons
 * connected to the ESP32-S3 for calculator input.
 * 
 * Button mapping:
 * GPIO 32 Up (UI_KEY_UP)
 * GPIO 33 Down (UI_KEY_DOWN)
 * GPIO 34 Back (UI_KEY_ESCAPE)
 * GPIO 35 Enter (UI_KEY_ENTER)
 * 
 * COMPILE with: -DPLATFORM_ESP32
 * Requires ESP-IDF GPIO driver.
 * 
 * THE FLEX: Four tactile buttons full calculator control.
 * No keyboard required. No display required. Just GPIO and vibes.
 */

#include "../../include/calc/input.h"
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_ESP32
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Button configuration
#define BTN_UP_PIN      GPIO_NUM_32
#define BTN_DOWN_PIN    GPIO_NUM_33
#define BTN_BACK_PIN    GPIO_NUM_34
#define BTN_ENTER_PIN   GPIO_NUM_35
#define BTN_DEBOUNCE_MS 50

typedef struct {
    uint8_t last_state[4];     // Debounce state
    uint8_t pressed[4];        // Edge-detected press
    TickType_t last_tick[4];
} ESPButtonContext;

// Button index mapping
enum { BTN_UP = 0, BTN_DOWN = 1, BTN_BACK = 2, BTN_ENTER = 3 };

// GPIO pin to button index
static int gpio_to_btn_idx(uint8_t pin) {
    switch (pin) {
        case BTN_UP_PIN:    return BTN_UP;
        case BTN_DOWN_PIN:  return BTN_DOWN;
        case BTN_BACK_PIN:  return BTN_BACK;
        case BTN_ENTER_PIN: return BTN_ENTER;
        default:            return -1;
    }
}

// key_available
static int esp_key_available(const InputDriver* self) {
    if (!self || !self->context) return 0;
    ESPButtonContext* ctx = (ESPButtonContext*)self->context;

    // Scan all 4 buttons with debounce
    const uint8_t pins[4] = {BTN_UP_PIN, BTN_DOWN_PIN, BTN_BACK_PIN, BTN_ENTER_PIN};
    TickType_t now = xTaskGetTickCount();

    for (int i = 0; i < 4; i++) {
        uint8_t state = (uint8_t)gpio_get_level(pins[i]);
        if (state != ctx->last_state[i] && 
            (now - ctx->last_tick[i]) > pdMS_TO_TICKS(BTN_DEBOUNCE_MS)) {
            ctx->last_state[i] = state;
            ctx->last_tick[i] = now;
            if (state == 0) { // Falling edge (button press = active low)
                ctx->pressed[i] = 1;
                return 1;
            }
        }
    }
    return 0;
}

// read_key
static uint32_t esp_read_key(const InputDriver* self) {
    if (!self || !self->context) return UI_KEY_NONE;
    ESPButtonContext* ctx = (ESPButtonContext*)self->context;

    static const uint32_t key_map[4] = {
        UI_KEY_UP, UI_KEY_DOWN, UI_KEY_ESCAPE, UI_KEY_ENTER
    };

    for (int i = 0; i < 4; i++) {
        if (ctx->pressed[i]) {
            ctx->pressed[i] = 0;
            return key_map[i];
        }
    }
    return UI_KEY_NONE;
}

// init
static int esp_input_init(const InputDriver* self) {
    if (!self || !self->context) return -1;
    ESPButtonContext* ctx = (ESPButtonContext*)self->context;

    // Configure GPIO pins as inputs with pull-up
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << BTN_UP_PIN) | (1ULL << BTN_DOWN_PIN) |
                        (1ULL << BTN_BACK_PIN) | (1ULL << BTN_ENTER_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf);

    // Read initial states
    ctx->last_state[0] = (uint8_t)gpio_get_level(BTN_UP_PIN);
    ctx->last_state[1] = (uint8_t)gpio_get_level(BTN_DOWN_PIN);
    ctx->last_state[2] = (uint8_t)gpio_get_level(BTN_BACK_PIN);
    ctx->last_state[3] = (uint8_t)gpio_get_level(BTN_ENTER_PIN);
    ctx->last_tick[0] = ctx->last_tick[1] = 
    ctx->last_tick[2] = ctx->last_tick[3] = xTaskGetTickCount();

    return 0;
}

// Public API
int esp_input_create(InputDriver* driver) {
    if (!driver) return -1;
    ESPButtonContext* ctx = (ESPButtonContext*)calloc(1, sizeof(ESPButtonContext));
    if (!ctx) return -2;

    driver->context = ctx;
    driver->name = "ESP32 GPIO Buttons";
    driver->key_available = esp_key_available;
    driver->read_key = esp_read_key;
    driver->init = esp_input_init;
    driver->deinit = NULL;
    return 0;
}

void esp_input_destroy(InputDriver* driver) {
    if (!driver || !driver->context) return;
    free(driver->context);
    driver->context = NULL;
}

#else // Not ESP32
int esp_input_create(InputDriver* driver) { (void)driver; return -99; }
void esp_input_destroy(InputDriver* driver) { (void)driver; }
#endif
