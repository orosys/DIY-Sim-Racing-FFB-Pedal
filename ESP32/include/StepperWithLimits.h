#include <FastAccelStepper.h>
#include "isv57communication.h"
#include "DiyActivePedal_types.h"
#include "Main.h"

// these are physical properties of the stepper
static const uint32_t MAXIMUM_STEPPER_RPM = 4000;     
static const uint32_t SECONDS_PER_MINUTE = 60;
//static const uint32_t MAXIMUM_STEPPER_SPEED = (  (float)MAXIMUM_STEPPER_RPM * (float)STEPS_PER_MOTOR_REVOLUTION) / (float)SECONDS_PER_MINUTE;   // steps/s
#define MAXIMUM_STEPPER_SPEED (uint32_t)300000//  max steps per second, see https://github.com/gin66/FastAccelStepper
static const int32_t MAXIMUM_STEPPER_ACCELERATION = INT32_MAX;                                                 // steps/s²


class StepperWithLimits {
private:
	FastAccelStepper* _stepper;
	uint8_t _pinMin,   _pinMax;      // pins that limit switches attach to
	int32_t _endstopLimitMin, _endstopLimitMax;    // stepper position at limit switches
	int32_t _posMin,   _posMax;      // stepper position at min/max of travel

	isv57communication isv57;
	

	/*public: void StartTask(void)
	{      
		//Start Task with input parameter set to "this" class
		xTaskCreatePinnedToCore(
		this->servoCommunicationTask,        //Function to implement the task 
		"servoCommunicationTask",            //Name of the task
		5000,                   //Stack size in words 
		this,                   //Task input parameter 
		1,           //Priority of the task 
		&task_iSV_Communication,                 //Task handle.
		0);              //Core where the task should run 
	}*/




	
	bool isv57LifeSignal_b = false;
	bool invertMotorDir_global_b = false;
	int32_t servoPos_i16 = 0;
	int32_t servo_offset_compensation_steps_i32 = 0;

	bool restartServo = false;
	void setLifelineSignal();

	

public:
	StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, uint8_t pinMin, uint8_t pinMax, bool invertMotorDir_b);
	bool hasValidStepper() const { return NULL != _stepper; }

	void checkLimitsAndResetIfNecessary();
	void updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct);
	bool isAtMinPos();
	void correctPos();
	void findMinMaxSensorless(DAP_config_st dap_config_st);
	int8_t moveTo(int32_t position, bool blocking = false);
	void moveSlowlyToPos(int32_t targetPos_ui32);
	void printStates();

	int32_t getCurrentPositionFromMin() const;
	int32_t getCurrentPosition() const;
	double getCurrentPositionFraction() const;
	double getCurrentPositionFractionFromExternalPos(int32_t extPos_i32) const;
	int32_t getTargetPositionSteps() const;

	int32_t getLimitMin() const { return _endstopLimitMin; }
	int32_t getLimitMax() const { return _endstopLimitMax; }
	int32_t getTravelSteps() const { return _posMax - _posMin; }
	void setSpeed(uint32_t speedInStepsPerSecond);

	int32_t getPositionDeviation();
	int32_t getServosInternalPosition();
	//int32_t getStepLossCompensation();
	int32_t getServosVoltage();
	int32_t getServosCurrent();
	int32_t getServosPos();
	bool getLifelineSignal();


	static void servoCommunicationTask( void * pvParameters );
};
