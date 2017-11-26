/*
 * BQ27441 Golden Image Data Memory Library
 * This library contains the Golden Image for each battery type, and functions for accessing 
 * the Data Memory which stores the default, calibration, configuration parameters.
 * Created: Aug 21, 2017 by David Chao
 */

#include <SparkFunBQ27441.h>
#include "bq27441gi.h"

// Compiler directives, comment out to disable
#define DEV_MODE        // Developer Mode

//#define BAT_1         // (eBay_803035)
//#define BAT_2         // (eBay_501235)
//#define BAT_3         // (PKCell_803860)
//#define BAT_4         // (Panasonic_B-Grn)
//#define BAT_5         // (Sanyo_GA-Red)
#define BAT_6         // (Samsung_30Q-Pink)
//#define BAT_7         // (Samsung_25R-Grn)

// BQ27441 Fuel Gauge Saved Golden Images
// Note: On Sparkfun BS, R_iset is changed to 825Ω, the new I_term is (0.1 * 890/820) = 110mA.
//       If bat < 1000mAh, Taper current = I-term + 90mA, otherwise Taper current = 0.1 C (±10%)

// BAT_1 (eBay_803035): 850mAh@3.7V Taper=45 Qmax=16422
#ifdef BAT_1
const unsigned int design_capacity = 850;   // (mAh)
const unsigned int design_energy = 850*3.7; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 45;         // Taper current = 850/4.5 = 190mA
const unsigned int saved_qmax = 16422;      // Set to -1 if battery data not available
uint16 saved_ra_table[] = {71,71,75,87,73,75,87,103,105,109,133,157,258,637,1014};
#endif //BAT_1

// BAT_2 (eBay_501235): 180mAh@3.7V Taper=75 Qmax=16521
#ifdef BAT_2
const unsigned int design_capacity = 180;    // (mAh)
const unsigned int design_energy = 180*3.7;  // = Capacity * Nominal Voltage
const unsigned int taper_rate = 75;          // Taper current = 180/7.5 = 24mA
const unsigned int saved_qmax = 16521;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {49,49,39,34,17,13,22,40,49,80,157,245,512,1361,2179};
#endif //BAT_2

// BAT_3 (PKCell_803860): 2000mAh@3.7V Taper=95 Qmax=16572
#ifdef BAT_3
const unsigned int design_capacity = 2000;   // (mAh)
const unsigned int design_energy = 2000*3.7; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 95;          // Taper current = 2000/9.5 = 210mA
const unsigned int saved_qmax = 16572;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {62,62,62,69,50,46,52,57,55,55,70,86,165,424,677};
#endif //BAT_3

// BAT_4 (Panasonic_B-Grn): 3400mAh@3.6V Taper=112 Qmax=16509
#ifdef BAT_4
const unsigned int design_capacity = 3400;   // (mAh)
const unsigned int design_energy = 3400*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 112;         // Taper current = 3400/11.2 = 300mA
const unsigned int saved_qmax = 16509;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {219,219,217,238,165,138,148,155,135,125,162,195,392,1026,1633};
#endif //BAT_4

// BAT_5 (Sanyo_GA-Red): 3500mAh@3.6V Taper=113 Qmax=16432
#ifdef BAT_5
const unsigned int design_capacity = 3500;   // (mAh)
const unsigned int design_energy = 3500*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 115;         // Taper current = 3500/11.5 = 305mA
const unsigned int saved_qmax = 16432;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {161,161,156,173,121,102,111,116,103,96,126,152,306,801,1271};
#endif //BAT_5

// BAT_6 (Samsung_30Q-Pink): 3000mAh@3.6V Taper=110 Qmax=16632
#ifdef BAT_6
const unsigned int design_capacity = 3000;   // (mAh)
const unsigned int design_energy = 3000*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 110;         // Taper current = 3000/11.0 = 270mA
const unsigned int saved_qmax = 16632;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {123,123,115,128,91,80,88,94,84,80,108,132,266,696,1108};

#endif //BAT_6

// BAT_7 (Samsung_25R-Grn): 2500mAh@3.6V Taper=108 Qmax=16474
#ifdef BAT_7
const unsigned int design_capacity = 2500;   // (mAh)
const unsigned int design_energy = 2500*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 108;         // Taper current = 2500/10.8 = 230mA
const unsigned int saved_qmax = 16474;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {142,142,139,176,173,210,298,362,371,400,557,655,1232,3152,4990};
#endif //BAT_7

// Default Qmax = 16384
// Default R_a Table = {102,102,99,107,72,59,62,63,53,47,60,70,140,369,588};


//
//  Additional BQ27441 Data Memory Access functions not available in Sparkfun library,
//  And we can also control the enter and exit of the configuration mode
//
bool bq27441_InitParameters(BQ27441 lipo, int terminateVoltage) 
{
  while ( !(lipo.status() & BQ27441_STATUS_INITCOMP) ) { delay(1); }
  lipo.enterConfig();
  bool success = lipo.setCapacity(design_capacity,design_energy);
  success = success && lipo.setTaperRate(taper_rate);
  success = success && lipo.setTerminateVoltage(terminateVoltage);
  if (saved_qmax == -1) {
    // No golden image. Do a learning cycle.
    success = success && lipo.setUpdateStatusReg(0x03);   // Fast updates of Qmax and R_a Table
    #ifdef DEV_MODE
    Serial.print(F("Learning Cycle. "));
    #endif 
  }
  else {
    success = success && lipo.setQmax(saved_qmax);
    success = success && lipo.setRaTable(saved_ra_table);
    #ifdef DEV_MODE
    success = success && lipo.setUpdateStatusReg(0x03);    // Dev Mode: Fast updates
    Serial.print(F("Fast Updates. "));
    #else
    success = success && lipo.setUpdateStatusReg(0x80);    // Production: Sealed the Fuel Gauge memory
    #endif
  }
  lipo.exitConfig(true);     // resim
  return success;
}

uint16 bq27441_ReadQmax(BQ27441 lipo) 
{
  lipo.enterConfig();
  uint16 qmax = lipo.Qmax();
  lipo.exitConfig(false);    // no resim
  return qmax;
}

bool bq27441_ReadRaTable(BQ27441 lipo, uint16 * raTable)
{
  lipo.enterConfig();
  lipo.RaTable(raTable);
  lipo.exitConfig(false);    // no resim
  return true;
}

