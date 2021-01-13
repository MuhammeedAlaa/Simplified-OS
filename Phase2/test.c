#include <stdio.h>
#include <math.h>
int main()
{
    double num = 5;

    int result = log2(num);
    printf("log(%.1f) = %d", num, result);

    return 0;
}