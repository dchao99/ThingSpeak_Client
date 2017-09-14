/*
 ESP8266 --> ThingSpeak Channel via ESP8266 Wi-Fi
 
 This sketch sends the value of Analog Input (A0), the BME280 sensors, and BQ27441 Battery 
 Fuel Gauge data on the I2C bus to a ThingSpeak channel using the ThingSpeak API 
 (https://www.mathworks.com/help/thingspeak).

 ThingSpeak Setup:
 
   * Sign Up for New User Account - https://thingspeak.com/users/sign_up
   * Create a new Channel by selecting Channels, My Channels, and then New Channel
   * Enable one field
   * Note the Channel ID and Write API Key

Deep-Sleep:
 
   * You must to connect GPIO 16 to RST to wake up (or reset) the device when Deep-Sleep is over

 Battery voltage level:
 
   * Ext-Power: ESP8266 powered by external USB, upload data in the main loop, no deep-sleep
   * Normal:    Normal battery, at end of upload, ESP8266 goes into deep-sleep with short timer
   * Hibernate: Battery very low, ESP8266 goes into deep-sleep with a longer timer
   * Shut-Down: Under voltage detected, ESP8266 goes inot Deep Sleep indefinitely, must press
                RST S/W to wake up the device again
  
 Tutorial: http://nothans.com/measure-wi-fi-signal-levels-with-the-esp8266-and-thingspeak
   
 Created: Feb 3, 2017 by Hans Scharler (http://nothans.com)
 Modified: Aug 21, 2017 by David Chao
*/

#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SparkFunBQ27441.h>
#include "bq27441gi.h"

// Compiler directives, comment out to disable
#define USE_SERIAL Serial               // Valid options: Serial and Serial1
//#define DEBUG_MAX_POWER               // Debug mode for fastest updates and battery discharge
#define BQ27441_FUEL_GAUGE              // BQ27441 Impedance Track Fuel Gauge
#define I2C_BME280_ADDR 0x76            // BME280 I2C address

// To read a max 4.2V from V(bat), a voltage divider is used to drop down to Vref=1.06V for the ADC
const float volt_div_const = 4.45*1.06/1.023; // multiplier = Vin_max*Vref/1.023 (mV)
                                              // WeMos BatShield: (350KΩ+100KΩ) Vin = 4.49
                                              // LM3671 Shield:   (340KΩ+100KΩ) Vin = 4.45

// Wi-Fi Settings
const char* ssid     = "San Leandro";      // your wireless network name (SSID)
const char* password = "xxxxxxxx";         // your Wi-Fi network password
const unsigned long wifi_connect_timeout = 10 * 1000;  // 10 seconds

// ThingSpeak Settings
const int channel_id     = 293299;                // Channel ID for ThingSpeak 
const String write_api_key = "XXXXXXXXXXXXXXXX";  // write API key for ThingSpeak Channel
const char* api_endpoint = "api.thingspeak.com";  // URL
const int upload_interval    =  30 * 1000;        // External power: posting data every 30 sec
const uint32 sleep_timer     = 060 * 1000000;     // Normal battery: Deep sleep timer = 60 sec
const uint32 hibernate_timer = 150 * 1000000;     // Hibernate: Deep sleep timer = 2.5 min

// ESP8266 settings
const int recharge_voltage  = 4130;  // (mV) Recharging threshold, above -> battery full/charging 
const int hibernate_voltage = 3550;  // (mV) Hibernate voltage (SoC~10%) -> reduce upload frequency
const int lockout_voltage   = 3100;  // (mV) Under voltage (UVLO) -> shut-down immediately
const int floating_voltage  = 500;   // (mV) No battery, VBAT is floating
enum Battery { BATTERY_FLOAT, BATTERY_CRITICAL, BATTERY_LOW, BATTERY_NORMAL, BATTERY_FULL };
Battery wemosBattery;

// BQ27441 settings
// Note: there is a small 20mV (@100mA) to 50mV (@1A) dropout between V(bat) and V(A0)
const int terminate_voltage = 3100;  // (mV) Host system lowest operating voltage 

// BME280 settings
const float thing_altitude = 30;     // My altitude (meters)

