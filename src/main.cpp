#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <lvgl.h>
#include <Preferences.h>
#include "esp_system.h"
#include "translations.h"
#include "bmpLoader.h"
#include <SD.h>
#include <SPI.h>
#include "esp_timer.h"

#include <lgfx/cheapYellowDisplay_2432S028R.h>

#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS   5

#define LOG_MEM_ON_IMAGE_SWITCH 1

#define LCD_BACKLIGHT_PIN 21

#define LATITUDE_DEFAULT "51.5074"
#define LONGITUDE_DEFAULT "-0.1278"
#define LOCATION_DEFAULT "London"
#define DEFAULT_CAPTIVE_SSID "Aura"
#define UPDATE_INTERVAL 600000UL  // 10 minutes

// Night mode starts at 10pm and ends at 6am
#define NIGHT_MODE_START_HOUR 22
#define NIGHT_MODE_END_HOUR 6


LV_FONT_DECLARE(lv_font_montserrat_latin_12);
LV_FONT_DECLARE(lv_font_montserrat_latin_14);
LV_FONT_DECLARE(lv_font_montserrat_latin_16);
LV_FONT_DECLARE(lv_font_montserrat_latin_20);
LV_FONT_DECLARE(lv_font_montserrat_latin_42);

static Language current_language = LANG_EN;

// Font selection based on language
const lv_font_t* get_font_12() {
  return &lv_font_montserrat_latin_12;
}

const lv_font_t* get_font_14() {
  return &lv_font_montserrat_latin_14;
}

const lv_font_t* get_font_16() {
  return &lv_font_montserrat_latin_16;
}

const lv_font_t* get_font_20() {
  return &lv_font_montserrat_latin_20;
}

const lv_font_t* get_font_42() {
  return &lv_font_montserrat_latin_42;
}

int x, y, z;

// Preferences
static Preferences prefs;
static bool use_fahrenheit = false;
static bool use_24_hour = false; 
static bool use_night_mode = false;
static char latitude[16] = LATITUDE_DEFAULT;
static char longitude[16] = LONGITUDE_DEFAULT;
static String location = String(LOCATION_DEFAULT);

static JsonDocument geoDoc;
static JsonArray geoResults;

// Screen dimming variables
static bool night_mode_active = false;
static bool temp_screen_wakeup_active = false;
static lv_timer_t *temp_screen_wakeup_timer = nullptr;

// UI components
static lv_obj_t *lbl_today_temp;
static lv_obj_t *lbl_today_feels_like;
static lv_obj_t *img_today_icon;
static lv_obj_t *lbl_forecast;
static lv_obj_t *box_daily;
static lv_obj_t *box_hourly;
static lv_obj_t *lbl_daily_day[7];
static lv_obj_t *lbl_daily_high[7];
static lv_obj_t *lbl_daily_low[7];
static lv_obj_t *img_daily[7];
static lv_obj_t *lbl_hourly[7];
static lv_obj_t *lbl_precipitation_probability[7];
static lv_obj_t *lbl_hourly_temp[7];
static lv_obj_t *img_hourly[7];
static lv_obj_t *lbl_loc;
static lv_obj_t *loc_ta;
static lv_obj_t *results_dd;
static lv_obj_t *btn_close_loc;
static lv_obj_t *btn_close_obj;
static lv_obj_t *kb;
static lv_obj_t *settings_win;
static lv_obj_t *location_win = nullptr;
static lv_obj_t *unit_switch;
static lv_obj_t *clock_24hr_switch;
static lv_obj_t *night_mode_switch;
static lv_obj_t *language_dropdown;
static lv_obj_t *lbl_clock;

static lv_obj_t *gesture_layer;
static lv_indev_t * g_indev = nullptr;


// Weather icons
LV_IMG_DECLARE(icon_blizzard);
LV_IMG_DECLARE(icon_blowing_snow);
LV_IMG_DECLARE(icon_clear_night);
LV_IMG_DECLARE(icon_cloudy);
LV_IMG_DECLARE(icon_drizzle);
LV_IMG_DECLARE(icon_flurries);
LV_IMG_DECLARE(icon_haze_fog_dust_smoke);
LV_IMG_DECLARE(icon_heavy_rain);
LV_IMG_DECLARE(icon_heavy_snow);
LV_IMG_DECLARE(icon_isolated_scattered_tstorms_day);
LV_IMG_DECLARE(icon_isolated_scattered_tstorms_night);
LV_IMG_DECLARE(icon_mostly_clear_night);
LV_IMG_DECLARE(icon_mostly_cloudy_day);
LV_IMG_DECLARE(icon_mostly_cloudy_night);
LV_IMG_DECLARE(icon_mostly_sunny);
LV_IMG_DECLARE(icon_partly_cloudy);
LV_IMG_DECLARE(icon_partly_cloudy_night);
LV_IMG_DECLARE(icon_scattered_showers_day);
LV_IMG_DECLARE(icon_scattered_showers_night);
LV_IMG_DECLARE(icon_showers_rain);
LV_IMG_DECLARE(icon_sleet_hail);
LV_IMG_DECLARE(icon_snow_showers_snow);
LV_IMG_DECLARE(icon_strong_tstorms);
LV_IMG_DECLARE(icon_sunny);
LV_IMG_DECLARE(icon_tornado);
LV_IMG_DECLARE(icon_wintry_mix_rain_snow);

// Weather Images
LV_IMG_DECLARE(image_blizzard);
LV_IMG_DECLARE(image_blowing_snow);
LV_IMG_DECLARE(image_clear_night);
LV_IMG_DECLARE(image_cloudy);
LV_IMG_DECLARE(image_drizzle);
LV_IMG_DECLARE(image_flurries);
LV_IMG_DECLARE(image_haze_fog_dust_smoke);
LV_IMG_DECLARE(image_heavy_rain);
LV_IMG_DECLARE(image_heavy_snow);
LV_IMG_DECLARE(image_isolated_scattered_tstorms_day);
LV_IMG_DECLARE(image_isolated_scattered_tstorms_night);
LV_IMG_DECLARE(image_mostly_clear_night);
LV_IMG_DECLARE(image_mostly_cloudy_day);
LV_IMG_DECLARE(image_mostly_cloudy_night);
LV_IMG_DECLARE(image_mostly_sunny);
LV_IMG_DECLARE(image_partly_cloudy);
LV_IMG_DECLARE(image_partly_cloudy_night);
LV_IMG_DECLARE(image_scattered_showers_day);
LV_IMG_DECLARE(image_scattered_showers_night);
LV_IMG_DECLARE(image_showers_rain);
LV_IMG_DECLARE(image_sleet_hail);
LV_IMG_DECLARE(image_snow_showers_snow);
LV_IMG_DECLARE(image_strong_tstorms);
LV_IMG_DECLARE(image_sunny);
LV_IMG_DECLARE(image_tornado);
LV_IMG_DECLARE(image_wintry_mix_rain_snow);

LGFX tft;

static bool startedConfigPortal = false;
static bool gesture_in_progress = false;
static int dialogIndex = 0; 

auto lv_last_tick = millis();

SPIClass sdSPI(HSPI);            

static lv_display_t * disp;

static bool lvgl_display_enabled = false;
static lv_color_t buf1[240 * 32];
static lv_color_t buf2[240 * 32];

int currentIconId = 0; 
static esp_timer_handle_t my_timer;

static const int MAX_BG = 64;
static String bg_list[MAX_BG];      // max numbers 
static int bg_count = 0;
static int bg_idx = 0;


static bool lv_sd_ready(lv_fs_drv_t* drv) {
  LV_UNUSED(drv);
  return true;
}

