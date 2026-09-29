#ifndef SIGNAL_FILTER_IMM_H
#define SIGNAL_FILTER_IMM_H

#include "Arduino.h"
#include <stdint.h>
#include "Main.h"

// Interacting Multiple Model (IMM) Kalman filter for the pedal force.
//
// Two hypotheses about the foot force are run in parallel and blended by how
// well each one explains the incoming loadcell samples:
//  - HOLD: force is (almost) constant  --> strong smoothing, noise cannot move the pedal
//  - MOVE: force changes at a rate     --> low-lag tracking of real presses
// Both models share the state [force; force rate] so they can be mixed.
class KalmanFilterIMM {
public:
  // Constructor
  KalmanFilterIMM(float varianceEstimate_fl32);

  // Main filter function
  // measurement_fl32:     force measurement (same unit as varianceEstimate, e.g. kg)
  // newSample_b:          true if the measurement is a fresh loadcell sample, false if it is a held value
  // deltaTime_s_fl32:     time since the last call in seconds
  // modelNoiseScaling_u8: 1..255, bandwidth of the MOVE model (slow to fast)
  float IRAM_ATTR_FLAG filteredValue(float measurement_fl32, bool newSample_b, float deltaTime_s_fl32, uint8_t modelNoiseScaling_u8);

  // Getters for the state variables
  float IRAM_ATTR_FLAG changeVelocity() const;
  float IRAM_ATTR_FLAG moveProbability() const;

  // Measurement noise variance in (measurement unit)^2
  void IRAM_ATTR_FLAG setMeasurementVariance(float varianceEstimate_fl32);

  // Re-initialize all states at the given force (e.g. after switching filters)
  void reset(float measurement_fl32);

private:
  static const uint8_t NMB_MODELS = 2;
  static const uint8_t MODEL_HOLD = 0;
  static const uint8_t MODEL_MOVE = 1;

  // Per model state vector [force; force rate] and covariance (2x2)
  float stateX_aafl32[NMB_MODELS][2];
  float stateCovarianceP_aaafl32[NMB_MODELS][2][2];

  // Mode probabilities [HOLD, MOVE]
  float modeProbability_afl32[NMB_MODELS];

  // Blended output
  float outputForce_fl32;
  float outputForceRate_fl32;

  // Measurement noise covariance R (scalar)
  float measurementNoiseR_fl32;

  // Averaged time between fresh samples, used to derive the process noise from a target bandwidth
  float sampleInterval_s_fl32;
  float timeSinceLastSample_s_fl32;

  bool isInitialized_b;
};

#endif // SIGNAL_FILTER_IMM_H
