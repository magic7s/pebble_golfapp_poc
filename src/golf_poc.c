#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "mini-printf.h"


#define MY_UUID { 0x6F, 0xC5, 0x2E, 0xDD, 0xF7, 0x0E, 0x44, 0xA5, 0xA4, 0x6C, 0x3C, 0x7D, 0x40, 0x1B, 0x7B, 0x0E }
PBL_APP_INFO(MY_UUID,
             "Golf POC", "magic7s",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
Window finalscore;
Layer holepar;
TextLayer holepar_hole;
TextLayer holepar_hole_num;
TextLayer holepar_par;
TextLayer holepar_par_num;
TextLayer roundtime;
TextLayer distance; /* To be updated by external app */
TextLayer shots_hole;
TextLayer shots_total;
TextLayer club;
TextLayer finalscore_score;
TextLayer finalscore_putts;

AppTimerHandle timer_handle;
#define COOKIE_MY_TIMER 1

AppContextRef *g_ctx;

static int thishole = 0;
static char s_shots_hole[25];
static int totalshots = 0;
static int totalputts = 0;
static char s_shots_total[25];
static char s_final_score[25];
static char s_final_putts[25];
static char s_roundtime[25];
static int round_start_hour;
static int round_start_minute;
bool round_started = false;

static int club_menu_index = 0;
static int club_menu_maxsize = 15; /*This should be one less than the size of the menu array */

static char club_menu[16][50] = {
"Start Round",
"Driver",
"3-Wood",
"5-Wood",
"4-iron",
"5-iron",
"6-iron",
"7-iron",
"8-iron",
"9-iron",
"Pitching Wedge",
"Approach Wedge",
"Sand Wedge",
"Putter",
"Next Hole",
// "Previous Hole", /* Future Support - b/c we forget the hole scores */
"End Round"
};

static int holenum_index = 0;

static char s_holenum[18][3] = {
"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18"
};


void end_round() {
    round_started = false;
    club_menu_index = 0;
	text_layer_set_text(&club, club_menu[club_menu_index]);
	window_init(&finalscore, "Final Score");
	window_stack_push(&finalscore, true /* Animated */);
	text_layer_init(&finalscore_score, GRect(0, 0, 144, 168-16));
	text_layer_set_background_color(&finalscore_score, GColorWhite);
	text_layer_set_text_color(&finalscore_score, GColorBlack);
	text_layer_set_font(&finalscore_score,fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD));
	mini_snprintf(s_final_score, 25, "Final Score %d", totalshots);
	text_layer_set_text(&finalscore_score, s_final_score);
	text_layer_set_text_alignment(&finalscore_score, GTextAlignmentCenter);
	layer_add_child(&finalscore.layer, &finalscore_score.layer);
	
	text_layer_init(&finalscore_putts, GRect(0, 126, 144, 168-16-126));
	text_layer_set_background_color(&finalscore_putts, GColorWhite);
	text_layer_set_text_color(&finalscore_putts, GColorBlack);
	text_layer_set_font(&finalscore_putts,fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	mini_snprintf(s_final_putts, 25, "Total Putts: %d", totalputts);
	text_layer_set_text(&finalscore_putts, s_final_putts);
	text_layer_set_text_alignment(&finalscore_putts, GTextAlignmentCenter);
	layer_add_child(&finalscore.layer, &finalscore_putts.layer);
	
	vibes_double_pulse();
	
}

void update_holepar() {
		if (holenum_index == 17) {
			club_menu_index = 15;
			text_layer_set_text(&club, club_menu[club_menu_index]);
		}
		holenum_index++;
    	text_layer_set_text(&holepar_hole_num, s_holenum[holenum_index]);
}

void update_total() {
	totalshots++;
	if (club_menu_index == 13) {totalputts++;}
	mini_snprintf(s_shots_total, 25, "Total: %d", totalshots);
	text_layer_set_text(&shots_total, s_shots_total);
}

void save_clubinfo() {
	// Save Club and location information to phone.
}

void update_shot() {
	thishole++;
	mini_snprintf(s_shots_hole, 25, "Shots: %d", thishole);
	text_layer_set_text(&shots_hole, s_shots_hole);
	update_total();
	text_layer_set_text(&club, "Shot Recorded");
	timer_handle = app_timer_send_event(g_ctx, 1500 /* milliseconds */, COOKIE_MY_TIMER);
}

void start_new_round() {
    holenum_index = 0;
    text_layer_set_text(&holepar_hole_num, s_holenum[holenum_index]);
	// Start Round timer
	totalshots = -1;
	thishole = -1;
	update_shot();
	club_menu_index = 1;
	text_layer_set_text(&club, club_menu[club_menu_index]);
	round_started = true;
	mini_snprintf(s_roundtime, 25, "Round Time: %02d:%02d", 0, 0);
   	text_layer_set_text(&roundtime, s_roundtime);
	PblTm roundstarttime;
	get_time(&roundstarttime);
	round_start_hour = roundstarttime.tm_hour;
	round_start_minute = roundstarttime.tm_min;
    
}


void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  if (round_started && (club_menu_index == 1)) { return;} /* Don't show Start Round if round already started */
  
  // Go up in the menu and update the club TextLayer
  if (club_menu_index > 0) {
  	club_menu_index = club_menu_index-1;
    text_layer_set_text(&club, club_menu[club_menu_index]);
  }	
}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  
  if ((club_menu_index == 0) && (round_started == false)) { return;}
  // Go down in the menu and update the club TextLayer
  if (club_menu_index < club_menu_maxsize) {
  	club_menu_index++;
    text_layer_set_text(&club, club_menu[club_menu_index]);
  }	


}


