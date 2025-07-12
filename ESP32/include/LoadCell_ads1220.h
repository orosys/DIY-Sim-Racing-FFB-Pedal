#pragma once

#include <stdint.h>
#include "Main.h"


#ifdef USES_ADS1220

/*  Uses ADS1256 */
class LoadCell_ADS1220 {
private:
  float _zeroPoint = 0.0;
  float _varianceEstimate = 0.0;
  float _standardDeviationEstimate = 0.0;

public:
  LoadCell_ADS1220();
  float getReadingKg() const;
  void setLoadcellRating(uint8_t loadcellRating_u8) const;
  void estimateBiasAndVariance();
  float getVarianceEstimate() const { return _varianceEstimate; }
  float getShiftingEstimate() const { return _zeroPoint; }
  float getSTDEstimate() const { return _standardDeviationEstimate; }
};

#endif
