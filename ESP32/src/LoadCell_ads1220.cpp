#include "LoadCell_ads1220.h"
#include "Main.h"

#ifdef USES_ADS1220

#include <SPI.h>
#include <ADS1220_WE.h>


static const int NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION = 1000;
static const float DEFAULT_VARIANCE_ESTIMATE = 0.2f * 0.2f;
static const float LOADCELL_VARIANCE_MIN = 7.0 * 1e-5; // on 8th april 2025, approx. 50g fluctuation were observed --> 6 * sigma = 50g --> sigma = 50g / 6 = 8g --> sigma^2 = (8g)^2 = 0.00064

float updatedConversionFactor_f64 = 1.0f;
#define CONVERSION_FACTOR LOADCELL_WEIGHT_RATING_KG / (LOADCELL_EXCITATION_V * (LOADCELL_SENSITIVITY_MV_V/1000.0f))
#define TIMEOUT_FOR_DRDY_TO_BECOME_LOW (uint32_t)2000
#define DELAY_IN_US_FOR_DRDY_TO_BECOME_LOW (uint32_t)20

// reference voltage in milli-volts
float refVoltageInMV_fl32 = 5000.0f;

// This flag will be set to true by the ISR
volatile bool newDataReady = false;

// This is our Interrupt Service Routine
void IRAM_ATTR drdyInterrupt() {
  newDataReady = true;
}


ADS1220_WE& ADC() {
  
  // Init custom SPI
  static SPIClass adsSPI(FSPI);  // Or use VSPI or HSPI for ESP32
  adsSPI.begin(FFB_ADS1220_SCLK, FFB_ADS1220_DOUT, FFB_ADS1220_DIN, FFB_ADS1220_CS);


  static ADS1220_WE adc(&adsSPI, FFB_ADS1220_CS, FFB_ADS1220_DRDY, true);
  
  //static ADS1220_WE adc(FFB_ADS1220_CS, FFB_ADS1220_DRDY);

  static bool firstTime = true;
  if (firstTime) {
    Serial.println("Starting ADC");  

    // Use custom SPI pins on ESP32-S3
    SPI.begin(FFB_ADS1220_SCLK, FFB_ADS1220_DOUT, FFB_ADS1220_DIN, FFB_ADS1220_CS);

    // Initialize ADS1220
    if (!adc.init()) {
      Serial.println("ADS1220 not found!");
      while (1);
    }

    // ADS1220 Configuration
    adc.setDataRate(ADS1220_DR_LVL_6);     // 2000SPS

    // PGA
    adc.setGain(ADS1220_GAIN_128);            // Gain for load cell

    // reference voltage
    adc.setVRefSource(ADS1220_VREF_AVDD_AVSS);
    //ads.setVRefValue_V(4.7f);    // set reference voltage in volts
    adc.setAvddAvssAsVrefAndCalibrate();

    float refVolt_fl32 = adc.getVRef_V();
    refVoltageInMV_fl32 = refVolt_fl32 * 1000.0f; // convert to mV
    Serial.print("Reference voltage: ");
    Serial.println(refVolt_fl32);

    // differential channels
    adc.setCompareChannels(ADS1220_MUX_0_1);              // Differential AIN0 - AIN1

    // set modulalar frequency
    adc.setOperatingMode(ADS1220_TURBO_MODE);

    // continous reading mode
    adc.setConversionMode(ADS1220_CONTINUOUS);  // Add this line in setup

    // set 50HZ and 60Hz FIR filter
    adc.setFIRFilter(ADS1220_50HZ_60HZ);

    // set 
    //adc.setDrdyMode(ADS1220_DOUT_DRDY);
    adc.setDrdyMode(ADS1220_DRDY);

    // assign interrupt to DRDY falling edge to make waiting more efficient
    attachInterrupt(digitalPinToInterrupt(FFB_ADS1220_DRDY), drdyInterrupt, FALLING);


    Serial.println("ADC Started");
    
    firstTime = false;
  }

  return adc;
}






