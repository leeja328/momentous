#include <pebble.h>

static Window *s_window;
static Layer *s_canvas_layer;
static int s_hour;
static int s_minute;
static int s_month;
static int s_day;

#define SETTINGS_KEY 1
#define WEATHER_KEY 2

enum { DATE_NONE = 0, DATE_MM_DD = 1, DATE_DD_MM = 2 };
enum { BOTTOM_NONE = 0, BOTTOM_TEMPERATURE = 1, BOTTOM_STEPS = 2 };
enum { WEIGHT_THIN = 0, WEIGHT_REGULAR = 1, WEIGHT_THICK = 2, WEIGHT_COUNT };

typedef struct {
  GColor hour_color;
  GColor minute_color;
  GColor background_color;
  bool large_font;
  uint8_t date_format;
  uint8_t bottom_info;
  uint8_t hour_weight;
  uint8_t minute_weight;
} Settings;

static Settings s_settings;

enum {
  WEATHER_UNKNOWN = 0, WEATHER_CLEAR, WEATHER_PARTLY_CLOUDY, WEATHER_CLOUDY,
  WEATHER_RAIN, WEATHER_SNOW, WEATHER_STORM, WEATHER_FOG, ICON_FOOT, ICON_COUNT
};

typedef struct {
  int16_t temperature;
  uint8_t condition;
  bool valid;
} Weather;

static Weather s_weather;

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
    // Plain vertical bar against the right edge of its box, like a seven-segment display
    graphics_fill_rect(ctx, GRect(x + w - t, y, t, h), 0, GCornerNone);
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

// Small block glyphs for the date and info rows: digits, '-', '/' and '*' (degree sign)
static int16_t prv_glyph_width(char c, int16_t glyph_w, int16_t stroke) {
  if (c == '*') return stroke * 3;
  return glyph_w;
}

static void prv_draw_glyph(GContext *ctx, char c, GRect box, int16_t stroke) {
  const int16_t x = box.origin.x, y = box.origin.y;
  const int16_t w = box.size.w, h = box.size.h, t = stroke;

  if (c >= '0' && c <= '9') {
    prv_draw_digit(ctx, c - '0', box, t);
  } else if (c == '-') {
    graphics_fill_rect(ctx, GRect(x, y + (h - t) / 2, w, t), 0, GCornerNone);
  } else if (c == '*') {
    // Hollow square degree sign sitting at the top of the line
    graphics_fill_rect(ctx, GRect(x, y, 3 * t, t), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x, y + 2 * t, 3 * t, t), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x, y, t, 3 * t), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x + 2 * t, y, t, 3 * t), 0, GCornerNone);
  } else if (c == '/') {
    // Diagonal made of stroke-sized blocks stepping up to the right
    const int steps = h / t;
    for (int i = 0; i < steps; i++) {
      const int16_t dx = steps > 1 ? (w - t) * (steps - 1 - i) / (steps - 1) : 0;
      graphics_fill_rect(ctx, GRect(x + dx, y + i * t, t, t), 0, GCornerNone);
    }
  }
}

static int16_t prv_text_width(const char *text, int16_t glyph_w, int16_t stroke, int16_t gap) {
  int16_t total = 0;
  for (const char *c = text; *c; c++) {
    total += prv_glyph_width(*c, glyph_w, stroke) + (c == text ? 0 : gap);
  }
  return total;
}

static void prv_draw_text(GContext *ctx, const char *text, GPoint origin, int16_t glyph_w,
                          int16_t glyph_h, int16_t stroke, int16_t gap) {
  int16_t x = origin.x;
  for (const char *c = text; *c; c++) {
    const int16_t w = prv_glyph_width(*c, glyph_w, stroke);
    prv_draw_glyph(ctx, *c, GRect(x, origin.y, w, glyph_h), stroke);
    x += w + gap;
  }
}

// 7x7 block icons; each row's bit 6 is the leftmost column
#define ICON_SIZE 7

