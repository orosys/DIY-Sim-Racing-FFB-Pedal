#include "SignalFilter_IMM.h"

// =========================================================
// TUNING CONSTANTS
// =========================================================
// Closed-loop bandwidth of the HOLD model. Only slow drift (e.g. the foot settling) passes.
static const float s_holdBandwidthHz_fl32 = 1.5f;

// Closed-loop bandwidth range of the MOVE model, selected with the "Loadcell denoising" slider (1..255).
static const float s_moveBandwidthMinHz_fl32 = 5.0f;
static const float s_moveBandwidthMaxHz_fl32 = 60.0f;

// Expected dwell time in each mode (Markov switching). Longer = fewer mode changes.
static const float s_holdDwellTime_s_fl32 = 0.3f;
static const float s_moveDwellTime_s_fl32 = 0.15f;

// Lower bound of each mode probability, so a model can always win back quickly.
static const float s_modeProbabilityMin_fl32 = 1e-3f;

// Initial and minimum values
static const float s_initialSampleInterval_s_fl32 = 1e-3f;
static const float s_sampleIntervalFilterAlpha_fl32 = 0.05f;
static const float s_measurementNoiseMin_fl32 = 1e-9f;
static const float s_innovationCovarianceMin_fl32 = 1e-12f;

static const float s_twoPi_fl32 = 6.283185307f;

// Constructor
KalmanFilterIMM::KalmanFilterIMM(float varianceEstimate_fl32)
  : outputForce_fl32(0.0f), outputForceRate_fl32(0.0f),
    measurementNoiseR_fl32(varianceEstimate_fl32),
    sampleInterval_s_fl32(s_initialSampleInterval_s_fl32), timeSinceLastSample_s_fl32(0.0f),
    isInitialized_b(false)
{
  setMeasurementVariance(varianceEstimate_fl32);
  reset(0.0f);
  isInitialized_b = false; // initialize on the first real measurement
}

void KalmanFilterIMM::setMeasurementVariance(float varianceEstimate_fl32) {
  measurementNoiseR_fl32 = (varianceEstimate_fl32 > s_measurementNoiseMin_fl32) ? varianceEstimate_fl32 : s_measurementNoiseMin_fl32;
}

void KalmanFilterIMM::reset(float measurement_fl32) {
  for (uint8_t modelIdx = 0; modelIdx < NMB_MODELS; modelIdx++) {
    stateX_aafl32[modelIdx][0] = measurement_fl32;
    stateX_aafl32[modelIdx][1] = 0.0f;

    stateCovarianceP_aaafl32[modelIdx][0][0] = measurementNoiseR_fl32;
    stateCovarianceP_aaafl32[modelIdx][0][1] = 0.0f;
    stateCovarianceP_aaafl32[modelIdx][1][0] = 0.0f;
    stateCovarianceP_aaafl32[modelIdx][1][1] = measurementNoiseR_fl32 * 1e4f;
  }

  modeProbability_afl32[MODEL_HOLD] = 0.9f;
  modeProbability_afl32[MODEL_MOVE] = 0.1f;

  outputForce_fl32 = measurement_fl32;
  outputForceRate_fl32 = 0.0f;
  timeSinceLastSample_s_fl32 = 0.0f;
  isInitialized_b = true;
}

