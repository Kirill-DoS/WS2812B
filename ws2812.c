/**
 * WS2812 Effects Demo
 * Каждый эффект работает по 10 секунд:
 * 1. Snakes! (10 сек)
 * 2. Random data (10 сек)
 * 3. Color gradient effect (10 сек) из effect_2.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// === Конфигурация ===
#define IS_RGBW false
#define NUM_PIXELS 180
#define LED_PIN 2  // или PICO_DEFAULT_WS2812_PIN, если определен

// Параметры для цветового эффекта (из effect_2.c)
#define LED_CENTER 89
#define HUE_MIN_FIRST_HALF  0.0f    // лайм
#define HUE_MAX_FIRST_HALF  150.0f  // маджента
#define HUE_MIN_SECOND_HALF 150.0f  // бирюзовый
#define HUE_MAX_SECOND_HALF 300.0f  // красный

// Проверка пина
#if LED_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

// === Глобальные переменные для цветового эффекта ===
static uint32_t led_strip[NUM_PIXELS];

// === Прототипы функций ===
static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb);
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
void pattern_snakes(PIO pio, uint sm, uint len, uint t);
void pattern_random(PIO pio, uint sm, uint len, uint t);
void pattern_color_gradient(PIO pio, uint sm, uint len, uint t);
void update_leds(PIO pio, uint sm);
uint32_t hsv_to_rgb(float h, float s, float v);

// === Вспомогательные функции ===
static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// === HSV -> RGB конвертер ===
uint32_t hsv_to_rgb(float h, float s, float v) {
    h = fmodf(h, 360.0f);
    if (h < 0) h += 360.0f;

    int sector = (int)(h / 60.0f);
    float f = h / 60.0f - sector;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    uint8_t r, g, b;
    switch(sector % 6){
        case 0: r = v * 255; g = t * 255; b = p * 255; break;
        case 1: r = q * 255; g = v * 255; b = p * 255; break;
        case 2: r = p * 255; g = v * 255; b = t * 255; break;
        case 3: r = p * 255; g = q * 255; b = v * 255; break;
        case 4: r = t * 255; g = p * 255; b = v * 255; break;
        case 5: r = v * 255; g = p * 255; b = q * 255; break;
    }
    return urgb_u32(r, g, b);
}

// === Функция обновления всей ленты ===
void update_leds(PIO pio, uint sm) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(pio, sm, led_strip[i]);
    }
}

// === Эффект 1: Snakes! ===
void pattern_snakes(PIO pio, uint sm, uint len, uint t) {
    for (uint i = 0; i < len; ++i) {
        uint x = (i + (t >> 1)) % 64;
        if (x < 10)
            put_pixel(pio, sm, urgb_u32(0xff, 0, 0));
        else if (x >= 15 && x < 25)
            put_pixel(pio, sm, urgb_u32(0, 0xff, 0));
        else if (x >= 30 && x < 40)
            put_pixel(pio, sm, urgb_u32(0, 0, 0xff));
        else
            put_pixel(pio, sm, 0);
    }
}

// === Эффект 2: Random data ===
void pattern_random(PIO pio, uint sm, uint len, uint t) {
    if (t % 8)
        return;
    for (uint i = 0; i < len; ++i)
        put_pixel(pio, sm, rand());
}

// === Эффект 3: Color gradient (из effect_2.c) ===
void pattern_color_gradient(PIO pio, uint sm, uint len, uint t) {
    static float phase = 0.0f;
    const float freq = 0.05f;

    phase += 0.1f;

    // Общая синусоида для всей ленты
    float sine_value = sinf(phase);

    // Цвет для первой половины (0 - 89)
    float ratio1 = (sine_value + 1.0f) / 2.0f;
    float hue1 = HUE_MIN_FIRST_HALF + ratio1 * (HUE_MAX_FIRST_HALF - HUE_MIN_FIRST_HALF);

    // Цвет для второй половины (90 - 179)
    float ratio2 = (sine_value + 1.0f) / 2.0f;
    float hue2 = HUE_MIN_SECOND_HALF + ratio2 * (HUE_MAX_SECOND_HALF - HUE_MIN_SECOND_HALF);

    // Заполняем первую половину
    for (int i = 0; i <= LED_CENTER; i++) {
        led_strip[i] = hsv_to_rgb(hue1, 1.0f, 1.0f);
    }

    // Заполняем вторую половину
    for (int i = LED_CENTER + 1; i < NUM_PIXELS; i++) {
        led_strip[i] = hsv_to_rgb(hue2, 1.0f, 1.0f);
    }

    update_leds(pio, sm);
}

// === Главная функция ===
int main() {
    stdio_init_all();
    printf("WS2812 Effects Demo - 10 seconds per effect\n");
    printf("LED Pin: %d, Number of LEDs: %d\n", LED_PIN, NUM_PIXELS);

    // Инициализация PIO для WS2812
    PIO pio;
    uint sm;
    uint offset;

    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
        &ws2812_program, &pio, &sm, &offset, LED_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, IS_RGBW);

    int effect_counter = 0;
    int frame_counter = 0;

    // Бесконечный цикл с переключением эффектов
    while (1) {
        effect_counter %= 3; // 3 эффекта

        switch(effect_counter) {
            case 0:
                // Эффект 1: Snakes! (10 секунд)
                printf("Effect 1: Snakes! (10 seconds)\n");
                for (int i = 0; i < 1000; ++i) { // 1000 * 10ms = 10 секунд
                    pattern_snakes(pio, sm, NUM_PIXELS, frame_counter);
                    sleep_ms(10);
                    frame_counter++;
                }
                break;

            case 1:
                // Эффект 2: Random data (10 секунд)
                printf("Effect 2: Random data (10 seconds)\n");
                for (int i = 0; i < 1000; ++i) { // 1000 * 10ms = 10 секунд
                    pattern_random(pio, sm, NUM_PIXELS, frame_counter);
                    sleep_ms(10);
                    frame_counter++;
                }
                break;

            case 2:
                // Эффект 3: Color gradient (10 секунд)
                printf("Effect 3: Color gradient (10 seconds)\n");
                for (int i = 0; i < 200; ++i) { // 200 * 50ms = 10 секунд
                    pattern_color_gradient(pio, sm, NUM_PIXELS, frame_counter);
                    sleep_ms(50); // как в оригинальном effect_2.c
                    frame_counter++;
                }
                break;
        }

        effect_counter++;
        printf("Switching to next effect...\n");
    }

    // Этот код никогда не выполнится, но оставим для завершенности
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
    return 0;
}
