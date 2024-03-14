#include "Arduino.h"
#include "PaletteGenerator.hpp"
#include <FastLED.h>
#include <vector>

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

RGBcolor::RGBcolor(){};
RGBcolor::RGBcolor(int r, int g, int b) : _r(r), _g(g), _b(b){};
bool RGBcolor::isValid()
{
    return _r >= 0 && _r <= 255 &&
           _g >= 0 && _g <= 255 &&
           _b >= 0 && _b <= 255;
}

HCLcolor::HCLcolor(){};
HCLcolor::HCLcolor(float h, float c, float l) : _h(h), _c(c), _l(l){};
bool HCLcolor::inRange(int hmin, int hmax, int cmin, int cmax, int lmin, int lmax)
{
    return _h >= hmin && _h <= hmax &&
           _c >= cmin && _c <= cmax &&
           _l >= lmin && _l <= lmax;
}

LABcolor::LABcolor(){};
LABcolor::LABcolor(float fl, float fa, float fb) : _l(fl), _a(fa), _b(fb){};
LABcolor::LABcolor(const LABcolor &lab)
{
    _l = lab._l;
    _a = lab._a;
    _b = lab._b;
}
float LABcolor::lab_xyz(float t)
{
    return (t > _constants.t1) ? t * t * t : _constants.t2 * (t - _constants.t0);
}
int LABcolor::xyz_rgb(float r)
{
    return round(255.0 * (r <= 0.00304 ? 12.92 * r : 1.055 * pow(r, 1 / 2.4) - 0.055));
};
LABcolor &LABcolor::operator=(const LABcolor &rhs)
{
    _l = rhs._l;
    _a = rhs._a;
    _b = rhs._b;
    return *this;
}
LABcolor &LABcolor::operator+=(const LABcolor &rhs)
{
    _l += rhs._l;
    _a += rhs._a;
    _b += rhs._b;
    return *this;
}
LABcolor &LABcolor::operator/=(const int &rhs)
{
    _l /= rhs;
    _a /= rhs;
    _b /= rhs;
    return *this;
}
void LABcolor::setLAB(float fl, float fa, float fb)
{
    _l = fl;
    _a = fa;
    _b = fb;
}
RGBcolor LABcolor::toRGB()
{
    float y = (_l + 16.0) / 116.0;
    float x = y + _a / 500.0;
    float z = y - _b / 200.0;

    x = _constants.xn * lab_xyz(x);
    y = _constants.yn * lab_xyz(y);
    z = _constants.zn * lab_xyz(z);

    int r = xyz_rgb(3.2404542 * x - 1.5371385 * y - 0.4985314 * z);
    int g = xyz_rgb(-0.9692660 * x + 1.8760108 * y + 0.0415560 * z);
    int b = xyz_rgb(0.0556434 * x - 0.2040259 * y + 1.0572252 * z);

    RGBcolor rgb = RGBcolor(r, g, b);
    return rgb;
}
HCLcolor LABcolor::toHCL()
{
    float h = atan2(_b, _a) * RAD_TO_DEG + 360;
    h = (int)trunc(h) % 360 + (h - trunc(h));
    float c = sqrt(_a * _a + _b * _b);
    float l = _l;

    if (round(c * 10000) == 0)
        h = -1;

    HCLcolor hcl = HCLcolor(h, c, l);
    return hcl;
}

