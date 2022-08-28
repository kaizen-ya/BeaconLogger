/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <M5Core2.h>
#include "types.h"

int scanTime = 5; // In seconds
int sampleCnt = 3;  // sampling time = scanTime * sampleCnt
int cnt = 0;
Beacon beacon[20];
int beaconCnt = 0;
String tuuid = ""; // target uuid
String devid = "";
String fname = "";
char conf_c;
String conf_txt = "";
String conf_key = "";
String conf_value = "";
File file;

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;
char timeStrbuff[64];
int brt = 5;

// quick sort descending order
void quicksort(Beacon a[], int left, int right) {
  String temp_uuid;
  int temp_rssi;
  int temp_major;
  int temp_minor;
  signed char temp_power;
  
  if (left < right) {
    int i = left, j = right;
    double tmp, pivot = med3(a[i].rssi, a[i + (j - i) / 2].rssi, a[j].rssi);
    while (1) {
      while (a[i].rssi > pivot) i++;
      while (pivot > a[j].rssi) j--;
      if (i >= j) break;
      temp_uuid = a[i].uuid;
      a[i].uuid = a[j].uuid;
      a[j].uuid = temp_uuid;
      temp_rssi = a[i].rssi;
      a[i].rssi = a[j].rssi;
      a[j].rssi = temp_rssi;
      temp_major = a[i].major;
      a[i].major = a[j].major;
      a[j].major = temp_major;
      temp_minor = a[i].minor;
      a[i].minor = a[j].minor;
      a[j].minor = temp_minor;
      temp_power = a[i].power;
      a[i].power = a[j].power;
      a[j].power = temp_power;
      i++; j--;
    }
    quicksort(a, left, i - 1);
    quicksort(a, j + 1, right);
  }
}

int med3(int x, int y, int z) {
  if (x < y) {
    if (y < z) return y; else if (z < x) return x; else return z;
  } else {
    if (z < y) return y; else if (x < z) return x; else return z;
  }
}

void setup() {
  // Initialize the M5Stack object
  //Serial.begin(9600); // it's not necessary for M5Stack Core2
  M5.begin();

  // Initialize display
  M5.Lcd.clear();
  M5.Axp.SetLcdVoltage(3300); // set brightness 2500 - 3300
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);

  int cur = 0;
  int incr = 0;
  while(true) {
    M5.update();  //Read the press state of the key.
    
    // display Button
    M5.Lcd.setCursor(50, 220);
    M5.Lcd.println("+");
    M5.Lcd.setCursor(155, 220);
    M5.Lcd.println("-");
    M5.Lcd.setCursor(250, 220);
    M5.Lcd.println("SET");
  
    // get Date & Time
    M5.Rtc.GetTime(&RTCtime);
    M5.Rtc.GetDate(&RTCDate);
  
    // display Date & Time
    sprintf(timeStrbuff,"%d/%02d/%02d  %02d:%02d:%02d",
      RTCDate.Year,RTCDate.Month,RTCDate.Date,
      RTCtime.Hours,RTCtime.Minutes,RTCtime.Seconds);
    M5.Lcd.setCursor(10, 50);
    M5.Lcd.println(timeStrbuff);
    if (cur < 6) {
      M5.Lcd.setCursor(30+cur*40, 70);
    } else if (cur == 6) {
      M5.Lcd.setCursor(150, 110);
    } else if (cur == 7) {
      M5.Lcd.setCursor(130, 150);
    }
    M5.Lcd.println("^^");
    M5.Lcd.setCursor(10, 90);
    M5.Lcd.printf("Brightness: %d", brt);
    M5.Lcd.setCursor(10, 130);
    M5.Lcd.printf("Interval: %d sec.", scanTime * sampleCnt);

    // check Button
    if (M5.BtnA.pressedFor(500)) {
      incr ++;
    } else if (M5.BtnB.pressedFor(500)) {
      incr --;
    } else if (M5.BtnC.wasReleasefor(500)) {
      cur ++;
    }

    if (cur > 7) {
      break;
    }

    // set Date & Time
    if (incr != 0) {
      if (cur == 0 && RTCDate.Year + incr >= 0) {
        RTCDate.Year = RTCDate.Year + incr;
      } else if (cur == 1 && RTCDate.Month + incr >= 1 && RTCDate.Month + incr <= 12) {
        RTCDate.Month = RTCDate.Month + incr;
      } else if (cur == 2 && RTCDate.Date + incr >= 1 && RTCDate.Date + incr <= 31) {
        RTCDate.Date = RTCDate.Date + incr;
      } else if (cur == 3 && RTCtime.Hours + incr >= 0 && RTCtime.Hours + incr <= 23) {
        RTCtime.Hours = RTCtime.Hours + incr;
      } else if (cur == 4 && RTCtime.Minutes + incr >= 0 && RTCtime.Minutes + incr <= 59) {
        RTCtime.Minutes = RTCtime.Minutes + incr;
      } else if (cur == 5 && RTCtime.Seconds + incr >= 0 && RTCtime.Seconds + incr <= 59) {
        RTCtime.Seconds = RTCtime.Seconds + incr;
      } else if (cur == 6 && brt + incr >= 0 && brt + incr <= 5) {
        brt = brt + incr;
      } else if (cur == 7 && sampleCnt + incr*3 >= 3 && sampleCnt + incr*3 <= 12) {
        sampleCnt = sampleCnt + incr*3;
      }
      M5.Rtc.SetDate(&RTCDate);
      M5.Rtc.SetTime(&RTCtime);
      M5.Axp.SetLcdVoltage(2500 + brt*0.2*(3300-2500)); // set brightness 2500 - 3300
    }

    incr = 0;
    delay(200);
    M5.Lcd.clear();
  }
  
  // read conf.txt
  file = SD.open("/conf.txt", FILE_READ);
  while (file.available()) {
    conf_c = file.read();
    if (conf_c == '=') {
      conf_key = conf_txt;
      conf_txt = "";
    } else if (conf_c == '\n') {
      conf_value = conf_txt;
      conf_txt = "";
      if (conf_key.compareTo("devid") == 0) {
        devid = conf_value;
      }
      if (conf_key.compareTo("uuid") == 0) {
        tuuid = conf_value;
        tuuid.toUpperCase();
      }
    } else if (conf_c != '\r') {
      conf_txt += char(conf_c);
    }
  }
  file.close();
  
  // display configure
  M5.Lcd.clear();
  M5.Lcd.setTextWrap(true);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("devid -> %s\n", devid.c_str());
  M5.Lcd.printf("uuid -> %s\n", tuuid.c_str());
  M5.Lcd.printf("sampling time -> %d\n", scanTime * sampleCnt);
  M5.Lcd.printf("%04d-%02d-%02d %02d:%02d:%02d",
    RTCDate.Year,RTCDate.Month,RTCDate.Date,
    RTCtime.Hours,RTCtime.Minutes,RTCtime.Seconds);
  M5.Axp.SetLed(false); // turn off led

  // puts header
  int ix = 0;
  while (1) {
    fname = "/" + devid + "-" + String(ix) + ".csv";
    if (!(SD.exists(fname.c_str()))) {
      break;
    }
    ix ++;
  }
  file = SD.open(fname.c_str(), FILE_WRITE);
  file.println("deviceID,DateTime,seconds,beaconID1,rssi1,beaconID2,rssi2,beaconID3,rssi3");
  file.close();
  
}

