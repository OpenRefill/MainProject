#ifndef LOAD_CELLS_H_FILE
#define LOAD_CELLS_H_FILE

/*
load cells are not working, due to orientation error (and PCB <v3 needs level shifter 3V-5V)

code stored here for backup

===================================================================
platformui.ini
  bogde/HX711@^0.7.5

===================================================================
MAIN
#include <HX711.h>

// Load Cell Factors
float scale1CalibrationFactor = 1000; 
float scale2CalibrationFactor = 1000; 
float scale3CalibrationFactor = 1000; 
float scale4CalibrationFactor = 1000; 
// HX711 objects
HX711 scale1;
HX711 scale2;
HX711 scale3;
HX711 scale4;

static long get_product1_mass()
{

    if (scale1.wait_ready_timeout(5000))
    {  
      long reading= scale1.get_units(10);
      // Serial.print("Result 1: ");
      // LogInfo(reading);
      return reading;
    }
    else return 0;

}

static long get_product2_mass()
{ 

    if (scale2.wait_ready_timeout(5000))
    {  
      long reading= scale2.get_units(10);
      // Serial.print("Result 2: ");
      // LogInfo(reading);
      return reading;
    }
    else return 0;


}

static long get_product3_mass()
{

    if (scale3.wait_ready_timeout(5000))
    {  
      long reading= scale3.get_units(10);
      // LogInfo("Result 3: ");
      // LogInfo(reading);
      return reading;
    }
    else return 0;


}

static long get_product4_mass()
{

    if (scale4.wait_ready_timeout(5000))
    {  
      long reading= scale4.get_units(10);
      // LogInfo("Result 4: ");
      // LogInfo(reading);
      return reading;
    }
    else return 0;
}

void setup()
{
....
    scale1.begin(LOADCELL_DOUT_PIN1, LOADCELL_SCK_PIN);
    scale2.begin(LOADCELL_DOUT_PIN2, LOADCELL_SCK_PIN);
    scale3.begin(LOADCELL_DOUT_PIN3, LOADCELL_SCK_PIN);
    scale4.begin(LOADCELL_DOUT_PIN4, LOADCELL_SCK_PIN);

    scale1.set_scale(scale1CalibrationFactor);
    scale2.set_scale(scale2CalibrationFactor);
    scale3.set_scale(scale3CalibrationFactor);
    scale4.set_scale(scale4CalibrationFactor);
    
...

}

*/
#endif