static void lvgl_flush_cb(lv_display_t *d, const lv_area_t *area, uint8_t *px_map)
{
  if (lvgl_display_enabled) {

    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);

    tft.setSwapBytes(true);
    tft.pushPixels((const uint16_t*)px_map, (uint32_t)(w * h));
    tft.setSwapBytes(false);

    tft.endWrite();
  }

  lv_display_flush_ready(d);
}


bool initSdSpi() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if(!SD.begin(SD_CS, sdSPI, 4000000)) {
    Serial.println("Card Mount Failed");
    return false;
  }

  Serial.println("SD SPI mounted");
  return true;
}


static bool hasBmpExt(const String &name) {
  String n = name; n.toLowerCase();
  return n.endsWith(".bmp");
}

static void scan_bg_dir(const char *dirpath)
{
  bg_count = 0;

  File dir = SD.open(dirpath);
  if(!dir || !dir.isDirectory()) {
    Serial.printf("BG dir not found: %s\n", dirpath);
    return;
  }

  while(true) {
    File f = dir.openNextFile();
    if(!f) break;

    if(!f.isDirectory()) {
      String fname = String(f.name()); 
      if(hasBmpExt(fname) && bg_count < MAX_BG) {
        String full = String(dirpath);
        if(!full.endsWith("/")) full += "/";
        full += fname;

        if(!full.startsWith("/")) full = "/" + full;

        bg_list[bg_count++] = full;
        Serial.printf("BG add: %s\n", full.c_str());
      }
    }

    f.close();
  }

  dir.close();
  Serial.printf("BG files found: %d\n", bg_count);
}

void add_fullscreen_overlay();
void create_ui();
void fetch_and_update_weather();
void create_settings_window();
static void settings_event_handler(lv_event_t *e);
const lv_img_dsc_t *choose_image(int wmo_code, int is_day);
const lv_img_dsc_t *choose_icon(int wmo_code, int is_day);


// Screen dimming functions
bool night_mode_should_be_active();
void activate_night_mode();
void deactivate_night_mode();
void check_for_night_mode();
void handle_temp_screen_wakeup_timeout(lv_timer_t *timer);

static const char* PREF_NS_TOUCH = "touch";
static const char* PREF_KEY_TCAL = "cal";   // blob key
static const size_t TCAL_SIZE = sizeof(uint16_t) * 8;

String urlencode(const String &str) {
  String encoded = "";
  char buf[5];
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    // Unreserved characters according to RFC 3986
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      // Percent-encode others
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

void populate_results_dropdown() {
  if (!results_dd || !btn_close_loc) return;

  String opts;
  opts.reserve(1024); // optional, reduziert Fragmentierung

  for (JsonObject item : geoResults) {
    const char* name  = item["name"]  | "";
    const char* admin = item["admin1"] | "";

    // Zeile zusammenbauen
    opts += name;
    if (admin && admin[0] != '\0') {
      opts += ", ";
      opts += admin;
    }
    opts += "\n";

    // harter Cut, damit wir nicht endlos wachsen (optional)
    if (opts.length() > 1500) break;
  }

  if (geoResults.size() > 0) {
    lv_dropdown_set_options(results_dd, opts.c_str()); // NICHT _static
    lv_obj_add_flag(results_dd, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_bg_color(btn_close_loc, lv_palette_main(LV_PALETTE_GREEN),
                              (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn_close_loc, LV_OPA_COVER,
                            (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_close_loc, lv_palette_darken(LV_PALETTE_GREEN, 1),
                              (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
    lv_obj_add_flag(btn_close_loc, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_dropdown_set_options(results_dd, "");
    lv_obj_clear_flag(results_dd, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btn_close_loc, LV_OBJ_FLAG_CLICKABLE);
  }
}

void do_geocode_query(const char *q) {
  geoDoc.clear();
  String url = String("http://geocoding-api.open-meteo.com/v1/search?name=") + urlencode(q) + "&count=10";

  HTTPClient http;
  http.begin(url);
  if (http.GET() == HTTP_CODE_OK) {

    Serial.println("Completed location search at open-meteo: " + url);
    WiFiClient *stream = http.getStreamPtr();
    DeserializationError err = deserializeJson(geoDoc, *stream);

    if (err) {
      Serial.printf("deserializeJson(stream) failed: %s\n", err.c_str());
      http.end();
      return;
    } else {
      geoResults = geoDoc["results"].as<JsonArray>();
      populate_results_dropdown();
    }
  } else {
      Serial.println("Failed location search at open-meteo: " + url);
  }
  http.end();
}

bool loadTouchCal(uint16_t cal[8]) {
  Preferences p;
  if (!p.begin(PREF_NS_TOUCH, true)) return false;
  size_t n = p.getBytes(PREF_KEY_TCAL, cal, TCAL_SIZE);
  p.end();
  return n == TCAL_SIZE;
}

bool saveTouchCal(const uint16_t cal[8]) {
  Preferences p;
  if (!p.begin(PREF_NS_TOUCH, false)) return false;
  size_t n = p.putBytes(PREF_KEY_TCAL, cal, TCAL_SIZE);
  p.end();
  return n == TCAL_SIZE;
}

void clearTouchCal() {
  Preferences p;
  if (!p.begin(PREF_NS_TOUCH, false)) return;
  p.remove(PREF_KEY_TCAL);
  p.end();
}

bool ensureTouchCalibrated(bool forceRecalibrate = false) {
  uint16_t cal[8];

  if (!forceRecalibrate && loadTouchCal(cal)) {
    tft.setTouchCalibrate(cal);
    Serial.println("Touch calibration loaded from Preferences");
    return true;
  }

  Serial.println("No touch calibration found -> starting calibration");

  tft.calibrateTouch(cal, TFT_MAGENTA, TFT_BLACK, 15);
  tft.setTouchCalibrate(cal);

  bool ok = saveTouchCal(cal);

  Serial.printf("Touch calibration %s\n", ok ? "saved" : "NOT saved");
  return ok;
}

static void reset_confirm_yes_cb(lv_event_t *e) {
  clearTouchCal();

  lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
  Serial.println("Clearing Wi-Fi creds and rebooting");
  WiFiManager wm;
  wm.resetSettings();
  delay(100);
  esp_restart();
}

static void reset_confirm_no_cb(lv_event_t *e) {
  lv_obj_t *mbox = (lv_obj_t *)lv_event_get_user_data(e);
  lv_obj_del(mbox);
}


int day_of_week(int y, int m, int d) {
  static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

String hour_of_day(int hour) {
  const LocalizedStrings* strings = get_strings(current_language);
  if(hour < 0 || hour > 23) return String(strings->invalid_hour);

  if (use_24_hour) {
    if (hour < 10)
      return String("0") + String(hour);
    else
      return String(hour);
  } else {
    if(hour == 0)   return String("12") + strings->am;
    if(hour == 12)  return String(strings->noon);

    bool isMorning = (hour < 12);
    String suffix = isMorning ? strings->am : strings->pm;

    int displayHour = hour % 12;

    return String(displayHour) + suffix;
  }
}

static void update_clock(lv_timer_t *timer) {
  struct tm timeinfo;

  check_for_night_mode();

  if (!getLocalTime(&timeinfo)) return;

  const LocalizedStrings* strings = get_strings(current_language);
  char buf[16];
  if (use_24_hour) {
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  } else {
    int hour = timeinfo.tm_hour % 12;
    if(hour == 0) hour = 12;
    const char *ampm = (timeinfo.tm_hour < 12) ? strings->am : strings->pm;
    snprintf(buf, sizeof(buf), "%d:%02d%s", hour, timeinfo.tm_min, ampm);
  }
  lv_label_set_text(lbl_clock, buf);
}

static void ta_event_cb(lv_event_t *e) {
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);

  // Show keyboard
  lv_keyboard_set_textarea(kb, ta);
  lv_obj_move_foreground(kb);
  lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void kb_event_cb(lv_event_t *e) {
  lv_obj_t *kb = static_cast<lv_obj_t *>(lv_event_get_target(e));
  lv_obj_add_flag((lv_obj_t *)lv_event_get_target(e), LV_OBJ_FLAG_HIDDEN);

  if (lv_event_get_code(e) == LV_EVENT_READY) {
    const char *loc = lv_textarea_get_text(loc_ta);
    if (strlen(loc) > 0) {
      do_geocode_query(loc);
    }
  }
}

static void ta_defocus_cb(lv_event_t *e) {
  lv_obj_add_flag((lv_obj_t *)lv_event_get_user_data(e), LV_OBJ_FLAG_HIDDEN);
}

static void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data)
{
  (void)indev;

  uint16_t tx, ty;
  bool touched = tft.getTouch(&tx, &ty);   

  if(!touched) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  data->state = LV_INDEV_STATE_PRESSED;
  data->point.x = tx;
  data->point.y = ty;
}

static void* lv_sd_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode)
{
  fs::FS* fs = static_cast<fs::FS*>(drv->user_data);
  String p = path;
  if (!p.startsWith("/")) p = "/" + p;

  File *fp = new File();
  if (mode == LV_FS_MODE_WR) *fp = fs->open(p.c_str(), FILE_WRITE);
  else                      *fp = fs->open(p.c_str(), FILE_READ);

  if (!(*fp)) {
    Serial.printf("LVGL open failed: %s\n", p.c_str());
    delete fp;
    return nullptr;
  }

  return fp;
}


static lv_fs_res_t lv_sd_close(lv_fs_drv_t* drv, void* file_p) {
  File* f = static_cast<File*>(file_p);
  f->close();
  delete f;
  return LV_FS_RES_OK;
}

static lv_fs_res_t lv_sd_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
  File* f = static_cast<File*>(file_p);
  *br = f->read(static_cast<uint8_t*>(buf), btr);
  return LV_FS_RES_OK;
}

static lv_fs_res_t lv_sd_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence) {
  File* f = static_cast<File*>(file_p);
  bool ok = false;
  if (whence == LV_FS_SEEK_SET) ok = f->seek(pos);
  else if (whence == LV_FS_SEEK_CUR) ok = f->seek(f->position() + pos);
  else if (whence == LV_FS_SEEK_END) ok = f->seek(f->size() - pos);
  return ok ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t lv_sd_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
  File* f = static_cast<File*>(file_p);
  *pos_p = f->position();
  return LV_FS_RES_OK;
}

static void lv_register_sd_fs(fs::FS& fs) {
  static lv_fs_drv_t drv;
  lv_fs_drv_init(&drv);
  drv.letter = 'S';
  drv.cache_size = 16*1024;   
  drv.user_data = &fs;

  drv.open_cb = lv_sd_open;
  drv.close_cb = lv_sd_close;
  drv.read_cb = lv_sd_read;
  drv.seek_cb = lv_sd_seek;
  drv.tell_cb = lv_sd_tell;
  drv.ready_cb = lv_sd_ready;

  lv_fs_drv_register(&drv);
}


void my_timer_callback(void* arg)
{
  tft.setSwapBytes(true);
  const lv_img_dsc_t *image = choose_image(currentIconId, 1);
  tft.pushImage((240-image->header.w) / 2, 30, image->header.w, image->header.h, (const uint16_t*)image->data);
  tft.setSwapBytes(false);

  currentIconId = (currentIconId + 1) % 3;
}

void create_timer()
{
    esp_timer_create_args_t timer_args = {
        .callback = &my_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK, 
        .name = "my_timer"
    };

    esp_timer_create(&timer_args, &my_timer);
    esp_timer_start_periodic(my_timer, 500000);
}

void removeTimer()
{
  if (my_timer) {
    esp_timer_stop(my_timer);
    esp_timer_delete(my_timer);
  }
}


void drawCenterString(String text, int y, uint16_t color) 
{
  tft.setTextColor(color);
  tft.drawString(text, (tft.width() - tft.textWidth(text)) / 2, y);
}


void flush_wifi_splashscreen(uint32_t ms = 200) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    lv_timer_handler();
    delay(5);
  }
}

void wifi_splash_screen() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x4c8cb9), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0xa6cdec), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

  const LocalizedStrings* strings = get_strings(current_language);
  lv_obj_t *lbl = lv_label_create(scr);
  lv_label_set_text(lbl, strings->wifi_config);
  lv_obj_set_style_text_font(lbl, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(lbl);
  lv_scr_load(scr);
}


