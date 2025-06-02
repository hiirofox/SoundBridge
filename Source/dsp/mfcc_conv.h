#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include "fft.h"

class MFCCConverter {
private:
	constexpr static int MaxFFTLen = 4096;
	float buf1re[MaxFFTLen + 3];
	float buf1im[MaxFFTLen + 3];
	inline float toexp(float x, float n)
	{
		float sign = x < 0 ? -1.0f : 1.0f;
		x = fabsf(x);
		return (expf((x - 1.0) * n) - expf(-n)) / (1.0 - expf(-n)) * sign;
	};
public:
	void Process(const float* in, float* mfcc, int numSamples, int numMFCC)
	{
		int FFTLen = 1 << (int)(ceilf(log2f(numSamples)));
		int len = numSamples;
		if (FFTLen > MaxFFTLen)FFTLen = MaxFFTLen;
		if (len > MaxFFTLen)len = MaxFFTLen;
		for (int i = 0; i < len; ++i)
		{
			float w = 0.5 - 0.5 * cosf(2.0 * M_PI * i / len);
			buf1re[i] = in[i] * w;
			buf1im[i] = 0;
		}
		for (int i = len; i < FFTLen; ++i)
		{
			buf1re[i] = 0;
			buf1im[i] = 0;
		}
		fft_f32(buf1re, buf1im, FFTLen, 1);
		FFTLen /= 2;		//后边镜像的没啥用了
		for (int i = 0; i < FFTLen; ++i)//功率谱
		{
			float r = sqrtf(buf1re[i] * buf1re[i] + buf1im[i] * buf1im[i]);
			buf1im[i] = logf(r + 0.00000001);
		}
		for (int i = 0; i < FFTLen; ++i)//搬过来挪三个采样，方便插值
		{
			buf1re[i + 3] = buf1im[i];
		}
		buf1re[0] = buf1re[1] = buf1re[2] = buf1re[3];
		buf1re[FFTLen] = buf1re[FFTLen + 1] = buf1re[FFTLen + 2] = buf1re[FFTLen - 1];
		float y0 = 0, y1 = 0, y2 = 0, y3 = 0, y4 = 0, y5 = 0;

		for (int i = 0; i < FFTLen; ++i)//插值到mfcc谱
		{
			float x = (float)i / FFTLen;
			float indexf = toexp(x, 5) * FFTLen;//我说这就是mfcc，你说是不是
			int index = indexf;

			y0 = buf1re[index + 0];
			y1 = buf1re[index + 1];
			y2 = buf1re[index + 2];
			y3 = buf1re[index + 3];
			y4 = buf1re[index + 4];
			y5 = buf1re[index + 5];

			// 计算五次Lagrange基函数
			float t = indexf - index;
			float t_plus_2 = t + 2;
			float t_plus_1 = t + 1;
			float t_minus_1 = t - 1;
			float t_minus_2 = t - 2;
			float t_minus_3 = t - 3;

			float L0 = (t_plus_1 * t * t_minus_1 * t_minus_2 * t_minus_3) / (-120.0f);
			float L1 = (t_plus_2 * t * t_minus_1 * t_minus_2 * t_minus_3) / 24.0f;
			float L2 = (t_plus_2 * t_plus_1 * t_minus_1 * t_minus_2 * t_minus_3) / (-12.0f);
			float L3 = (t_plus_2 * t_plus_1 * t * t_minus_2 * t_minus_3) / 12.0f;
			float L4 = (t_plus_2 * t_plus_1 * t * t_minus_1 * t_minus_3) / (-24.0f);
			float L5 = (t_plus_2 * t_plus_1 * t * t_minus_1 * t_minus_2) / 120.0f;

			float v = y0 * L0 + y1 * L1 + y2 * L2 + y3 * L3 + y4 * L4 + y5 * L5;
			buf1im[i] = v;//是mfcc谱了！
		}
		for (int i = 0; i < FFTLen; ++i)//清空
		{
			buf1re[i] = 0;
		}
		float lpfv = buf1im[0];//对mfcc谱滤波
		float ctof = (float)numMFCC / FFTLen;//这个cutoff要调整的
		for (int i = 0; i < FFTLen; ++i)//正向
		{
			lpfv += ctof * (buf1im[i] - lpfv);
			buf1re[i] += lpfv;
		}
		lpfv = buf1im[FFTLen - 1];
		for (int i = FFTLen - 1; i >= 0; i--)//反向
		{
			lpfv += ctof * (buf1im[i] - lpfv);
			buf1re[i] += lpfv;
		}
		//现在buf1re就是滤波过后的mfcc谱（不是标准的），可以直接抽取来用
		int FFTLen2 = 1 << (int)(ceilf(log2f(numMFCC)));
		for (int i = 0; i < FFTLen2; ++i)
		{
			buf1im[i] = buf1re[i * FFTLen / FFTLen2];
			buf1re[i] = 0;
		}
		for (int i = 0; i < FFTLen2; ++i)
		{
			buf1im[FFTLen2 - i - 1] = buf1im[i];
			buf1re[FFTLen2 - i - 1] = 0;
		}
		fft_f32(buf1im, buf1re, FFTLen2 * 2, -1);
		for (int i = 0; i < numMFCC; ++i)
		{
			mfcc[i] = buf1im[i];
		}
	}
};