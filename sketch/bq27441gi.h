/*
 BQ27441 Fuel Gauge final parameters for each battery type (The Golden Image)
 Note: Sparkfun BS, R_iset is changed to 825Ω, the new I_term is (0.1 * 890/820) = 110mA.
       If bat < 850mAh, Taper current = I-term + 90mA, otherwise Taper current = 0.1 C (±10%)

 BAT_1 (eBay)
 BAT_2 (Panasonic_B-Grn)
 BAT_3 (Samsung_30Q-Pink)
 BAT_4 (Samsung_25R-Grn)
 BAT_5 (PKCell_803035)
*/


#define BAT_2


// BAT_1 (eBay): 850mAh@3.7V Taper=42 Qmax=16535
#ifdef BAT_1
const unsigned int lipoDesignCapacity = 850;   // (mAh)
const unsigned int lipoDesignEnergy = 850*3.7; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 42;         // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16535;      // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {86,86,90,104,83,82,95,112,113,115,141,169,314,815,1300};
#endif //BAT_1


// BAT_2 (Panasonic_B-Grn): 3400mAh@3.6V Taper=112 Qmax=16460
#ifdef BAT_2
const unsigned int lipoDesignCapacity = 3400;   // (mAh)
const unsigned int lipoDesignEnergy = 3400*3.6; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 112;         // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16460;       // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {28,284,280,306,210,175,189,195,169,155,200,240,481,1262,2005}; 
#endif //BAT_2


// BAT_3 (Samsung_30Q-Pink): 3000mAh@3.6V Taper=110 Qmax=16603
#ifdef BAT_3
const unsigned int lipoDesignCapacity = 3000;   // (mAh)
const unsigned int lipoDesignEnergy = 3000*3.6; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 110;         // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16603;       // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {225,225,213,230,159,135,147,154,136,126,164,197,400,1047,1669};
#endif //BAT_3


// BAT_4 (Samsung_25R-Grn): 2500mAh@3.6V Taper=108 Qmax=16415
#ifdef BAT_4
const unsigned int lipoDesignCapacity = 2500;   // (mAh)
const unsigned int lipoDesignEnergy = 2500*3.6; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 108;         // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16415;       // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {210,210,202,221,155,133,146,154,134,127,169,202,396,1039,1653};
#endif //BAT_4


// BAT_5 (PKCell_803035): 2000mAh@3.7V Taper=95 Qmax=16572
#ifdef BAT_5
const unsigned int lipoDesignCapacity = 2000;   // (mAh)
const unsigned int lipoDesignEnergy = 2000*3.7; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 95;          // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16572;       // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {62,62,62,69,50,46,52,57,55,55,70,86,165,424,677};
#endif //BAT_5


// Default Qmax = 16384
// Default R_a Table = {102,102,99,107,72,59,62,63,53,47,60,70,140,369,588};

