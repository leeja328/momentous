#include <pebble.h>

static Window *s_window;
static Layer *s_canvas_layer;
static int s_hour;
static int s_minute;

#define SETTINGS_KEY 1

typedef struct {
  GColor hour_color;
  GColor minute_color;
  GColor background_color;
  bool large_font;
} Settings;

static Settings s_settings;

// Segment bits: a=top, b=top-right, c=bottom-right, d=bottom, e=bottom-left, f=top-left, g=middle
enum { SEG_A = 1, SEG_B = 2, SEG_C = 4, SEG_D = 8, SEG_E = 16, SEG_F = 32, SEG_G = 64 };

static const uint8_t s_digit_segments[10] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // 0
  SEG_B | SEG_C,                                         // 1 (drawn as a single bar)
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                 // 2
  SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,                 // 3
  SEG_F | SEG_G | SEG_B | SEG_C,                         // 4
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                 // 5
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,         // 6
  SEG_A | SEG_B | SEG_C,                                 // 7
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,         // 9
};

static void prv_draw_digit(GContext *ctx, int digit, GRect box, int16_t stroke) {
  const int16_t x = box.origin.x, y = box.origin.y;
  const int16_t w = box.size.w, h = box.size.h, t = stroke;
  const int16_t mid = y + (h - t) / 2;

  if (digit == 1) {
    // Plain vertical bar; its cell is only one stroke wide (see prv_digit_width)
    graphics_fill_rect(ctx, GRect(x, y, t, h), 0, GCornerNone);
    return;
  }

  const uint8_t segs = s_digit_segments[digit];
  if (segs & SEG_A) graphics_fill_rect(ctx, GRect(x, y, w, t), 0, GCornerNone);
  if (segs & SEG_D) graphics_fill_rect(ctx, GRect(x, y + h - t, w, t), 0, GCornerNone);
  if (segs & SEG_G) graphics_fill_rect(ctx, GRect(x, mid, w, t), 0, GCornerNone);
  if (segs & SEG_F) graphics_fill_rect(ctx, GRect(x, y, t, mid - y + t), 0, GCornerNone);
  if (segs & SEG_B) graphics_fill_rect(ctx, GRect(x + w - t, y, t, mid - y + t), 0, GCornerNone);
  if (segs & SEG_E) graphics_fill_rect(ctx, GRect(x, mid, t, y + h - mid), 0, GCornerNone);
  if (segs & SEG_C) graphics_fill_rect(ctx, GRect(x + w - t, mid, t, y + h - mid), 0, GCornerNone);
}

// Width of the ink a digit occupies, so gaps between digits look even
static int16_t prv_digit_width(int digit, int16_t digit_w, int16_t stroke) {
  return digit == 1 ? stroke : digit_w;
}

// Draws a two-digit number centered horizontally in area, digits filling its height
static void prv_draw_number(GContext *ctx, int value, GRect area, int16_t digit_w, int16_t gap,
                            int16_t stroke) {
  const int digits[2] = { value / 10, value % 10 };
  const int16_t w0 = prv_digit_width(digits[0], digit_w, stroke);
  const int16_t w1 = prv_digit_width(digits[1], digit_w, stroke);
  const int16_t x = area.origin.x + (area.size.w - (w0 + gap + w1)) / 2;

  prv_draw_digit(ctx, digits[0], GRect(x, area.origin.y, w0, area.size.h), stroke);
  prv_draw_digit(ctx, digits[1], GRect(x + w0 + gap, area.origin.y, w1, area.size.h), stroke);
}

