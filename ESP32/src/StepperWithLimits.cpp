#include "StepperWithLimits.h"
#include "RTDebugOutput.h"
#include "Main.h"
#include "Math.h"


#define STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT 20
#define MIN_POS_MAX_ENDSTOP STEPS_PER_MOTOR_REVOLUTION * 3 // servo has to drive minimum N steps before it allows the detection of the max endstop


//uint32_t speed_in_hz = TICKS_PER_S / ticks;
// TICKS_PER_S = 16000000L
// ticks = TICKS_PER_S / speed_in_hz
#define maxSpeedInTicks  (TICKS_PER_S / MAXIMUM_STEPPER_SPEED)

static const uint8_t LIMIT_TRIGGER_VALUE = LOW;                                   // does endstop trigger high or low
static const int32_t ENDSTOP_MOVEMENT = (float)STEPS_PER_MOTOR_REVOLUTION / 100.0f;         // how much to move between trigger checks
static const int32_t ENDSTOP_MOVEMENT_SENSORLESS = ENDSTOP_MOVEMENT * 5;




TaskHandle_t task_iSV_Communication;
unsigned long cycleTimeLastCall_lifelineCheck = 0;//micros();
bool previousIsv57LifeSignal_b = true;
#define TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_STANDSTILL_IN_MS 200
#define TWO_TO_THE_POWER_OF_15_MINUS_1 (uint32_t)32767 // 2^15 - 1
//#define INT16_MAX (int32_t)65536
static SemaphoreHandle_t semaphore_lifelineSignal = xSemaphoreCreateMutex();
static SemaphoreHandle_t semaphore_resetServoPos = xSemaphoreCreateMutex();
static SemaphoreHandle_t semaphore_readServoValues = xSemaphoreCreateMutex();




FastAccelStepperEngine& stepperEngine() {
  static FastAccelStepperEngine myEngine = FastAccelStepperEngine();   // this is a factory and manager for all stepper instances

  static bool firstTime = true;
  if (firstTime) {
     myEngine.init();
     firstTime = false;
  }

  return myEngine;
}



StepperWithLimits::StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, uint8_t pinMin, uint8_t pinMax, bool invertMotorDir_b)
  : _pinMin(pinMin), _pinMax(pinMax)
  , _endstopLimitMin(0),    _endstopLimitMax(0)
  , _posMin(0),      _posMax(0)
{

  
  pinMode(pinMin, INPUT);
  pinMode(pinMax, INPUT);
  
  
  _stepper = stepperEngine().stepperConnectToPin(pinStep);

  

  // Stepper Parameters
  if (_stepper) {
    _stepper->setDirectionPin(pinDirection, invertMotorDir_b);
    _stepper->setAutoEnable(true);
    _stepper->setAbsoluteSpeedLimit( maxSpeedInTicks ); // ticks
    _stepper->setSpeedInTicks( maxSpeedInTicks ); // ticks
    _stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION);  // steps/s²
	_stepper->setLinearAcceleration(0);
    _stepper->setForwardPlanningTimeInMs(8);

	
	
	
	
	
	
	
	/************************************************************/
	/* 					iSV57 initialization					*/
	/************************************************************/
	//delay(3000);
	// find iSV57 servo ID
	bool isv57slaveIdFound_b = isv57.findServosSlaveId();
	Serial.print("iSV57 slaveId found:  ");
	Serial.println( isv57slaveIdFound_b );
	
	// restart ESP when no servo was detected
	if (!isv57slaveIdFound_b)
	{
		Serial.println( "No servo found! Restarting ESP" );
		ESP.restart();
	}

	// check whether iSV57 is connected
	// isv57LifeSignal_b = isv57.checkCommunication();
	setLifelineSignal();
	if (getLifelineSignal() == false)
	{
		Serial.println( "No lifeline detected! Restarting ESP" );
		ESP.restart();
	}
	else
	{
		// read servos alarm history
		isv57.readAlarmHistory();

		// reset iSV57 alarms
		bool servoAlarmsCleared = isv57.clearServoAlarms();

		Serial.print("iSV57 communication state:  ");
		Serial.println( getLifelineSignal() );

		// flash iSV57 registers
		isv57.setupServoStateReading();
		invertMotorDir_global_b = invertMotorDir_b;
		isv57.sendTunedServoParameters(invertMotorDir_global_b);
		
		// ToDo: 
		// - set servos internal rotation direction via debug port, thus ESPs and servos direction are aligned
		
		
		// print all servo registers
		/*if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_PRINT_ALL_SERVO_REGISTERS) 
		{
			isv57.readAllServoParameters();
		}*/


		// start read task
		xTaskCreatePinnedToCore(
						  this->servoCommunicationTask,   
						  "servoCommunicationTask", 
						  5000,  
						  //STACK_SIZE_FOR_TASK_2,    
						  this,//NULL,      
						  1,         
						  &task_iSV_Communication,    
						  0);   


						  
	}

	
	
	
	
	
  }
}


