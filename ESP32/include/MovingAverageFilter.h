#pragma once
#define MAX_DATA_POINTS 100
// moving average filter:https://github.com/sebnil/Moving-Avarage-Filter--Arduino-Library-/tree/master
class MovingAverageFilter
{
public:
  //construct without coefs
  MovingAverageFilter(unsigned int newDataPointsCount);
  int dataPointsCount;
  float process(float in);
  

private:
  float values[MAX_DATA_POINTS];
  int k; // k stores the index of the current array read to create a circular memory through the array
  
  float out;
  int i; // just a loop counter
};

MovingAverageFilter::MovingAverageFilter(unsigned int newDataPointsCount)
{
  k = 0; //initialize so that we start to write at index 0
  if (newDataPointsCount < MAX_DATA_POINTS)
    dataPointsCount = newDataPointsCount;
  else
    dataPointsCount = MAX_DATA_POINTS;

  for (i = 0; i < dataPointsCount; i++)
  {
    values[i] = 0.0f; // fill the array with 0's
  }
}

float MovingAverageFilter::process(float in)
{
  out = 0.0f;

  values[k] = in;
  k = (k + 1) % dataPointsCount;

  for (i = 0; i < dataPointsCount; i++)
  {
    out += values[i];
  }

  float retValue= 0.0f;
  if (dataPointsCount> 0)
  {
    retValue= out / dataPointsCount;
  }
  
  return retValue;
}