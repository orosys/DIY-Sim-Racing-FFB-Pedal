#include "CubicInterpolatorFloat.h"
#include <math.h>

void Cubic::Interpolate1D(const float *xs, const float *ys, int inputCount, int outputCount)
{
    //Result result;
    float inputDistances[MAX_POINTS] = {0};

    for (int i = 1; i < inputCount; i++)
    {
        float dx = xs[i] - xs[i - 1];
        inputDistances[i] = inputDistances[i - 1] + fabsf(dx);
    }

    float meanDistance = inputDistances[inputCount - 1] / (outputCount - 1);
    for (int i = 0; i < outputCount; i++)
    {
        _result.evenDistances[i] = i * meanDistance;
    }

    FitMatrix(xs, ys, inputCount, _result.a, _result.b);
    Interpolate(xs, ys, inputCount, _result.evenDistances, outputCount, _result.a, _result.b, _result.yInterp);
    //_result=result;
    _result.count = outputCount;
    //return result;
}

void Cubic::FitMatrix(const float *x, const float *y, int n, float *a, float *b)
{
    float r[MAX_POINTS] = {0}, A[MAX_POINTS] = {0}, B[MAX_POINTS] = {0}, C[MAX_POINTS] = {0};
    float dx1, dx2, dy1, dy2;

    dx1 = x[1] - x[0];
    C[0] = 1.0f / dx1;
    B[0] = 2.0f * C[0];
    r[0] = 3.0f * (y[1] - y[0]) / (dx1 * dx1);

    for (int i = 1; i < n - 1; i++)
    {
        dx1 = x[i] - x[i - 1];
        dx2 = x[i + 1] - x[i];
        A[i] = 1.0f / dx1;
        C[i] = 1.0f / dx2;
        B[i] = 2.0f * (A[i] + C[i]);
        dy1 = y[i] - y[i - 1];
        dy2 = y[i + 1] - y[i];
        r[i] = 3.0f * (dy1 / (dx1 * dx1) + dy2 / (dx2 * dx2));
    }

    dx1 = x[n - 1] - x[n - 2];
    dy1 = y[n - 1] - y[n - 2];
    A[n - 1] = 1.0f / dx1;
    B[n - 1] = 2.0f * A[n - 1];
    r[n - 1] = 3.0f * (dy1 / (dx1 * dx1));

    float cPrime[MAX_POINTS] = {0};
    float dPrime[MAX_POINTS] = {0};
    float k[MAX_POINTS] = {0};

    cPrime[0] = C[0] / B[0];
    for (int i = 1; i < n; i++)
        cPrime[i] = C[i] / (B[i] - cPrime[i - 1] * A[i]);

    dPrime[0] = r[0] / B[0];
    for (int i = 1; i < n; i++)
        dPrime[i] = (r[i] - dPrime[i - 1] * A[i]) / (B[i] - cPrime[i - 1] * A[i]);

    k[n - 1] = dPrime[n - 1];
    for (int i = n - 2; i >= 0; i--)
        k[i] = dPrime[i] - cPrime[i] * k[i + 1];

    for (int i = 1; i < n; i++)
    {
        dx1 = x[i] - x[i - 1];
        dy1 = y[i] - y[i - 1];
        a[i - 1] = k[i - 1] * dx1 - dy1;
        b[i - 1] = -k[i] * dx1 + dy1;
    }
}

void Cubic::Interpolate(const float *xOrig, const float *yOrig, int nOrig,
                        const float *xInterp, int nInterp,
                        const float *a, const float *b,
                        float *yInterp)
{
    for (int i = 0; i < nInterp; i++)
    {
        int j;
        for (j = 0; j < nOrig - 2; j++)
        {
            if (xInterp[i] <= xOrig[j + 1])
                break;
        }

        float dx = xOrig[j + 1] - xOrig[j];
        float t = (xInterp[i] - xOrig[j]) / dx;
        yInterp[i] = (1.0f - t) * yOrig[j] + t * yOrig[j + 1] +
                     t * (1.0f - t) * (a[j] * (1.0f - t) + b[j] * t);
    }
}