void StepperWithLimits::findMinMaxSensorless(DAP_config_st dap_config_st)
{

  if (! hasValidStepper()) return;


	if ( getLifelineSignal() )
	{
		
		// reduce speed and acceleration
		_stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED / 10);
		//_stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION / 10);


		/************************************************************/
		/* 					min endstop	detection					*/
		/************************************************************/
		bool endPosDetected = abs( isv57.servo_current_percent) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		int32_t setPosition = 0;
		
		// run continously in one direction until endstop is hit
		//_stepper->runForward();

		_stepper->move(INT32_MIN, false);
		
		while( (!endPosDetected) && (getLifelineSignal()) ){
			delay(10);
			endPosDetected = abs( isv57.servo_current_percent) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		}
		setPosition = - 5 * ENDSTOP_MOVEMENT_SENSORLESS;
		_stepper->forceStopAndNewPosition(setPosition);
		
		// move slightly away from the block to prevent mechanical hits during normal operation
		//setPosition = setPosition + 5 * ENDSTOP_MOVEMENT_SENSORLESS;
		//_stepper->moveTo(setPosition, true);
		//_stepper->forceStopAndNewPosition(0);
		_stepper->moveTo(0);
		_endstopLimitMin = 0;
		delay(100);
		isv57.setZeroPos();
		
		Serial.println("Min endstop reached.");
		
		
		// ToDo:
		// try to reset servos encoder value via debug port thus ESPs position and servos position are aligned
		// Alterantively see Pr0.15 from https://www.leadshine.com/upfiles/downloads/a3d7d12a120fd8e114f6288b6235ac1a_1690179981835.pdf
		// _stepper->disableAxis();
		// _stepper->enableAxis();
		//delay(500);
		// Serial.println("Servos position before 0 compensation: ");
		// Serial.println(isv57.servo_pos_given_p);
		
		// isv57.clearServoUnitPosition(); // Resets servos internal position to 0 to align it with the ESPs position
		
		// //delay(500);
		// Serial.println("Servos position after 0 compensation: ");
		// Serial.println(isv57.servo_pos_given_p);
		
		
		
		
		
		/************************************************************/
		/* 					max endstop	detection					*/
		/************************************************************/
		// calculate max steps for endstop limit
		float spindlePitch = max( dap_config_st.payLoadPedalConfig_.spindlePitch_mmPerRev_u8, (uint8_t)1 );
		float maxRevToReachEndPos = (float)dap_config_st.payLoadPedalConfig_.lengthPedal_travel / spindlePitch;
		float maxStepsToReachEndPos = maxRevToReachEndPos * (float)STEPS_PER_MOTOR_REVOLUTION;
  
  
		endPosDetected = false; //abs( isv57.servo_current_percent) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		
		// run continously in one direction until endstop is hit
		//_stepper->runBackward();
		_stepper->move(INT32_MAX, false);
		
		// if endstop is reached, communication is lost or virtual endstop is hit
		while( (!endPosDetected) && (getLifelineSignal()) ){
			delay(10);
			if (_stepper->getCurrentPosition() > MIN_POS_MAX_ENDSTOP)
    		{
				endPosDetected = abs( isv57.servo_current_percent) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
			}

			// virtual endstop
			endPosDetected |= (_stepper->getCurrentPosition() > maxStepsToReachEndPos);
		}
		_stepper->forceStop();
		_endstopLimitMax = _stepper->getCurrentPosition();

		Serial.println("Max endstop reached.");
		
		// move slowly to min position
		moveSlowlyToPos(_posMin);
		
		
		
		// increase speed and accelerartion back to normal
		_stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION);
		_stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED);
	}	
	

}