LoadCell_ADS1220::LoadCell_ADS1220()
  : _zeroPoint(0.0f), _varianceEstimate(DEFAULT_VARIANCE_ESTIMATE)
{
  // differential channels
  ADC().setCompareChannels(ADS1220_MUX_0_1);              // Differential AIN0 - AIN1
}






void LoadCell_ADS1220::setLoadcellRating(uint8_t loadcellRating_u8) const {
  ADS1220_WE& adc = ADC();
  float originalConversionFactor_f64 = CONVERSION_FACTOR;
  
  updatedConversionFactor_f64 = 1.0f;
  if (LOADCELL_WEIGHT_RATING_KG>0)
  {
      float excitationVoltage = refVoltageInMV_fl32 / 1000.0f;
      float fullScale_mV = LOADCELL_SENSITIVITY_MV_V * excitationVoltage; // 2 mV/V * Vexc
      float loadcellRatingInGram_fl32 = (((float)loadcellRating_u8) * 1000.0f); // convert kg to gram
      float gramsPerMillivolt =  loadcellRatingInGram_fl32  / fullScale_mV;  // g per mV
      updatedConversionFactor_f64 = gramsPerMillivolt;
  }
}



float LoadCell_ADS1220::getReadingKg() const {
  ADS1220_WE& adc = ADC();
  unsigned int timeout_us = 0;//TIMEOUT_FOR_DRDY_TO_BECOME_LOW;
  bool timeoutReached_b = false;
  float voltage_mV = 0.0f;
  
  while(!newDataReady){
    if(timeout_us < TIMEOUT_FOR_DRDY_TO_BECOME_LOW){
      timeout_us += DELAY_IN_US_FOR_DRDY_TO_BECOME_LOW;  
      delayMicroseconds(DELAY_IN_US_FOR_DRDY_TO_BECOME_LOW);
    } else {
      timeoutReached_b = true;
    }
  }

  // read current voltage
  if (false == timeoutReached_b) {
    // Reset the flag immediately to be ready for the next conversion
    newDataReady = false;
    
    // Read the voltage from the ADS1220
    voltage_mV = adc.getVoltage_mV();
  }
  
  float weight_grams = voltage_mV * updatedConversionFactor_f64;

  float weight_kg = weight_grams / 1000.0f; // convert grams to kg
  
  // correct bias, assume AWGN --> 3 * sigma is 99.9 %
  return weight_kg - ( _zeroPoint + 3.0f * _standardDeviationEstimate );
}



void LoadCell_ADS1220::estimateBiasAndVariance() {
  ADS1220_WE& adc = ADC();
  
  Serial.println("Identify loadcell bias and variance");
  float varEstimate;
  float mean = 0.0f;
  float M2 = 0.0f;
  long n = 0;

  // capturer N measurements on do regressive mean and variance estimate
  // Use Welford-algorithm
  for (long i = 0; i < NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION; i++){
    float loadcellReading = getReadingKg();
    n++;
    float delta = loadcellReading - mean;
    mean += delta / n;
    M2 += delta * (loadcellReading - mean);
  }

  varEstimate = M2 / ((float)n - 1.0f); // empirical variance 
  // make sure estimate is nonzero
  if (varEstimate < LOADCELL_VARIANCE_MIN) { 
    varEstimate = LOADCELL_VARIANCE_MIN;
  }

  _zeroPoint = mean;
  _standardDeviationEstimate = sqrt(varEstimate);
  _varianceEstimate = varEstimate;

  Serial.print("Offset ");
  Serial.print(_zeroPoint, 5);
  Serial.println("kg");

  // Serial.print("Variance est.: ");
  // Serial.print(varEstimate, 5);
  // Serial.println("kg");

  Serial.print("Stddev. est.: ");
  Serial.print(_standardDeviationEstimate, 5);
  Serial.println("kg");
}

#endif