void apModeCallback(WiFiManager *mgr) {
  lvgl_display_enabled = true;
  removeTimer();
  startedConfigPortal = true;
  wifi_splash_screen();
  flush_wifi_splashscreen();
}


static lv_image_dsc_t bg_small_dsc;
static lv_image_dsc_t bg_dsc[2];
static int bg_dsc_idx = 0;

static lv_timer_t *bg_timer = nullptr;

static lv_obj_t *bg_img = NULL;


static bool set_bg_from_file(const char *path, bool fillZero = false) 
{
  const uint16_t* px = load_bg_bmp_565_120x160(path);
  if(!px) return false;

  bg_dsc_idx ^= 1;

  if (fillZero == true) {
    lv_memzero(&bg_dsc[bg_dsc_idx], sizeof(bg_dsc[bg_dsc_idx]));
  }
  bg_dsc[bg_dsc_idx].header.w  = BG_W2;
  bg_dsc[bg_dsc_idx].header.h  = BG_H2;
  bg_dsc[bg_dsc_idx].header.cf = LV_COLOR_FORMAT_RGB565;
  bg_dsc[bg_dsc_idx].data      = (const uint8_t*)px;
  bg_dsc[bg_dsc_idx].data_size = BG_W2 * BG_H2 * 2;

  lv_img_set_src(bg_img, &bg_dsc[bg_dsc_idx]);
  lv_obj_invalidate(bg_img);
  
  #if defined(LOG_MEM_ON_IMAGE_SWITCH) && (LOG_MEM_ON_IMAGE_SWITCH == 1)
    Serial.printf("Free / Min Free / Max Alloc Heap: %u %u %u\n", ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
  #endif

  return true;

}

static void screen_gesture_cb(lv_event_t * e)
{

  lv_indev_t * indev = lv_indev_get_act();
  if(!indev) return;

  lv_dir_t dir = lv_indev_get_gesture_dir(indev);

  if(dir == LV_DIR_BOTTOM) {
    create_settings_window();
    gesture_in_progress = true; 
  } else if(dir == LV_DIR_LEFT) {
    gesture_in_progress = true; 
    if(bg_count <= 0) return;

    bg_idx = (bg_idx + 1) % bg_count;
    set_bg_from_file(bg_list[bg_idx].c_str());
   
  } else if(dir == LV_DIR_RIGHT) {
    gesture_in_progress = true; 
    if(bg_count <= 0) return;

    bg_idx = (bg_idx - 1 + bg_count) % bg_count;
    set_bg_from_file(bg_list[bg_idx].c_str());
  }
}

static void bg_timer_cb(lv_timer_t *t)
{
  (void)t;
  if(bg_count <= 0) return;

  bg_idx = (bg_idx + 1) % bg_count;
  set_bg_from_file(bg_list[bg_idx].c_str());
}


static void changeDialog(int index)
{
  const LocalizedStrings* strings = get_strings(current_language);

  switch (index)
  {
  case 0:
    lv_obj_remove_flag(box_daily, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(box_hourly, LV_OBJ_FLAG_HIDDEN); 
    lv_label_set_text(lbl_forecast, strings->seven_day_forecast);
    break;
  case 1: 
    lv_obj_add_flag(box_daily, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(box_hourly, LV_OBJ_FLAG_HIDDEN); 
    lv_label_set_text(lbl_forecast, strings->hourly_forecast);
    break;
  default:
    lv_obj_add_flag(box_daily, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(box_hourly, LV_OBJ_FLAG_HIDDEN); 
    lv_label_set_text(lbl_forecast, "");
    break;
  }

}

static void overlay_touch_cb(lv_event_t * e) {

  if(gesture_in_progress) {
        gesture_in_progress = false;
        return;                       
    }
  
  dialogIndex = (dialogIndex + 1) % (bg_count > 0 ? 3 : 2);
  changeDialog(dialogIndex);
}


void add_fullscreen_overlay() {
  lv_obj_t *ov = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(ov);

  lv_obj_set_size(ov, tft.width(), tft.height());
  lv_obj_set_pos(ov, 0, 0);

  lv_obj_clear_flag(ov, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_style_bg_opa(ov, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(ov, LV_OPA_TRANSP, 0);

  lv_obj_add_flag(ov, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_add_event_cb(ov, overlay_touch_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ov, screen_gesture_cb, LV_EVENT_GESTURE, NULL);
  lv_obj_move_foreground(ov);
}

void set_std_background(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x4c8cb9), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0xa6cdec), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
}

void create_ui() {
  lv_obj_t *scr = lv_scr_act();
 
  lv_obj_add_event_cb(scr, screen_gesture_cb, LV_EVENT_GESTURE, NULL);
     
  if (bg_count == 0) {
    set_std_background(scr);
  } else {    
    bg_img = lv_img_create(scr);
    if (set_bg_from_file(bg_list[0].c_str(), true)) {
      lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
      lv_img_set_zoom(bg_img, 512);
      lv_obj_center(bg_img);
      lv_obj_move_background(bg_img);
      lv_obj_add_flag(bg_img, LV_OBJ_FLAG_GESTURE_BUBBLE);
      if(!bg_timer) bg_timer = lv_timer_create(bg_timer_cb, 30000, NULL);
    } else {
      set_std_background(scr);
      bg_count = 0; 
    }
  }

  img_today_icon = lv_img_create(scr);
  lv_img_set_src(img_today_icon, &image_partly_cloudy);
  lv_obj_align(img_today_icon, LV_ALIGN_TOP_MID, -64, 4);

  static lv_style_t default_label_style;
  lv_style_init(&default_label_style);
  lv_style_set_text_color(&default_label_style, lv_color_hex(0xFFFFFF));
  lv_style_set_text_opa(&default_label_style, LV_OPA_COVER);

  const LocalizedStrings* strings = get_strings(current_language);

  // Today temp
  lbl_today_temp = lv_label_create(scr);
  lv_label_set_text(lbl_today_temp, strings->temp_placeholder);
  lv_obj_set_style_text_font(lbl_today_temp, get_font_42(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align(lbl_today_temp, LV_ALIGN_TOP_MID, 45, 25);
  lv_obj_add_style(lbl_today_temp, &default_label_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

  lbl_today_feels_like = lv_label_create(scr);
  lv_label_set_text(lbl_today_feels_like, strings->feels_like_temp);
  lv_obj_set_style_text_font(lbl_today_feels_like, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_today_feels_like, lv_color_hex(0xe4ffff), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align(lbl_today_feels_like, LV_ALIGN_TOP_MID, 45, 75);

  lbl_forecast = lv_label_create(scr);
  lv_label_set_text(lbl_forecast, strings->seven_day_forecast);
  lv_obj_set_style_text_font(lbl_forecast, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_forecast, lv_color_hex(0xe4ffff), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align(lbl_forecast, LV_ALIGN_TOP_LEFT, 20, 110);

  box_daily = lv_obj_create(scr);
  lv_obj_set_size(box_daily, 220, 180);
  lv_obj_align(box_daily, LV_ALIGN_TOP_LEFT, 10, 135);
  lv_obj_set_style_bg_color(box_daily, lv_color_hex(0x5e9bc8), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(box_daily, LV_OPA_30, LV_PART_MAIN);
  lv_obj_set_style_radius(box_daily, 4, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box_daily, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_clear_flag(box_daily, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(box_daily, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_all(box_daily, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(box_daily, 0, LV_PART_MAIN);

  for (int i = 0; i < 7; i++) {
    lbl_daily_day[i] = lv_label_create(box_daily);
    lbl_daily_high[i] = lv_label_create(box_daily);
    lbl_daily_low[i] = lv_label_create(box_daily);
    img_daily[i] = lv_img_create(box_daily);

    lv_obj_add_style(lbl_daily_day[i], &default_label_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_daily_day[i], get_font_16(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_align(lbl_daily_day[i], LV_ALIGN_TOP_LEFT, 2, i * 24);

    lv_obj_add_style(lbl_daily_high[i], &default_label_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_daily_high[i], get_font_16(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_align(lbl_daily_high[i], LV_ALIGN_TOP_RIGHT, 0, i * 24);

    lv_label_set_text(lbl_daily_low[i], "");
    lv_obj_set_style_text_color(lbl_daily_low[i], lv_color_hex(0xb9ecff), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_daily_low[i], get_font_16(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_align(lbl_daily_low[i], LV_ALIGN_TOP_RIGHT, -50, i * 24);

    lv_img_set_src(img_daily[i], &icon_partly_cloudy);
    lv_obj_align(img_daily[i], LV_ALIGN_TOP_LEFT, 72, i * 24);
  }

  box_hourly = lv_obj_create(scr);
  lv_obj_set_size(box_hourly, 220, 180);
  lv_obj_align(box_hourly, LV_ALIGN_TOP_LEFT, 10, 135);
  lv_obj_set_style_bg_color(box_hourly, lv_color_hex(0x5e9bc8), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(box_hourly, LV_OPA_30, LV_PART_MAIN);
  lv_obj_set_style_radius(box_hourly, 4, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(box_hourly, 0, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_clear_flag(box_hourly, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(box_hourly, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_all(box_hourly, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(box_hourly, 0, LV_PART_MAIN);

  for (int i = 0; i < 7; i++) {
    lbl_hourly[i] = lv_label_create(box_hourly);
    lbl_precipitation_probability[i] = lv_label_create(box_hourly);
    lbl_hourly_temp[i] = lv_label_create(box_hourly);
    img_hourly[i] = lv_img_create(box_hourly);

    lv_obj_add_style(lbl_hourly[i], &default_label_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_hourly[i], get_font_16(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_align(lbl_hourly[i], LV_ALIGN_TOP_LEFT, 2, i * 24);

    lv_obj_add_style(lbl_hourly_temp[i], &default_label_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_hourly_temp[i], get_font_16(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_align(lbl_hourly_temp[i], LV_ALIGN_TOP_RIGHT, 0, i * 24);

    lv_label_set_text(lbl_precipitation_probability[i], "");
    lv_obj_set_style_text_color(lbl_precipitation_probability[i], lv_color_hex(0xb9ecff), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl_precipitation_probability[i], get_font_16(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
    lv_obj_align(lbl_precipitation_probability[i], LV_ALIGN_TOP_RIGHT, -55, i * 24);

    lv_img_set_src(img_hourly[i], &icon_partly_cloudy);
    lv_obj_align(img_hourly[i], LV_ALIGN_TOP_LEFT, 72, i * 24);
  }

  lbl_clock = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_clock, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0xb9ecff), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_label_set_text(lbl_clock, "");
  lv_obj_align(lbl_clock, LV_ALIGN_TOP_RIGHT, -10, 2);

  changeDialog(dialogIndex);

}

static void location_save_event_cb(lv_event_t *e) {
  JsonArray *pres = static_cast<JsonArray *>(lv_event_get_user_data(e));
  uint16_t idx = lv_dropdown_get_selected(results_dd);

  JsonObject obj = (*pres)[idx];
  double lat = obj["latitude"].as<double>();
  double lon = obj["longitude"].as<double>();

  snprintf(latitude, sizeof(latitude), "%.6f", lat);
  snprintf(longitude, sizeof(longitude), "%.6f", lon);
  prefs.putString("latitude", latitude);
  prefs.putString("longitude", longitude);

  String opts;
  const char *name = obj["name"];
  const char *admin = obj["admin1"];
  const char *country = obj["country_code"];
  opts += name;
  if (admin) {
    opts += ", ";
    opts += admin;
  }

  prefs.putString("location", opts);
  location = prefs.getString("location");

  // Re‐fetch weather immediately
  lv_label_set_text(lbl_loc, opts.c_str());
  fetch_and_update_weather();

  lv_obj_del(location_win);
  location_win = nullptr;
}

static void location_cancel_event_cb(lv_event_t *e) {
  lv_obj_del(location_win);
  location_win = nullptr;
}

static void reset_wifi_event_handler(lv_event_t *e) {
  const LocalizedStrings* strings = get_strings(current_language);
  lv_obj_t *mbox = lv_msgbox_create(lv_scr_act());
  lv_obj_t *title = lv_msgbox_add_title(mbox, strings->reset);
  lv_obj_set_style_margin_left(title, 10, 0);
  lv_obj_set_style_text_font(title, get_font_16(), 0);

  lv_obj_t *text = lv_msgbox_add_text(mbox, strings->reset_confirmation);
  lv_obj_set_style_text_font(text, get_font_12(), 0);
  lv_msgbox_add_close_button(mbox);

  lv_obj_t *btn_no = lv_msgbox_add_footer_button(mbox, strings->cancel);
  lv_obj_set_style_text_font(btn_no, get_font_12(), 0);
  lv_obj_t *btn_yes = lv_msgbox_add_footer_button(mbox, strings->reset);
  lv_obj_set_style_text_font(btn_yes, get_font_12(), 0);

  lv_obj_set_style_bg_color(btn_yes, lv_palette_main(LV_PALETTE_RED), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_yes, lv_palette_darken(LV_PALETTE_RED, 1), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
  lv_obj_set_style_text_color(btn_yes, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

  lv_obj_set_width(mbox, 230);
  lv_obj_center(mbox);

  lv_obj_set_style_border_width(mbox, 2, LV_PART_MAIN);
  lv_obj_set_style_border_color(mbox, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_border_opa(mbox, LV_OPA_COVER,   LV_PART_MAIN);
  lv_obj_set_style_radius(mbox, 4, LV_PART_MAIN);

  lv_obj_add_event_cb(btn_yes, reset_confirm_yes_cb, LV_EVENT_CLICKED, mbox);
  lv_obj_add_event_cb(btn_no, reset_confirm_no_cb, LV_EVENT_CLICKED, mbox);
}



void create_location_dialog() {
  const LocalizedStrings* strings = get_strings(current_language);
  location_win = lv_win_create(lv_scr_act());
  lv_obj_t *title = lv_win_add_title(location_win, strings->change_location);
  lv_obj_t *header = lv_win_get_header(location_win);
  lv_obj_set_style_height(header, 30, 0);
  lv_obj_set_style_text_font(title, get_font_16(), 0);
  lv_obj_set_style_margin_left(title, 10, 0);
  lv_obj_set_size(location_win, 240, 320);
  lv_obj_center(location_win);

  lv_obj_t *cont = lv_win_get_content(location_win);

  lv_obj_t *lbl = lv_label_create(cont);
  lv_label_set_text(lbl, strings->city);
  lv_obj_set_style_text_font(lbl, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 5, 10);

  loc_ta = lv_textarea_create(cont);
  lv_textarea_set_one_line(loc_ta, true);
  lv_textarea_set_placeholder_text(loc_ta, strings->city_placeholder);
  lv_obj_set_width(loc_ta, 170);
  lv_obj_align_to(loc_ta, lbl, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  lv_obj_add_event_cb(loc_ta, ta_event_cb, LV_EVENT_CLICKED, kb);
  lv_obj_add_event_cb(loc_ta, ta_defocus_cb, LV_EVENT_DEFOCUSED, kb);

  lv_obj_t *lbl2 = lv_label_create(cont);
  lv_label_set_text(lbl2, strings->search_results);
  lv_obj_set_style_text_font(lbl2, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align(lbl2, LV_ALIGN_TOP_LEFT, 5, 50);

  results_dd = lv_dropdown_create(cont);
  lv_obj_set_width(results_dd, 200);
  lv_obj_align(results_dd, LV_ALIGN_TOP_LEFT, 5, 70);
  lv_obj_set_style_text_font(results_dd, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(results_dd, get_font_14(), (lv_style_selector_t)LV_PART_SELECTED | (lv_style_selector_t)LV_STATE_DEFAULT);

  lv_obj_t *list = lv_dropdown_get_list(results_dd);
  lv_obj_set_style_text_font(list, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

  lv_dropdown_set_options(results_dd, "");
  lv_obj_clear_flag(results_dd, LV_OBJ_FLAG_CLICKABLE);

  btn_close_loc = lv_btn_create(cont);
  lv_obj_set_size(btn_close_loc, 80, 40);
  lv_obj_align(btn_close_loc, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lv_obj_add_event_cb(btn_close_loc, location_save_event_cb, LV_EVENT_CLICKED, &geoResults);
  lv_obj_set_style_bg_color(btn_close_loc, lv_palette_main(LV_PALETTE_GREY), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn_close_loc, LV_OPA_COVER, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_close_loc, lv_palette_darken(LV_PALETTE_GREY, 1), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
  lv_obj_clear_flag(btn_close_loc, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *lbl_close = lv_label_create(btn_close_loc);
  lv_label_set_text(lbl_close, strings->save);
  lv_obj_set_style_text_font(lbl_close, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_center(lbl_close);

  lv_obj_t *btn_cancel_loc = lv_btn_create(cont);
  lv_obj_set_size(btn_cancel_loc, 80, 40);
  lv_obj_align_to(btn_cancel_loc, btn_close_loc, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  lv_obj_add_event_cb(btn_cancel_loc, location_cancel_event_cb, LV_EVENT_CLICKED, &geoResults);

  lv_obj_t *lbl_cancel = lv_label_create(btn_cancel_loc);
  lv_label_set_text(lbl_cancel, strings->cancel);
  lv_obj_set_style_text_font(lbl_cancel, get_font_14(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_center(lbl_cancel);
}

static void change_location_event_cb(lv_event_t *e) {
  if (location_win) return;

  create_location_dialog();
}


void create_settings_window() {
  if (settings_win) return;

  int vertical_element_spacing = 21;

  const LocalizedStrings* strings = get_strings(current_language);
  settings_win = lv_win_create(lv_scr_act());

  lv_obj_t *header = lv_win_get_header(settings_win);
  lv_obj_set_style_height(header, 30, 0);

  lv_obj_t *title = lv_win_add_title(settings_win, strings->aura_settings);
  lv_obj_set_style_text_font(title, get_font_16(), 0);
  lv_obj_set_style_margin_left(title, 10, 0);

  lv_obj_center(settings_win);
  lv_obj_set_width(settings_win, 240);

  lv_obj_t *cont = lv_win_get_content(settings_win);

  // Brightness
  lv_obj_t *lbl_b = lv_label_create(cont);
  lv_label_set_text(lbl_b, strings->brightness);
  lv_obj_set_style_text_font(lbl_b, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align(lbl_b, LV_ALIGN_TOP_LEFT, 0, 5);
  lv_obj_t *slider = lv_slider_create(cont);
  lv_slider_set_range(slider, 1, 255);
  uint32_t saved_b = prefs.getUInt("brightness", 128);
  lv_slider_set_value(slider, saved_b, LV_ANIM_OFF);
  lv_obj_set_width(slider, 100);
  lv_obj_align_to(slider, lbl_b, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

  lv_obj_add_event_cb(slider, [](lv_event_t *e){
    lv_obj_t *s = (lv_obj_t*)lv_event_get_target(e);
    uint32_t v = lv_slider_get_value(s);
    analogWrite(LCD_BACKLIGHT_PIN, v);
    prefs.putUInt("brightness", v);
  }, LV_EVENT_VALUE_CHANGED, NULL);

  // 'Night mode' switch
  lv_obj_t *lbl_night_mode = lv_label_create(cont);
  lv_label_set_text(lbl_night_mode, strings->use_night_mode);
  lv_obj_set_style_text_font(lbl_night_mode, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align_to(lbl_night_mode, lbl_b, LV_ALIGN_OUT_BOTTOM_LEFT, 0, vertical_element_spacing);

  night_mode_switch = lv_switch_create(cont);
  lv_obj_align_to(night_mode_switch, lbl_night_mode, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  if (use_night_mode) {
    lv_obj_add_state(night_mode_switch, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(night_mode_switch, LV_STATE_CHECKED);
  }
  lv_obj_add_event_cb(night_mode_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // 'Use F' switch
  lv_obj_t *lbl_u = lv_label_create(cont);
  lv_label_set_text(lbl_u, strings->use_fahrenheit);
  lv_obj_set_style_text_font(lbl_u, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align_to(lbl_u, lbl_night_mode, LV_ALIGN_OUT_BOTTOM_LEFT, 0, vertical_element_spacing);

  unit_switch = lv_switch_create(cont);
  lv_obj_align_to(unit_switch, lbl_u, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  if (use_fahrenheit) {
    lv_obj_add_state(unit_switch, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(unit_switch, LV_STATE_CHECKED);
  }
  lv_obj_add_event_cb(unit_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // 24-hr time switch
  lv_obj_t *lbl_24hr = lv_label_create(cont);
  lv_label_set_text(lbl_24hr, strings->use_24hr);
  lv_obj_set_style_text_font(lbl_24hr, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align_to(lbl_24hr, unit_switch, LV_ALIGN_OUT_RIGHT_MID, 6, 0);

  clock_24hr_switch = lv_switch_create(cont);
  lv_obj_align_to(clock_24hr_switch, lbl_24hr, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
  if (use_24_hour) {
    lv_obj_add_state(clock_24hr_switch, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(clock_24hr_switch, LV_STATE_CHECKED);
  }
  lv_obj_add_event_cb(clock_24hr_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // Current Location label
  lv_obj_t *lbl_loc_l = lv_label_create(cont);
  lv_label_set_text(lbl_loc_l, strings->location);
  lv_obj_set_style_text_font(lbl_loc_l, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align_to(lbl_loc_l, lbl_u, LV_ALIGN_OUT_BOTTOM_LEFT, 0, vertical_element_spacing);

  lbl_loc = lv_label_create(cont);
  lv_label_set_text(lbl_loc, location.c_str());
  lv_obj_set_style_text_font(lbl_loc, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align_to(lbl_loc, lbl_loc_l, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Language selection
  lv_obj_t *lbl_lang = lv_label_create(cont);
  lv_label_set_text(lbl_lang, strings->language_label);
  lv_obj_set_style_text_font(lbl_lang, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align_to(lbl_lang, lbl_loc_l, LV_ALIGN_OUT_BOTTOM_LEFT, 0, vertical_element_spacing);

  language_dropdown = lv_dropdown_create(cont);
  lv_dropdown_set_options(language_dropdown, "English\nEspañol\nDeutsch\nFrançais\nTürkçe\nSvenska\nItaliano");
  lv_dropdown_set_selected(language_dropdown, current_language);
  lv_obj_set_width(language_dropdown, 120);
  lv_obj_set_style_text_font(language_dropdown, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(language_dropdown, get_font_12(), (lv_style_selector_t)LV_PART_SELECTED | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_t *list = lv_dropdown_get_list(language_dropdown);
  lv_obj_set_style_text_font(list, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_align_to(language_dropdown, lbl_lang, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
  lv_obj_add_event_cb(language_dropdown, settings_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // Location search button
  lv_obj_t *btn_change_loc = lv_btn_create(cont);
  lv_obj_align_to(btn_change_loc, lbl_lang, LV_ALIGN_OUT_BOTTOM_LEFT, 0, vertical_element_spacing);

  lv_obj_set_size(btn_change_loc, 100, 40);
  lv_obj_add_event_cb(btn_change_loc, change_location_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl_chg = lv_label_create(btn_change_loc);
  lv_label_set_text(lbl_chg, strings->location_btn);
  lv_obj_set_style_text_font(lbl_chg, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_center(lbl_chg);

  // Hidden keyboard object
  if (!kb) {
    kb = lv_keyboard_create(lv_scr_act());
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_CANCEL, NULL);
  }

  // Reset WiFi button
  lv_obj_t *btn_reset = lv_btn_create(cont);
  lv_obj_set_style_bg_color(btn_reset, lv_palette_main(LV_PALETTE_RED), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(btn_reset, lv_palette_darken(LV_PALETTE_RED, 1), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
  lv_obj_set_style_text_color(btn_reset, lv_color_white(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_set_size(btn_reset, 100, 40);
  lv_obj_align_to(btn_reset, btn_change_loc, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

  lv_obj_add_event_cb(btn_reset, reset_wifi_event_handler, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *lbl_reset = lv_label_create(btn_reset);
  lv_label_set_text(lbl_reset, strings->reset_wifi);
  lv_obj_set_style_text_font(lbl_reset, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_center(lbl_reset);

  // Close Settings button
  btn_close_obj = lv_btn_create(cont);
  lv_obj_set_size(btn_close_obj, 80, 40);
  lv_obj_align(btn_close_obj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_event_cb(btn_close_obj, settings_event_handler, LV_EVENT_CLICKED, NULL);

  // Cancel button
  lv_obj_t *lbl_btn = lv_label_create(btn_close_obj);
  lv_label_set_text(lbl_btn, strings->close);
  lv_obj_set_style_text_font(lbl_btn, get_font_12(), (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
  lv_obj_center(lbl_btn);
}

static void settings_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *tgt = (lv_obj_t *)lv_event_get_target(e);

  if (tgt == unit_switch && code == LV_EVENT_VALUE_CHANGED) {
    use_fahrenheit = lv_obj_has_state(unit_switch, LV_STATE_CHECKED);
  }

  if (tgt == clock_24hr_switch && code == LV_EVENT_VALUE_CHANGED) {
    use_24_hour = lv_obj_has_state(clock_24hr_switch, LV_STATE_CHECKED);
  }

  if (tgt == night_mode_switch && code == LV_EVENT_VALUE_CHANGED) {
    use_night_mode = lv_obj_has_state(night_mode_switch, LV_STATE_CHECKED);
  }

  if (tgt == language_dropdown && code == LV_EVENT_VALUE_CHANGED) {
    current_language = (Language)lv_dropdown_get_selected(language_dropdown);
    // Update the UI immediately to reflect language change
    lv_obj_del(settings_win);
    settings_win = nullptr;
    
    // Save preferences and recreate UI with new language
    prefs.putBool("useFahrenheit", use_fahrenheit);
    prefs.putBool("use24Hour", use_24_hour);
    prefs.putBool("useNightMode", use_night_mode);
    prefs.putUInt("language", current_language);

    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    
    // Recreate the main UI with the new language
    lv_obj_clean(lv_scr_act());
    create_ui();
    fetch_and_update_weather();
    return;
  }

  if (tgt == btn_close_obj && code == LV_EVENT_CLICKED) {
    prefs.putBool("useFahrenheit", use_fahrenheit);
    prefs.putBool("use24Hour", use_24_hour);
    prefs.putBool("useNightMode", use_night_mode);
    prefs.putUInt("language", current_language);

    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    lv_obj_del(settings_win);
    settings_win = nullptr;

    fetch_and_update_weather();
  }
}

// Screen dimming functions implementation
bool night_mode_should_be_active() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return false;

  if (!use_night_mode) return false;
  
  int hour = timeinfo.tm_hour;
  return (hour >= NIGHT_MODE_START_HOUR || hour < NIGHT_MODE_END_HOUR);
}

void activate_night_mode() {
  analogWrite(LCD_BACKLIGHT_PIN, 0);
  night_mode_active = true;
}

void deactivate_night_mode() {
  analogWrite(LCD_BACKLIGHT_PIN, prefs.getUInt("brightness", 128));
  night_mode_active = false;
}

void check_for_night_mode() {
  bool night_mode_time = night_mode_should_be_active();

  if (night_mode_time && !night_mode_active && !temp_screen_wakeup_active) {
    activate_night_mode();
  } else if (!night_mode_time && night_mode_active) {
    deactivate_night_mode();
  }
}

void handle_temp_screen_wakeup_timeout(lv_timer_t *timer) {
  if (temp_screen_wakeup_active) {
    temp_screen_wakeup_active = false;

    if (night_mode_should_be_active()) {
      activate_night_mode();
    }
  }
  
  if (temp_screen_wakeup_timer) {
    lv_timer_del(temp_screen_wakeup_timer);
    temp_screen_wakeup_timer = nullptr;
  }
}

void fetch_and_update_weather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi no longer connected. Attempting to reconnect...");
    WiFi.disconnect();
    WiFiManager wm;  
    wm.autoConnect(DEFAULT_CAPTIVE_SSID);
    delay(1000);  
    if (WiFi.status() != WL_CONNECTED) { 
      Serial.println("WiFi connection still unavailable.");
      return;   
    }
    Serial.println("WiFi connection reestablished.");
  }


  String url = String("http://api.open-meteo.com/v1/forecast?latitude=")
               + latitude + "&longitude=" + longitude
               + "&current=temperature_2m,apparent_temperature,is_day,weather_code"
               + "&daily=temperature_2m_min,temperature_2m_max,weather_code"
               + "&hourly=temperature_2m,precipitation_probability,is_day,weather_code"
               + "&forecast_hours=7"
               + "&timezone=auto";

  HTTPClient http;
  http.begin(url);

  if (http.GET() == HTTP_CODE_OK) {
    Serial.println("Updated weather from open-meteo: " + url);

    String payload = http.getString();
    JsonDocument doc;

    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      float t_now = doc["current"]["temperature_2m"].as<float>();
      float t_ap = doc["current"]["apparent_temperature"].as<float>();
      int code_now = doc["current"]["weather_code"].as<int>();
      int is_day = doc["current"]["is_day"].as<int>();

      if (use_fahrenheit) {
        t_now = t_now * 9.0 / 5.0 + 32.0;
        t_ap = t_ap * 9.0 / 5.0 + 32.0;
      }
      const LocalizedStrings* strings = get_strings(current_language);

      int utc_offset_seconds = doc["utc_offset_seconds"].as<int>();
      configTime(utc_offset_seconds, 0, "pool.ntp.org", "time.nist.gov");
      Serial.print("Updating time from NTP with UTC offset: ");
      Serial.println(utc_offset_seconds);

      char unit = use_fahrenheit ? 'F' : 'C';
      lv_label_set_text_fmt(lbl_today_temp, "%.0f°%c", t_now, unit);
      lv_label_set_text_fmt(lbl_today_feels_like, "%s %.0f°%c", strings->feels_like_temp, t_ap, unit);
      lv_img_set_src(img_today_icon, choose_image(code_now, is_day));

      JsonArray times = doc["daily"]["time"].as<JsonArray>();
      JsonArray tmin = doc["daily"]["temperature_2m_min"].as<JsonArray>();
      JsonArray tmax = doc["daily"]["temperature_2m_max"].as<JsonArray>();
      JsonArray weather_codes = doc["daily"]["weather_code"].as<JsonArray>();

      for (int i = 0; i < 7; i++) {
        const char *date = times[i];
        int year = atoi(date + 0);
        int mon = atoi(date + 5);
        int dayd = atoi(date + 8);
        int dow = day_of_week(year, mon, dayd);
        const char *dayStr = (i == 0 && current_language != LANG_FR) ? strings->today : strings->weekdays[dow];

        float mn = tmin[i].as<float>();
        float mx = tmax[i].as<float>();
        if (use_fahrenheit) {
          mn = mn * 9.0 / 5.0 + 32.0;
          mx = mx * 9.0 / 5.0 + 32.0;
        }

        lv_label_set_text_fmt(lbl_daily_day[i], "%s", dayStr);
        lv_label_set_text_fmt(lbl_daily_high[i], "%.0f°%c", mx, unit);
        lv_label_set_text_fmt(lbl_daily_low[i], "%.0f°%c", mn, unit);
        lv_img_set_src(img_daily[i], choose_icon(weather_codes[i].as<int>(), (i == 0) ? is_day : 1));
      }

      JsonArray hours = doc["hourly"]["time"].as<JsonArray>();
      JsonArray hourly_temps = doc["hourly"]["temperature_2m"].as<JsonArray>();
      JsonArray precipitation_probabilities = doc["hourly"]["precipitation_probability"].as<JsonArray>();
      JsonArray hourly_weather_codes = doc["hourly"]["weather_code"].as<JsonArray>();
      JsonArray hourly_is_day = doc["hourly"]["is_day"].as<JsonArray>();

      for (int i = 0; i < 7; i++) {
        const char *date = hours[i];  // "YYYY-MM-DD"
        int hour = atoi(date + 11);
        int minute = atoi(date + 14);
        String hour_name = hour_of_day(hour);

        float precipitation_probability = precipitation_probabilities[i].as<float>();
        float temp = hourly_temps[i].as<float>();
        if (use_fahrenheit) {
          temp = temp * 9.0 / 5.0 + 32.0;
        }

        if (i == 0 && current_language != LANG_FR) {
          lv_label_set_text(lbl_hourly[i], strings->now);
        } else {
          lv_label_set_text(lbl_hourly[i], hour_name.c_str());
        }
        lv_label_set_text_fmt(lbl_precipitation_probability[i], "%.0f%%", precipitation_probability);
        lv_label_set_text_fmt(lbl_hourly_temp[i], "%.0f°%c", temp, unit);
        lv_img_set_src(img_hourly[i], choose_icon(hourly_weather_codes[i].as<int>(), hourly_is_day[i].as<int>()));
      }


    } else {
      Serial.println("JSON parse failed on result from " + url);
    }
  } else {
    Serial.println("HTTP GET failed at " + url);
  }
  http.end();
}

const lv_img_dsc_t* choose_image(int code, int is_day) {
  switch (code) {
    // Clear sky
    case  0:
      return is_day
        ? &image_sunny
        : &image_clear_night;

    // Mainly clear
    case  1:
      return is_day
        ? &image_mostly_sunny
        : &image_mostly_clear_night;

    // Partly cloudy
    case  2:
      return is_day
        ? &image_partly_cloudy
        : &image_partly_cloudy_night;

    // Overcast
    case  3:
      return &image_cloudy;

    // Fog / mist
    case 45:
    case 48:
      return &image_haze_fog_dust_smoke;

    // Drizzle (light → dense)
    case 51:
    case 53:
    case 55:
      return &image_drizzle;

    // Freezing drizzle
    case 56:
    case 57:
      return &image_sleet_hail;

    // Rain: slight showers
    case 61:
      return is_day
        ? &image_scattered_showers_day
        : &image_scattered_showers_night;

    // Rain: moderate
    case 63:
      return &image_showers_rain;

    // Rain: heavy
    case 65:
      return &image_heavy_rain;

    // Freezing rain
    case 66:
    case 67:
      return &image_wintry_mix_rain_snow;

    // Snow fall (light, moderate, heavy) & snow showers (light)
    case 71:
    case 73:
    case 75:
    case 85:
      return &image_snow_showers_snow;

    // Snow grains
    case 77:
      return &image_flurries;

    // Rain showers (slight → moderate)
    case 80:
    case 81:
      return is_day
        ? &image_scattered_showers_day
        : &image_scattered_showers_night;

    // Rain showers: violent
    case 82:
      return &image_heavy_rain;

    // Heavy snow showers
    case 86:
      return &image_heavy_snow;

    // Thunderstorm (light)
    case 95:
      return is_day
        ? &image_isolated_scattered_tstorms_day
        : &image_isolated_scattered_tstorms_night;

    // Thunderstorm with hail
    case 96:
    case 99:
      return &image_strong_tstorms;

    // Fallback for any other code
    default:
      return is_day
        ? &image_mostly_cloudy_day
        : &image_mostly_cloudy_night;
  }
}

const lv_img_dsc_t* choose_icon(int code, int is_day) {
  switch (code) {
    // Clear sky
    case  0:
      return is_day
        ? &icon_sunny
        : &icon_clear_night;

    // Mainly clear
    case  1:
      return is_day
        ? &icon_mostly_sunny
        : &icon_mostly_clear_night;

    // Partly cloudy
    case  2:
      return is_day
        ? &icon_partly_cloudy
        : &icon_partly_cloudy_night;

    // Overcast
    case  3:
      return &icon_cloudy;

    // Fog / mist
    case 45:
    case 48:
      return &icon_haze_fog_dust_smoke;

    // Drizzle (light → dense)
    case 51:
    case 53:
    case 55:
      return &icon_drizzle;

    // Freezing drizzle
    case 56:
    case 57:
      return &icon_sleet_hail;

    // Rain: slight showers
    case 61:
      return is_day
        ? &icon_scattered_showers_day
        : &icon_scattered_showers_night;

    // Rain: moderate
    case 63:
      return &icon_showers_rain;

    // Rain: heavy
    case 65:
      return &icon_heavy_rain;

    // Freezing rain
    case 66:
    case 67:
      return &icon_wintry_mix_rain_snow;

    // Snow fall (light, moderate, heavy) & snow showers (light)
    case 71:
    case 73:
    case 75:
    case 85:
      return &icon_snow_showers_snow;

    // Snow grains
    case 77:
      return &icon_flurries;

    // Rain showers (slight → moderate)
    case 80:
    case 81:
      return is_day
        ? &icon_scattered_showers_day
        : &icon_scattered_showers_night;

    // Rain showers: violent
    case 82:
      return &icon_heavy_rain;

    // Heavy snow showers
    case 86:
      return &icon_heavy_snow;

    // Thunderstorm (light)
    case 95:
      return is_day
        ? &icon_isolated_scattered_tstorms_day
        : &icon_isolated_scattered_tstorms_night;

    // Thunderstorm with hail
    case 96:
    case 99:
      return &icon_strong_tstorms;

    // Fallback for any other code
    default:
      return is_day
        ? &icon_mostly_cloudy_day
        : &icon_mostly_cloudy_night;
  }
}

// --------------------------------------------------------------------------------------------

void setup() {
  lvgl_display_enabled = false;
  Serial.begin(115200);
  delay(100);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  tft.init();
  tft.setColorDepth(16);
  tft.initDMA();
#ifdef CYD_PANEL_ST7789
  tft.setRotation(0);       
#else
  tft.setRotation(2);       
#endif  
  tft.fillScreen(TFT_BLACK);
  digitalWrite(CYD_BL_PIN, HIGH);

  ensureTouchCalibrated(false); 
  create_timer();

  drawCenterString ("A U R A", 150, TFT_WHITE);
  
  bool ok = initSdSpi();
  Serial.printf("SD init ok? %s\n", ok ? "YES" : "NO");

  lv_init();

  int W = tft.width();
  int H = tft.height();

  disp = lv_display_create(W, H);
  lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, lvgl_flush_cb);

  lv_register_sd_fs(SD);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  lv_timer_create(update_clock, 1000, NULL);
  lv_obj_clean(lv_scr_act());
 
  // Load saved prefs
  prefs.begin("weather", false);
  String lat = prefs.getString("latitude", LATITUDE_DEFAULT);
  lat.toCharArray(latitude, sizeof(latitude));
  String lon = prefs.getString("longitude", LONGITUDE_DEFAULT);
  lon.toCharArray(longitude, sizeof(longitude));
  use_fahrenheit = prefs.getBool("useFahrenheit", false);
  location = prefs.getString("location", LOCATION_DEFAULT);
  use_night_mode = prefs.getBool("useNightMode", false);
  uint32_t brightness = prefs.getUInt("brightness", 255);
  use_24_hour = prefs.getBool("use24Hour", false);
  current_language = (Language)prefs.getUInt("language", LANG_EN);
  analogWrite(LCD_BACKLIGHT_PIN, brightness);

  // Check for Wi-Fi config and request it if not available
  WiFiManager wm;
  wm.setAPCallback(apModeCallback);
  wm.autoConnect(DEFAULT_CAPTIVE_SSID);

  if (startedConfigPortal && WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi configured -> rebooting...");
    delay(300);
    esp_restart();
  }

  scan_bg_dir("/backgrounds"); 
 
  create_ui();
  add_fullscreen_overlay();
  fetch_and_update_weather();
  removeTimer();
  lvgl_display_enabled = true;

}

// --------------------------------------------------------------------------------------------

void loop() {

  static uint32_t lastWeatherUpdate = millis();
  auto const now = millis();

  if (millis() - lastWeatherUpdate >= UPDATE_INTERVAL) {
    fetch_and_update_weather();
    lastWeatherUpdate = millis();
  }

  lv_tick_inc(now - lv_last_tick);
  lv_last_tick = now;
  lv_timer_handler();
}