void StepperWithLimits::moveSlowlyToPos(int32_t targetPos_ui32) {
  // reduce speed and accelerartion
  _stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED / 4);
  _stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION / 4);

  // move to min
  _stepper->moveTo(targetPos_ui32, true);

  // increase speed and accelerartion
  _stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION);
  _stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED);
}




void StepperWithLimits::updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct) {
  int32_t limitRange = _endstopLimitMax - _endstopLimitMin;
  _posMin = _endstopLimitMin + ((limitRange * pedalStartPosPct) / 100);
  _posMax = _endstopLimitMin + ((limitRange * pedalEndPosPct) / 100);
}

int8_t StepperWithLimits::moveTo(int32_t position, bool blocking) {
  return _stepper->moveTo(position, blocking);
}

int32_t StepperWithLimits::getCurrentPositionFromMin() const {
  return _stepper->getCurrentPosition() - _posMin;
}

int32_t StepperWithLimits::getCurrentPosition() const {
  return _stepper->getCurrentPosition();
}

double StepperWithLimits::getCurrentPositionFraction() const {
  return double(getCurrentPositionFromMin()) / getTravelSteps();
}

double StepperWithLimits::getCurrentPositionFractionFromExternalPos(int32_t extPos_i32) const {
  return (double(extPos_i32) - _posMin)/ getTravelSteps();
}

int32_t StepperWithLimits::getTargetPositionSteps() const {
  return _stepper->getPositionAfterCommandsCompleted();
}


void StepperWithLimits::printStates()
{
  int32_t currentStepperPos = _stepper->getCurrentPosition();
  int32_t currentStepperVel = _stepper->getCurrentSpeedInUs();
  int32_t currentStepperVel2 = _stepper->getCurrentSpeedInMilliHz();


  //Serial.println(currentStepperVel);
  
  int32_t currentStepperAccel = _stepper->getCurrentAcceleration();

  static RTDebugOutput<int32_t, 4> rtDebugFilter({ "Pos", "Vel", "Vel2", "Accel"});
  rtDebugFilter.offerData({ currentStepperPos, currentStepperVel, currentStepperVel2, currentStepperAccel});
}


void StepperWithLimits::setSpeed(uint32_t speedInStepsPerSecond) 
{
  _stepper->setSpeedInHz(speedInStepsPerSecond);            // steps/s 
}

bool StepperWithLimits::isAtMinPos()
{

  bool isNotRunning = !_stepper->isRunning();
  bool isAtMinPos = getCurrentPositionFromMin() == 0;

  return isAtMinPos && isNotRunning;
}




/************************************************************/
/* 					Step loss recovery						*/
/************************************************************/
void StepperWithLimits::correctPos()
{
	if(semaphore_resetServoPos!=NULL)
	{
		// Take the semaphore and just update the config file, then release the semaphore
		if(xSemaphoreTake(semaphore_resetServoPos, (TickType_t)1)==pdTRUE)
		{
			// tune the current servo position to compesnate the position offset
			int32_t stepOffset =(int32_t)constrain(servo_offset_compensation_steps_i32, -10, 10);
			_stepper->setCurrentPosition(_stepper->getCurrentPosition() + stepOffset);
			servo_offset_compensation_steps_i32 = 0; // reset lost step variable to prevent overcompesnation
			xSemaphoreGive(semaphore_resetServoPos);
		}
	}
	else
	{
		semaphore_resetServoPos = xSemaphoreCreateMutex();
	}
}





