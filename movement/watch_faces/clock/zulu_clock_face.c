/*
 * MIT License
 *
 * Copyright (c) 2023 Dante M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include "zulu_clock_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_private_display.h"

void zulu_clock_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(zulu_clock_state_t));
        zulu_clock_state_t *state = (zulu_clock_state_t *)*context_ptr;
        state->watch_face_index = watch_face_index;
    }
}

void zulu_clock_face_activate(movement_settings_t *settings, void *context) {
    zulu_clock_state_t *state = (zulu_clock_state_t *)context;

    state->previous_clock_mode = settings->bit.clock_mode_24h;
    settings->bit.clock_mode_24h = true;

    if (watch_tick_animation_is_running()) watch_stop_tick_animation();

    if (settings->bit.clock_mode_24h) watch_set_indicator(WATCH_INDICATOR_24H);

    watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->previous_date_time = 0xFFFFFFFF;
}

bool zulu_clock_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    zulu_clock_state_t *state = (zulu_clock_state_t *)context;
    char buf[11];
    char week[2];
    uint8_t pos;

    watch_date_time date_time;
    uint32_t previous_date_time;
    uint32_t timestamp;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            date_time = watch_rtc_get_date_time();
            timestamp = watch_utility_date_time_to_unix_time(date_time, movement_timezone_offsets[settings->bit.time_zone] * 60);
            date_time = watch_utility_date_time_from_unix_time(timestamp, 0 * 60);
            previous_date_time = state->previous_date_time;
            state->previous_date_time = date_time.reg;

            if ((date_time.reg >> 6) == (previous_date_time >> 6) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                // everything before seconds is the same, don't waste cycles setting those segments.
                watch_display_character_lp_seconds('0' + date_time.unit.second / 10, 8);
                watch_display_character_lp_seconds('0' + date_time.unit.second % 10, 9);
                break;
            } else if ((date_time.reg >> 12) == (previous_date_time >> 12) && event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                // everything before minutes is the same.
                pos = 6;
                sprintf(buf, "%02d%02d", date_time.unit.minute, date_time.unit.second);
            } else {
                pos = 0;
                if (event.event_type == EVENT_LOW_ENERGY_UPDATE) {
                    if (!watch_tick_animation_is_running()) watch_start_tick_animation(500);
                    sprintf(buf, "%s%2d%2d%02d  ", "UT", date_time.unit.day, date_time.unit.hour, date_time.unit.minute);
                } else {
                    sprintf(buf, "%s%2d%2d%02d%02d", "UT", date_time.unit.day, date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
                }
            }
            watch_display_string(buf, pos);
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            date_time = watch_rtc_get_date_time();
            sprintf(week, "%s", watch_utility_get_weekday(date_time));
            watch_display_string(week, 0);
            break;
        case EVENT_ALARM_BUTTON_UP:
            watch_display_string("UT", 0);
            break;
        case EVENT_ALARM_LONG_UP:
            watch_display_string("UT", 0);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void zulu_clock_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    zulu_clock_state_t *state = (zulu_clock_state_t *)context;

    settings->bit.clock_mode_24h = state->previous_clock_mode;
}