PaletteGenerator::PaletteGenerator(){
    // int a,b;
    // a = rnd()*360;
    // b = rnd()*360;
    // _hmin = _min(a,b);
    // _hmax = _max(a,b);
    // a = rnd()*100;
    // b = rnd()*100;
    // _cmin = _min(a,b);
    // _cmax = _max(a,b);
    // a = rnd()*100;
    // b = rnd()*100;
    // _lmin = _min(a,b);
    // _lmax = _max(a,b);
    _hmin = 0;
    _hmax = 360;
    _cmin = 0;
    _cmax = 100;
    _lmin = 0;
    _lmax = 100;
};
bool PaletteGenerator::checkColor(LABcolor lab)
{
    return lab.toRGB().isValid() && lab.toHCL().inRange(_hmin,_hmax,_cmin,_cmax,_lmin,_lmax);
}
CRGBPalette16 PaletteGenerator::generate(const byte colorsCount)
{

    LABcolor colors[colorsCount];
    initPalette(colors, colorsCount);

    forceVector(colors,colorsCount);
    //kMeans(colors, colorsCount);

    // // sort by hue
    // for (size_t i = 1; i < 16; i++)
    // {
    //   for (size_t j = i; j > 0; j--)
    //   {
    //     if (colors[j - 1].toHCL()._h > colors[j].toHCL()._h)
    //     {
    //       LABcolor tmp = LABcolor(colors[j - 1]._l, colors[j - 1]._a, colors[j - 1]._b);
    //       colors[j - 1].setLAB(colors[j]._l, colors[j]._a, colors[j]._b);
    //       colors[j].setLAB(tmp._l, tmp._a, tmp._b);
    //     }
    //   }
    // }

    
    LABcolor colors_sorted[16];
    byte r = esp_random()/(UINT32_MAX/16);
    colors_sorted[0] = colors[r];
    colors[r].sorted = true;
    byte sorted = 1;
    while (sorted<colorsCount){
        float minD = 99999.0;
        byte closest = 0;
        for (byte i = 0; i<colorsCount; i++){
            if ( colors[i].sorted )
                continue;
            float d = euclidean(colors_sorted[sorted-1],colors[i]);
            if (d < minD){
                minD = d;
                closest = i;
            }
        }
        sorted++;
        colors_sorted[sorted-1] = colors[closest];
        colors[closest].sorted = true;
    }

    CRGB ca[16];
    CRGBPalette16 palette;
    for (byte i = 0; i < colorsCount; i++)
    {
        RGBcolor rgb;
        rgb = colors_sorted[i].toRGB();
        ca[i].setRGB(rgb._r, rgb._g, rgb._b);
    }
    palette = ca;
    return palette;
}

float PaletteGenerator::rnd()
{
    return (float)esp_random() / UINT32_MAX;
}

float PaletteGenerator::euclidean(LABcolor lab1, LABcolor lab2)
{
    return sqrt(
        sq(lab1._l - lab2._l) +
        sq(lab1._a - lab2._a) +
        sq(lab1._b - lab2._b));
}

void PaletteGenerator::initPalette(LABcolor *colors, byte colorsCount)
{
    for (byte i = 0; i < colorsCount; i++)
    {
        do
        {
            colors[i].setLAB(
                100 * rnd(),
                100 * (2 * rnd() - 1),
                100 * (2 * rnd() - 1));
        } while (!checkColor(colors[i]));
        colors[i].sorted = false;
    }
}

