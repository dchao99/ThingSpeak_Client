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
#define DEBUG_ESP8266                    // Debug output to terminal
//#define DEBUG_MAX_POWER                // Debug mode for fastest updates and battery discharge
#define BQ27441_FUEL_GAUGE               // BQ27441 Impedance Track Fuel Gauge
#define DEBUG_BQ27441_IT                 // Debug BQ27441 Impedance Tracking algorithm
#define I2C_BME280_ADDR 0x76             // BME280 I2C address

// To read a max 4.2V from V(bat), a voltage divider is used to drop down to Vref=1.06V for the ADC
const float volt_div_const = 4.44*1.06/1.023; // multiplier = Vin_max*Vref/1.023 (mV)
                                              // WeMos BatShield: (350KΩ+100KΩ) Vin = 4.47
                                              // LM3671 Shield:   (340KΩ+100KΩ) Vin = 4.44

// Wi-Fi Settings
const char* ssid     = "San Leandro";      // your wireless network name (SSID)
const char* password = "xxxxxxxx";         // your Wi-Fi network password
const unsigned long wifi_connect_timeout = 10 * 1000;  // 10 seconds

// ThingSpeak Settings
const int channel_id     = 293299;                // Channel ID for ThingSpeak 
String write_api_key     = "XXXXXXXXXXXXXXXX";    // write API key for ThingSpeak Channel
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
// Note: there is a small 50mV drop between V(bat) and V(A0)
const int terminate_voltage = 3000;  // (mV) Host system lowest operating voltage 

// BME280 settings
const float thingAltitude = 30;     // My altitude (meters)

// Initialize class objects
WiFiClient client;
Adafruit_BME280 bme; 
BQ27441 lipo;


