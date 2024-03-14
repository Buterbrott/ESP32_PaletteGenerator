#ifndef PaletteGenerator_h
#define PaletteGenerator_h

#include "Arduino.h"
#include <FastLED.h>

class RGBcolor
{
public:
    RGBcolor();
    RGBcolor(int r, int g, int b);
    bool isValid();
    int _r;
    int _g;
    int _b;

private:
};

class HCLcolor
{
public:
    HCLcolor();
    HCLcolor(float h, float c, float l);
    bool inRange(int hmin, int hmax, int cmin, int cmax, int lmin, int lmax);
    int _h;
    int _c;
    int _l;

private:
};

class LABcolor
{
public:
    float _l;
    float _a;
    float _b;
    bool sorted;
    LABcolor();
    LABcolor(float l, float a, float b);
    LABcolor(const LABcolor &lab);
    LABcolor &operator=(const LABcolor &rhs);
    LABcolor &operator+=(const LABcolor &rhs);
    LABcolor &operator/=(const int &rhs);
    void setLAB(float fl, float fa, float fb);
    RGBcolor toRGB();
    HCLcolor toHCL();
    bool checkColor();

private:
    const struct
    {
        float xn = 0.950470;
        float yn = 1;
        float zn = 1.088830;

        float t0 = 0.137931034; // 4 / 29
        float t1 = 0.206896552; // 6 / 29
        float t2 = 0.12841855;  // 3 * t1 * t1
        float t3 = 0.008856452; // t1 * t1 * t1
    } _constants;
    float lab_xyz(float t);
    int xyz_rgb(float r);
};

class PaletteGenerator
{
public:
    static const int all[6];
    int _hmin;
    int _hmax;
    int _cmin;
    int _cmax;
    int _lmin;
    int _lmax;
    PaletteGenerator();
    void setColorSpace(const int *space);
    CRGBPalette16 generate(const byte colorsCount);

private:
    struct Vector
    {
        float dl, da, db;
        Vector(){};
        Vector(float dl, float da, float db) : dl(dl), da(da), db(db){};
    };
    float rnd();
    float euclidean(LABcolor lab1, LABcolor lab2);
    void initPalette(LABcolor *colors, byte colorsCount);
    void forceVector(LABcolor *colors, byte colorsCount);
    void kMeans(LABcolor *colors, byte colorsCount);
    bool checkColor(LABcolor lab);
};

#endif