#pragma once
#define _NOMINMAX
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <iostream>

//using namespace std;

#define DEFCOLOR_SRC(name, r, g, b) \
	static Color name##(uint8_t a = 255){ return Color(r, g, b, a); }

class Color
{
public:
    Color();
    Color(int _r, int _g, int _b);
    Color(int _r, int _g, int _b, int _a);
    Color(float _r, float _g, float _b, float _a = 255.f)
        : Color(
            static_cast<int>(_r),
            static_cast<int>(_g),
            static_cast<int>(_b),
            static_cast<int>(_a))
    {
    }
    explicit Color(float* rgb) : Color(rgb[0], rgb[1], rgb[2], 1.0f) {}
    explicit Color(unsigned long argb)
    {
        _CColor[2] = (unsigned char)((argb & 0x000000FF) >> (0 * 8));
        _CColor[1] = (unsigned char)((argb & 0x0000FF00) >> (1 * 8));
        _CColor[0] = (unsigned char)((argb & 0x00FF0000) >> (2 * 8));
        _CColor[3] = (unsigned char)((argb & 0xFF000000) >> (3 * 8));
    }

    void    SetRawColor(int color32);
    int     GetRawColor() const;
    void    SetColor(int _r, int _g, int _b, int _a = 0);
    void    SetColor(float _r, float _g, float _b, float _a = 0);
    void    GetColor(int &_r, int &_g, int &_b, int &_a) const;

    int r() const { return _CColor[0]; }
    int g() const { return _CColor[1]; }
    int b() const { return _CColor[2]; }
    int a() const { return _CColor[3]; }

    unsigned char &operator[](int index)
    {
        return _CColor[index];
    }
    const unsigned char &operator[](int index) const
    {
        return _CColor[index];
    }

    bool operator==(const Color &rhs) const;
    bool operator!=(const Color &rhs) const;
    Color &operator=(const Color &rhs);

	DEFCOLOR_SRC(Black, 0, 0, 0);
	DEFCOLOR_SRC(White, 255, 255, 255);
	DEFCOLOR_SRC(Red, 255, 0, 0);
	DEFCOLOR_SRC(Green, 0, 255, 0);
	DEFCOLOR_SRC(Blue, 0, 0, 255);
	DEFCOLOR_SRC(Lime, 0, 255, 0);
	DEFCOLOR_SRC(Yellow, 255, 255, 0);
	DEFCOLOR_SRC(Cyan, 0, 255, 255);
	DEFCOLOR_SRC(Magenta, 255, 0, 255);
	DEFCOLOR_SRC(Silver, 192, 192, 192);
	DEFCOLOR_SRC(Gray, 128, 128, 128);
	DEFCOLOR_SRC(Maroon, 128, 0, 0);
	DEFCOLOR_SRC(Olive, 128, 128, 0);
	DEFCOLOR_SRC(Purple, 128, 0, 128);
	DEFCOLOR_SRC(Teal, 0, 128, 128);
	DEFCOLOR_SRC(Navy, 0, 0, 128);
	DEFCOLOR_SRC(DarkRed, 139, 0, 0);
	DEFCOLOR_SRC(Brown, 165, 42, 42);
	DEFCOLOR_SRC(Firebrick, 178, 34, 34);
	DEFCOLOR_SRC(Crimson, 220, 20, 60);
	DEFCOLOR_SRC(IndianRed, 205, 92, 92);
	DEFCOLOR_SRC(LightCoral, 240, 128, 128);
	DEFCOLOR_SRC(DarkSalmon, 233, 150, 122);
	DEFCOLOR_SRC(Salmon, 250, 128, 114);
	DEFCOLOR_SRC(LightSalmon, 255, 160, 122);
	DEFCOLOR_SRC(OrangeRed, 255, 69, 0);
	DEFCOLOR_SRC(DarkOrange, 255, 140, 0);
	DEFCOLOR_SRC(Orange, 255, 165, 0);
	DEFCOLOR_SRC(Gold, 255, 215, 0);
	DEFCOLOR_SRC(DarkGoldenRod, 184, 134, 11);
	DEFCOLOR_SRC(GoldenRod, 218, 165, 32);
	DEFCOLOR_SRC(YellowGreen, 154, 205, 50);
	DEFCOLOR_SRC(DarkOliveGreen, 85, 107, 47);
	DEFCOLOR_SRC(OliveDrab, 107, 142, 35);
	DEFCOLOR_SRC(LawnGreen, 124, 252, 0);
	DEFCOLOR_SRC(ChartReuse, 127, 255, 0);
	DEFCOLOR_SRC(GreenYellow, 173, 255, 47);
	DEFCOLOR_SRC(DarkGreen, 0, 100, 0);
	DEFCOLOR_SRC(ForestGreen, 34, 139, 34);
	DEFCOLOR_SRC(LimeGreen, 50, 205, 50);
	DEFCOLOR_SRC(DarkCyan, 0, 139, 139);
	DEFCOLOR_SRC(Aqua, 0, 255, 255);
	DEFCOLOR_SRC(LightCyan, 224, 255, 255);
	DEFCOLOR_SRC(DarkTurquoise, 0, 206, 209);
	DEFCOLOR_SRC(Turquoise, 64, 224, 208);
	DEFCOLOR_SRC(MediumTurquoise, 72, 209, 204);
	DEFCOLOR_SRC(PaleTurquoise, 175, 238, 238);
	DEFCOLOR_SRC(Aquamarine, 127, 255, 212);
	DEFCOLOR_SRC(PowderBlue, 176, 224, 230);
	DEFCOLOR_SRC(DodgerBlue, 30, 144, 255);
	DEFCOLOR_SRC(Lightblue, 173, 216, 230);
	DEFCOLOR_SRC(SkyBlue, 135, 206, 235);
	DEFCOLOR_SRC(LightSkyBlue, 135, 206, 250);
	DEFCOLOR_SRC(MidnightBlue, 25, 25, 112);
	DEFCOLOR_SRC(DarkBlue, 0, 0, 139);
	DEFCOLOR_SRC(MediumBlue, 0, 0, 205);
	DEFCOLOR_SRC(RoyalBlue, 65, 105, 225);
	DEFCOLOR_SRC(BlueViolet, 138, 43, 226);
	DEFCOLOR_SRC(Indigo, 75, 0, 130);
	DEFCOLOR_SRC(DarkSlateBlue, 72, 61, 139);
	DEFCOLOR_SRC(SlateBlue, 106, 90, 205);
	DEFCOLOR_SRC(MediumSlateBlue, 123, 104, 238);
	DEFCOLOR_SRC(MediumPurple, 147, 112, 219);
	DEFCOLOR_SRC(Darkmagenta, 139, 0, 139);
	DEFCOLOR_SRC(Darkviolet, 148, 0, 211);
	DEFCOLOR_SRC(DarkGray, 169, 169, 169);
	DEFCOLOR_SRC(LightGray, 211, 211, 211);
	DEFCOLOR_SRC(Gainsboro, 220, 220, 220);

private:
    unsigned char _CColor[4];
};