void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  
  switch (club_menu_index)
  {
  	case 0 : { /* Start Round*/ 
		if (round_started == false) {
			start_new_round();
		}
  		break;
  	}
  	case 14 : { /* Next Hole*/ 
  		thishole = 0;
  		mini_snprintf(s_shots_hole, 25, "Shots: %d", thishole);
		text_layer_set_text(&shots_hole, s_shots_hole);
		update_holepar();
		club_menu_index = 1;
    	text_layer_set_text(&club, club_menu[club_menu_index]);
  		break;
  	}
//  	case 15 : { /* Previous Hole */
//		break;
//  	}
  	case 15 : {  /* End Round*/ 
  		// Stop Round Timer
  		// Clear hole par
  		// Add new layer with final score big?
  		end_round();
  		break;
  	}
    default : { /* Record Shot*/ 
    	save_clubinfo();
    	update_shot();
    }
  
  }
}


void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
}

void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;
  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;
  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}



void holepar_layer_callback(Layer *me, GContext* ctx) {
  (void)me;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  // Vertical lines
  graphics_draw_line(ctx, GPoint(72, 19), GPoint(72, 40));
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

  if (cookie == COOKIE_MY_TIMER) {
      text_layer_set_text(&club, club_menu[club_menu_index]);
  }
}


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  if (round_started) {
  	PblTm currenttime;
  	get_time(&currenttime);	
  	int rt_hour;
  	int rt_min;
  	rt_hour = (currenttime.tm_hour - round_start_hour);
  	rt_min = (currenttime.tm_min - round_start_minute);
  	if (rt_min < 0) { rt_hour--; rt_min += 60; };
  	if (rt_min > 59) { rt_hour++; rt_min -= 60; };
 	mini_snprintf(s_roundtime, 25, "Round Time: %02d:%02d", rt_hour, rt_min);
   	text_layer_set_text(&roundtime, s_roundtime);
  }
}