static const uint8_t s_icons[ICON_COUNT][ICON_SIZE] = {
  [WEATHER_UNKNOWN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  [WEATHER_CLEAR] = { 0x08, 0x22, 0x1C, 0x5D, 0x1C, 0x22, 0x08 },
  [WEATHER_PARTLY_CLOUDY] = { 0x03, 0x0B, 0x1C, 0x3E, 0x7F, 0x7F, 0x00 },
  [WEATHER_CLOUDY] = { 0x00, 0x0C, 0x3E, 0x7F, 0x7F, 0x00, 0x00 },
  [WEATHER_RAIN] = { 0x1C, 0x3E, 0x7F, 0x00, 0x2A, 0x54, 0x00 },
  [WEATHER_SNOW] = { 0x1C, 0x3E, 0x7F, 0x00, 0x55, 0x00, 0x2A },
  [WEATHER_STORM] = { 0x1C, 0x3E, 0x7F, 0x0C, 0x18, 0x0C, 0x10 },
  [WEATHER_FOG] = { 0x00, 0x7F, 0x00, 0x3E, 0x00, 0x7F, 0x00 },
  [ICON_FOOT] = { 0x00, 0x30, 0x30, 0x38, 0x3E, 0x7F, 0x7F },
};

static void prv_draw_icon(GContext *ctx, int icon, GPoint origin, int16_t cell) {
  for (int row = 0; row < ICON_SIZE; row++) {
    for (int col = 0; col < ICON_SIZE; col++) {
      if (s_icons[icon][row] & (1 << (ICON_SIZE - 1 - col))) {
        graphics_fill_rect(ctx, GRect(origin.x + col * cell, origin.y + row * cell, cell, cell),
                           0, GCornerNone);
      }
    }
  }
}

// Sizes for the small date / info rows, scaled from the screen size
typedef struct {
  int16_t stroke;
  int16_t glyph_w;
  int16_t glyph_h;
  int16_t gap;
} InfoMetrics;

static InfoMetrics prv_info_metrics(GRect bounds) {
  const int16_t base = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  const int16_t t = base / 66 > 0 ? base / 66 : 1;
  return (InfoMetrics) {
    .stroke = t,
    .glyph_w = t * 9 / 2,
    .glyph_h = t * ICON_SIZE,
    .gap = t,
  };
}

static void prv_draw_date(GContext *ctx, int16_t center_x, int16_t y, InfoMetrics m) {
  if (s_settings.date_format == DATE_NONE) {
    return;
  }
  char text[8];
  if (s_settings.date_format == DATE_DD_MM) {
    snprintf(text, sizeof(text), "%02d/%02d", s_day, s_month);
  } else {
    snprintf(text, sizeof(text), "%02d/%02d", s_month, s_day);
  }
  const int16_t w = prv_text_width(text, m.glyph_w, m.stroke, m.gap);
  graphics_context_set_fill_color(ctx, s_settings.minute_color);
  prv_draw_text(ctx, text, GPoint(center_x - w / 2, y), m.glyph_w, m.glyph_h, m.stroke, m.gap);
}

static int prv_step_count(void) {
#if defined(PBL_HEALTH)
  return (int)health_service_sum_today(HealthMetricStepCount);
#else
  return -1;
#endif
}

static void prv_draw_bottom_info(GContext *ctx, int16_t center_x, int16_t y, InfoMetrics m) {
  char text[12];
  int icon;
  if (s_settings.bottom_info == BOTTOM_TEMPERATURE) {
    icon = s_weather.valid ? s_weather.condition : WEATHER_UNKNOWN;
    if (s_weather.valid) {
      snprintf(text, sizeof(text), "%d*", s_weather.temperature);
    } else {
      snprintf(text, sizeof(text), "--*");
    }
  } else if (s_settings.bottom_info == BOTTOM_STEPS) {
    icon = ICON_FOOT;
    const int steps = prv_step_count();
    if (steps >= 0) {
      snprintf(text, sizeof(text), "%d", steps);
    } else {
      snprintf(text, sizeof(text), "--");
    }
  } else {
    return;
  }

  const bool has_icon = icon != WEATHER_UNKNOWN;
  const int16_t icon_w = has_icon ? m.stroke * ICON_SIZE + m.stroke * 2 : 0;
  const int16_t total_w = icon_w + prv_text_width(text, m.glyph_w, m.stroke, m.gap);
  const int16_t x = center_x - total_w / 2;

  graphics_context_set_fill_color(ctx, s_settings.minute_color);
  if (has_icon) {
    prv_draw_icon(ctx, icon, GPoint(x, y), m.stroke);
  }
  prv_draw_text(ctx, text, GPoint(x + icon_w, y), m.glyph_w, m.glyph_h, m.stroke, m.gap);
}

// Draws a two-digit number centered horizontally in area, digits filling its height
static void prv_draw_number(GContext *ctx, int value, GRect area, int16_t digit_w, int16_t gap,
                            int16_t stroke) {
  const int digits[2] = { value / 10, value % 10 };
  const int16_t x = area.origin.x + (area.size.w - (digit_w * 2 + gap)) / 2;

  prv_draw_digit(ctx, digits[0], GRect(x, area.origin.y, digit_w, area.size.h), stroke);
  prv_draw_digit(ctx, digits[1], GRect(x + digit_w + gap, area.origin.y, digit_w, area.size.h),
                 stroke);
}

// Stroke for a font weight, as numerators[weight] / denom of size (at least 1px)
static int16_t prv_weight_stroke(uint8_t weight, int16_t size,
                                 const int16_t numerators[WEIGHT_COUNT], int16_t denom) {
  const int16_t stroke = size * numerators[weight < WEIGHT_COUNT ? weight : WEIGHT_THICK] / denom;
  return stroke > 0 ? stroke : 1;
}

// Large mode: hour fills the top half, minutes fill the bottom half
static void prv_draw_large(GContext *ctx, GRect bounds) {
  const InfoMetrics m = prv_info_metrics(bounds);
  const int16_t center_x = bounds.size.w / 2;
#if defined(PBL_ROUND)
  // Keep the digits inside the largest square that fits in the circle; the info rows sit
  // in the space between that square and the edge of the circle
  const GRect area = grect_inset(bounds, GEdgeInsets(bounds.size.w * 19 / 100));
  const int16_t top_y = (area.origin.y - m.glyph_h) / 2 + m.stroke;
  const int16_t bottom_y = bounds.size.h - top_y - m.glyph_h;
#else
  // Give up a strip at the top / bottom of the digit area for each enabled info row
  const int16_t margin = bounds.size.w * 6 / 144;
  GRect area = grect_inset(bounds, GEdgeInsets(margin));
  const int16_t strip = m.glyph_h + margin;
  if (s_settings.date_format != DATE_NONE) {
    area.origin.y += strip;
    area.size.h -= strip;
  }
  if (s_settings.bottom_info != BOTTOM_NONE) {
    area.size.h -= strip;
  }
  const int16_t top_y = margin;
  const int16_t bottom_y = bounds.size.h - margin - m.glyph_h;
#endif
  prv_draw_date(ctx, center_x, top_y, m);
  prv_draw_bottom_info(ctx, center_x, bottom_y, m);

  const int16_t row_gap = area.size.h * 8 / 156;
  const int16_t digit_gap = area.size.w * 8 / 132;
  const int16_t digit_h = (area.size.h - row_gap) / 2;
  const int16_t digit_w = (area.size.w - digit_gap) / 2;
  static const int16_t weights[WEIGHT_COUNT] = { 5, 8, 14 };
  const int16_t hour_stroke = prv_weight_stroke(s_settings.hour_weight, digit_w, weights, 62);
  const int16_t minute_stroke = prv_weight_stroke(s_settings.minute_weight, digit_w, weights, 62);

  graphics_context_set_fill_color(ctx, s_settings.hour_color);
  prv_draw_number(ctx, s_hour, GRect(area.origin.x, area.origin.y, area.size.w, digit_h),
                  digit_w, digit_gap, hour_stroke);

  graphics_context_set_fill_color(ctx, s_settings.minute_color);
  prv_draw_number(ctx, s_minute,
                  GRect(area.origin.x, area.origin.y + digit_h + row_gap, area.size.w, digit_h),
                  digit_w, digit_gap, minute_stroke);
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
  static const int16_t weights[WEIGHT_COUNT] = { 3, 4, 6 };
  const int16_t hour_stroke = prv_weight_stroke(s_settings.hour_weight, base, weights, 144);
  const int16_t minute_stroke = prv_weight_stroke(s_settings.minute_weight, base, weights, 144);

  const int digits[4] = { s_hour / 10, s_hour % 10, s_minute / 10, s_minute % 10 };
  const int16_t strokes[4] = { hour_stroke, hour_stroke, minute_stroke, minute_stroke };
  const int16_t gaps[3] = { digit_gap, group_gap, digit_gap };

  // Every digit gets a full-width box so the layout doesn't shift as the time changes
  const int16_t total_w = digit_w * 4 + gaps[0] + gaps[1] + gaps[2];
  int16_t x = (bounds.size.w - total_w) / 2;
  const int16_t y = (bounds.size.h - digit_h) / 2;

  const InfoMetrics m = prv_info_metrics(bounds);
  const int16_t info_gap = base * 18 / 144;
  prv_draw_date(ctx, bounds.size.w / 2, y - info_gap - m.glyph_h, m);
  prv_draw_bottom_info(ctx, bounds.size.w / 2, y + digit_h + info_gap, m);

  for (int i = 0; i < 4; i++) {
    graphics_context_set_fill_color(ctx, i < 2 ? s_settings.hour_color : s_settings.minute_color);
    prv_draw_digit(ctx, digits[i], GRect(x, y, digit_w, digit_h), strokes[i]);
    if (i < 3) {
      x += digit_w + gaps[i];
    }
  }
}

static void prv_default_settings(void) {
  s_settings.hour_color = GColorWhite;
  s_settings.minute_color = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite);
  s_settings.background_color = GColorBlack;
  s_settings.large_font = false;
  s_settings.date_format = DATE_NONE;
  s_settings.bottom_info = BOTTOM_NONE;
  s_settings.hour_weight = WEIGHT_REGULAR; // TEMP
  s_settings.minute_weight = WEIGHT_REGULAR; // TEMP
}

