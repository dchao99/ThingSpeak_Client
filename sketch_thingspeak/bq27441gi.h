#ifndef bq27441gi_h
#define bq27441gi_h

#include <SparkFunBQ27441.h>

bool bq27441_InitParameters(BQ27441 lipo, int terminate_voltage);
uint16 bq27441_ReadQmax(BQ27441 lipo);
bool bq27441_ReadRaTable(BQ27441 lipo, uint16 * ra_table);

#endif //BQ27441GI_h