// Large mode: hour fills the top half, minutes fill the bottom half
static void prv_draw_large(GContext *ctx, GRect bounds) {
  // On round screens, keep the digits inside the largest square that fits in the circle
  const GRect area = PBL_IF_ROUND_ELSE(
      grect_inset(bounds, GEdgeInsets(bounds.size.w * 19 / 100)),
      grect_inset(bounds, GEdgeInsets(bounds.size.w * 6 / 144)));

  const int16_t row_gap = area.size.h * 8 / 156;
  const int16_t digit_gap = area.size.w * 8 / 132;
  const int16_t digit_h = (area.size.h - row_gap) / 2;
  const int16_t digit_w = (area.size.w - digit_gap) / 2;
  const int16_t bold_stroke = digit_w * 14 / 62;
  const int16_t light_stroke = digit_w * 3 / 62 > 0 ? digit_w * 3 / 62 : 1;

  graphics_context_set_fill_color(ctx, s_settings.hour_color);
  prv_draw_number(ctx, s_hour, GRect(area.origin.x, area.origin.y, area.size.w, digit_h),
                  digit_w, digit_gap, bold_stroke);

  graphics_context_set_fill_color(ctx, s_settings.minute_color);
  prv_draw_number(ctx, s_minute,
                  GRect(area.origin.x, area.origin.y + digit_h + row_gap, area.size.w, digit_h),
                  digit_w, digit_gap, light_stroke);
}

static void prv_canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  if (s_settings.large_font) {
    prv_draw_large(ctx, bounds);
    return;
  }

  // Proportions are tuned for a 144px-wide screen and scaled for others
  const int16_t base = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  // Wide, short digits that span nearly the full screen width
  const int16_t digit_w = base * 27 / 144;
  const int16_t digit_h = base * 26 / 144;
  const int16_t digit_gap = base * 5 / 144;
  const int16_t group_gap = base * 14 / 144;
  const int16_t bold_stroke = base * 6 / 144;
  const int16_t light_stroke = base * 2 / 144 > 0 ? base * 2 / 144 : 1;

  const int digits[4] = { s_hour / 10, s_hour % 10, s_minute / 10, s_minute % 10 };
  const int16_t strokes[4] = { bold_stroke, bold_stroke, light_stroke, light_stroke };
  const int16_t gaps[3] = { digit_gap, group_gap, digit_gap };

  int16_t widths[4];
  int16_t total_w = gaps[0] + gaps[1] + gaps[2];
  for (int i = 0; i < 4; i++) {
    widths[i] = prv_digit_width(digits[i], digit_w, strokes[i]);
    total_w += widths[i];
  }

  int16_t x = (bounds.size.w - total_w) / 2;
  const int16_t y = (bounds.size.h - digit_h) / 2;

  for (int i = 0; i < 4; i++) {
    graphics_context_set_fill_color(ctx, i < 2 ? s_settings.hour_color : s_settings.minute_color);
    prv_draw_digit(ctx, digits[i], GRect(x, y, widths[i], digit_h), strokes[i]);
    if (i < 3) {
      x += widths[i] + gaps[i];
    }
  }
}

static void prv_default_settings(void) {
  s_settings.hour_color = GColorWhite;
  s_settings.minute_color = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite);
  s_settings.background_color = GColorBlack;
  s_settings.large_font = false;
}

static void prv_load_settings(void) {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static void prv_apply_settings(void) {
  window_set_background_color(s_window, s_settings.background_color);
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *hour_color_t = dict_find(iter, MESSAGE_KEY_HourColor);
  if (hour_color_t) {
    s_settings.hour_color = GColorFromHEX(hour_color_t->value->int32);
  }
  Tuple *minute_color_t = dict_find(iter, MESSAGE_KEY_MinuteColor);
  if (minute_color_t) {
    s_settings.minute_color = GColorFromHEX(minute_color_t->value->int32);
  }
  Tuple *background_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (background_color_t) {
    s_settings.background_color = GColorFromHEX(background_color_t->value->int32);
  }

  Tuple *large_font_t = dict_find(iter, MESSAGE_KEY_LargeFont);
  if (large_font_t) {
    s_settings.large_font = large_font_t->value->int32 == 1;
  }

  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  prv_apply_settings();
}

static void prv_update_time(void) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  int hour = tick_time->tm_hour;
  if (!clock_is_24h_style()) {
    hour %= 12;
    if (hour == 0) {
      hour = 12;
    }
  }
  s_hour = hour;
  s_minute = tick_time->tm_min;
  layer_mark_dirty(s_canvas_layer);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, prv_canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  prv_update_time();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void prv_init(void) {
  prv_load_settings();

  s_window = window_create();
  window_set_background_color(s_window, s_settings.background_color);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
