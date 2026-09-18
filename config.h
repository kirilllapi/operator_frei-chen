#ifndef __CONFIG_MODULE__
#define __CONFIG_MODULE__


#define INITIAL_HUE_LOW         0
#define INITIAL_HUE_HIGH        250
#define INITIAL_SATURATION_LOW  150
#define INITIAL_SATURATION_HIGH 255
#define INITIAL_VALUE_LOW       200
#define INITIAL_VALUE_HIGH      255

#define N_PASSES                3   // Количество проходов фильтром Гаусса скользящего среднего
#define SIGMA                   1.70
#define ITER_DILATE             10


// NOTE  =========== Константы для перевода в LAB =========== 
#define EPSILON 0.008856
#define KAPPA 903.3
#define Xn 0.95047  // D65
#define Yn 1.00000  // D65
#define Zn 1.08883  // D65

#define L_MIN  0
#define A_MIN 125
#define B_MIN 200
#define L_MAX 255
#define A_MAX 255
#define B_MAX 255




#endif