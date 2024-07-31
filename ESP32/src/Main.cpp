
/* Todo*/
// https://github.com/espressif/arduino-esp32/issues/7779


#define ESTIMATE_LOADCELL_VARIANCE
#define ISV_COMMUNICATION
//#define PRINT_SERVO_STATES

#define DEBUG_INFO_0_CYCLE_TIMER 1
#define DEBUG_INFO_0_STEPPER_POS 2
#define DEBUG_INFO_0_LOADCELL_READING 4
#define DEBUG_INFO_0_SERVO_READINGS 8
#define DEBUG_INFO_0_PRINT_ALL_SERVO_REGISTERS 16
#define DEBUG_INFO_0_STATE_BASIC_INFO_STRUCT 32
#define DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT 64
#define DEBUG_INFO_0_CONTROL_LOOP_ALGO 128

bool resetServoEncoder = true;
bool isv57LifeSignal_b = false;
#ifdef ISV_COMMUNICATION
  #include "isv57communication.h"
  int32_t servo_offset_compensation_steps_i32 = 0; 
#endif

#define OTA_update

#define PI 3.14159267
#define DEG_TO_RAD PI / 180

#include "Arduino.h"
#include "Main.h"

#ifdef Using_analog_output_ESP32_S3
#include <Wire.h>
#include <Adafruit_MCP4725.h>
  TwoWire MCP4725_I2C= TwoWire(1);
  //MCP4725 MCP(0x60, &MCP4725_I2C);
  Adafruit_MCP4725 dac;
  int current_use_mcp_index;
  bool MCP_status =false;
#endif



//#define ALLOW_SYSTEM_IDENTIFICATION

/**********************************************************************************************/
/*                                                                                            */
/*                         function declarations                                              */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters();
void pedalUpdateTask( void * pvParameters );
void serialCommunicationTask( void * pvParameters );
void servoCommunicationTask( void * pvParameters );
void OTATask( void * pvParameters );
void I2C_SyncTask( void * pvParameters);
void ESPNOW_SyncTask( void * pvParameters);
// https://www.tutorialspoint.com/cyclic-redundancy-check-crc-in-arduino
uint16_t checksumCalculator(uint8_t * data, uint16_t length)
{
   uint16_t curr_crc = 0x0000;
   uint8_t sum1 = (uint8_t) curr_crc;
   uint8_t sum2 = (uint8_t) (curr_crc >> 8);
   int index;
   for(index = 0; index < length; index = index+1)
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }
   return (sum2 << 8) | sum1;
}


bool systemIdentificationMode_b = false;

int16_t servoPos_i16 = 0;



bool splineDebug_b = false;



#include <EEPROM.h>



#include "ABSOscillation.h"
ABSOscillation absOscillation;
RPMOscillation _RPMOscillation;
BitePointOscillation _BitePointOscillation;
G_force_effect _G_force_effect;
WSOscillation _WSOscillation;
Road_impact_effect _Road_impact_effect;
Custom_vibration CV1;
Custom_vibration CV2;
Rudder _rudder;
#define ABS_OSCILLATION



#include "DiyActivePedal_types.h"
DAP_config_st dap_config_st;
DAP_calculationVariables_st dap_calculationVariables_st;
DAP_state_basic_st dap_state_basic_st;
DAP_state_extended_st dap_state_extended_st;


#include "CycleTimer.h"





#include "RTDebugOutput.h"


/**********************************************************************************************/
/*                                                                                            */
/*                         iterpolation  definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "ForceCurve.h"
ForceCurve_Interpolated forceCurve;



/**********************************************************************************************/
/*                                                                                            */
/*                         multitasking  definitions                                          */
/*                                                                                            */
/**********************************************************************************************/
#ifndef CONFIG_IDF_TARGET_ESP32S3
  #include "soc/rtc_wdt.h"
#endif

//#define PRINT_USED_STACK_SIZE
// https://stackoverflow.com/questions/55998078/freertos-task-priority-and-stack-size
#define STACK_SIZE_FOR_TASK_1 0.2 * (configTOTAL_HEAP_SIZE / 4)
#define STACK_SIZE_FOR_TASK_2 0.2 * (configTOTAL_HEAP_SIZE / 4)


TaskHandle_t Task1;
TaskHandle_t Task2;
#ifdef ISV_COMMUNICATION
  isv57communication isv57;
  #define STACK_SIZE_FOR_TASK_3 0.2 * (configTOTAL_HEAP_SIZE / 4) 
  TaskHandle_t Task3;
#endif

static SemaphoreHandle_t semaphore_updateConfig=NULL;
  bool configUpdateAvailable = false;                              // semaphore protected data
  DAP_config_st dap_config_st_local;

static SemaphoreHandle_t semaphore_updateJoystick=NULL;
  int32_t joystickNormalizedToInt32 = 0;                           // semaphore protected data

static SemaphoreHandle_t semaphore_resetServoPos=NULL;
bool resetPedalPosition = false;

static SemaphoreHandle_t semaphore_readServoValues=NULL;

static SemaphoreHandle_t semaphore_updatePedalStates=NULL;

/**********************************************************************************************/
/*                                                                                            */
/*                         target-specific  definitions                                       */
/*                                                                                            */
/**********************************************************************************************/




/**********************************************************************************************/
/*                                                                                            */
/*                         controller  definitions                                            */
/*                                                                                            */
/**********************************************************************************************/

#include "Controller.h"




/**********************************************************************************************/
/*                                                                                            */
/*                         pedal mechanics definitions                                        */
/*                                                                                            */
/**********************************************************************************************/

#include "PedalGeometry.h"


/**********************************************************************************************/
/*                                                                                            */
/*                         Kalman filter definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "SignalFilter.h"
KalmanFilter* kalman = NULL;


#include "SignalFilter_2nd_order.h"
KalmanFilter_2nd_order* kalman_2nd_order = NULL;




/**********************************************************************************************/
/*                                                                                            */
/*                         loadcell definitions                                               */
/*                                                                                            */
/**********************************************************************************************/

#include "LoadCell.h"
LoadCell_ADS1256* loadcell = NULL;



/**********************************************************************************************/
/*                                                                                            */
/*                         stepper motor definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "StepperWithLimits.h"
StepperWithLimits* stepper = NULL;
//static const int32_t MIN_STEPS = 5;

#include "StepperMovementStrategy.h"
/**********************************************************************************************/
/*                                                                                            */
/*                         OTA                                                                */
/*                                                                                            */
/**********************************************************************************************/
//OTA update
#ifdef OTA_update
#include "ota.h"
TaskHandle_t Task4;
#endif


//I2C sync
#ifdef Using_I2C_Sync
  
  #include "I2CSync.h"
  TaskHandle_t Task5;
#endif

//ESPNOW
#ifdef ESPNOW_Enable
  #include "ESPNOW_lib.h"
  TaskHandle_t Task6;
#endif