// Initialize class objects
WiFiClient client;
#ifdef I2C_BME280_ADDR
Adafruit_BME280 bme; 
#endif
#ifdef BQ27441_FUEL_GAUGE
BQ27441 lipo;
#endif


//
// Main program to read all sensor data and upload it to ThingSpeak
//
void uploadData(const char * server) 
{
    if (client.connect(server, 80)) {
    
    int adc_mV = analogRead(A0) * volt_div_const;
    // Take average of two readings to get rid of noise
    delay(1);
    adc_mV = ( adc_mV + analogRead(A0)*volt_div_const ) / 2 ;
    float adcVoltage = adc_mV/1000.0F;
    
    String thingStatus;
    if (adc_mV < floating_voltage) {
      wemosBattery = BATTERY_FLOAT;
      thingStatus = F("No Battery ");
    }
    else if (adc_mV < lockout_voltage) {
      wemosBattery = BATTERY_CRITICAL;
      thingStatus = F("Battery Critical ");
    }
    #ifndef DEBUG_MAX_POWER  // Skip deep-sleep modes for fastest updates and battery discharge
    else if (adc_mV < hibernate_voltage) {
      wemosBattery = BATTERY_LOW;
      thingStatus = F("Battery Low ");
    }
    else if (adc_mV < recharge_voltage) {
      wemosBattery = BATTERY_NORMAL;
      thingStatus = F("Battery Normal ");
    }
    #endif //DEBUG_MAX_POWER
    else {
      wemosBattery = BATTERY_FULL;
      thingStatus = F("Battery Full ");
    }

    #ifdef I2C_BME280_ADDR
    // Measure BME280 sensors
    float bmeTemperature = bme.readTemperature();
    float bmeHumidity = bme.readHumidity();
    float seaLevelPressure = bme.seaLevelForAltitude(thing_altitude,bme.readPressure()) / 100.0F; //(hPa)
    #endif //I2C_BME280_ADDR
       
    #ifdef BQ27441_FUEL_GAUGE
    // Get battery data from Battery Fuel Gauge
    float        lipoVoltage = (float)lipo.voltage() / 1000.0F;
    unsigned int lipoSOC = lipo.soc();
    int          lipoCurrent = lipo.current(AVG);
    unsigned int lipoCapacity = lipo.capacity(AVAIL_FULL);
    uint8  lipoSoHStat = lipo.soh(SOH_STAT);
    uint16 lipoFlags = lipo.flags();
    thingStatus += "(" + String(lipoCapacity) + "mAh"+")";
    if (lipoSoHStat == 0x02)   // SoH based on default Qmax - Estimation
      thingStatus += "*";
    if (lipoSoHStat == 0x03)   // SoH based on learned Qmax - Most accurate
      thingStatus += "**";
    thingStatus += " " + String(lipoCurrent) + "mA [ ";
    if (lipoFlags & BQ27441_FLAG_DSG)
      thingStatus += "Dsg ";
    if (lipoFlags & BQ27441_FLAG_FC)
      thingStatus += "Ful ";
    uint16 lipoGaugeStat = lipo.status();
    uint16 lipoQmax = bq27441_ReadQmax(lipo);
    uint16 lipoRaTable[15];
    bq27441_ReadRaTable(lipo,lipoRaTable);
    if (lipoGaugeStat & BQ27441_STATUS_VOK)
      thingStatus += "Vok ";
    if (lipoGaugeStat & BQ27441_STATUS_RUP_DIS)
      thingStatus += "Rdi ";
    if (lipoGaugeStat & BQ27441_STATUS_QMAX_UP)  
      thingStatus += "Qup ";
    if (lipoGaugeStat & BQ27441_STATUS_RES_UP)
      thingStatus += "Rup ";
    thingStatus +=  "] Q=" + String(lipoQmax) + " R=";
    for (int i = 0; i < 15; i++)
      thingStatus += String(lipoRaTable[i])+",";
    #endif //BQ27441_FUEL_GAUGE

    // Construct API request body
    String body = F("field1=");
    body += String(adcVoltage,3);
    #ifdef I2C_BME280_ADDR
    body += F("&field2=");
    body += String(bmeTemperature);
    body += F("&field3=");
    body += String(bmeHumidity);
    body += F("&field4=");
    body += String(seaLevelPressure);
    #endif //I2C_BME280_ADDR
    
    #ifdef BQ27441_FUEL_GAUGE
    body += F("&field5=");
    body += String(lipoVoltage,3);    
    body += F("&field6=");
    body += String(lipoSOC);
    #endif //BQ27441_FUEL_GAUGE
    body += F("&status=");
    body += thingStatus;

    #ifdef USE_SERIAL
    USE_SERIAL.print(F("ESP8266: "));
    USE_SERIAL.print(adcVoltage,3); USE_SERIAL.println("V"); 
    #ifdef I2C_BME280_ADDR
    USE_SERIAL.print(F("BME280: "));
    USE_SERIAL.println(String(bmeTemperature,1)+"C, " + String(bmeHumidity,1) + "%, " + String(seaLevelPressure,1) + "hPa"); 
    #endif //I2C_BME280_ADDR
    #ifdef BQ27441_FUEL_GAUGE    
    USE_SERIAL.print(F("BQ27441: "));
    USE_SERIAL.print("(Bat=" + String(lipoCapacity) + "mAh" + ")");
    if (lipoSoHStat == 0x02)   // SoH based on default Qmax - Estimation
      USE_SERIAL.print("*");
    if (lipoSoHStat == 0x03)   // SoH based on learned Qmax - Most accurate
      USE_SERIAL.print("**");
    USE_SERIAL.print(" "+String(lipoVoltage,3) + "V, " + String(lipoSOC) + "%, " + String(lipoCurrent) + "mA [ ");
    if (lipoFlags & BQ27441_FLAG_DSG)
      USE_SERIAL.print("Dsg ");
    if (lipoFlags & BQ27441_FLAG_FC)
      USE_SERIAL.print("Ful ");
    if (lipoGaugeStat & BQ27441_STATUS_VOK)
      USE_SERIAL.print("Vok ");
    if (lipoGaugeStat & BQ27441_STATUS_RUP_DIS)
      USE_SERIAL.print("Rdi ");
    if (lipoGaugeStat & BQ27441_STATUS_QMAX_UP)  
      USE_SERIAL.print("Qup ");
    if (lipoGaugeStat & BQ27441_STATUS_RES_UP)
      USE_SERIAL.print("Rup ");
    USE_SERIAL.print("] Qmax="); USE_SERIAL.println(lipoQmax);
    USE_SERIAL.print("R_a=");
    for (int i = 0; i < 15; i++) {
      USE_SERIAL.print(lipoRaTable[i]); 
      USE_SERIAL.print(",");
    }
    USE_SERIAL.println();
    #endif //BQ27441_FUEL_GAUGE
    #endif //USE_SERIAL

    // Prepare the HTML document to post
    client.print( F("POST /update HTTP/1.1\n") );
    client.print( F("Host: api.thingspeak.com\nConnection: close\nX-THINGSPEAKAPIKEY: ") );
    client.print( write_api_key );
    client.print( F("\nContent-Type: application/x-www-form-urlencoded\nContent-Length: ") );
    client.print( body.length() );
    client.print( "\n\n" );
    client.print( body );
    client.print( "\n\n" );
  }
  client.stop();
} // end of uploadData()


