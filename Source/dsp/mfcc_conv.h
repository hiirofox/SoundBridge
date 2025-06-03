#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include "fft.h"

class MFCCConverter {
private:
	constexpr static int MaxFFTLen = 16384;
	float buf1re[MaxFFTLen];
	float buf1im[MaxFFTLen];
	inline float toexp(float x, float n)
	{
		float sign = x < 0 ? -1.0f : 1.0f;
		x = fabsf(x);
		return (expf((x - 1.0) * n) - expf(-n)) / (1.0 - expf(-n)) * sign;
	};
public:
	MFCCConverter()
	{
		memset(buf1re, 0, sizeof(buf1re));
		memset(buf1im, 0, sizeof(buf1im));
	}
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

		FFTLen /= 2;		//后边镜像的没啥用了!!!!!!!!!!!!!!!!!!!!!!!!!!

		for (int i = 0; i < FFTLen; ++i)//功率谱
		{
			float r = sqrtf(buf1re[i] * buf1re[i] + buf1im[i] * buf1im[i]);
			buf1im[i] = logf(r + 0.00000001);//倒谱的
			//buf1im[i] = r;
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
			float indexf = toexp(x, 8) * FFTLen;//我说这就是mfcc，你说是不是
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

		/*
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
		float sum = 0.0001;
		for (int i = 0; i < numMFCC; ++i)
		{
			sum += buf1im[i];
			mfcc[i] = buf1im[i];
		}
		for (int i = 0; i < numMFCC; ++i)
		{
			mfcc[i] /= sum;
		}*/


		float sum = 0;
		for (int i = 0; i < numMFCC; ++i)
		{
			float v = buf1re[i * FFTLen / numMFCC];
			sum += v;
			mfcc[i] = v;
		}
		for (int i = 0; i < numMFCC; ++i)
		{
			//mfcc[i] /= sum;
		}
	}
};

/*
#include <cmath>
#include <vector>
#include <algorithm>

class MFCCConverter {
private:
	constexpr static int MaxFFTLen = 16384;
	float buf1re[MaxFFTLen];
	float buf1im[MaxFFTLen];
	int sampleRate = 48000; // 默认采样率16000Hz

	// 辅助函数：Mel频率转换
	inline float hzToMel(float hz) {
		return 2595.0f * log10f(1.0f + hz / 700.0f);
	}

	// 辅助函数：Mel频率逆转换
	inline float melToHz(float mel) {
		return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
	}

public:
	MFCCConverter() {
		memset(buf1re, 0, sizeof(buf1re));
		memset(buf1im, 0, sizeof(buf1im));
	}

	// 设置采样率
	void SetSampleRate(int rate) {
		sampleRate = rate;
	}

	void Process(const float* in, float* mfcc, int numSamples, int numMFCC) {
		int FFTLen = 1 << (int)(ceilf(log2f(numSamples)));
		int len = numSamples;
		if (FFTLen > MaxFFTLen) FFTLen = MaxFFTLen;
		if (len > MaxFFTLen) len = MaxFFTLen;

		// 加窗（汉宁窗）
		for (int i = 0; i < len; ++i) {
			float w = 0.5f - 0.5f * cosf(2.0f * M_PI * i / len);
			buf1re[i] = in[i] * w;
			buf1im[i] = 0;
		}
		for (int i = len; i < FFTLen; ++i) {
			buf1re[i] = 0;
			buf1im[i] = 0;
		}

		// 执行FFT
		fft_f32(buf1re, buf1im, FFTLen, 1);

		// 计算功率谱（只保留非负频率部分）
		int nfft = FFTLen / 2 + 1; // 非负频率点数
		for (int i = 0; i < nfft; ++i) {
			float real = buf1re[i];
			float imag = buf1im[i];
			float power = real * real + imag * imag;
			buf1im[i] = power; // 存储功率谱
		}

		// ========== 标准MFCC处理开始 ==========

		// 1. 设置Mel滤波器参数
		int nfilters = numMFCC + 2; // 滤波器数量 = MFCC系数+2
		if (nfilters < 26) nfilters = 26; // 最小26个滤波器
		if (nfilters > nfft) nfilters = nfft; // 不能超过频点数

		// 2. 创建Mel滤波器组
		float maxMel = hzToMel(sampleRate / 2.0f);
		std::vector<float> melPoints(nfilters + 2);
		for (int i = 0; i < nfilters + 2; i++) {
			melPoints[i] = maxMel * i / (nfilters + 1);
		}

		// 3. 将Mel频率转换为线性频率并映射到FFT bin
		std::vector<int> binIndices(nfilters + 2);
		for (int i = 0; i < nfilters + 2; i++) {
			float hz = melToHz(melPoints[i]);
			binIndices[i] = static_cast<int>((hz / (sampleRate / 2.0f)) * (nfft - 1));
			binIndices[i] = std::min(std::max(binIndices[i], 0), nfft - 1);
		}

		// 4. 计算Mel滤波器能量
		std::vector<float> filterEnergies(nfilters, 0.0f);
		for (int i = 0; i < nfilters; i++) {
			int left = binIndices[i];
			int center = binIndices[i + 1];
			int right = binIndices[i + 2];

			// 上升沿
			for (int j = left; j <= center; j++) {
				if (center != left) {
					float weight = static_cast<float>(j - left) / (center - left);
					filterEnergies[i] += buf1im[j] * weight;
				}
			}

			// 下降沿
			for (int j = center + 1; j <= right; j++) {
				if (right != center) {
					float weight = 1.0f - static_cast<float>(j - center) / (right - center);
					filterEnergies[i] += buf1im[j] * weight;
				}
			}

			// 避免对数运算错误
			if (filterEnergies[i] < 1e-10f) {
				filterEnergies[i] = 1e-10f;
			}
		}

		// 5. 取对数能量
		for (int i = 0; i < nfilters; i++) {
			filterEnergies[i] = logf(filterEnergies[i]);
		}

		// 6. DCT变换获取MFCC系数
		int numToCompute = std::min(numMFCC, nfilters);
		for (int k = 0; k < numToCompute; k++) {
			float sum = 0.0f;
			for (int n = 0; n < nfilters; n++) {
				sum += filterEnergies[n] *
					cosf(M_PI * k * (2.0f * n + 1.0f) / (2.0f * nfilters));
			}

			// DCT-II缩放因子
			if (k == 0) {
				sum *= sqrtf(1.0f / nfilters);
			}
			else {
				sum *= sqrtf(2.0f / nfilters);
			}

			mfcc[k] = sum;
		}

		// 7. 如果请求的系数多于滤波器数量，剩余部分置零
		for (int k = numToCompute; k < numMFCC; k++) {
			mfcc[k] = 0.0f;
		}
	}
};*/