void PaletteGenerator::forceVector(LABcolor *colors, byte colorsCount)
{
    Vector vectors[colorsCount];
    float repulsion = 100.0;
    float speed = 100.0;
    int steps = 1000;
    while (steps-- > 0)
    {

        // reset vectors
        for (byte i = 0; i < colorsCount; i++)
            vectors[i] = {0, 0, 0};

        // Computing force
        for (byte i = 0; i < colorsCount; i++)
        {
            for (byte j = 0; j < i; j++)
            {
                float d = euclidean(colors[i], colors[j]);
                if (d > 0)
                {
                    float dl = colors[i]._l - colors[j]._l;
                    float da = colors[i]._a - colors[j]._a;
                    float db = colors[i]._b - colors[j]._b;

                    float d1 = 1 / d;
                    float force = repulsion / sq(d);

                    vectors[i].dl += (dl * force) * d1;
                    vectors[i].da += (da * force) * d1;
                    vectors[i].db += (db * force) * d1;

                    vectors[j].dl -= (dl * force) * d1;
                    vectors[j].da -= (da * force) * d1;
                    vectors[j].db -= (db * force) * d1;
                }
                else
                {
                    // Jitter
                    vectors[j].dl += 2 - 4 * rnd();
                    vectors[j].da += 2 - 4 * rnd();
                    vectors[j].db += 2 - 4 * rnd();
                }
            }
        }

        // Applying force
        for (byte i = 0; i < colorsCount; i++)
        {
            float displacement = speed * sqrt(
                                             sq(vectors[i].dl) +
                                             sq(vectors[i].da) +
                                             sq(vectors[i].db));

            if (displacement > 0)
            {
                float ratio = (speed * _min(0.1, displacement)) / displacement;

                LABcolor candidateLab = LABcolor(
                    colors[i]._l + vectors[i].dl * ratio,
                    colors[i]._a + vectors[i].da * ratio,
                    colors[i]._b + vectors[i].db * ratio);

                if (checkColor(candidateLab))
                {
                    colors[i] = candidateLab;
                }
            }
        }
    }
}

void PaletteGenerator::kMeans(LABcolor *colors, byte colorsCount)
{

    byte linc = 5;
    byte ainc = 10;
    byte binc = 10;

    std::vector<LABcolor *> colorSamples;
    std::vector<LABcolor *> freeColorSamples;

    for (byte l = 0; l <= 100; l += linc)
    {
        for (int a = -100; a <= 100; a += ainc)
        {
            for (int b = -100; b <= 100; b += binc)
            {
                LABcolor *c = new LABcolor(l, a, b);
                if (checkColor(*c))
                {
                    colorSamples.push_back(c);
                }
                else
                {
                    delete c;
                }
            }
        }
    }

    uint16_t s = colorSamples.size();
    byte *samplesClosest = new byte[s];

    byte steps = 50;

    while (steps-- > 0)
    {

        freeColorSamples = colorSamples;
        auto fcsIterator = freeColorSamples.cbegin();

        // Finding closest color
        for (int i = 0; i < s; i++)
        {
            samplesClosest[i] = 0;
            float minDistance = 9999.0;

            for (byte j = 0; j < colorsCount; j++)
            {
                float d = euclidean(colors[j], *colorSamples[i]);

                if (d < minDistance)
                {
                    minDistance = d;
                    samplesClosest[i] = j;
                }
            }
        }

        LABcolor candidate;
        for (byte j = 0; j < colorsCount; j++)
        {
            int count = 0;
            candidate.setLAB(0, 0, 0);

            for (int i = 0; i < s; i++)
            {
                if (samplesClosest[i] == j)
                {
                    count++;
                    candidate += *colorSamples[i];
                }
            }

            if (count != 0)
            {

                candidate /= count;

                if (checkColor(candidate))
                {
                    colors[j] = candidate;
                }
                else
                {
                    Serial.println(freeColorSamples.size());
                    // The candidate is out of the boundaries of our color space or unfound
                    if (!freeColorSamples.empty())
                    {
                        // We just search for the closest free color
                        float minDistance = UINT32_MAX;
                        int closest = -1;

                        for (int i = 0; i < freeColorSamples.size(); i++)
                        {
                            float d = euclidean(*freeColorSamples[i], candidate);
                            if (d < minDistance)
                            {
                                minDistance = d;
                                closest = i;
                            }
                        }

                        colors[j] = *freeColorSamples[closest];
                        freeColorSamples.erase(fcsIterator + closest);
                    }
                    else
                    {
                        // Then we just search for the closest color
                        float minDistance = 9999.9;
                        int closest = -1;

                        for (int i = 0; i < s; i++)
                        {
                            float d = euclidean(*colorSamples[i], candidate);
                            if (d < minDistance)
                            {
                                minDistance = d;
                                closest = i;
                            }
                        }
                        colors[j] = *colorSamples[closest];
                    }
                }
            }
        }
    }
}