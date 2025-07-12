#include "ForceCurve.h"
//#include "InterpolationLib.h"
#include "Arduino.h"



/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: force computation                            */
/*                                                                                            */
/**********************************************************************************************/

// see https://swharden.com/blog/2022-01-22-spline-interpolation/
float ForceCurve_Interpolated::EvalForceCubicSpline(const DAP_config_st* config_st, const DAP_calculationVariables_st* calc_st, float fractionalPos)
{

  float fractionalPos_lcl = constrain(fractionalPos, 0, 1);
  float fractionalPos_float=fractionalPos_lcl*100.0f;
  //float splineSegment_fl32 = fractionalPos_lcl * 5.0f;
  float splineSegment_fl32 = 0;
  for(int i=0;i<config_st->payLoadPedalConfig_.quantityOfControl;i++)
  {
    if(fractionalPos_float>calc_st->travel[i])
    {
      if(i==config_st->payLoadPedalConfig_.quantityOfControl-1)
      {
        splineSegment_fl32=(float)i;
      }
      else
      {
        float diff= (fractionalPos_float-(float)calc_st->travel[i])/(float)(calc_st->travel[i+1]-calc_st->travel[i]);
        splineSegment_fl32=(float)i+diff;
      }  
    }
    else
    {
      break;
    }
  }
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  if (splineSegment_u8 > (config_st->payLoadPedalConfig_.quantityOfControl-1) )
  {
    splineSegment_u8 = config_st->payLoadPedalConfig_.quantityOfControl - 1;
  }
  float a = calc_st->interpolatorA[splineSegment_u8];
  float b = calc_st->interpolatorB[splineSegment_u8];

  float yOrig[config_st->payLoadPedalConfig_.quantityOfControl];
  for(int i=0;i<config_st->payLoadPedalConfig_.quantityOfControl;i++)
  {
    yOrig[i]=calc_st->force[i];
  }

  //double dx = 1.0f;
  float t = (splineSegment_fl32 - (float)splineSegment_u8);// / dx;
  float y = (1.0f - t) * yOrig[splineSegment_u8] + t * yOrig[splineSegment_u8 + 1] + t * (1.0f - t) * (a * (1.0f - t) + b * t);
  
  if (calc_st->Force_Range> 0)
  {
      y = calc_st->Force_Min + y / 100.0f * calc_st->Force_Range;
  }
  else
  {
    y = calc_st->Force_Min;
  }


  return y;
}


/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: gradient computation                         */
/*                                                                                            */
/**********************************************************************************************/

float ForceCurve_Interpolated::EvalForceGradientCubicSpline(const DAP_config_st* config_st, const DAP_calculationVariables_st* calc_st, float fractionalPos, bool normalized_b)
{
  float fractionalPos_lcl = constrain(fractionalPos, 0, 1);
  float fractionalPos_float=fractionalPos_lcl*100.0f;
  //float splineSegment_fl32 = fractionalPos_lcl * 5.0f;
  float splineSegment_fl32 = 0;
  for(int i=0;i<config_st->payLoadPedalConfig_.quantityOfControl;i++)
  {
    if(fractionalPos_float>calc_st->travel[i])
    {
      if(i==config_st->payLoadPedalConfig_.quantityOfControl-1)
      {
        splineSegment_fl32=(float)i;
      }
      else
      {
        float diff= (fractionalPos_float-(float)calc_st->travel[i])/(float)(calc_st->travel[i+1]-calc_st->travel[i]);
        splineSegment_fl32=(float)i+diff;
      }  
    }
    else
    {
      break;
    }
  }
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  if (splineSegment_u8 > (config_st->payLoadPedalConfig_.quantityOfControl-1) )
  {
    splineSegment_u8 = config_st->payLoadPedalConfig_.quantityOfControl - 1;
  }
  float a = calc_st->interpolatorA[splineSegment_u8];
  float b = calc_st->interpolatorB[splineSegment_u8];

  float yOrig[config_st->payLoadPedalConfig_.quantityOfControl];
  for(int i=0;i<config_st->payLoadPedalConfig_.quantityOfControl;i++)
  {
    yOrig[i]=calc_st->force[i];
  }



  float Delta_x_orig = 100.0f; // total horizontal range [0,100]
  float dx = Delta_x_orig / config_st->payLoadPedalConfig_.quantityOfControl; // spline segment horizontal range
  float t = (splineSegment_fl32 - (float)splineSegment_u8); // relative position in spline segment [0, 1]
  
  float dy = yOrig[splineSegment_u8 + 1] - yOrig[splineSegment_u8]; // spline segment vertical range
  float y_prime = 0.0f;
  if (fabsf(dx) > 0)
  {
      y_prime = dy / dx + (1.0f - 2.0f * t) * (a * (1.0f - t) + b * t) / dx + t * (1.0f - t) * (b - a) / dx;
  }
  // when the spline was identified, x and y were givin in the unit of percent --> 0-100
  // --> conversion of the gradient to the proper axis scaling is performed
  if (normalized_b == false)
  {
    float d_y_scale = calc_st->Force_Range / 100.0f;
    float d_x_scale=0.0f;
    if (fabs(calc_st->stepperPosRange) > 0.01f)
    {
        d_x_scale = 100.0f / calc_st->stepperPosRange;
    }
    
    y_prime *= d_x_scale * d_y_scale;
  }

  return y_prime;
}