/**********************************************************************************************/
/*                                                                                            */
/*                         setup function                                                     */
/*                                                                                            */
/**********************************************************************************************/
void setup()
{
  //Serial.begin(115200);
  //Serial.begin(921600);
  //Serial.begin(512000);
  //
  

  #if PCB_VERSION == 6
    Serial.setTxTimeoutMs(0);
  #else
    Serial.begin(921600);
    Serial.setTimeout(5);
  #endif
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  
  // init controller
  SetupController();
  delay(3000);
  Serial.println("This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.");
  Serial.println("Please check github repo for more detail: https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal");

// check whether iSV57 communication can be established
// and in case, (a) send tuned servo parameters and (b) prepare the servo for signal read
#ifdef ISV_COMMUNICATION

  bool isv57slaveIdFound_b = isv57.findServosSlaveId();
  Serial.print("iSV57 slaveId found:  ");
  Serial.println( isv57slaveIdFound_b );

  if (!isv57slaveIdFound_b)
  {
    Serial.println( "Restarting ESP" );
    ESP.restart();
  }

  
  // check whether iSV57 is connected
  isv57LifeSignal_b = isv57.checkCommunication();
  if (!isv57LifeSignal_b)
  {
    Serial.println( "Restarting ESP" );
    ESP.restart();
  }


  Serial.print("iSV57 communication state:  ");
  Serial.println(isv57LifeSignal_b);

  if (isv57LifeSignal_b)
  {
    isv57.setupServoStateReading();
  	isv57.sendTunedServoParameters();
  }
  delay(200);
#endif


// initialize configuration and update local variables
  dap_config_st.initialiseDefaults();

  // Load config from EEPROM, if valid, overwrite initial config
  EEPROM.begin(2048);
  dap_config_st.loadConfigFromEprom(dap_config_st_local);


  // check validity of data from EEPROM  
  bool structChecker = true;
  uint16_t crc;
  if ( dap_config_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG ){ 
    structChecker = false;
    /*Serial.print("Payload type expected: ");
    Serial.print(DAP_PAYLOAD_TYPE_CONFIG);
    Serial.print(",   Payload type received: ");
    Serial.println(dap_config_st_local.payLoadHeader_.payloadType);*/
  }
  if ( dap_config_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
    structChecker = false;
    /*Serial.print("Config version expected: ");
    Serial.print(DAP_VERSION_CONFIG);
    Serial.print(",   Config version received: ");
    Serial.println(dap_config_st_local.payLoadHeader_.version);*/
  }
  // checksum validation
  crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
  if (crc != dap_config_st_local.payloadFooter_.checkSum){ 
    structChecker = false;
    /*Serial.print("CRC expected: ");
    Serial.print(crc);
    Serial.print(",   CRC received: ");
    Serial.println(dap_config_st_local.payloadFooter_.checkSum);*/
  }






  // if checks are successfull, overwrite global configuration struct
  if (structChecker == true)
  {
    Serial.println("Updating pedal config from EEPROM");
    dap_config_st = dap_config_st_local;          
  }
  else
  {

    Serial.println("Couldn't load config from EPROM due to mismatch: ");

    Serial.print("Payload type expected: ");
    Serial.print(DAP_PAYLOAD_TYPE_CONFIG);
    Serial.print(",   Payload type received: ");
    Serial.println(dap_config_st_local.payLoadHeader_.payloadType);

    
    Serial.print("Target version: ");
    Serial.print(DAP_VERSION_CONFIG);
    Serial.print(",    Source version: ");
    Serial.println(dap_config_st_local.payLoadHeader_.version);

    Serial.print("CRC expected: ");
    Serial.print(crc);
    Serial.print(",   CRC received: ");
    Serial.println(dap_config_st_local.payloadFooter_.checkSum);

  }


  // interprete config values
  dap_calculationVariables_st.updateFromConfig(dap_config_st);



  bool invMotorDir = dap_config_st.payLoadPedalConfig_.invertMotorDirection_u8 > 0;
  stepper = new StepperWithLimits(stepPinStepper, dirPinStepper, minPin, maxPin, invMotorDir);
  loadcell = new LoadCell_ADS1256();

  loadcell->setLoadcellRating(dap_config_st.payLoadPedalConfig_.loadcell_rating);

  loadcell->setZeroPoint();
  #ifdef ESTIMATE_LOADCELL_VARIANCE
    loadcell->estimateVariance();       // automatically identify sensor noise for KF parameterization
  #endif

  // find the min & max endstops
  Serial.print("Start homing");
  if (isv57LifeSignal_b && SENSORLESS_HOMING)
  {
    
    stepper->findMinMaxSensorless(&isv57, dap_config_st);
  }
  else
  {
    stepper->findMinMaxEndstops();
  }

 
  Serial.print("Min Position is "); Serial.println(stepper->getLimitMin());
  Serial.print("Max Position is "); Serial.println(stepper->getLimitMax());


  // setup Kalman filter
  Serial.print("Given loadcell variance: ");
  Serial.println(loadcell->getVarianceEstimate());
  kalman = new KalmanFilter(loadcell->getVarianceEstimate());

  kalman_2nd_order = new KalmanFilter_2nd_order(1);
  








  

  

  // activate parameter update in first cycle
  configUpdateAvailable = true;
  // equalize pedal config for both tasks
  dap_config_st_local = dap_config_st;





  // setup multi tasking
  semaphore_updateJoystick = xSemaphoreCreateMutex();
  semaphore_updateConfig = xSemaphoreCreateMutex();
  semaphore_resetServoPos = xSemaphoreCreateMutex();
  semaphore_updatePedalStates = xSemaphoreCreateMutex();
  delay(10);


  if(semaphore_updateJoystick==NULL)
  {
    Serial.println("Could not create semaphore");
    ESP.restart();
  }
  if(semaphore_updateConfig==NULL)
  {
    Serial.println("Could not create semaphore");
    ESP.restart();
  }



  // print all servo registers
  if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_PRINT_ALL_SERVO_REGISTERS) 
  {
    if (isv57LifeSignal_b)
    {
      isv57.readAllServoParameters();
    }
  }
  
  


  disableCore0WDT();

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    pedalUpdateTask,   /* Task function. */
                    "pedalUpdateTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    //STACK_SIZE_FOR_TASK_1,
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(500);

  xTaskCreatePinnedToCore(
                    serialCommunicationTask,   
                    "serialCommunicationTask", 
                    10000,  
                    //STACK_SIZE_FOR_TASK_2,    
                    NULL,      
                    1,         
                    &Task2,    
                    0);     
  delay(500);

  #ifdef ISV_COMMUNICATION
    
    xTaskCreatePinnedToCore(
                      servoCommunicationTask,   
                      "servoCommunicationTask", 
                      10000,  
                      //STACK_SIZE_FOR_TASK_2,    
                      NULL,      
                      1,         
                      &Task3,    
                      0);     
    delay(500);
