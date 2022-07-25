#include "ADS1115-SOLDERED.h"
#include <LMP91000.h>
#include <LiquidCrystal_I2C.h>

LMP91000 LMP;
ADS1115 ADS;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// consts
// hardwired
const int LMP_address = 0x48;
const int ADC_address = 0x49;
const double amper_per_ppm = 6E-7;
const float LMP_ref = 3.3;
const float ext_gain = 22000.0;

// different setups
const int use_external_adc = 1;
const int ADC_mode = 1; // 0 -cont, 1 - single
const int ADC_gain = 2;  // 0- 6,144V , 1- 4.096V , 2- 2.048V , 4- 1.024V , 8- 0.512V, 16- 0.256V
const int LMP_gain = 0;  // 0- External resistor , 1- 2,75k , 2- 3.5k , 3- 7k , 4- 14k , 5- 35k , 6- 120k , 7 - 350k 
const int LMP_ref_per = 1; // 0- 20% , 1- 50% , 2- 67% , 3- bypassed

// globals
float LSB_size = 1.0;
float ADC_full_scale_range = 0.0;
int ADC_num_bites = 0;
    
void setup()
{     
    // open serial port
    Serial.begin(115200);
    Serial.println("Entering setup...\n");

    // open I2C
    Wire.begin();

    // setup LMP
    LMP.standby();
    delay(1000); //warm-up time for the gas sensor

    LMP.disableFET();
    LMP.setGain(LMP_gain);
    LMP.setRLoad(0);
    LMP.setExtRefSource();
    LMP.setIntZ(LMP_ref_per);
    LMP.setThreeLead();
    LMP.setNegBias();
    LMP.setBias(0);
  
    delay(500);
    Serial.println("LMP registers values:");
    Serial.println(readReg(LMP_address, 0x10), HEX);
    Serial.println(readReg(LMP_address, 0x11), HEX);
    Serial.println(readReg(LMP_address, 0x12), HEX);

    // setup ADC
    if(use_external_adc)
    {
      Serial.println("Using external ADC...");
      if(ADS.begin())
      {
        Serial.println("External ADC started!");
        ADS.setGain(ADC_gain); 
        ADS.setMode(ADC_mode); // 0 -cont, 1 - single
        ADS.setDataRate(0); //slowest sample rate
        

        ADC_full_scale_range = 2*ADS.getMaxVoltage(); // ADS has FSR of -ADS.getMaxVoltage to +ADS.getMaxVoltage
        ADC_num_bites = 16;
      }
    }
    else
    {
      ADC_full_scale_range = 4.3;
      ADC_num_bites = 10;
    }

    LSB_size = ADC_full_scale_range / pow(2, ADC_num_bites);

    delay(500);
    Serial.println("ADC registers values:");
    Serial.println(readReg2(ADC_address, 0x01), HEX);

    Serial.println("ADC LSB size(uV/bit): ");
    Serial.println(LSB_size * 1e6, 2);

    // print header
    Serial.println("ADC_value, Volts(mV),  Current(nA), PPM");

    //setup LCD

    lcd.begin(16,2);              
                                 
    lcd.backlight();
}

void loop()
{       
    // get ADC values
    int16_t ADC_value = 1;
    if(use_external_adc)
      ADC_value = ADS.readADC(0);
    else
      ADC_value = analogRead(A0);

    // ADC to volts
    double volts = ADC_value * LSB_size;

    // convert to current on sensor
    double current = LMP.getCurrent(ADC_value, ADC_full_scale_range, ADC_num_bites, ext_gain, true, LMP_ref);
    
    // calculate ppm
    double ppm = current / amper_per_ppm;
    
    
    // debug / output
    // ADC_value, Volts, Current(nA), PPM_value
    Serial.print(ADC_value);
    Serial.print("; ");
    Serial.print(volts*1e3, 2);
    Serial.print("; ");
    Serial.print(current*1e9, 2);
    Serial.print("; ");
    Serial.println(ppm);
        
    // loop delay
    delay(500);

    //LCD 
    lcd.setCursor(0,0);
    lcd.print("PPM:");
    lcd.setCursor(0,1);
    lcd.print(abs(ppm)); 
}

void writeReg(uint8_t address, uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readReg(uint8_t address, uint8_t reg)
{
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();

  int rv = Wire.requestFrom(address, (uint8_t) 1);
  if (rv == 1) 
  {
    uint16_t value = Wire.read();
    return value;
  }
  return 0x0000;
}

uint16_t readReg2(uint8_t address, uint8_t reg)
{
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();

  int rv = Wire.requestFrom(address, (uint8_t) 2);
  if (rv == 2) 
  {
    uint16_t value = Wire.read() << 8;
    value += Wire.read();
    return value;
  }
  return 0x0000;
}
