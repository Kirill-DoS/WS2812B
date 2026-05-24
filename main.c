#include <stdio.h>
#include <math.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// === Параметры ===
#define LED_PIN 2
#define NUM_LEDS 180

// === Цветовая палитра (RGB) ===
const uint32_t fire_palette[] = {
    0x000000, // чёрный
    0x220000, // очень тёмно-красный
    0x660000, // тёмно-красный
    0xCC3300, // красный
    0xFF6600, // оранжевый
    0xFFFF00, // жёлтый
};

#define PALETTE_SIZE (sizeof(fire_palette) / sizeof(fire_palette[0]))

// === Глобальные переменные ===
uint32_t led_strip[NUM_LEDS];
float time_counter = 0.0f;

// === Вспомогательные функции ===

// Псевдослучайное число (возвращает от 0 до 1)
float random_at(int x, int y) {
    unsigned int seed = x * 1973 + y * 9277;
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return (seed % 1000) / 1000.0f;
}

// Линейная интерполяция
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Билинейная интерполяция
float interpolated_noise(float x, float y) {
    int int_x = (int)x;
    float frac_x = x - int_x;
    int int_y = (int)y;
    float frac_y = y - int_y;

    float v1 = random_at(int_x, int_y);
    float v2 = random_at(int_x + 1, int_y);
    float v3 = random_at(int_x, int_y + 1);
    float v4 = random_at(int_x + 1, int_y + 1);

    float i1 = lerp(v1, v2, frac_x);
    float i2 = lerp(v3, v4, frac_x);

    return lerp(i1, i2, frac_y);
}

// Улучшенный шум с нормализацией в [0, 1]
float improved_smooth_noise(float x, float y) {
    float n = 0.0f;
    float scale = 0.0f;

    // Октавы шума для гладкости
    float octave_weights[] = {0.5f, 0.25f, 0.25f};
    float scales[] = {0.05f, 0.1f, 0.2f};

    for (int i = 0; i < 3; i++) {
        float val = interpolated_noise(x * scales[i], y * scales[i]);
        n += val * octave_weights[i];
        scale += octave_weights[i];
    }

    n /= scale; // нормализация
    return n < 0.0f ? 0.0f : n > 1.0f ? 1.0f : n; // финальное ограничение
}

// Получить цвет из палитры по индексу (0.0 - 1.0)
uint32_t get_color_from_palette(float t) {
    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    float scaled = t * (PALETTE_SIZE - 1);
    int index = (int)scaled;
    float blend = scaled - index;

    if (index >= PALETTE_SIZE - 1) {
        return fire_palette[PALETTE_SIZE - 1];
    }

    uint32_t c1 = fire_palette[index];
    uint32_t c2 = fire_palette[index + 1];

    uint8_t r1 = (c1 >> 16) & 0xFF;
    uint8_t g1 = (c1 >> 8) & 0xFF;
    uint8_t b1 = c1 & 0xFF;

    uint8_t r2 = (c2 >> 16) & 0xFF;
    uint8_t g2 = (c2 >> 8) & 0xFF;
    uint8_t b2 = c2 & 0xFF;

    uint8_t r = (uint8_t)(r1 * (1.0f - blend) + r2 * blend);
    uint8_t g = (uint8_t)(g1 * (1.0f - blend) + g2 * blend);
    uint8_t b = (uint8_t)(b1 * (1.0f - blend) + b2 * blend);

    return (r << 16) | (g << 8) | b;
}

void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

void update_leds() {
    for (int i = 0; i < NUM_LEDS; i++) {
        put_pixel(led_strip[i]);
    }
}

int main() {
    stdio_init_all();

    // === Настройка PIO ===
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, 0);

    while (true) {
        for (int i = 0; i < NUM_LEDS; i++) {
            float noise_val = improved_smooth_noise((float)i, time_counter);
            uint32_t color = get_color_from_palette(noise_val);
            led_strip[i] = color;
        }

        update_leds();
        time_counter += 0.1f; // увеличенное изменение времени
        sleep_ms(50);
    }
}