void loop() {
  if (cnt % sampleCnt == 0) {
    M5.Axp.SetLed(true);  // turn on led
    M5.Axp.SetLed(false); // turn off led
    
    // scan iBeacon
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
  
    char uuid[60];
    for (int i = 0; i < 20; i++) {
      beacon[i].uuid = tuuid;
      beacon[i].rssi = 0;
      beacon[i].major = 0;
      beacon[i].minor = 0;
      beacon[i].power = 0;
    }
    int ix = 0;
    beaconCnt = foundDevices.getCount();
    for (int i = 0; i < beaconCnt; i++) {
      BLEAddress addr = foundDevices.getDevice(i).getAddress();
      int rssi = foundDevices.getDevice(i).getRSSI();
      std::string data = foundDevices.getDevice(i).getManufacturerData();
  
      if(data.length() == 25) {
        if((data[0] == 0x4c) && (data[1] == 0x00) && (data[2] == 0x02) && (data[3] == 0x15)) {
          sprintf(uuid,"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X"
            ,data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13]
            ,data[14],data[15],data[16],data[17],data[18],data[19]);
          int major = (int)(((data[20]&0xff) << 8) + (data[21] & 0xff));
          int minor = (int)(((data[22]&0xff) << 8) + (data[23] & 0xff));
          signed char power = (signed char)(data[24]&0xff);
  
          if (tuuid.compareTo(uuid) == 0) {
            beacon[ix].uuid = uuid;
            beacon[ix].rssi = rssi;
            beacon[ix].major = major;
            beacon[ix].minor = minor;
            beacon[ix].power = power;
            ix ++;
          }
        }
      }
    }
    if (ix > 0) {
      quicksort(beacon, 0, ix-1);
    }
  
    // get date & time
    M5.Rtc.GetTime(&RTCtime);
    M5.Rtc.GetDate(&RTCDate);
    sprintf(timeStrbuff,"%d/%02d/%02d  %02d:%02d:%02d",
      RTCDate.Year,RTCDate.Month,RTCDate.Date,
      RTCtime.Hours,RTCtime.Minutes,RTCtime.Seconds);

    // display
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println(timeStrbuff);
    M5.Lcd.printf("devID: %s, time: %d sec.\n", devid.c_str(), cnt * scanTime);
    for (int i = 0; i < ix; i++) {
      //M5.Lcd.printf("rssi=%d, uuid=%s, major=%d, minor=%d, power=%d\n",
      //  beacon[i].rssi, beacon[i].uuid.c_str(), beacon[i].major, beacon[i].minor, beacon[i].power);
      M5.Lcd.printf("%d : %d-%d, %d\n",
        i, beacon[i].major, beacon[i].minor, beacon[i].rssi);
    }
    M5.Lcd.println();
  
    // write file
    file = SD.open(fname.c_str(), FILE_APPEND);
    file.printf("%s,%s,%d,", devid.c_str(), timeStrbuff, cnt * scanTime);
    if (ix > 0) {
      file.printf("bcn%d-%d,%d", beacon[0].major, beacon[0].minor, beacon[0].rssi);
    } else {
      file.print(",");
    }
    if (ix > 1) {
      file.printf(",bcn%d-%d,%d", beacon[1].major, beacon[1].minor, beacon[1].rssi);
    } else {
      file.print(",,");
    }
    if (ix > 2) {
      file.printf(",bcn%d-%d,%d\n", beacon[2].major, beacon[2].minor, beacon[2].rssi);
    } else {
      file.println(",,");
    }
    file.close();
    M5.Lcd.println("SD write!");
    
  } else {
    M5.Axp.SetLed(true);  // turn on led
    M5.Axp.SetLed(false); // turn off led
    
    // light sleep
    M5.Axp.LightSleep(SLEEP_SEC(scanTime));
  }
  cnt ++;
}
