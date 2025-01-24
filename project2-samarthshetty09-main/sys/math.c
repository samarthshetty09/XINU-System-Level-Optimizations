#include "../h/math.h"
#include <stdio.h>

double log(double x) {
    if (x > 0 && x <= 1) {
        double y = (x - 1) / (x + 1);
        double y_squared = y * y;
        int n;
        double result = 0;
        double term = y;

        for (n = 1; n <= TAYLOR_ITERATIONS; n += 2) {
            result += term / n;
            term *= y_squared; 
        }

        return 2 * result;
    }
    return -1;
}

double expdev(double lambda)
{
    double dummy;
    do
    {
        dummy = (double)rand() / RAND_MAX;
    } while (dummy == 0.0);
    return -log(dummy) / lambda;
}

double pow(double base, int exponent)
{
    if (base == 0)
        return 0;
    if (exponent == 0)
        return 1;
    int i;
    double res = 1.0;
    for (i = 0; i < exponent; i++)
    {
        res *= base;
    }
    return res;
}
