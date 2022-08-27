/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <M5Stack.h>
#include "types.h"

int scanTime = 5; //In seconds
int intvalTime = 25;  //In seconds
int sampleCnt = 1;
int cnt = 0;
int total_time = 0;
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
  Serial.begin(9600);

  // Initialize the M5Stack object
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setBrightness(5);
  M5.Lcd.setTextWrap(true);

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
  M5.Lcd.print("devid -> ");
  M5.Lcd.println(devid);
  M5.Lcd.print("uuid -> ");
  M5.Lcd.println(tuuid);

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
  file.println("deviceID,seconds,beaconID,rssi");
  file.close();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  char uuid[60];

  int ix = 0;
  beaconCnt = foundDevices.getCount();
  for (int i = 0; i < beaconCnt; i++) {
    if (tuuid.compareTo(uuid) == 0) {
      beacon[ix].uuid = uuid;
      beacon[ix].rssi = 0;
      beacon[ix].major = 0;
      beacon[ix].minor = 0;
      beacon[ix].power = 0;
    }
  }
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
  
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("devID: %s, time: %d sec.\n", devid.c_str(), total_time);
  for (int i = 0; i < ix; i++) {
    //M5.Lcd.printf("rssi=%d, uuid=%s, major=%d, minor=%d, power=%d\n",
    //  beacon[i].rssi, beacon[i].uuid.c_str(), beacon[i].major, beacon[i].minor, beacon[i].power);
    M5.Lcd.printf("%d : %d-%d, %d\n",
      i, beacon[i].major, beacon[i].minor, beacon[i].rssi);
  }
  M5.Lcd.println();

  if (cnt % sampleCnt == 0) {
    file = SD.open(fname.c_str(), FILE_APPEND);
    file.print(devid + ",");
    file.print(total_time);
    file.print(",bcn");
    if (ix > 0) {
      if (beacon[0].rssi == 0) {
        if (ix > 1) {
          file.print(beacon[1].major);
          file.print("-");
          file.print(beacon[1].minor);
          file.print(",");
          file.println(beacon[1].rssi);
        } else {
          file.println(",");
        }
      } else {
        file.print(beacon[0].major);
        file.print("-");
        file.print(beacon[0].minor);
        file.print(",");
        file.println(beacon[0].rssi);
      }
    } else {
      file.println(",");
    }
    file.close();
    M5.Lcd.println("SD write!");
    total_time = total_time + scanTime;
  }
  cnt ++;

  delay(intvalTime * 1000);
  total_time = total_time + intvalTime;
}
