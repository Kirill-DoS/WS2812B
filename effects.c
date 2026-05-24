#include "effects.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"

static PIO pio = pio0;
static int sm = 0;

// Инициализация PIO (нужно вызвать перед использованием!)
void effects_init(PIO p, int state_machine) {
    pio = p;
    sm = state_machine;
}

// Функция отправки цвета БЕЗ сдвига!
void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb);
}

// Функция отправки всего массива
void update_leds(uint32_t *led_strip, int num_leds) {
    for (int i = 0; i < num_leds; i++) {
        put_pixel(led_strip[i]);
    }
}

void blink_every_n_led(uint32_t color, int n, int leds) {
    for (int i = 0; i < leds; i++) {
        if ((i % n) == 0) {
            put_pixel(color);
        } else {
            put_pixel(0x000000);
        }
    }
}

void blink_loop_led(uint32_t color, int leds, int delay) {
    for(int i = 0; i < leds; i++) {
        // Включаем текущий светодиод
        put_pixel(color);

        // Ждём
        sleep_ms(delay);

        // Выключаем его
        put_pixel(0x000000);
    }
}
