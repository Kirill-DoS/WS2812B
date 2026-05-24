#include <stdio.h>
#include <math.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// === Параметры ===
#define LED_PIN 0
#define NUM_LEDS 180
#define LED_CENTER 89

// === Диапазоны цветов (в градусах HSV)
// Меняйте эти значения, чтобы задать желаемый диапазон цветов для каждой половины
#define HUE_MIN_FIRST_HALF  0.0f    // лайм
#define HUE_MAX_FIRST_HALF  150.0f   // маджента
#define HUE_MIN_SECOND_HALF 150.0f   // бирюзовый
#define HUE_MAX_SECOND_HALF 300.0f     // красный

// === Глобальные переменные ===
uint32_t led_strip[NUM_LEDS];

// === HSV -> RGB ===
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
        case 0: r=v*255; g=t*255; b=p*255; break;
        case 1: r=q*255; g=v*255; b=p*255; break;
        case 2: r=p*255; g=v*255; b=t*255; break;
        case 3: r=p*255; g=q*255; b=v*255; break;
        case 4: r=t*255; g=p*255; b=v*255; break;
        case 5: r=v*255; g=p*255; b=q*255; break;
    }
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

    // === Переменные для синусоид ===
    float phase = 0.0f;
    const float freq = 0.05f; // частота изменения цвета

    while (true) {
        phase += 0.1f; // изменяем общую фазу

        // === Считаем общий цвет для всей ленты ===
        float sine_value = sinf(phase); // одна синусоида на всё

        // === Цвет для первой половины (0 - 89) ===
        float ratio1 = (sine_value + 1.0f) / 2.0f; // от -1..1 → 0..1
        float hue1 = HUE_MIN_FIRST_HALF + ratio1 * (HUE_MAX_FIRST_HALF - HUE_MIN_FIRST_HALF);

        // === Цвет для второй половины (90 - 179) ===
        float ratio2 = (sine_value + 1.0f) / 2.0f; // от -1..1 → 0..1
        float hue2 = HUE_MIN_SECOND_HALF + ratio2 * (HUE_MAX_SECOND_HALF - HUE_MIN_SECOND_HALF);

        // === Заполняем первую половину ===
        for (int i = 0; i <= LED_CENTER; i++) {
            led_strip[i] = hsv_to_rgb(hue1, 1.0f, 1.0f);
        }

        // === Заполняем вторую половину ===
        for (int i = LED_CENTER + 1; i < NUM_LEDS; i++) {
            led_strip[i] = hsv_to_rgb(hue2, 1.0f, 1.0f);
        }

        update_leds();
        sleep_ms(50); // обновление каждые 50 мс
    }
}
