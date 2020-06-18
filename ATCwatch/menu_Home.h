
#pragma once
#include "Arduino.h"
#include "classScreen.h"
#include "images.h"
#include "menu.h"
#include "display.h"
#include "menuAppsBase.h"
#include "ble.h"
#include "time.h"
#include "battery.h"
#include "accl.h"
#include "push.h"
#include "heartrate.h"


class HomeScreen : public TheScreen
{
  public:
    HomeScreen() {
    }

    virtual void pre()
    {
      displayRect(0, 0, 240, 240, 0x0000);
      charge_symbol_change = false;
      if (!is_night()) {
        displayImage(0, 136, 24, 24, symbolHeartSmall);
        displayImage(0, 136 + 24 + 2, 24, 24, symbolStepsSmall);
        displayImage(0, 136 + 24 + 2 + 24 + 2, 24, 24, symbolMsgSmall);
      }
    }

    virtual void main()
    {
      uint16_t textcolor = 0xFFFF;
      uint16_t bgcolor = 0x0000;
      uint16_t bgbattery = 0xFFFF;
      
      time_data_struct time_data = get_time();
      accl_data_struct accl_data = get_accl_data();
      
      if (time_data.hr >= 23 || time_data.hr < 7) {
        textcolor = 0xF800;
      }

      char time_string[14];
      sprintf(time_string, "%02i:%02i:%02i", time_data.hr, time_data.min, time_data.sec);
      char date_string[14];
      sprintf(date_string, "%02i.%02i.%04i", time_data.day, time_data.month, time_data.year);
      displayPrintln(0, 56, time_string, textcolor, bgcolor, 5);
      displayPrintln(28, 104, date_string, textcolor, bgcolor, 3);

      displayPrintln(30, 140, (String)get_last_heartrate() + "     ", textcolor, bgcolor, 2);
      displayPrintln(30, 140 + 24 + 2, (String)accl_data.steps + "     ", textcolor, bgcolor, 2);
      displayPrintln(30, 140 + 24 + 2 + 24 + 2, get_push_msg(17), textcolor, bgcolor, 2);

      if (!is_night()) {
        if (get_vars_ble_connected())
          displayImage(176, 0, 24, 24, symbolBle1);
        else
          displayImage(176, 0, 24, 24, symbolBle2);
      }


      if (get_charge()) {
        int batteryPer = get_battery_percent();
        String batteryDisplay = "";

        if (batteryPer > 40) bgbattery = 0x07E0; //green
        else if (batteryPer > 30) bgbattery = 0xFFE0; //yellow
        else if (batteryPer > 20) bgbattery = 0xFC00; //orange
        else bgbattery = 0xF800; //red
        if (!charge_symbol_change) {
          //displayImage(216, 0, 24, 24, symbolBattery2);
          //displayRect(216, 0, 24, 24, bgbattery);
        }
        charge_symbol_change = true;
        if (batteryPer < 10) {
          batteryDisplay = " " + (String)batteryPer;
        } else {
          batteryDisplay = (String)batteryPer;
        }
        batteryDisplay = (String)batteryPer + "%";
        //displayPrintln(200, 0, batteryDisplay, 0x0000, bgbattery, 2); //black text, colorful bg
        displayPrintln(204, 0, batteryDisplay, bgbattery, 0x0000, 2); //colorful text, black bg
      } else {
        displayImage(216, 0, 24, 24, symbolBattery1);
        charge_symbol_change = false;
      }
    }

    virtual void up()
    {
      inc_vars_menu();
    }

    virtual void down()
    {
      dec_vars_menu();
    }

    virtual void left()
    {
    }
    virtual void right()
    {
    }
  private:
    bool charge_symbol_change = false;

};
