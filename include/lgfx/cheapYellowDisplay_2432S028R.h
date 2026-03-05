#pragma once
#include <LovyanGFX.hpp>

/*
  Cheap Yellow Display (ESP32-2432S028R / CYD 2.8" 240x320 + XPT2046)
  Funktioniert mit board=esp32dev, weil alle Pins hier lokal definiert sind.

  Default: ILI9341 (häufig bei 1x microUSB)
  Alternative: ST7789 (häufig bei 2x USB) -> per -D CYD_PANEL_ST7789 aktivieren
  Backlight: per -D CYD_BL_PIN=21 (default 21), manche Boards brauchen 27
*/

// ---------- Defaults, per build_flags überschreibbar ----------
#ifndef CYD_PANEL_ST7789
  // Default ILI9341
#endif

#ifndef CYD_BL_PIN
  #define CYD_BL_PIN 21
#endif

// LCD Pins (CYD Standard)
#ifndef CYD_TFT_SCK
  #define CYD_TFT_SCK   14
#endif
#ifndef CYD_TFT_MOSI
  #define CYD_TFT_MOSI  13
#endif
#ifndef CYD_TFT_MISO
  #define CYD_TFT_MISO  12
#endif
#ifndef CYD_TFT_DC
  #define CYD_TFT_DC     2
#endif
#ifndef CYD_TFT_CS
  #define CYD_TFT_CS    15
#endif

// Touch Pins (XPT2046, CYD Standard)
#ifndef CYD_TP_CLK
  #define CYD_TP_CLK    25
#endif
#ifndef CYD_TP_MOSI
  #define CYD_TP_MOSI   32
#endif
#ifndef CYD_TP_MISO
  #define CYD_TP_MISO   39
#endif
#ifndef CYD_TP_CS
  #define CYD_TP_CS     33
#endif
#ifndef CYD_TP_IRQ
  #define CYD_TP_IRQ    36
#endif

class LGFX : public lgfx::LGFX_Device
{
#ifdef CYD_PANEL_ST7789
  lgfx::Panel_ST7789  _panel_instance;
#else
  lgfx::Panel_ILI9341 _panel_instance;
#endif

  lgfx::Bus_SPI        _bus_instance;
  lgfx::Touch_XPT2046  _touch_instance;


public:
  LGFX(void)
  {
    // ---------- BUS (LCD) ----------
    {
      auto cfg = _bus_instance.config();

      cfg.spi_mode   = 0;
      cfg.spi_host   = SPI2_HOST;   
      cfg.freq_read  = 16000000;

      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      cfg.pin_sclk = CYD_TFT_SCK;
      cfg.pin_mosi = CYD_TFT_MOSI;
      cfg.pin_dc   = CYD_TFT_DC;

#ifdef CYD_PANEL_ST7789
      cfg.freq_write = 80000000;   
      cfg.pin_miso   = -1;         
#else
      cfg.freq_write = 40000000;
      cfg.pin_miso   = CYD_TFT_MISO;
#endif

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();

      cfg.pin_cs   = CYD_TFT_CS;
      cfg.pin_rst  = -1;   
      cfg.pin_busy = -1;

      cfg.memory_width  = 240;
      cfg.memory_height = 320;
      cfg.panel_width   = 240;
      cfg.panel_height  = 320;

      cfg.offset_x = 0;
      cfg.offset_y = 0;

#ifdef CYD_PANEL_ST7789
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 16;
      cfg.readable         = false;
      cfg.rgb_order        = false; 
      cfg.invert           = false; 
#else
      cfg.offset_rotation  = 0;  
      cfg.dummy_read_pixel = 8;
      cfg.readable         = false;
      cfg.rgb_order        = false;
      cfg.invert           = false;
#endif
      cfg.dummy_read_bits = 1;

      cfg.dlen_16bit = false;
      cfg.bus_shared = true;

      _panel_instance.config(cfg);
    }

    {
      this->fillScreen(TFT_BLACK);
      pinMode(CYD_BL_PIN, OUTPUT);
      digitalWrite(CYD_BL_PIN, LOW);
    }

    // ---------- TOUCH ----------
    { // Touch XPT2046
    auto cfg = _touch_instance.config();

      cfg.x_min =  240;
      cfg.x_max = 3800;
      cfg.y_min = 3700;
      cfg.y_max =  200;


#ifdef CYD_PANEL_ST7789
      cfg.offset_rotation = 2;
#else
      cfg.offset_rotation = 0;
#endif

      cfg.pin_int  = 36;        
      cfg.bus_shared = true;  

      cfg.spi_host = VSPI_HOST; 
      cfg.freq     = 2000000;   
      cfg.pin_sclk = CYD_TP_CLK;
      cfg.pin_mosi = CYD_TP_MOSI;
      cfg.pin_cs   = CYD_TP_CS;
      cfg.pin_miso = CYD_TP_MISO;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);    
  }
  setPanel(&_panel_instance);
  }
};