#endif



  //Serial.begin(115200);
  #ifdef OTA_update
  char* APhost;
    switch(dap_config_st.payLoadPedalConfig_.pedal_type)
    {
      case 0:
        APhost="FFBPedalClutch";
        break;
      case 1:
        APhost="FFBPedalBrake";
        break;
      case 2:
        APhost="FFBPedalGas";
        break;
      default:
        APhost="FFBPedal";
        break;        

    }
    if(dap_config_st.payLoadPedalConfig_.OTA_flag==1)
    {
      ota_wifi_initialize(APhost);
      xTaskCreatePinnedToCore(
                    OTATask,   
                    "OTATask", 
                    16000,  
                    //STACK_SIZE_FOR_TASK_2,    
                    NULL,      
                    1,         
                    &Task4,    
                    0);     
      delay(500);
    }
  #endif

  //MCP setup
  #ifdef Using_analog_output_ESP32_S3
    //Wire.begin(MCP_SDA,MCP_SCL,400000);
    MCP4725_I2C.begin(MCP_SDA,MCP_SCL,400000);
    uint8_t i2c_address[8]={0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67};
    int index_address=0;
    int found_address=0;
    int error;
    for(index_address=0;index_address<8;index_address++)
    {
      MCP4725_I2C.beginTransmission(i2c_address[index_address]);
      error = MCP4725_I2C.endTransmission();
      if (error == 0)
      {
        Serial.print("I2C device found at address");
        Serial.print(i2c_address[index_address]);
        Serial.println("  !");
        found_address=index_address;
        break;
        
      }
      else
      {
        Serial.print("try address");
        Serial.println(i2c_address[index_address]);
      }
    }
    
    if(dac.begin(i2c_address[found_address], &MCP4725_I2C)==false)
    {
      Serial.println("Couldn't find MCP, will not have analog output");
      MCP_status=false;
    }
    else
    {
      Serial.println("MCP founded");
      MCP_status=true;
      //MCP.begin();
    }
  #endif
  #ifdef Using_I2C_Sync
    dap_calculationVariables_st.Rudder_status=false;
      if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
      {
        I2C_initialize();

      }
      if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
      {
        I2C_initialize_slave();        
      }
      if(dap_config_st.payLoadPedalConfig_.pedal_type==0)
      {
          Serial.println("No sync with Clutch");
      }
      if(dap_config_st.payLoadPedalConfig_.pedal_type>2)
      {
          Serial.println("Please send a config in and try again");
      }
              // make the update as task
        xTaskCreatePinnedToCore(
                      I2C_SyncTask,   
                      "I2C_update_Task", 
                      3000,  
                      //STACK_SIZE_FOR_TASK_2,    
                      NULL,      
                      1,         
                      &Task5,    
                      0);     
        delay(500);

  #endif

  //enable ESP-NOW
  #ifdef ESPNOW_Enable
  
  if(dap_config_st.payLoadPedalConfig_.OTA_flag==0)
  {
    /*
    WiFi.mode(WIFI_MODE_STA);
    Serial.print("MAC Address:  ");  
    Serial.println(WiFi.macAddress());
    if(dap_config_st.payLoadPedalConfig_.pedal_type==0)
    {
      esp_wifi_set_mac(WIFI_IF_STA, &Clu_mac[0]);
    }
    if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
    {
      esp_wifi_set_mac(WIFI_IF_STA, &Brk_mac[0]);
    }
    if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
    {
      esp_wifi_set_mac(WIFI_IF_STA, &Gas_mac[0]);
    }
    delay(300);
    Serial.print("new MAC Address:  ");  
    Serial.println(WiFi.macAddress());
    ESPNow.init();
    Serial.println("wait for ESPNOW initialized");
    delay(10000);

    if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
    {
      Recv_mac=Gas_mac;      
    }

    if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
    {
      Recv_mac=Brk_mac;
    }
    if(ESPNow.add_peer(Recv_mac)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add peer");
    }
    else
    {
      ESPNOW_status=false;
      Serial.println("Fail to add peer");
    }
    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(OnSent);
    */
    xTaskCreatePinnedToCore(
                      ESPNOW_SyncTask,   
                      "ESPNOW_update_Task", 
                      3000,  
                      //STACK_SIZE_FOR_TASK_2,    
                      NULL,      
                      1,         
                      &Task6,    
                      0);     
    delay(500);
      
    
}
    

  #endif

  Serial.println("Setup end");
  
}




/**********************************************************************************************/
/*                                                                                            */
/*                         Calc update function                                               */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters()
{
  dap_calculationVariables_st.updateFromConfig(dap_config_st);
  dap_calculationVariables_st.updateEndstops(stepper->getLimitMin(), stepper->getLimitMax());
  stepper->updatePedalMinMaxPos(dap_config_st.payLoadPedalConfig_.pedalStartPosition, dap_config_st.payLoadPedalConfig_.pedalEndPosition);
  //stepper->findMinMaxLimits(dap_config_st.payLoadPedalConfig_.pedalStartPosition, dap_config_st.payLoadPedalConfig_.pedalEndPosition);
  dap_calculationVariables_st.updateStiffness();

  // tune the PID settings
  tunePidValues(dap_config_st);

  // equalize pedal config for both tasks
  dap_config_st_local = dap_config_st;
}



/**********************************************************************************************/
/*                                                                                            */
/*                         Main function                                                      */
/*                                                                                            */
/**********************************************************************************************/
void loop() {
  taskYIELD();
  /*
  #ifdef OTA_update
  server.handleClient();
  //delay(1);
  #endif
  */
  
  
}


/**********************************************************************************************/
/*                                                                                            */
/*                         pedal update task                                                  */
/*                                                                                            */
/**********************************************************************************************/


//long lastCallTime = micros();
unsigned long cycleTimeLastCall = micros();
unsigned long minCyclesForFirToInit = 1000;
unsigned long firCycleIncrementer = 0;

float filteredReading_exp_filter = 0;
unsigned long printCycleCounter = 0;