static void prv_load_settings(void) {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  persist_read_data(WEATHER_KEY, &s_weather, sizeof(s_weather));
}

static void prv_request_weather(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_RequestWeather, 1);
    app_message_outbox_send();
  }
}

// Clay sends select values as strings, other values as integers
static int32_t prv_tuple_int(const Tuple *tuple) {
  return tuple->type == TUPLE_CSTRING ? atoi(tuple->value->cstring) : tuple->value->int32;
}

static void prv_apply_settings(void) {
  window_set_background_color(s_window, s_settings.background_color);
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *temperature_t = dict_find(iter, MESSAGE_KEY_Temperature);
  Tuple *condition_t = dict_find(iter, MESSAGE_KEY_WeatherCondition);
  if (temperature_t && condition_t) {
    s_weather.temperature = prv_tuple_int(temperature_t);
    s_weather.condition = prv_tuple_int(condition_t);
    s_weather.valid = s_weather.condition > WEATHER_UNKNOWN && s_weather.condition < ICON_FOOT;
    persist_write_data(WEATHER_KEY, &s_weather, sizeof(s_weather));
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
    return;
  }

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
  Tuple *date_format_t = dict_find(iter, MESSAGE_KEY_DateFormat);
  if (date_format_t) {
    s_settings.date_format = prv_tuple_int(date_format_t);
  }
  Tuple *bottom_info_t = dict_find(iter, MESSAGE_KEY_BottomInfo);
  if (bottom_info_t) {
    s_settings.bottom_info = prv_tuple_int(bottom_info_t);
  }
  Tuple *hour_weight_t = dict_find(iter, MESSAGE_KEY_HourWeight);
  if (hour_weight_t) {
    s_settings.hour_weight = prv_tuple_int(hour_weight_t);
  }
  Tuple *minute_weight_t = dict_find(iter, MESSAGE_KEY_MinuteWeight);
  if (minute_weight_t) {
    s_settings.minute_weight = prv_tuple_int(minute_weight_t);
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
  s_month = tick_time->tm_mon + 1;
  s_day = tick_time->tm_mday;  layer_mark_dirty(s_canvas_layer);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
  if (s_settings.bottom_info == BOTTOM_TEMPERATURE && tick_time->tm_min % 30 == 0) {
    prv_request_weather();
  }
}

#if defined(PBL_HEALTH)
static void prv_health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate && s_settings.bottom_info == BOTTOM_STEPS &&
      s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}
#endif

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
#if defined(PBL_HEALTH)
  health_service_events_subscribe(prv_health_handler, NULL);
#endif

  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
