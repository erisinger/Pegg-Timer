#include <pebble.h>

//middle button: short press -- start/pause, long press -- restart
//upper button: short press -- minutes++, long press -- minutes += 0.5
//lower button: short press -- minutes--, long press -- minutes -= 0.5
//implicit: back button to clear

//future: multiple timers available through scrollable list

//functions
static void display_time();
static void timer_handler();
static void set_timer();
static void start_stop_timer();
//static void reset_timer();
static void add_time(int delta);
static void timer_complete();
static void vibrate();

//variables
static AppTimer *timer;
//static double minutes;
static int remaining = 0;

static Window *window;
static TextLayer *text_layer;

bool running = false;

static void display_time(){
	char rem[64];
	int minutes = remaining  / 1000 / 60;
	int seconds = (remaining - minutes * 1000 * 60) / 1000;
	snprintf(rem, 64, "%02d:%02d", minutes, seconds);
	text_layer_set_text(text_layer, rem);
}

static void timer_handler(){
	if (!running) {
		return;
	}
	remaining -= 1000;
	if (remaining > 0) {
		set_timer();
		display_time();
	}
	else {
		timer_complete();
	}
	
}

static void set_timer(){
	if(running){
		timer = app_timer_register(1000, timer_handler, NULL);
	}
}

static void start_stop_timer(){
	running = !running;
	set_timer();
}

static void add_time(int delta_milliseconds){
	remaining += delta_milliseconds;
}

static void timer_complete(){
	//buzz, display, the whole nine
	running = false;
	text_layer_set_text(text_layer, "00:00");
	vibrate();
	display_time();
}

static void vibrate(){
	// Vibe pattern: ON for 200ms, OFF for 100ms, ON for 400ms:
	static const uint32_t const segments[] = { 100, 100, 100, 500, 100, 100, 100, 500, 100, 100, 100, 500, 100, 100, 100 };
	VibePattern pat = {
	  .durations = segments,
	  .num_segments = ARRAY_LENGTH(segments),
	};
	for (int i = 0; i < 4; i++) {
		vibes_enqueue_custom_pattern(pat);
	}	
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	//start/pause the timer
	start_stop_timer();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (remaining < 99 * 60 * 1000 + 59 * 1000) {
		add_time(1000 * 60);
	}
	display_time();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (remaining > 500 * 60) {
		add_time(-500 * 60);
	}
	
	display_time();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 52 }, .size = { bounds.size.w, bounds.size.h } });
  text_layer_set_text(text_layer, "00:00");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  set_timer();
  
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