uint printCntr = 0;
//void loop()
void pedalUpdateTask( void * pvParameters )
{

  for(;;){


    // system identification mode
    #ifdef ALLOW_SYSTEM_IDENTIFICATION
      if (systemIdentificationMode_b == true)
      {
        measureStepResponse(stepper, &dap_calculationVariables_st, &dap_config_st, loadcell);
        systemIdentificationMode_b = false;
      }
    #endif
    

    // controll cycle time. Delay did not work with the multi tasking, thus this workaround was integrated
    unsigned long now = micros();
    if (now - cycleTimeLastCall < PUT_TARGET_CYCLE_TIME_IN_US) // 100us = 10kHz
    {
      // skip 
      continue;
    }
    {
      // if target cycle time is reached, update last time
      cycleTimeLastCall = now;
    }

    

    // print the execution time averaged over multiple cycles
    if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
    {
      static CycleTimer timerPU("PU cycle time");
      timerPU.Bump();
    }
      

    // if a config update was received over serial, update the variables required for further computation
    if (configUpdateAvailable == true)
    {
      if(semaphore_updateConfig!=NULL)
      {

        bool configWasUpdated_b = false;
        // Take the semaphore and just update the config file, then release the semaphore
        if(xSemaphoreTake(semaphore_updateConfig, (TickType_t)1)==pdTRUE)
        {
          Serial.println("Updating pedal config");
          configUpdateAvailable = false;
          dap_config_st = dap_config_st_local;
          configWasUpdated_b = true;
          xSemaphoreGive(semaphore_updateConfig);
        }

        // update the calc params
        if (true == configWasUpdated_b)
        {
          Serial.println("Updating the calc params");
          configWasUpdated_b = false;

          if (true == dap_config_st.payLoadHeader_.storeToEeprom)
          {
            dap_config_st.payLoadHeader_.storeToEeprom = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
            uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
            dap_config_st.payloadFooter_.checkSum = crc;
            dap_config_st.storeConfigToEprom(dap_config_st); // store config to EEPROM
          }
          
          updatePedalCalcParameters(); // update the calc parameters
        }

      }
      else
      {
        semaphore_updateConfig = xSemaphoreCreateMutex();
        //Serial.println("semaphore_updateConfig == 0");
      }
    }



    // if reset pedal position was requested, reset pedal now
    // This function is implemented, so that in case of lost steps, the user can request a reset of the pedal psotion
    if (resetPedalPosition) {

      if (isv57LifeSignal_b && SENSORLESS_HOMING)
      {
        stepper->refindMinLimitSensorless(&isv57);
      }
      else
      {
        stepper->refindMinLimit();
      }
      
      resetPedalPosition = false;
      resetServoEncoder = true;
    }


    //#define RECALIBRATE_POSITION
    #ifdef RECALIBRATE_POSITION
      stepper->checkLimitsAndResetIfNecessary();
    #endif


      // compute pedal oscillation, when ABS is active
    float absForceOffset_fl32 = 0.0f;

    float absForceOffset = 0;
    float absPosOffset = 0;
    #ifdef ABS_OSCILLATION
      absOscillation.forceOffset(&dap_calculationVariables_st, dap_config_st.payLoadPedalConfig_.absPattern, dap_config_st.payLoadPedalConfig_.absForceOrTarvelBit, &absForceOffset, &absPosOffset);
      _RPMOscillation.trigger();
      _RPMOscillation.forceOffset(&dap_calculationVariables_st);
      _BitePointOscillation.forceOffset(&dap_calculationVariables_st);
      _G_force_effect.forceOffset(&dap_calculationVariables_st, dap_config_st.payLoadPedalConfig_.G_multi);
      _WSOscillation.forceOffset(&dap_calculationVariables_st);
      _Road_impact_effect.forceOffset(&dap_calculationVariables_st, dap_config_st.payLoadPedalConfig_.Road_multi);
      CV1.forceOffset(dap_config_st.payLoadPedalConfig_.CV_freq_1,dap_config_st.payLoadPedalConfig_.CV_amp_1);
      CV2.forceOffset(dap_config_st.payLoadPedalConfig_.CV_freq_2,dap_config_st.payLoadPedalConfig_.CV_amp_2);
      //_rudder.offset_calculate(&dap_calculationVariables_st);
      _rudder.force_offset_calculate(&dap_calculationVariables_st);
    #endif

    //update max force with G force effect
    movingAverageFilter.dataPointsCount=dap_config_st.payLoadPedalConfig_.G_window;
    movingAverageFilter_roadimpact.dataPointsCount=dap_config_st.payLoadPedalConfig_.Road_window;
    dap_calculationVariables_st.reset_maxforce();
    dap_calculationVariables_st.Force_Max+=_G_force_effect.G_force;
    dap_calculationVariables_st.Force_Max+=_Road_impact_effect.Road_Impact_force;
    dap_calculationVariables_st.dynamic_update();
    dap_calculationVariables_st.updateStiffness();

    // compute the pedal incline angle 
    //#define COMPUTE_PEDAL_INCLINE_ANGLE
    #ifdef COMPUTE_PEDAL_INCLINE_ANGLE
      float sledPosition = sledPositionInMM(stepper);
      float pedalInclineAngle = pedalInclineAngleDeg(sledPosition, dap_config_st);

      // compute angular velocity & acceleration of incline angke
      float pedalInclineAngle_Accel = pedalInclineAngleAccel(pedalInclineAngle);

      //float legRotationalMoment = 0.0000001;
      //float forceCorrection = pedalInclineAngle_Accel * legRotationalMoment;

      //Serial.print(pedalInclineAngle_Accel);
      //Serial.println(" ");

    #endif


    // Get the loadcell reading
    float loadcellReading = loadcell->getReadingKg();

    // Invert the loadcell reading digitally if desired
    if (dap_config_st.payLoadPedalConfig_.invertLoadcellReading_u8 == 1)
    {
      loadcellReading *= -1;
    }


    // Convert loadcell reading to pedal force
    float sledPosition = sledPositionInMM(stepper, dap_config_st);
    float pedalInclineAngleInDeg_fl32 = pedalInclineAngleDeg(sledPosition, dap_config_st);
    float pedalForce_fl32 = convertToPedalForce(loadcellReading, sledPosition, dap_config_st);
    float d_phi_d_x = convertToPedalForceGain(sledPosition, dap_config_st);

    // compute gain for horizontal foot model
    float b = dap_config_st.payLoadPedalConfig_.lengthPedal_b;
    float d = dap_config_st.payLoadPedalConfig_.lengthPedal_d;
    float d_x_hor_d_phi = -(b+d) * sinf(pedalInclineAngleInDeg_fl32 * DEG_TO_RAD);

    /*printCntr++;
    if (printCntr >= 100) 
    {
      
      Serial.print("MPC_0th_order_gain: ");
      Serial.print(dap_config_st.payLoadPedalConfig_.MPC_0th_order_gain);
      printCntr = 0;
    }*/


    /*printCntr++;
    if (printCntr >= 100) 
    {
      Serial.print("Angle: ");
      Serial.print(pedalInclineAngleInDeg_fl32);

      Serial.print(",   sin(angle): ");
      Serial.print( sinf(pedalInclineAngleInDeg_fl32 * DEG_TO_RAD) );

      Serial.print(",   d_phi_d_x: ");
      Serial.print( d_phi_d_x );

      Serial.print(",   d_x_hor_d_phi: ");
      Serial.print( d_x_hor_d_phi );

      //Serial.print(",   a: ");
      //Serial.print( dap_config_st.payLoadPedalConfig_.lengthPedal_a );

      //Serial.print(",   b: ");
      //Serial.print( dap_config_st.payLoadPedalConfig_.lengthPedal_b );

      //Serial.print(",   c_ver: ");
      //Serial.print( dap_config_st.payLoadPedalConfig_.lengthPedal_c_vertical );

      //Serial.print(",   c_ver: ");
      //Serial.print( dap_config_st.payLoadPedalConfig_.lengthPedal_c_horizontal );

      //Serial.print(",   d: ");
      //Serial.print( dap_config_st.payLoadPedalConfig_.lengthPedal_d );

      Serial.println();
      printCntr = 0;
    }*/
    
    



    // Do the loadcell signal filtering
    float filteredReading = 0;
    float changeVelocity = 0;

    // const velocity model denoising filter
    if (dap_config_st.payLoadPedalConfig_.kf_modelOrder == 0)
    {
      filteredReading = kalman->filteredValue(pedalForce_fl32, 0, dap_config_st.payLoadPedalConfig_.kf_modelNoise);
      changeVelocity = kalman->changeVelocity();
    }

    // const acceleration model denoising filter
    if (dap_config_st.payLoadPedalConfig_.kf_modelOrder == 1)
    {
      filteredReading = kalman_2nd_order->filteredValue(pedalForce_fl32, 0, dap_config_st.payLoadPedalConfig_.kf_modelNoise);
      changeVelocity = kalman->changeVelocity();
    }

    // exponential denoising filter
    if (dap_config_st.payLoadPedalConfig_.kf_modelOrder == 2)
    {
      float alpha_exp_filter = 1.0f - ( (float)dap_config_st.payLoadPedalConfig_.kf_modelNoise) / 5000.0f;
      float filteredReading_exp_filter = filteredReading_exp_filter * alpha_exp_filter + pedalForce_fl32 * (1.0-alpha_exp_filter);
      filteredReading = filteredReading_exp_filter;
    }

    //filteredReading=constrain(filteredReading, dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max_default);

    


    // Do position state estimate
    float stepper_pos_filtered_fl32 = 0;//kalman_2nd_order->filteredValue(stepper->getCurrentPosition(), 0, dap_config_st.payLoadPedalConfig_.kf_modelNoise);
    float stepper_vel_filtered_fl32 = 0;//kalman_2nd_order->changeVelocity();
    float stepper_accel_filtered_fl32 = 0;//kalman_2nd_order->changeAccel();


    /*Serial.print(stepper->getCurrentPosition());
    Serial.print(",   ");
    Serial.print(stepper_pos_filtered_fl32);
    Serial.print(",   ");
    Serial.print(stepper_vel_filtered_fl32);
    Serial.println("   ");*/

    

    

    //#define DEBUG_FILTER
    if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_LOADCELL_READING) 
    {
      static RTDebugOutput<float, 3> rtDebugFilter({ "rawReading_g", "pedalForce_fl32", "filtered_g"});
      rtDebugFilter.offerData({ loadcellReading * 1000, pedalForce_fl32*1000, filteredReading * 1000});
    }
      

    /*#ifdef ABS_OSCILLATION
      filteredReading += forceAbsOffset;
    #endif*/


    //Add effect by force
    float effect_force=absForceOffset+ _BitePointOscillation.BitePoint_Force_offset+_WSOscillation.WS_Force_offset+CV1.CV_Force_offset+CV2.CV_Force_offset;
    effect_force=effect_force+_rudder.force_offset_filter;
    // use interpolation to determine local linearized spring stiffness
    double stepperPosFraction = stepper->getCurrentPositionFraction();
    int32_t Position_Next = 0;

    //Position_Next=_rudder.offset_filter;
    
    


    

    

    // select control loop algo
    if (dap_config_st.payLoadPedalConfig_.control_strategy_b <= 1)
    {
      Position_Next = MoveByPidStrategy(filteredReading, stepperPosFraction, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_st, effect_force, changeVelocity);
    }
       
    if (dap_config_st.payLoadPedalConfig_.control_strategy_b == 2) 
    {
      Position_Next = MoveByForceTargetingStrategy(filteredReading, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_st, effect_force, changeVelocity, stepper_vel_filtered_fl32, stepper_accel_filtered_fl32, d_phi_d_x, d_x_hor_d_phi);
    }

    //add rudder

    
    
    


    //#define DEBUG_STEPPER_POS
    if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STEPPER_POS) 
    {
      static RTDebugOutput<int32_t, 5> rtDebugFilter({ "ESP_pos", "ESP_tar_pos", "ISV_pos", "frac1"});
      rtDebugFilter.offerData({ stepper->getCurrentPositionFromMin(), Position_Next, -(int32_t)(isv57.servo_pos_given_p + isv57.servo_pos_error_p - isv57.getZeroPos()), (int32_t)(stepperPosFraction * 10000.)});
    }

    
    //stepper->printStates();
    

    // add dampening
    if (dap_calculationVariables_st.dampingPress  > 0.0001)
    {
      // dampening is proportional to velocity --> D-gain for stability
      Position_Next -= dap_calculationVariables_st.dampingPress * changeVelocity * dap_calculationVariables_st.springStiffnesssInv;
    }
      


    // clip target position to configured target interval with RPM effect movement in the endstop
    Position_Next = (int32_t)constrain(Position_Next, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax);

  
    //Adding effects
    Position_Next +=_RPMOscillation.RPM_position_offset;
    Position_Next +=absPosOffset;
    Position_Next = (int32_t)constrain(Position_Next, dap_calculationVariables_st.stepperPosMinEndstop, dap_calculationVariables_st.stepperPosMaxEndstop);
    
    dap_calculationVariables_st.current_pedal_position=Position_Next;
    //bitepoint trigger

    int32_t BP_trigger_value=dap_config_st.payLoadPedalConfig_.BP_trigger_value;
    int32_t BP_trigger_min=(BP_trigger_value-4);
    int32_t BP_trigger_max=(BP_trigger_value+4);
    int32_t Position_check=100*((Position_Next-dap_calculationVariables_st.stepperPosMin) / dap_calculationVariables_st.stepperPosRange);
    //Serial.println(Position_check);
    if(dap_config_st.payLoadPedalConfig_.BP_trigger==1)
    {
      if(Position_check > BP_trigger_min)
      {
        if(Position_check < BP_trigger_max)
        {
          _BitePointOscillation.trigger();
        }
      }
    }

    // if pedal in min position, recalibrate position 
    #ifdef ISV_COMMUNICATION
    // Take the semaphore and just update the config file, then release the semaphore
        if(xSemaphoreTake(semaphore_resetServoPos, (TickType_t)1)==pdTRUE)
        {
          if (stepper->isAtMinPos())
          {
            stepper->correctPos(servo_offset_compensation_steps_i32);
            servo_offset_compensation_steps_i32 = 0;
          }
          xSemaphoreGive(semaphore_resetServoPos);
        }
    #endif



    // get current stepper position right before sheduling a new move
    //int32_t stepperPosCurrent = stepper->getCurrentPositionFromMin();
    //int32_t stepperPosCurrent = stepper->getTargetPositionSteps();
    //int32_t movement = abs(stepperPosCurrent - Position_Next);
    //if (movement > MIN_STEPS)
    {
      stepper->moveTo(Position_Next, false);
    }



   

    

    // compute controller output
    dap_calculationVariables_st.reset_maxforce();
    dap_calculationVariables_st.dynamic_update();
    dap_calculationVariables_st.updateStiffness();
    if(semaphore_updateJoystick!=NULL)
    {
      if(xSemaphoreTake(semaphore_updateJoystick, (TickType_t)1)==pdTRUE) {

        if (1 == dap_config_st.payLoadPedalConfig_.travelAsJoystickOutput_u8)
        {
          joystickNormalizedToInt32 = NormalizeControllerOutputValue(Position_Next, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax, dap_config_st.payLoadPedalConfig_.maxGameOutput);
        }
        else
        {
          //joystickNormalizedToInt32 = NormalizeControllerOutputValue(loadcellReading, dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_st.payLoadPedalConfig_.maxGameOutput);
          
          joystickNormalizedToInt32 = NormalizeControllerOutputValue(filteredReading, dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_st.payLoadPedalConfig_.maxGameOutput);
        }
        
        xSemaphoreGive(semaphore_updateJoystick);
      }
    }
    else
    {
      semaphore_updateJoystick = xSemaphoreCreateMutex();
      //Serial.println("semaphore_updateJoystick == 0");
    }

    // provide joystick output on PIN
    #ifdef Using_analog_output
      int dac_value=(int)(joystickNormalizedToInt32*255/10000);
      dacWrite(D_O,dac_value);
    #endif

    #ifdef Using_analog_output_ESP32_S3
      if(MCP_status)
      {
        int dac_value=(int)(joystickNormalizedToInt32*4096*0.9/10000);//limit the max to 5V*0.9=4.5V to prevent the overvolatage
        dac.setVoltage(dac_value, false);
      }
    #endif

    
    float normalizedPedalReading_fl32 = 0;
    if ( fabs(dap_calculationVariables_st.Force_Range) > 0.01)
    {
        normalizedPedalReading_fl32 = constrain((filteredReading - dap_calculationVariables_st.Force_Min) / dap_calculationVariables_st.Force_Range, 0, 1);
    }
    
    // simulate ABS trigger 
    if(dap_config_st.payLoadPedalConfig_.Simulate_ABS_trigger==1)
    {
      int32_t ABS_trigger_value=dap_config_st.payLoadPedalConfig_.Simulate_ABS_value;
      if( (normalizedPedalReading_fl32*100) > ABS_trigger_value)
      {
        absOscillation.trigger();
      }
    }



    // update pedal states
    if(semaphore_updatePedalStates!=NULL)
    {
      if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)1)==pdTRUE) 
      {
        
        // update basic pedal state struct
        dap_state_basic_st.payloadPedalState_Basic_.pedalForce_u16 =  normalizedPedalReading_fl32 * 65535;
        dap_state_basic_st.payloadPedalState_Basic_.pedalPosition_u16 = constrain(stepperPosFraction, 0, 1) * 65535;
        dap_state_basic_st.payloadPedalState_Basic_.joystickOutput_u16 = (float)joystickNormalizedToInt32 / 10000. * 32767.0;//65535;

        dap_state_basic_st.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_BASIC;
        dap_state_basic_st.payLoadHeader_.version = DAP_VERSION_CONFIG;
        dap_state_basic_st.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_basic_st.payLoadHeader_)), sizeof(dap_state_basic_st.payLoadHeader_) + sizeof(dap_state_basic_st.payloadPedalState_Basic_));


        // update extended struct 
        dap_state_extended_st.payloadPedalState_Extended_.timeInMs_u32 = millis();
        dap_state_extended_st.payloadPedalState_Extended_.pedalForce_raw_fl32 =  loadcellReading;
        dap_state_extended_st.payloadPedalState_Extended_.pedalForce_filtered_fl32 =  filteredReading;
        dap_state_extended_st.payloadPedalState_Extended_.forceVel_est_fl32 =  changeVelocity;

        if(semaphore_readServoValues!=NULL)
        {
          if(xSemaphoreTake(semaphore_readServoValues, (TickType_t)1)==pdTRUE) {
            dap_state_extended_st.payloadPedalState_Extended_.servoPosition_i16 = servoPos_i16;
            dap_state_extended_st.payloadPedalState_Extended_.servo_voltage_0p1V =  isv57.servo_voltage_0p1V;
            dap_state_extended_st.payloadPedalState_Extended_.servo_current_percent_i16 = isv57.servo_current_percent;
            
            xSemaphoreGive(semaphore_readServoValues);
          }
        }
        else
        {
          semaphore_readServoValues = xSemaphoreCreateMutex();
        }

        dap_state_extended_st.payloadPedalState_Extended_.servoPositionTarget_i16 = stepper->getCurrentPositionFromMin();
        dap_state_extended_st.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_EXTENDED;
        dap_state_extended_st.payLoadHeader_.version = DAP_VERSION_CONFIG;
        dap_state_extended_st.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_extended_st.payLoadHeader_)), sizeof(dap_state_extended_st.payLoadHeader_) + sizeof(dap_state_extended_st.payloadPedalState_Extended_));

        // release semaphore
        xSemaphoreGive(semaphore_updatePedalStates);
      }
    }
    else
    {
      semaphore_updatePedalStates = xSemaphoreCreateMutex();
    }
    
    
    


    


    #ifdef PRINT_USED_STACK_SIZE
      unsigned int temp2 = uxTaskGetStackHighWaterMark(nullptr);
      Serial.print("PU task stack size="); Serial.println(temp2);
    #endif
    /*
    #ifdef OTA_update
    server.handleClient();
    //delay(1);
    #endif
    */

  }
}

  








