/*
 BQ27441 Fuel Gauge final parameters for each battery type (The Golden Image)
 Note: Sparkfun BS, R_iset is changed to 825Ω, the new I_term is (0.1 * 890/820) = 110mA.
       If bat < 850mAh, Taper current = I-term + 90mA, otherwise Taper current = 0.1 C (±10%)
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


// BAT_2 (X5C): 650mAh@3.7V Taper=32 Qmax=16429
#ifdef BAT_2
const unsigned int lipoDesignCapacity = 650;   // (mAh)
const unsigned int lipoDesignEnergy = 650*3.7; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 32;         // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16429;      // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {32,32,28,30,17,13,14,14,13,11,14,16,32,86,137}; 
#endif //BAT_2


// BAT_3 (Panasonic_B-Grn): 3400mAh@3.6V Taper=113 Qmax=16571
#ifdef BAT_3
const unsigned int lipoDesignCapacity = 3400;   // (mAh)
const unsigned int lipoDesignEnergy = 3400*3.6; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 113;         // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16571;       // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {35,291,287,317,219,181,196,203,175,160,205,247,496,1301,2073}; 
#endif //BAT_3


// BAT_4 (Samsung_30Q-Pink): 3000mAh@3.6V Taper=110 Qmax=16603
#ifdef BAT_4
const unsigned int lipoDesignCapacity = 3000;   // (mAh)
const unsigned int lipoDesignEnergy = 3000*3.6; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 110;         // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16603;       // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {225,225,213,230,159,135,147,154,136,126,164,197,400,1047,1669};
#endif //BAT_4


// BAT_5 (PKCell): 2000mAh@3.7V Taper=95 Qmax=16572
#ifdef BAT_5
const unsigned int lipoDesignCapacity = 2000;   // (mAh)
const unsigned int lipoDesignEnergy = 2000*3.7; // = Capacity * Nominal Voltage
const unsigned int lipoTaperRate = 95;          // = Capacity / (0.1 * Taper current)
const unsigned int lipoSavedQmax = 16572;       // Set to -1 if battery data not available
uint16 lipoSavedRaTable[] = {62,62,62,69,50,46,52,57,55,55,70,86,165,424,677};
#endif //BAT_5


// Default Qmax = 16384
// Default R_a Table = {102,102,99,107,72,59,62,63,53,47,60,70,140,369,588};

