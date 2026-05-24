#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include "hardware/pio.h"

// Инициализация
void effects_init(PIO p, int state_machine);

// Основные функции
void put_pixel(uint32_t pixel_grb);
void update_leds(uint32_t *led_strip, int num_leds);

// Эффекты
void blink_every_n_led(uint32_t color, int n, int leds);
void blink_loop_led(uint32_t color, int leds, int delay);

#endif