/**********************************************************************************************/
/*                                                                                            */
/*                         communication task                                                 */
/*                                                                                            */
/**********************************************************************************************/





int32_t joystickNormalizedToInt32_local = 0;
void serialCommunicationTask( void * pvParameters )
{

  for(;;){

    // average cycle time averaged over multiple cycles 
    if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
    {
      static CycleTimer timerSC("SC cycle time");
      timerSC.Bump();
    }

    uint16_t crc;




    delay( SERIAL_COOMUNICATION_TASK_DELAY_IN_MS );

   
    { 
      // read serial input 
      uint8_t n = Serial.available();

      bool structChecker = true;
      
      if (n > 0)
      {
        switch (n) {

          // likely config structure 
          case sizeof(DAP_config_st):
              
              if(semaphore_updateConfig!=NULL)
              {
                if(xSemaphoreTake(semaphore_updateConfig, (TickType_t)1)==pdTRUE)
                {
                  DAP_config_st * dap_config_st_local_ptr;
                  dap_config_st_local_ptr = &dap_config_st_local;
                  Serial.readBytes((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));

                  

                  // check if data is plausible
                  
                  if ( dap_config_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG ){ 
                    structChecker = false;
                    Serial.print("Payload type expected: ");
                    Serial.print(DAP_PAYLOAD_TYPE_CONFIG);
                    Serial.print(",   Payload type received: ");
                    Serial.println(dap_config_st_local.payLoadHeader_.payloadType);
                  }
                  if ( dap_config_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
                    structChecker = false;
                    Serial.print("Config version expected: ");
                    Serial.print(DAP_VERSION_CONFIG);
                    Serial.print(",   Config version received: ");
                    Serial.println(dap_config_st_local.payLoadHeader_.version);
                  }
                  // checksum validation
                  crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
                  if (crc != dap_config_st_local.payloadFooter_.checkSum){ 
                    structChecker = false;
                    Serial.print("CRC expected: ");
                    Serial.print(crc);
                    Serial.print(",   CRC received: ");
                    Serial.println(dap_config_st_local.payloadFooter_.checkSum);
                  }


                  // if checks are successfull, overwrite global configuration struct
                  if (structChecker == true)
                  {
                    Serial.println("Updating pedal config");
                    configUpdateAvailable = true;          
                  }
                  xSemaphoreGive(semaphore_updateConfig);
                }
              }
            break;

          // likely action structure 
          case sizeof(DAP_actions_st) :

            DAP_actions_st dap_actions_st;
            Serial.readBytes((char*)&dap_actions_st, sizeof(DAP_actions_st));

            if ( dap_actions_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ACTION ){ 
              structChecker = false;
              Serial.print("Payload type expected: ");
              Serial.print(DAP_PAYLOAD_TYPE_ACTION);
              Serial.print(",   Payload type received: ");
              Serial.println(dap_config_st_local.payLoadHeader_.payloadType);
            }
            if ( dap_actions_st.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
              structChecker = false;
              Serial.print("Config version expected: ");
              Serial.print(DAP_VERSION_CONFIG);
              Serial.print(",   Config version received: ");
              Serial.println(dap_config_st_local.payLoadHeader_.version);
            }
            crc = checksumCalculator((uint8_t*)(&(dap_actions_st.payLoadHeader_)), sizeof(dap_actions_st.payLoadHeader_) + sizeof(dap_actions_st.payloadPedalAction_));
            if (crc != dap_actions_st.payloadFooter_.checkSum){ 
              structChecker = false;
              Serial.print("CRC expected: ");
              Serial.print(crc);
              Serial.print(",   CRC received: ");
              Serial.println(dap_actions_st.payloadFooter_.checkSum);
            }



            if (structChecker == true)
            {

              // trigger reset pedal position
              if (dap_actions_st.payloadPedalAction_.resetPedalPos_u8)
              {
                resetPedalPosition = true;
              }

              // trigger ABS effect
              if (dap_actions_st.payloadPedalAction_.triggerAbs_u8)
              {
                absOscillation.trigger();
              }
              //RPM effect
              _RPMOscillation.RPM_value=dap_actions_st.payloadPedalAction_.RPM_u8;
              //G force effect
              _G_force_effect.G_value=dap_actions_st.payloadPedalAction_.G_value-128;       
              //wheel slip
              if (dap_actions_st.payloadPedalAction_.WS_u8)
              {
                _WSOscillation.trigger();
              }     
              //Road impact
              _Road_impact_effect.Road_Impact_value=dap_actions_st.payloadPedalAction_.impact_value_u8;
              // trigger system identification
              if (dap_actions_st.payloadPedalAction_.startSystemIdentification_u8)
              {
                systemIdentificationMode_b = true;
              }
              // trigger Custom effect effect 1
              if (dap_actions_st.payloadPedalAction_.Trigger_CV_1)
              {
                CV1.trigger();
              }
              // trigger Custom effect effect 2
              if (dap_actions_st.payloadPedalAction_.Trigger_CV_2)
              {
                CV2.trigger();
              }
              // trigger return pedal position
              if (dap_actions_st.payloadPedalAction_.returnPedalConfig_u8)
              {
                DAP_config_st * dap_config_st_local_ptr;
                dap_config_st_local_ptr = &dap_config_st;
                //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
                crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
                dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
                Serial.write((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
                Serial.print("\r\n");
              }
              if(dap_actions_st.payloadPedalAction_.Rudder_action==1)
              {
                if(dap_calculationVariables_st.Rudder_status==false)
                {
                  dap_calculationVariables_st.Rudder_status=true;
                  Serial.println("Rudder on");
                  //Serial.print("status:");
                  //Serial.println(dap_calculationVariables_st.Rudder_status);
                }
                else
                {
                  dap_calculationVariables_st.Rudder_status=false;
                  Serial.println("Rudder off");
                  //Serial.print("status:");
                  //Serial.println(dap_calculationVariables_st.Rudder_status);
                }
              }


            }

            break;

          default:

            // flush the input buffer
            while (Serial.available()) Serial.read();
            //Serial.flush();

            Serial.println("\nIn byte size: ");
            Serial.println(n);
            Serial.println("    Exp config size: ");
            Serial.println(sizeof(DAP_config_st) );
            Serial.println("    Exp action size: ");
            Serial.println(sizeof(DAP_actions_st) );

            break;  


            

        }
      }


      // send pedal state structs
      // update pedal states
      printCycleCounter++;
      DAP_state_basic_st dap_state_basic_st_lcl;
      DAP_state_extended_st dap_state_extended_st_lcl;

      if(semaphore_updatePedalStates!=NULL)
      {
        
        if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)1)==pdTRUE) 
        {
        
          // UPDATE basic pedal state struct
          dap_state_basic_st_lcl = dap_state_basic_st;

          // UPDATE extended pedal state struct
          dap_state_extended_st_lcl = dap_state_extended_st;
            
          // release semaphore
          xSemaphoreGive(semaphore_updatePedalStates);

        }
      }
      else
      {
        semaphore_updatePedalStates = xSemaphoreCreateMutex();
      }



      // send the pedal state structs
      // send basic pedal state struct
      if ( !(dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_BASIC_INFO_STRUCT) )
      {
        if (printCycleCounter >= 2)
        {
          printCycleCounter = 0;
          Serial.write((char*)&dap_state_basic_st_lcl, sizeof(DAP_state_basic_st));
          Serial.print("\r\n");
        }
      }

      if ( (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT) )
      {
        Serial.write((char*)&dap_state_extended_st_lcl, sizeof(DAP_state_extended_st));
        Serial.print("\r\n");
      }


      // wait until transmission is finished
      // flush argument = true, do not clear Rx buffer
      //Serial.flush();
      //Serial.flush(true);

    }

    // transmit controller output
    //Serial.print("Joy 1");
    delay( SERIAL_COOMUNICATION_TASK_DELAY_IN_MS );

    //Serial.print(" 2");
    if(semaphore_updateJoystick!=NULL)
    {
      if(xSemaphoreTake(semaphore_updateJoystick, (TickType_t)1)==pdTRUE)
      {
        //Serial.print(" 3");
        joystickNormalizedToInt32_local = joystickNormalizedToInt32;
        xSemaphoreGive(semaphore_updateJoystick);
      }
    }


    if (IsControllerReady()) 
    {
      //Serial.print(" 4");
      //Serial.print("\r\n");
      SetControllerOutputValue(joystickNormalizedToInt32_local);
    }



     // send pedal state to esp master
    if(ESPNOW_status==false)
    {
      ESPNow_initialize();
    }
    else
    {
      sendMessageToMaster(joystickNormalizedToInt32_local);
    }


  /*#ifdef SERIAL_TIMEOUT
    delay(10);
  #endif
*/

  }
}
//OTA multitask
void OTATask( void * pvParameters )
{

  for(;;)
  {
    #ifdef OTA_update
    server.handleClient();
    //delay(1);
    #endif
  }
}

