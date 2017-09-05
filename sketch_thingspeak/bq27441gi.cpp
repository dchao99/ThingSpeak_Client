/*
 * BQ27441 Golden Image Data Memory Library
 * This library contains the Golden Image for each battery type, and functions for accessing 
 * the Data Memory which stores the default, calibration, configuration parameters.
 * Created: Aug 21, 2017 by David Chao
 */

#include <SparkFunBQ27441.h>
#include "bq27441gi.h"

#define DEBUG_BQ27441   // Debugging mode

//#define BAT_1         // (eBay_803035)
//#define BAT_2         // (PKCell_803860)
//#define BAT_3         // (Panasonic_B-Grn)
//#define BAT_4         // (Sanyo_GA-Red)
//#define BAT_5         // (Samsung_30Q-Pink)
#define BAT_6         // (Samsung_25R-Grn)


// BQ27441 Fuel Gauge Golden Image
// Note: On Sparkfun BS, R_iset is changed to 825Ω, the new I_term is (0.1 * 890/820) = 110mA.
//       If bat < 1000mAh, Taper current = I-term + 90mA, otherwise Taper current = 0.1 C (±10%)

// BAT_1 (eBay_803035): 850mAh@3.7V Taper=42 Qmax=16496
#ifdef BAT_1
const unsigned int design_capacity = 850;   // (mAh)
const unsigned int design_energy = 850*3.7; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 42;         // = Capacity / (0.1 * Taper current)
const unsigned int saved_qmax = 16496;      // Set to -1 if battery data not available
uint16 saved_ra_table[] = {67,67,71,82,68,70,84,104,110,116,143,170,301,762,1212};
#endif //BAT_1

// BAT_2 (PKCell_803860): 2000mAh@3.7V Taper=95 Qmax=16572
#ifdef BAT_2
const unsigned int design_capacity = 2000;   // (mAh)
const unsigned int design_energy = 2000*3.7; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 95;          // = Capacity / (0.1 * Taper current)
const unsigned int saved_qmax = 16572;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {62,62,62,69,50,46,52,57,55,55,70,86,165,424,677};
#endif //BAT_2

// BAT_3 (Panasonic_B-Grn): 3400mAh@3.6V Taper=112 Qmax=16460
#ifdef BAT_3
const unsigned int design_capacity = 3400;   // (mAh)
const unsigned int design_energy = 3400*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 112;         // = Capacity / (0.1 * Taper current)
const unsigned int saved_qmax = 16460;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {28,284,280,306,210,175,189,195,169,155,200,240,481,1262,2005}; 
#endif //BAT_3

// BAT_4 (Sanyo_GA-Red): 3500mAh@3.6V Taper=113 Qmax=16447
#ifdef BAT_4
const unsigned int design_capacity = 3500;   // (mAh)
const unsigned int design_energy = 3500*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 113;         // = Capacity / (0.1 * Taper current)
const unsigned int saved_qmax = 16447;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {24,225,217,238,168,145,159,169,149,140,188,227,452,1183,1887};
#endif //BAT_4

// BAT_5 (Samsung_30Q-Pink): 3000mAh@3.6V Taper=110 Qmax=16546
#ifdef BAT_5
const unsigned int design_capacity = 3000;   // (mAh)
const unsigned int design_energy = 3000*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 110;         // = Capacity / (0.1 * Taper current)
const unsigned int saved_qmax = 16546;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {251,251,233,252,178,157,176,189,172,165,218,267,550,1431,2278};
#endif //BAT_5

// BAT_6 (Samsung_25R-Grn): 2500mAh@3.6V Taper=108 Qmax=16420
#ifdef BAT_6
const unsigned int design_capacity = 2500;   // (mAh)
const unsigned int design_energy = 2500*3.6; // = Capacity * Nominal Voltage
const unsigned int taper_rate = 108;         // = Capacity / (0.1 * Taper current)
const unsigned int saved_qmax = 16420;       // Set to -1 if battery data not available
uint16 saved_ra_table[] = {251,251,238,269,202,187,225,253,231,237,335,415,813,2119,3363};
                       /* {174,174,166,186,136,122,142,155,139,139,193,235,464,1213,1930} */
#endif //BAT_6

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
    #ifdef DEBUG_BQ27441
    Serial.print(F("Learning Cycle. "));
    #endif 
  }
  else {
    success = success && lipo.setQmax(saved_qmax);
    success = success && lipo.setRaTable(saved_ra_table);
    #ifdef DEBUG_BQ27441
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