//
// Main program to read all sensor data and upload it to ThingSpeak
//
void uploadData(const char * server) 
{
    if (client.connect(server, 80)) {
    
    int adcVoltage = analogRead(A0) * volt_div_const;
    // Take average of two readings to get rid of noise
    delay(1);
    adcVoltage = ( adcVoltage + analogRead(A0)*volt_div_const ) / 2 ;
    
    String thingStatus;
    if (adcVoltage < floating_voltage) {
      wemosBattery = BATTERY_FLOAT;
      thingStatus = F("No Battery ");
    }
    else if (adcVoltage < lockout_voltage) {
      wemosBattery = BATTERY_CRITICAL;
      thingStatus = F("Battery Critical ");
    }
    #ifndef DEBUG_MAX_POWER  // Skip deep-sleep modes for fastest updates and battery discharge
    else if (adcVoltage < hibernate_voltage) {
      wemosBattery = BATTERY_LOW;
      thingStatus = F("Battery Low ");
    }
    else if (adcVoltage < recharge_voltage) {
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
    float bmeTemp     = bme.readTemperature();
    float bmeHumidity = bme.readHumidity();
    float seaLevelPressure = bme.seaLevelForAltitude(thingAltitude,bme.readPressure()) / 100.0F; //(hPa)
    #endif //I2C_BME280_ADDR
       
    #ifdef BQ27441_FUEL_GAUGE
    // Get battery data from Battery Fuel Gauge
    float        lipoVoltage = (float)lipo.voltage() / 1000.0F;
    unsigned int lipoSOC = lipo.soc();
    int          lipoCurrent = lipo.current(AVG);
    unsigned int lipoCapacity = lipo.capacity(AVAIL_FULL);
    uint8  lipoSoHStat = lipo.soh(SOH_STAT);
    uint16 lipoFlags = lipo.flags();
    thingStatus += String(lipoCurrent) + "mA (" + String(lipoCapacity) + "mAh"+")";
    if (lipoSoHStat == 0x02)   // SoH based on default Qmax - Estimation
      thingStatus += "*";
    if (lipoSoHStat == 0x03)   // SoH based on learned Qmax - Most accurate
      thingStatus += "**";
    if (lipoFlags & BQ27441_FLAG_DSG)
      thingStatus += " Dsg";
    if (lipoFlags & BQ27441_FLAG_FC)
      thingStatus += " Ful";
    #ifdef DEBUG_BQ27441_IT
    uint16 lipoGaugeStat = lipo.status();
    uint16 lipoQmax = bq27441_ReadQmax(lipo);
    uint16 lipoRaTable[15];
    bq27441_ReadRaTable(lipo,lipoRaTable);
    if (lipoGaugeStat & BQ27441_STATUS_VOK)
      thingStatus += " Vok";
    if (lipoGaugeStat & BQ27441_STATUS_RUP_DIS)
      thingStatus += " Rdi";
    if (lipoGaugeStat & BQ27441_STATUS_QMAX_UP)  
      thingStatus += " Qup";
    if (lipoGaugeStat & BQ27441_STATUS_RES_UP)
      thingStatus += " Rup";
    thingStatus +=  " Q=" + String(lipoQmax) + " R=";
    for (int i = 0; i < 15; i++)
      thingStatus += String(lipoRaTable[i])+",";
    #endif //DEBUG_BQ27441_IT
    #endif //BQ27441_FUEL_GAUGE

    // Construct API request body
    String body = F("field1=");
    float volt = adcVoltage/1000.0F;
    body += String(volt,3);
    #ifdef I2C_BME280_ADDR
    body += F("&field2=");
    body += String(bmeTemp);
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

    #ifdef DEBUG_ESP8266
    Serial.print(F("ESP8266: "));
    Serial.println(String(volt,3)+"V"); 
    #ifdef I2C_BME280_ADDR
    Serial.print(F("BME280: "));
    Serial.println(String(bmeTemp)+"C, " + String(bmeHumidity) + "%, " + String(seaLevelPressure) + "hPa"); 
    #endif //I2C_BME280_ADDR
    #ifdef BQ27441_FUEL_GAUGE    
    Serial.print(F("BQ27441: "));
    Serial.print(String(lipoVoltage,3) + "V, " + String(lipoSOC) + "%, " + String(lipoCurrent) + "mA ");
    Serial.print("(Bat=" + String(lipoCapacity) + "mAh" + ")");
    if (lipoSoHStat == 0x02)   // SoH based on default Qmax - Estimation
      Serial.print("*");
    if (lipoSoHStat == 0x03)   // SoH based on learned Qmax - Most accurate
      Serial.print("**");
    if (lipoFlags & BQ27441_FLAG_DSG)
      Serial.print(" Dsg");
    if (lipoFlags & BQ27441_FLAG_FC)
      Serial.print(" Ful");
    #ifdef DEBUG_BQ27441_IT
    if (lipoGaugeStat & BQ27441_STATUS_VOK)
      Serial.print(" Vok");
    if (lipoGaugeStat & BQ27441_STATUS_RUP_DIS)
      Serial.print(" Rdi");
    if (lipoGaugeStat & BQ27441_STATUS_QMAX_UP)  
      Serial.print(" Qup");
    if (lipoGaugeStat & BQ27441_STATUS_RES_UP)
      Serial.print(" Rup");
    Serial.println(" Qmax=" + String(lipoQmax));
    Serial.print("R_a Table: ");
    for (int i = 0; i < 15; i++)
      Serial.print(String(lipoRaTable[i])+",");
    #endif //DEBUG_BQ27441_IT
    Serial.println();
    #endif //BQ27441_FUEL_GAUGE
    #endif //DEBUG_ESP8266

    // Prepare the HTML document to post
    client.print( F("POST /update HTTP/1.1\n") );
    client.print( F("Host: api.thingspeak.com\nConnection: close\n") );
    client.print( F("X-THINGSPEAKAPIKEY: ") );
    client.print( write_api_key );
    client.print( F("\nContent-Type: application/x-www-form-urlencoded\n") );
    client.print( F("Content-Length: ") );
    client.print( body.length() );
    client.print( F("\n\n") );
    client.print( body );
    client.print( F("\n\n") );
  }
  client.stop();
}


//
// Arduino initialization entry point
//
void setup() 
{
  #ifdef DEBUG_ESP8266
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  #endif

  // Beginning hardware checks

  #ifdef I2C_BME280_ADDR
  if (!bme.begin(I2C_BME280_ADDR)) {
    #ifdef DEBUG_ESP8266
    Serial.println(F("Error: Couldn't find BME280 sensor."));
    #endif
    ESP.deepSleep(0);
  }
  #ifdef DEBUG_ESP8266
  Serial.println(F("BME280 connected."));
  #endif
  #endif //I2C_BME280_ADDR

  #ifdef BQ27441_FUEL_GAUGE
  if (!lipo.begin()) // begin() will return true if communication is successful
  {
    #ifdef DEBUG_ESP8266
    Serial.println(F("Error: Couldn't find BQ27441. (Battery must be plugged in)"));
    #endif 
    ESP.deepSleep(0);
  }
  #ifdef DEBUG_ESP8266
  Serial.println(F("BQ27441 connected."));
  #endif
  if (lipo.flags() & BQ27441_FLAG_ITPOR) {
    #ifdef DEBUG_ESP8266
    Serial.print(F("BQ27441: POR detected. "));
    #endif 
    if ( bq27441_InitParameters(lipo,terminate_voltage) ) {
      #ifdef DEBUG_ESP8266
      Serial.println(F("Fuel Gauge initialized."));
      #endif 
    } else {
      #ifdef DEBUG_ESP8266
      Serial.println();
      Serial.println(F("Warning: Failed to initialize Fuel Gauge parameters."));
      #endif 
    }
  }
  #endif //BQ27441_FUEL_GAUGE
  
  // Make the hostname up from our Chip ID (MAC addr)
  char hostString[16]  = {0};
  sprintf(hostString, "esp8266_%06x", ESP.getChipId());

  // Connecting to a WiFi network
  #ifdef DEBUG_ESP8266
  Serial.print(F("Hostname: "));
  Serial.println(hostString);
  Serial.print(F("Connecting to "));
  Serial.print(ssid);
  Serial.print(F(" "));
  #endif 
  
  WiFi.hostname(hostString);
  WiFi.mode(WIFI_STA); 
  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  if (WiFi.status() != WL_CONNECTED)
    WiFi.begin(ssid, password);
  
  unsigned long wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG_ESP8266
    Serial.print(".");
    #endif 
    if ( (millis()-wifiConnectStart) > wifi_connect_timeout ) {
      #ifdef DEBUG_ESP8266
      Serial.println();
      Serial.println(F("Warning: Unable to connect to WiFi."));
      #endif 
      ESP.deepSleep(sleep_timer);
    }
  }

  #ifdef DEBUG_ESP8266
  Serial.println();
  Serial.println(F("WiFi connected."));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  #endif 

  uploadData(api_endpoint);

  switch(wemosBattery) {
    
    case BATTERY_FLOAT:
      // If VBAT is floating and code is running, we must be external power
      #ifdef DEBUG_ESP8266
      Serial.println(F("External power, exit deep-sleep."));
      #endif
      break;

    case BATTERY_CRITICAL:
      #ifdef DEBUG_ESP8266
      Serial.println(F("Warning: Under voltage detected. Shut-Down ESP8266."));
      #endif
      ESP.deepSleep(0);  

    case BATTERY_LOW:
      #ifdef DEBUG_ESP8266
      Serial.println(F("Warning: Hibernate voltage detected. Long deep-Sleep timer."));
      #endif
      ESP.deepSleep(hibernate_timer);

    case BATTERY_NORMAL:
      #ifdef DEBUG_ESP8266
      Serial.println(F("Running on battery, short deep-sleep timer."));
      #endif
      ESP.deepSleep(sleep_timer);  
   
    case BATTERY_FULL:
      #ifdef DEBUG_ESP8266
      Serial.println(F("Battery fully charged or external power, exit deep-sleep."));
      #endif
      break;
  }
}  // end of setup


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
      #ifdef DEBUG_ESP8266
      Serial.println(F("Running on battery, set deep-sleep mode."));
      #endif
      ESP.deepSleep(sleep_timer);  
    case BATTERY_FLOAT:
    case BATTERY_FULL:
      break;
  }
}  // end of loop