//pedal I2C multitask
#ifdef Using_I2C_Sync
int I2C_count=0;
int error_count=0;
int print_count=0;
uint8_t error_out;
void I2C_SyncTask( void * pvParameters )
{

  for(;;)
  {
    //Serial.println("syncing");
    if(dap_calculationVariables_st.Rudder_status)
    {
      //Serial.println("Rudder status:1");
      if(I2C_count>100)
      {
        if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
        {
          //Serial.println("syncing");

    
          Wire.beginTransmission((uint8_t)I2C_slave_address);
          //I2C_sync.endTransmission();
          error_out=Wire.endTransmission();
          if(error_out==0)
          {
            //Serial.print("Start get I2C sync");
            Wire.beginTransmission((uint8_t)I2C_slave_address);          
            //uint8_t Pedal_position_u8=(uint8_t)(dap_state_basic_st.payloadPedalState_Basic_.pedalPosition_u16/65535*256);
            //Wire.write(Pedal_position_u8);
            I2C_writeAnything(dap_state_basic_st.payloadPedalState_Basic_.pedalPosition_u16);
            delay(1);
            Wire.endTransmission();
            //Serial.println(Pedal_position_u8);
            //Serial.print("sended---------------Get--------------");
            
            uint8_t bytesReceived=Wire.requestFrom((uint8_t)I2C_slave_address, 2);
            //Serial.println(bytesReceived);
            delay(1);
            while(Wire.available())
            {
              //uint8_t temp=Wire.read();
              uint16_t temp=0;
              I2C_readAnything(temp);
              dap_calculationVariables_st.sync_pedal_position=temp;


            }
            /*
            if(print_count>300)
            {
              Serial.print("-------Read:----------");
              Serial.println(dap_calculationVariables_st.sync_pedal_position);
              print_count=0;
            }
            print_count++;
            */
            /*
            if ((bool)bytesReceived)
            {
              
              
              uint8_t temp[bytesReceived];
              I2C_sync.readBytes(temp, bytesReceived);
              log_print_buf(temp, bytesReceived);
              
            }
            */
            
            
            //I2C_sync.write(Pedal_position_u8);
            /*
            I2C_sync.printf("Hello World! %lu", iii++);
            delay(1);
            I2C_sync.endTransmission();
            uint8_t bytesReceived = I2C_sync.requestFrom((uint8_t)I2C_slave_address, 16);
            Serial.printf("requestFrom: %u\n", bytesReceived);
            if ((bool)bytesReceived) {  //If received more than zero bytes
              uint8_t temp[bytesReceived];
              I2C_sync.readBytes(temp, bytesReceived);
              log_print_buf(temp, bytesReceived);
            }
            */
            
            

            
          }

        }
        if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
        {
          
          if(I2C_data_read)
          {
            
            I2C_send=dap_state_basic_st.payloadPedalState_Basic_.pedalPosition_u16;
            dap_calculationVariables_st.sync_pedal_position=I2C_Read;
            I2C_data_read=false;
          }
          
         /*
          if(print_count>10)
          {
            Serial.print("Slave_Sync:");
            Serial.println(I2C_Read);
            print_count=0;
          }
          print_count++;
          */
            
          
        }
      
        
        
        I2C_count=0;
      }
      I2C_count=I2C_count+1;      
    }
    #ifdef I2C_debug_out
        if(print_count>30000000)
        {
          Serial.print("Rudder Status:");
          Serial.println(dap_calculationVariables_st.Rudder_status);
          Serial.print("Pedal type:");
          Serial.println(dap_config_st.payLoadPedalConfig_.pedal_type);
          Serial.print("---Sync Value--");
          Serial.println(dap_calculationVariables_st.sync_pedal_position);     
          Serial.print("---Send Value--");
          Serial.println(dap_calculationVariables_st.current_pedal_position);         
          if(error_out>0)
          {
            Serial.println("I2C failed");
          }
          
          
          print_count=0;
        }
        else
        {
          print_count++;
        } 
    #endif
    



  }
  
}
#endif