/************************************************************/
/* 					lifeline set and get					*/
/************************************************************/

bool StepperWithLimits::getLifelineSignal()
{
	bool signal_b = false;
	/*if(semaphore_lifelineSignal!=NULL)
	{

		// Take the semaphore and just update the config file, then release the semaphore
		if(xSemaphoreTake(semaphore_lifelineSignal, (TickType_t)1)==pdTRUE)
		{
		  signal_b = isv57LifeSignal_b;
		}

	}
	else
	{
		semaphore_lifelineSignal = xSemaphoreCreateMutex();
	}*/
	

	signal_b = isv57LifeSignal_b;

	return signal_b;
}

void StepperWithLimits::setLifelineSignal()
{

	isv57LifeSignal_b = isv57.checkCommunication();


	/*if(semaphore_lifelineSignal!=NULL)
	{

		// Take the semaphore and just update the config file, then release the semaphore
		if(xSemaphoreTake(semaphore_lifelineSignal, (TickType_t)1)==pdTRUE)
		{
		  isv57LifeSignal_b = isv57.checkCommunication();
		}

	}
	else
	{
		semaphore_lifelineSignal = xSemaphoreCreateMutex();
	}*/
}








int32_t StepperWithLimits::getServosVoltage()
{
	return isv57.servo_voltage_0p1V;
}

int32_t StepperWithLimits::getServosCurrent()
{
	return isv57.servo_current_percent;
}

int32_t StepperWithLimits::getServosPos()
{
	return isv57.servo_pos_given_p;
}





int32_t StepperWithLimits::getServosInternalPosition()
{
	int32_t servoPos_local_i32 = 0;
	
	if(semaphore_readServoValues!=NULL)
	{
	  if(xSemaphoreTake(semaphore_readServoValues, (TickType_t)1)==pdTRUE) {
		servoPos_local_i32 = servoPos_i16;
		xSemaphoreGive(semaphore_readServoValues);
	  }
	}
	else
	{
	  semaphore_readServoValues = xSemaphoreCreateMutex();
	}
	
	return servoPos_local_i32;
}




int64_t timeSinceLastServoPosChange_l = 0;
int64_t timeDiff = 0;
int16_t servoPos_last_i16 = 0;
int64_t timeNow_l = 0;