float KalmanFilterIMM::filteredValue(float measurement_fl32, bool newSample_b, float deltaTime_s_fl32, uint8_t modelNoiseScaling_u8) {
  if (!isInitialized_b) {
    reset(measurement_fl32);
    return outputForce_fl32;
  }

  float dt_fl32 = constrain(deltaTime_s_fl32, 1e-5f, 5e-3f);

  // =========================================================
  // PROCESS NOISE FROM TARGET BANDWIDTHS
  // =========================================================
  // With continuous-equivalent measurement noise density r = R * T (T = sample interval):
  //  - random walk (HOLD):           bandwidth w = sqrt(q / r)   --> q = r * w^2
  //  - constant rate (MOVE):         bandwidth w = (q / r)^(1/4) --> q = r * w^4
  timeSinceLastSample_s_fl32 += dt_fl32;
  float noiseDensity_fl32 = measurementNoiseR_fl32 * sampleInterval_s_fl32;

  float holdOmega_fl32 = s_twoPi_fl32 * s_holdBandwidthHz_fl32;
  float qHold_fl32 = noiseDensity_fl32 * holdOmega_fl32 * holdOmega_fl32;

  float sliderValue01_fl32 = constrain(((float)modelNoiseScaling_u8 - 1.0f) / 254.0f, 0.0f, 1.0f);
  float moveBandwidthHz_fl32 = s_moveBandwidthMinHz_fl32 * powf(s_moveBandwidthMaxHz_fl32 / s_moveBandwidthMinHz_fl32, sliderValue01_fl32);
  float moveOmega_fl32 = s_twoPi_fl32 * moveBandwidthHz_fl32;
  float moveOmegaPow2_fl32 = moveOmega_fl32 * moveOmega_fl32;
  float qMove_fl32 = noiseDensity_fl32 * moveOmegaPow2_fl32 * moveOmegaPow2_fl32;

  // =========================================================
  // 1. MIXING (interaction step)
  // =========================================================
  // Markov transition probabilities p[i][j] = P(model j now | model i before)
  float pStayHold_fl32 = expf(-dt_fl32 / s_holdDwellTime_s_fl32);
  float pStayMove_fl32 = expf(-dt_fl32 / s_moveDwellTime_s_fl32);
  float transition_aafl32[NMB_MODELS][NMB_MODELS] = {
    {pStayHold_fl32, 1.0f - pStayHold_fl32},
    {1.0f - pStayMove_fl32, pStayMove_fl32}
  };

  // Predicted mode probabilities c_j = sum_i p[i][j] * mu_i
  float predictedModeProbability_afl32[NMB_MODELS];
  for (uint8_t j = 0; j < NMB_MODELS; j++) {
    predictedModeProbability_afl32[j] = 0.0f;
    for (uint8_t i = 0; i < NMB_MODELS; i++) {
      predictedModeProbability_afl32[j] += transition_aafl32[i][j] * modeProbability_afl32[i];
    }
  }

  // Mixed initial state and covariance per model
  float mixedX_aafl32[NMB_MODELS][2];
  float mixedP_aaafl32[NMB_MODELS][2][2];
  for (uint8_t j = 0; j < NMB_MODELS; j++) {
    float mixingWeight_afl32[NMB_MODELS];
    for (uint8_t i = 0; i < NMB_MODELS; i++) {
      mixingWeight_afl32[i] = transition_aafl32[i][j] * modeProbability_afl32[i] / predictedModeProbability_afl32[j];
    }

    mixedX_aafl32[j][0] = 0.0f;
    mixedX_aafl32[j][1] = 0.0f;
    for (uint8_t i = 0; i < NMB_MODELS; i++) {
      mixedX_aafl32[j][0] += mixingWeight_afl32[i] * stateX_aafl32[i][0];
      mixedX_aafl32[j][1] += mixingWeight_afl32[i] * stateX_aafl32[i][1];
    }

    mixedP_aaafl32[j][0][0] = mixedP_aaafl32[j][0][1] = mixedP_aaafl32[j][1][1] = 0.0f;
    for (uint8_t i = 0; i < NMB_MODELS; i++) {
      float dx0_fl32 = stateX_aafl32[i][0] - mixedX_aafl32[j][0];
      float dx1_fl32 = stateX_aafl32[i][1] - mixedX_aafl32[j][1];
      mixedP_aaafl32[j][0][0] += mixingWeight_afl32[i] * (stateCovarianceP_aaafl32[i][0][0] + dx0_fl32 * dx0_fl32);
      mixedP_aaafl32[j][0][1] += mixingWeight_afl32[i] * (stateCovarianceP_aaafl32[i][0][1] + dx0_fl32 * dx1_fl32);
      mixedP_aaafl32[j][1][1] += mixingWeight_afl32[i] * (stateCovarianceP_aaafl32[i][1][1] + dx1_fl32 * dx1_fl32);
    }
    mixedP_aaafl32[j][1][0] = mixedP_aaafl32[j][0][1];
  }

  // =========================================================
  // 2. PREDICT per model
  // =========================================================
  // HOLD: F = [1, 0; 0, 0], force random walk, rate forced to zero
  {
    float* x = stateX_aafl32[MODEL_HOLD];
    float (*P)[2] = stateCovarianceP_aaafl32[MODEL_HOLD];
    x[0] = mixedX_aafl32[MODEL_HOLD][0];
    x[1] = 0.0f;
    P[0][0] = mixedP_aaafl32[MODEL_HOLD][0][0] + qHold_fl32 * dt_fl32;
    P[0][1] = 0.0f;
    P[1][0] = 0.0f;
    P[1][1] = s_measurementNoiseMin_fl32; // keep P positive definite
  }

  // MOVE: F = [1, dt; 0, 1], white noise on the force rate
  // Q = q * [dt^3/3, dt^2/2; dt^2/2, dt]
  {
    float* x = stateX_aafl32[MODEL_MOVE];
    float (*P)[2] = stateCovarianceP_aaafl32[MODEL_MOVE];
    float (*P0)[2] = mixedP_aaafl32[MODEL_MOVE];
    float dtPow2_fl32 = dt_fl32 * dt_fl32;
    float dtPow3_fl32 = dtPow2_fl32 * dt_fl32;

    x[0] = mixedX_aafl32[MODEL_MOVE][0] + dt_fl32 * mixedX_aafl32[MODEL_MOVE][1];
    x[1] = mixedX_aafl32[MODEL_MOVE][1];
    P[0][0] = P0[0][0] + dt_fl32 * (P0[0][1] + P0[1][0]) + dtPow2_fl32 * P0[1][1] + qMove_fl32 * dtPow3_fl32 / 3.0f;
    P[0][1] = P0[0][1] + dt_fl32 * P0[1][1] + qMove_fl32 * dtPow2_fl32 / 2.0f;
    P[1][0] = P[0][1];
    P[1][1] = P0[1][1] + qMove_fl32 * dt_fl32;
  }

  // =========================================================
  // 3. UPDATE per model and mode probabilities
  // =========================================================
  if (newSample_b) {
    // Averaged sample interval for the process noise scaling
    sampleInterval_s_fl32 += s_sampleIntervalFilterAlpha_fl32 * (timeSinceLastSample_s_fl32 - sampleInterval_s_fl32);
    timeSinceLastSample_s_fl32 = 0.0f;

    float logLikelihood_afl32[NMB_MODELS];
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      float* x = stateX_aafl32[j];
      float (*P)[2] = stateCovarianceP_aaafl32[j];

      // H = [1, 0]
      float innovation_fl32 = measurement_fl32 - x[0];
      float innovationCov_fl32 = P[0][0] + measurementNoiseR_fl32;
      if (innovationCov_fl32 < s_innovationCovarianceMin_fl32) innovationCov_fl32 = s_innovationCovarianceMin_fl32;
      float invInnovationCov_fl32 = 1.0f / innovationCov_fl32;

      float gain0_fl32 = P[0][0] * invInnovationCov_fl32;
      float gain1_fl32 = P[1][0] * invInnovationCov_fl32;

      x[0] += gain0_fl32 * innovation_fl32;
      x[1] += gain1_fl32 * innovation_fl32;

      // P = (I - K*H) * P
      float p00_fl32 = (1.0f - gain0_fl32) * P[0][0];
      float p01_fl32 = (1.0f - gain0_fl32) * P[0][1];
      float p11_fl32 = P[1][1] - gain1_fl32 * P[0][1];
      P[0][0] = (p00_fl32 > s_measurementNoiseMin_fl32) ? p00_fl32 : s_measurementNoiseMin_fl32;
      P[0][1] = p01_fl32;
      P[1][0] = p01_fl32;
      P[1][1] = (p11_fl32 > s_measurementNoiseMin_fl32) ? p11_fl32 : s_measurementNoiseMin_fl32;

      // Gaussian log likelihood (constant term dropped, it cancels in the normalization)
      logLikelihood_afl32[j] = -0.5f * (innovation_fl32 * innovation_fl32 * invInnovationCov_fl32 + logf(innovationCov_fl32));
    }

    // mu_j ~ Lambda_j * c_j, evaluated relative to the max log likelihood to avoid underflow
    float maxLogLikelihood_fl32 = max(logLikelihood_afl32[MODEL_HOLD], logLikelihood_afl32[MODEL_MOVE]);
    float normalization_fl32 = 0.0f;
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      modeProbability_afl32[j] = expf(logLikelihood_afl32[j] - maxLogLikelihood_fl32) * predictedModeProbability_afl32[j];
      normalization_fl32 += modeProbability_afl32[j];
    }
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      modeProbability_afl32[j] /= normalization_fl32;
    }
  } else {
    // No fresh measurement: only the Markov prior changes the mode probabilities
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      modeProbability_afl32[j] = predictedModeProbability_afl32[j];
    }
  }

  // Keep every model alive so it can win back quickly
  if (modeProbability_afl32[MODEL_HOLD] < s_modeProbabilityMin_fl32) modeProbability_afl32[MODEL_HOLD] = s_modeProbabilityMin_fl32;
  if (modeProbability_afl32[MODEL_MOVE] < s_modeProbabilityMin_fl32) modeProbability_afl32[MODEL_MOVE] = s_modeProbabilityMin_fl32;
  float probabilitySum_fl32 = modeProbability_afl32[MODEL_HOLD] + modeProbability_afl32[MODEL_MOVE];
  modeProbability_afl32[MODEL_HOLD] /= probabilitySum_fl32;
  modeProbability_afl32[MODEL_MOVE] /= probabilitySum_fl32;

  // =========================================================
  // 4. OUTPUT: probability weighted combination
  // =========================================================
  outputForce_fl32 = modeProbability_afl32[MODEL_HOLD] * stateX_aafl32[MODEL_HOLD][0]
                   + modeProbability_afl32[MODEL_MOVE] * stateX_aafl32[MODEL_MOVE][0];
  outputForceRate_fl32 = modeProbability_afl32[MODEL_HOLD] * stateX_aafl32[MODEL_HOLD][1]
                       + modeProbability_afl32[MODEL_MOVE] * stateX_aafl32[MODEL_MOVE][1];

  return outputForce_fl32;
}

float KalmanFilterIMM::changeVelocity() const {
  return outputForceRate_fl32;
}

float KalmanFilterIMM::moveProbability() const {
  return modeProbability_afl32[MODEL_MOVE];
}