#ifdef ESPNOW_Enable
int ESPNOW_count=0;
int error_count=0;
int print_count=0;
uint8_t error_out;
void ESPNOW_SyncTask( void * pvParameters )
{
  for(;;)
  {
      if(ESPNOW_count>20)
      {
        if(dap_calculationVariables_st.Rudder_status)
        {
          if(ESPNow_initial_status==false)
          {
            ESPNow_initialize();
          }
          else
          {
            dap_calculationVariables_st.current_pedal_position_ratio=((float)(dap_calculationVariables_st.current_pedal_position-dap_calculationVariables_st.stepperPosMin))/((float)dap_calculationVariables_st.stepperPosRange);
            _ESPNow_Send.pedal_position_ratio=dap_calculationVariables_st.current_pedal_position_ratio;
            _ESPNow_Send.pedal_position=dap_calculationVariables_st.current_pedal_position;
            //ESPNow_send=dap_calculationVariables_st.current_pedal_position; 
            esp_err_t result = ESPNow.send_message(Recv_mac,(uint8_t *) &_ESPNow_Send,sizeof(_ESPNow_Send));  
            if (result != ESP_OK) 
            {
              Serial.println("Error sending the data");
            }

            if(ESPNow_update)
            {
              //dap_calculationVariables_st.sync_pedal_position=ESPNow_recieve;
              dap_calculationVariables_st.sync_pedal_position=_ESPNow_Recv.pedal_position;
              dap_calculationVariables_st.Sync_pedal_position_ratio=_ESPNow_Recv.pedal_position_ratio;
              ESPNow_update=false;
            }
            ESPNOW_count=0;
          }
        }
      } 
      else
      {
        ESPNOW_count++;          
      }
      
      #ifdef ESPNow_debug
          if(print_count>1500)
          {
            Serial.print("Rudder Status:");
            Serial.println(dap_calculationVariables_st.Rudder_status);
            Serial.print("Pedal type:");
            Serial.println(dap_config_st.payLoadPedalConfig_.pedal_type);
            Serial.print("---Sync Value--");
            Serial.println(dap_calculationVariables_st.sync_pedal_position);
            Serial.print("---Recieve Value--");
            Serial.println(_ESPNow_Recv.pedal_position_ratio);        
            Serial.print("---Send Value--");
            Serial.println(dap_calculationVariables_st.current_pedal_position);                  
            
            print_count=0;
          }
          else
          {
            print_count++;
            
          } 
          
               
      #endif
      delay(1);
  }
}
#endif


