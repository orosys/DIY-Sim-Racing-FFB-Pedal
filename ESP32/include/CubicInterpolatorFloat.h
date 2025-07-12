#pragma once
#include <Arduino.h> // For malloc, free, sqrtf, etc.
#define MAX_POINTS 100
// source https://swharden.com/blog/2022-01-22-spline-interpolation/
struct Result
{
    int count;
    float evenDistances[MAX_POINTS];
    float yInterp[MAX_POINTS];
    float a[MAX_POINTS];
    float b[MAX_POINTS];
};
class Cubic
{
public:
    Result _result;
    void Interpolate1D(const float *xs, const float *ys, int inputCount, int outputCount);
    
private:
    void FitMatrix(const float *x, const float *y, int n, float *a, float *b);
    void Interpolate(const float *xOrig, const float *yOrig, int nOrig,
                            const float *xInterp, int nInterp,
                            const float *a, const float *b,
                            float *yInterp);
};
//Cubic _cubicInterpolator;