void startWiFi()
{
  #ifdef USE_SERIAL
  USE_SERIAL.print(F("Hostname: "));
  USE_SERIAL.println(WiFi.hostname());
  USE_SERIAL.print(F("Connecting ."));
  #endif 

  // WiFi auto-connect is ON by default, when we are called, WiFi maybe connected already.
  // https://github.com/esp8266/Arduino/issues/2186
  if (WiFi.status() != WL_CONNECTED)
    WiFi.begin(ssid, password);
  
  unsigned long wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef USE_SERIAL
    USE_SERIAL.print(".");
    #endif 
    if ( (millis()-wifiConnectStart) > wifi_connect_timeout ) {
      #ifdef USE_SERIAL
      USE_SERIAL.println();
      USE_SERIAL.println(F("Warning: Unable to connect to WiFi."));
      #endif 
      ESP.deepSleep(sleep_timer);
    }
  }

  #ifdef USE_SERIAL
  USE_SERIAL.println();
  USE_SERIAL.print(F("Connected to "));
  USE_SERIAL.println(ssid);
  USE_SERIAL.print(F("IP address: "));
  USE_SERIAL.println(WiFi.localIP());
  #endif 
}


//
// Arduino initialization entry point
//
void setup() 
{ 
  #ifdef USE_SERIAL
  USE_SERIAL.begin(115200);
  USE_SERIAL.println();
  USE_SERIAL.println();
  //USE_SERIAL.setDebugOutput(true);
  #endif
  
  // First thing is to set hostname before WiFi is reconnected (auto-connect is ON)
  // Make up new hostname from our Chip ID (The MAC addr)
  char hostString[32]  = {0};
  sprintf(hostString, "esp8266_%06x", ESP.getChipId());
  WiFi.hostname(hostString);
  WiFi.mode(WIFI_STA); 

  // Start hardware checks
  #ifdef I2C_BME280_ADDR
  if (!bme.begin(I2C_BME280_ADDR)) {
    #ifdef USE_SERIAL
    USE_SERIAL.println(F("Error: Couldn't find a BME280 sensor."));
    #endif
    ESP.deepSleep(0);
  }
  #ifdef USE_SERIAL
  USE_SERIAL.println(F("BME280 connected."));
  #endif
  #endif //I2C_BME280_ADDR

  #ifdef BQ27441_FUEL_GAUGE
  if (!lipo.begin()) // begin() will return true if communication is successful
  {
    #ifdef USE_SERIAL
    USE_SERIAL.println(F("Error: Couldn't find BQ27441. (Battery must be plugged in)"));
    #endif 
    ESP.deepSleep(0);
  }
  #ifdef USE_SERIAL
  USE_SERIAL.println(F("BQ27441 connected."));
  #endif
  if (lipo.flags() & BQ27441_FLAG_ITPOR) {
    #ifdef USE_SERIAL
    USE_SERIAL.print(F("BQ27441: POR detected. "));
    #endif 
    if ( bq27441_InitParameters(lipo,terminate_voltage) ) {
      #ifdef USE_SERIAL
      USE_SERIAL.println(F("Fuel Gauge initialized."));
      #endif 
    } else {
      #ifdef USE_SERIAL
      USE_SERIAL.println();
      USE_SERIAL.println(F("Warning: Failed to initialize Fuel Gauge parameters."));
      #endif 
    }
  }
  #endif //BQ27441_FUEL_GAUGE

  startWiFi();

  uploadData(api_endpoint);

  switch(wemosBattery) {
    
    case BATTERY_FLOAT:
      // If VBAT is floating and code is running, we must be external power
      #ifdef USE_SERIAL
      USE_SERIAL.println(F("External power, exit deep-sleep."));
      #endif
      break;

    case BATTERY_CRITICAL:
      #ifdef USE_SERIAL
      USE_SERIAL.println(F("Warning: Under voltage detected. Shut-Down ESP8266."));
      #endif
      ESP.deepSleep(0);  

    case BATTERY_LOW:
      #ifdef USE_SERIAL
      USE_SERIAL.println(F("Warning: Hibernate voltage detected. Long deep-Sleep timer."));
      #endif
      ESP.deepSleep(hibernate_timer);

    case BATTERY_NORMAL:
      #ifdef USE_SERIAL
      USE_SERIAL.println(F("Running on battery, short deep-sleep timer."));
      #endif
      ESP.deepSleep(sleep_timer);  
   
    case BATTERY_FULL:
      #ifdef USE_SERIAL
      USE_SERIAL.println(F("Battery fully charged or external power, exit deep-sleep."));
      #endif
      break;
  } // end of switch()
}  // end of setup()


//
// Arduino loop entry point -> Normal http client 
// Enter only if external USB power or battery is fully charged
//
void loop() 
{
  delay(upload_interval);
  uploadData(api_endpoint);

  // If running on battery, exit main loop and use deep-sleep to save battery
  switch(wemosBattery) {
    case BATTERY_CRITICAL:
    case BATTERY_LOW:
    case BATTERY_NORMAL:
      #ifdef USE_SERIAL
      USE_SERIAL.println(F("Running on battery, set deep-sleep mode."));
      #endif
      ESP.deepSleep(sleep_timer);  
    case BATTERY_FLOAT:
    case BATTERY_FULL:
      break;
  } // end of switch()
}  // end of loop()