#ifdef ISV_COMMUNICATION


int16_t servoPos_last_i16 = 0;
int64_t timeSinceLastServoPosChange_l = 0;
int64_t timeNow_l = 0;
int64_t timeDiff = 0;

#define TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_STANDSTILL_IN_MS 200



uint64_t print_cycle_counter_u64 = 0;
unsigned long cycleTimeLastCall_lifelineCheck = 0;//micros();
void servoCommunicationTask( void * pvParameters )
{
  
  for(;;){

    if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
    {
      static CycleTimer timerServoCommunication("Servo Com. cycle time");
      timerServoCommunication.Bump();
    }

    // check if servo communication is still there every N milliseconds
    unsigned long now = millis();
    if ( (now - cycleTimeLastCall_lifelineCheck) > 5000) 
    {
      // if target cycle time is reached, update last time
      cycleTimeLastCall_lifelineCheck = now;

      isv57LifeSignal_b = isv57.checkCommunication();
      //Serial.println("Lifeline check");
    }



    if (isv57LifeSignal_b)
    {

        //delay(5);
        isv57.readServoStates();

        if(semaphore_readServoValues!=NULL)
        {
          if(xSemaphoreTake(semaphore_readServoValues, (TickType_t)1)==pdTRUE) {
            servoPos_i16 = -( isv57.servo_pos_given_p - isv57.getZeroPos() );
            xSemaphoreGive(semaphore_readServoValues);
          }
        }
        else
        {
          semaphore_readServoValues = xSemaphoreCreateMutex();
        }
        
        

        int32_t servo_offset_compensation_steps_local_i32 = 0;

        
        // condition 1: servo must be at halt
        // condition 2: the esp accel lib must be at halt
        bool cond_1 = false;;
        bool cond_2 = false;

        // check whether target position from ESP hasn't changed and is at min endstop position
        cond_2 = stepper->isAtMinPos();

        if (cond_2 == true)
        {
          //isv57.readServoStates();
          int16_t servoPos_now_i16 = isv57.servo_pos_given_p;
          timeNow_l = millis();

//#define PRINT_SERVO_POS_EVERY_N_CYCLES
#ifdef PRINT_SERVO_POS_EVERY_N_CYCLES
          print_cycle_counter_u64++;
          // print servo pos every N cycles
          if ( (print_cycle_counter_u64 % 2000) == 0 )
          {
            Serial.println(servoPos_now_i16);
            print_cycle_counter_u64 = 0;
          }
#endif


          // check whether servo position has changed, in case, update the halt detection variable
          if (servoPos_last_i16 != servoPos_now_i16)
          {
            servoPos_last_i16 = servoPos_now_i16;
            timeSinceLastServoPosChange_l = timeNow_l;
          }

          // compute the time difference since last servo position change
          timeDiff = timeNow_l - timeSinceLastServoPosChange_l;

          // if time between last servo position is larger than a threshold, detect servo standstill 
          if ( (timeDiff > TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_STANDSTILL_IN_MS) 
            && (timeNow_l > 0) )
          {
            cond_1 = true;
          }
          else
          {
            cond_1 = false;
          }
        }


        

        // calculate zero position offset
        if (cond_1 && cond_2)
        {

          // reset encoder position, when pedal is at min position
          if (resetServoEncoder == true)
          {
            isv57.setZeroPos();
            resetServoEncoder = false;
          }

          // calculate encoder offset
          // movement to the back will reduce encoder value
          servo_offset_compensation_steps_local_i32 = (int32_t)isv57.getZeroPos() - (int32_t)isv57.servo_pos_given_p;
          // when pedal has moved to the back due to step losses --> offset will be positive 

          // since the encoder positions are defined in int16 space, they wrap at multiturn
          // to correct overflow, we apply modulo to take smallest possible deviation
          if (servo_offset_compensation_steps_local_i32 > pow(2,15)-1)
          {
            servo_offset_compensation_steps_local_i32 -= pow(2,16);
          }

          if (servo_offset_compensation_steps_local_i32 < -pow(2,15))
          {
            servo_offset_compensation_steps_local_i32 += pow(2,16);
          }
        }


        // invert the compensation wrt the motor direction
        if (dap_config_st.payLoadPedalConfig_.invertMotorDirection_u8 == 1)
        {
          servo_offset_compensation_steps_local_i32 *= -1;
        }


        if(semaphore_resetServoPos!=NULL)
          {

            // Take the semaphore and just update the config file, then release the semaphore
            if(xSemaphoreTake(semaphore_resetServoPos, (TickType_t)1)==pdTRUE)
            {
              servo_offset_compensation_steps_i32 = servo_offset_compensation_steps_local_i32;
              xSemaphoreGive(semaphore_resetServoPos);
            }

          }
          else
          {
            semaphore_resetServoPos = xSemaphoreCreateMutex();
            //Serial.println("semaphore_resetServoPos == 0");
          }



        if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_SERVO_READINGS) 
        {
          static RTDebugOutput<int16_t, 4> rtDebugFilter({ "pos_p", "pos_error_p", "curr_per", "offset"});
          rtDebugFilter.offerData({ isv57.servo_pos_given_p, isv57.servo_pos_error_p, isv57.servo_current_percent, (int16_t)servo_offset_compensation_steps_i32});
        }

       

        
    }
    else
    {
      Serial.println("Servo communication lost!");
      delay(1000);
    }


  }
}

#endif