class PizdaColor {
public:
	float r, g, b, a;
	float h, s, v;
	bool rainbow;
	PizdaColor* next;
	PizdaColor() : r(0), g(0), b(0), a(255), rainbow(false)
	{

	}
	PizdaColor(Color col) : r(col.r()), g(col.g()), b(col.b()), a(col.a()), rainbow(false)
	{

	}
	PizdaColor(Color col, bool rainbow) : r(col.r()), g(col.g()), b(col.b()), a(col.a()), rainbow(rainbow)
	{

	}
	PizdaColor(float r, float g, float b, float a, bool rainbow) : r(r), g(g), b(b), a(a), rainbow(rainbow)
	{

	}
	PizdaColor(float r, float g, float b) : r(r), g(g), b(b), a(255), rainbow(false)
	{

	}
	static auto fromHSV(float fH, float fS, float fV, float& fR, float& fG, float& fB) -> void
	{
		float fC = fV * fS; // Chroma
		float fHPrime = fmod(fH / 60.0, 6);
		float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
		float fM = fV - fC;

		if (0 <= fHPrime && fHPrime < 1) {
			fR = fC;
			fG = fX;
			fB = 0;
		}
		else if (1 <= fHPrime && fHPrime < 2) {
			fR = fX;
			fG = fC;
			fB = 0;
		}
		else if (2 <= fHPrime && fHPrime < 3) {
			fR = 0;
			fG = fC;
			fB = fX;
		}
		else if (3 <= fHPrime && fHPrime < 4) {
			fR = 0;
			fG = fX;
			fB = fC;
		}
		else if (4 <= fHPrime && fHPrime < 5) {
			fR = fX;
			fG = 0;
			fB = fC;
		}
		else if (5 <= fHPrime && fHPrime < 6) {
			fR = fC;
			fG = 0;
			fB = fX;
		}
		else {
			fR = 0;
			fG = 0;
			fB = 0;
		}

		fR += fM;
		fG += fM;
		fB += fM;
	}

	static auto fromRGB(float fR, float fG, float fB, float& fH, float& fS, float& fV) -> void {
		float fCMax = std::fmaxf(std::fmaxf(fR, fG), fB);
		float fCMin = std::fminf(std::fminf(fR, fG), fB);
		float fDelta = fCMax - fCMin;

		if (fDelta > 0) {
			if (fCMax == fR) {
				fH = 60 * (fmod(((fG - fB) / fDelta), 6));
			}
			else if (fCMax == fG) {
				fH = 60 * (((fB - fR) / fDelta) + 2);
			}
			else if (fCMax == fB) {
				fH = 60 * (((fR - fG) / fDelta) + 4);
			}

			if (fCMax > 0) {
				fS = fDelta / fCMax;
			}
			else {
				fS = 0;
			}

			fV = fCMax;
		}
		else {
			fH = 0;
			fS = 0;
			fV = fCMax;
		}

		if (fH < 0) {
			fH = 360 + fH;
		}
	}

	void UpdateByHSV(float h, float s, float v) {
		fromHSV(h, s, v, r, g, b);
		r *= 255.f;
		g *= 255.f;
		b *= 255.f;
		this->h = h == 0 ? this->h : h;
		this->s = s;
		this->v = v;
	}
	void Update() {
		fromRGB(r / 255.f, g / 255.f, b / 255.f, h, s, v);
		if (rainbow)
		{
			h += 1.f;
			h = h >= 360 ? h - 360 : h;
			h = h == 0 ? 1 : h;
			UpdateByHSV(h, s, v);
		}
	}
	operator Color() { return Color(r, g, b); }
	Color ReturnEngineColor(float alpha = 255.f) {
		return Color(r, g, b, alpha);
	}
};