void handle_init(AppContextRef ctx) {
  (void)ctx;
  g_ctx = ctx;
  
  window_init(&window, "Golf Dashboard");
  window_stack_push(&window, true /* Animated */);
  
  GFont defaultfont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  
  layer_init(&holepar, GRect(0, 0, 144 /* width */, 40 /* height */));
  holepar.update_proc = &holepar_layer_callback;
  layer_add_child(&window.layer, &holepar);
  
  text_layer_init(&holepar_hole, GRect(0, 0, 72, 17));
  text_layer_set_background_color(&holepar_hole, GColorClear);
  text_layer_set_text_color(&holepar_hole, GColorBlack);
  text_layer_set_text(&holepar_hole, "Hole");
  text_layer_set_text_alignment(&holepar_hole, GTextAlignmentCenter);
  layer_add_child(&holepar, &holepar_hole.layer);
  
  GFont holeparfont = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  text_layer_init(&holepar_hole_num, GRect(0, 10, 60, 30));
  text_layer_set_background_color(&holepar_hole_num, GColorClear);
  text_layer_set_text_color(&holepar_hole_num, GColorBlack);
  text_layer_set_font(&holepar_hole_num,holeparfont);
  text_layer_set_text(&holepar_hole_num, "");
  text_layer_set_text_alignment(&holepar_hole_num, GTextAlignmentCenter);
  layer_add_child(&holepar, &holepar_hole_num.layer);
    
  text_layer_init(&holepar_par, GRect(72, 0, 72, 17));
  text_layer_set_background_color(&holepar_par, GColorClear);
  text_layer_set_text_color(&holepar_par, GColorBlack);
  text_layer_set_text(&holepar_par, "Par");
  text_layer_set_text_alignment(&holepar_par, GTextAlignmentCenter);
  layer_add_child(&holepar, &holepar_par.layer);
  
//  GFont holeparfont = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  text_layer_init(&holepar_par_num, GRect(75, 10, 60, 30));
  text_layer_set_background_color(&holepar_par_num, GColorClear);
  text_layer_set_text_color(&holepar_par_num, GColorBlack);
  text_layer_set_font(&holepar_par_num,holeparfont);
  text_layer_set_text(&holepar_par_num, "0");
  text_layer_set_text_alignment(&holepar_par_num, GTextAlignmentCenter);
  layer_add_child(&holepar, &holepar_par_num.layer);
  
  text_layer_init(&roundtime, GRect(0, 40, 144 /* width */, 15 /* height */));
  text_layer_set_background_color(&roundtime, GColorBlack);
  text_layer_set_text_color(&roundtime, GColorWhite);
  text_layer_set_font(&roundtime, defaultfont);
  text_layer_set_text(&roundtime, "Round Time:      ");
  text_layer_set_text_alignment(&roundtime, GTextAlignmentCenter);
  layer_add_child(&window.layer, &roundtime.layer);
  
  GFont yrdfont = fonts_get_system_font(FONT_KEY_GOTHAM_42_MEDIUM_NUMBERS);
  text_layer_init(&distance, GRect(0, 55, 144 /* width */, 50 /* height */));
  text_layer_set_background_color(&distance, GColorBlack);
  text_layer_set_text_color(&distance, GColorWhite);
// Placeholder for Sports SDK
  text_layer_set_text(&distance, "000");
  text_layer_set_font(&distance, yrdfont);
  text_layer_set_text_alignment(&distance, GTextAlignmentCenter);
  layer_add_child(&window.layer, &distance.layer);
  
 
  mini_snprintf(s_shots_hole, 25, "Shots: %d", thishole);
  text_layer_init(&shots_hole, GRect(0, 105, 72 /* width */, 15 /* height */));
  text_layer_set_background_color(&shots_hole, GColorBlack);
  text_layer_set_text_color(&shots_hole, GColorWhite);
  text_layer_set_font(&shots_hole, defaultfont);
  text_layer_set_text(&shots_hole, s_shots_hole);
  text_layer_set_text_alignment(&shots_hole, GTextAlignmentLeft);
  layer_add_child(&window.layer, &shots_hole.layer);
  
  mini_snprintf(s_shots_total, 25, "Total: %d", totalshots);
  text_layer_init(&shots_total, GRect(72, 105, 72 /* width */, 15 /* height */));
  text_layer_set_background_color(&shots_total, GColorBlack);
  text_layer_set_text_color(&shots_total, GColorWhite);
  text_layer_set_font(&shots_total, defaultfont);
  text_layer_set_text(&shots_total, s_shots_total);
  text_layer_set_text_alignment(&shots_total, GTextAlignmentRight);
  layer_add_child(&window.layer, &shots_total.layer);
  
  GFont clubfont = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  text_layer_init(&club, GRect(0, 120, 144 /* width */, 168-120-16 /* height */));
  text_layer_set_background_color(&club, GColorWhite);
  text_layer_set_text_color(&club, GColorBlack);
  text_layer_set_font(&club, clubfont);
  text_layer_set_text(&club, club_menu[club_menu_index]);
  text_layer_set_text_alignment(&club, GTextAlignmentCenter);
  layer_add_child(&window.layer, &club.layer);
 
  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
  
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .timer_handler = &handle_timer,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
	}
  };
  app_event_loop(params, &handlers);
}