void StepperWithLimits::servoCommunicationTask(void *pvParameters)
{
  
  	// Cast the parameter to StepperWithLimits pointer
	StepperWithLimits* stepper_cl = static_cast<StepperWithLimits*>(pvParameters);

	for(;;){

	  
		// induce a small pause to decrease CPU workload
		delay(1);

		
		/************************************************************/
		/* 					recheck lifeline						*/
		/************************************************************/
		// check if servo communication is still there every N milliseconds
		unsigned long now = millis();
		if ( (now - cycleTimeLastCall_lifelineCheck) > 500) 
		{
			// if target cycle time is reached, update last time
			cycleTimeLastCall_lifelineCheck = now;
			stepper_cl->setLifelineSignal();
		}

		
		
		/************************************************************/
		/* 					read servo states 						*/
		/*				and calculate step loss 					*/
		/************************************************************/
		if ( stepper_cl->getLifelineSignal() )
		{

			// when servo has been restarted, the read states need to be initialized first
			if (false == previousIsv57LifeSignal_b)
			{
			stepper_cl->isv57.setupServoStateReading();
			previousIsv57LifeSignal_b = true;
			delay(50);
			}
			
			
			
			
			
			
			// read servo states
			stepper_cl->isv57.readServoStates();

			if(semaphore_readServoValues!=NULL)
			{
			if(xSemaphoreTake(semaphore_readServoValues, (TickType_t)1)==pdTRUE) {
				stepper_cl->servoPos_i16 = -( stepper_cl->isv57.servo_pos_given_p - stepper_cl->isv57.getZeroPos() );
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
			bool cond_1 = false;
			bool cond_2 = false;

			// check whether target position from ESP hasn't changed and is at min endstop position
			cond_2 = stepper_cl->isAtMinPos();

			

			if (cond_2 == true)
			{
			//isv57.readServoStates();
			int16_t servoPos_now_i16 = stepper_cl->isv57.servo_pos_given_p;
			timeNow_l = millis();

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


				// calculate encoder offset
				// movement to the back will reduce encoder value
				servo_offset_compensation_steps_local_i32 = (int32_t)stepper_cl->isv57.getZeroPos() - (int32_t)stepper_cl->isv57.servo_pos_given_p;
				// when pedal has moved to the back due to step losses --> offset will be positive 





				// When the servo turned off during driving, the servo loses its zero position and the correction might not be valid anymore. If still applied, the servo will somehow srive against the block
				// resulting in excessive servo load --> current load. We'll detect whether min or max block was reached, depending on the position error sign
				bool servoCurrentLow_b = abs(stepper_cl->isv57.servo_current_percent) < 200;
				if (!servoCurrentLow_b)
				{

					// positive current means positive rotation 
					bool minBlockCrashDetected_b = false;
					bool maxBlockCrashDetected_b = false;
					if (stepper_cl->isv57.servo_current_percent > 0) // if current is positive, the rotation will be positive and thus the sled will move towards the user
					{
					minBlockCrashDetected_b = true; 
					stepper_cl->isv57.applyOfsetToZeroPos(-500); // bump up a bit to prevent the servo from pushing against the endstop continously
					}
					else
					{
					maxBlockCrashDetected_b = true;
					stepper_cl->isv57.applyOfsetToZeroPos(500); // bump up a bit to prevent the servo from pushing against the endstop continously
					}

					/*print_cycle_counter_u64++;
					print_cycle_counter_u64 %= 10;

					if (print_cycle_counter_u64 == 0)
					{
					Serial.print("minDet: ");
					Serial.print(minBlockCrashDetected_b);

					Serial.print("curr: ");
					Serial.print(isv57.servo_current_percent);
					
					Serial.print("posError: ");
					Serial.print(isv57.servo_pos_error_p);

					Serial.println();
					}*/


					//servo_offset_compensation_steps_local_i32 = isv57.servo_pos_error_p;
				}





				// since the encoder positions are defined in int16 space, they wrap at multiturn
				// to correct overflow, we apply modulo to take smallest possible deviation
				if (servo_offset_compensation_steps_local_i32 > TWO_TO_THE_POWER_OF_15_MINUS_1)
				{
					servo_offset_compensation_steps_local_i32 -= INT16_MAX;
				}

				if (servo_offset_compensation_steps_local_i32 < -TWO_TO_THE_POWER_OF_15_MINUS_1)
				{
					servo_offset_compensation_steps_local_i32 += INT16_MAX;
				}
			
			
				// invert the compensation wrt the motor direction
				if (true == stepper_cl->invertMotorDir_global_b)
				{
				servo_offset_compensation_steps_local_i32 *= -1;
				}

				//servo_offset_compensation_steps_local_i32 *= -1;


				if(semaphore_resetServoPos!=NULL)
				{

					// Take the semaphore and just update the config file, then release the semaphore
					if(xSemaphoreTake(semaphore_resetServoPos, (TickType_t)1)==pdTRUE)
					{
					stepper_cl->servo_offset_compensation_steps_i32 = servo_offset_compensation_steps_local_i32;
					xSemaphoreGive(semaphore_resetServoPos);
					}

				}
				else
				{
					semaphore_resetServoPos = xSemaphoreCreateMutex();
				}
					
				
			}


			
		

			
		}
		else
		{
		Serial.println("Servo communication lost!");
		delay(100);
		previousIsv57LifeSignal_b = false;
		}


	}
}


/*int32_t StepperWithLimits::getStepLossCompensation()
{
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

	return servo_offset_compensation_steps_i32;
}*/




