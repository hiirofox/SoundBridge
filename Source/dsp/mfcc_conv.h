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

		FFTLen /= 2;		//��߾����ûɶ����!!!!!!!!!!!!!!!!!!!!!!!!!!

		for (int i = 0; i < FFTLen; ++i)//������
		{
			float r = sqrtf(buf1re[i] * buf1re[i] + buf1im[i] * buf1im[i]);
			buf1im[i] = logf(r + 0.00000001);//���׵�
			//buf1im[i] = r;
		}
		for (int i = 0; i < FFTLen; ++i)//�����Ų���������������ֵ
		{
			buf1re[i + 3] = buf1im[i];
		}
		buf1re[0] = buf1re[1] = buf1re[2] = buf1re[3];
		buf1re[FFTLen] = buf1re[FFTLen + 1] = buf1re[FFTLen + 2] = buf1re[FFTLen - 1];
		float y0 = 0, y1 = 0, y2 = 0, y3 = 0, y4 = 0, y5 = 0;

		for (int i = 0; i < FFTLen; ++i)//��ֵ��mfcc��
		{
			float x = (float)i / FFTLen;
			float indexf = toexp(x, 8) * FFTLen;//��˵�����mfcc����˵�ǲ���
			int index = indexf;

			y0 = buf1re[index + 0];
			y1 = buf1re[index + 1];
			y2 = buf1re[index + 2];
			y3 = buf1re[index + 3];
			y4 = buf1re[index + 4];
			y5 = buf1re[index + 5];

			// �������Lagrange������
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
			buf1im[i] = v;//��mfcc���ˣ�
		}
		for (int i = 0; i < FFTLen; ++i)//���
		{
			buf1re[i] = 0;
		}
		float lpfv = buf1im[0];//��mfcc���˲�
		float ctof = (float)numMFCC / FFTLen;//���cutoffҪ������
		for (int i = 0; i < FFTLen; ++i)//����
		{
			lpfv += ctof * (buf1im[i] - lpfv);
			buf1re[i] += lpfv;
		}
		lpfv = buf1im[FFTLen - 1];
		for (int i = FFTLen - 1; i >= 0; i--)//����
		{
			lpfv += ctof * (buf1im[i] - lpfv);
			buf1re[i] += lpfv;
		}
		//����buf1re�����˲������mfcc�ף����Ǳ�׼�ģ�������ֱ�ӳ�ȡ����

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
	int sampleRate = 48000; // Ĭ�ϲ�����16000Hz

	// ����������MelƵ��ת��
	inline float hzToMel(float hz) {
		return 2595.0f * log10f(1.0f + hz / 700.0f);
	}

	// ����������MelƵ����ת��
	inline float melToHz(float mel) {
		return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
	}

public:
	MFCCConverter() {
		memset(buf1re, 0, sizeof(buf1re));
		memset(buf1im, 0, sizeof(buf1im));
	}

	// ���ò�����
	void SetSampleRate(int rate) {
		sampleRate = rate;
	}

	void Process(const float* in, float* mfcc, int numSamples, int numMFCC) {
		int FFTLen = 1 << (int)(ceilf(log2f(numSamples)));
		int len = numSamples;
		if (FFTLen > MaxFFTLen) FFTLen = MaxFFTLen;
		if (len > MaxFFTLen) len = MaxFFTLen;

		// �Ӵ�����������
		for (int i = 0; i < len; ++i) {
			float w = 0.5f - 0.5f * cosf(2.0f * M_PI * i / len);
			buf1re[i] = in[i] * w;
			buf1im[i] = 0;
		}
		for (int i = len; i < FFTLen; ++i) {
			buf1re[i] = 0;
			buf1im[i] = 0;
		}

		// ִ��FFT
		fft_f32(buf1re, buf1im, FFTLen, 1);

		// ���㹦���ף�ֻ�����Ǹ�Ƶ�ʲ��֣�
		int nfft = FFTLen / 2 + 1; // �Ǹ�Ƶ�ʵ���
		for (int i = 0; i < nfft; ++i) {
			float real = buf1re[i];
			float imag = buf1im[i];
			float power = real * real + imag * imag;
			buf1im[i] = power; // �洢������
		}

		// ========== ��׼MFCC����ʼ ==========

		// 1. ����Mel�˲�������
		int nfilters = numMFCC + 2; // �˲������� = MFCCϵ��+2
		if (nfilters < 26) nfilters = 26; // ��С26���˲���
		if (nfilters > nfft) nfilters = nfft; // ���ܳ���Ƶ����

		// 2. ����Mel�˲�����
		float maxMel = hzToMel(sampleRate / 2.0f);
		std::vector<float> melPoints(nfilters + 2);
		for (int i = 0; i < nfilters + 2; i++) {
			melPoints[i] = maxMel * i / (nfilters + 1);
		}

		// 3. ��MelƵ��ת��Ϊ����Ƶ�ʲ�ӳ�䵽FFT bin
		std::vector<int> binIndices(nfilters + 2);
		for (int i = 0; i < nfilters + 2; i++) {
			float hz = melToHz(melPoints[i]);
			binIndices[i] = static_cast<int>((hz / (sampleRate / 2.0f)) * (nfft - 1));
			binIndices[i] = std::min(std::max(binIndices[i], 0), nfft - 1);
		}

		// 4. ����Mel�˲�������
		std::vector<float> filterEnergies(nfilters, 0.0f);
		for (int i = 0; i < nfilters; i++) {
			int left = binIndices[i];
			int center = binIndices[i + 1];
			int right = binIndices[i + 2];

			// ������
			for (int j = left; j <= center; j++) {
				if (center != left) {
					float weight = static_cast<float>(j - left) / (center - left);
					filterEnergies[i] += buf1im[j] * weight;
				}
			}

			// �½���
			for (int j = center + 1; j <= right; j++) {
				if (right != center) {
					float weight = 1.0f - static_cast<float>(j - center) / (right - center);
					filterEnergies[i] += buf1im[j] * weight;
				}
			}

			// ��������������
			if (filterEnergies[i] < 1e-10f) {
				filterEnergies[i] = 1e-10f;
			}
		}

		// 5. ȡ��������
		for (int i = 0; i < nfilters; i++) {
			filterEnergies[i] = logf(filterEnergies[i]);
		}

		// 6. DCT�任��ȡMFCCϵ��
		int numToCompute = std::min(numMFCC, nfilters);
		for (int k = 0; k < numToCompute; k++) {
			float sum = 0.0f;
			for (int n = 0; n < nfilters; n++) {
				sum += filterEnergies[n] *
					cosf(M_PI * k * (2.0f * n + 1.0f) / (2.0f * nfilters));
			}

			// DCT-II��������
			if (k == 0) {
				sum *= sqrtf(1.0f / nfilters);
			}
			else {
				sum *= sqrtf(2.0f / nfilters);
			}

			mfcc[k] = sum;
		}

		// 7. ��������ϵ�������˲���������ʣ�ಿ������
		for (int k = numToCompute; k < numMFCC; k++) {
			mfcc[k] = 0.0f;
		}
	}
};*/