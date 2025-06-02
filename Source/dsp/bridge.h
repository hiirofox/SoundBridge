#pragma once

#include "soundDB.h"

class Bridge
{
private:
	SoundDB* pSdb = NULL;
	constexpr static int MaxBlockSize = 2048;
	constexpr static int MaxBufSize = MaxBlockSize * 2;
	int blockSize = MaxBlockSize / 1;
	int hopSize = MaxBlockSize / 2;
	float bufin[MaxBufSize];
	float bufout[MaxBufSize];
	int pos = 0, poshop = 0;

	float globalBlock[MaxBlockSize];
	float searchBlock[MaxBlockSize];
	float searchBlock2[MaxBlockSize];
	constexpr static int FFTLen = MaxBlockSize;
	int range = blockSize / 2;
	float re1[FFTLen];
	float im1[FFTLen];
	float re2[FFTLen];
	float im2[FFTLen];
	int GetMaxCorrIndex2()//找最大相关(fft) 修好了
	{
		for (int i = 0; i < MaxBlockSize; ++i) {
			//float w = window2[i];
			float w = 1.0;
			re1[i] = searchBlock[i] * w;
			im1[i] = 0.0f;
		}
		for (int i = MaxBlockSize; i < FFTLen; ++i) {
			re1[i] = 0.0f;
			im1[i] = 0.0f;
		}
		for (int i = 0; i < blockSize; ++i) {
			//float w = window[i];
			float w = 1.0;
			re2[i] = globalBlock[i] * w;
			im2[i] = 0.0f;
		}
		for (int i = blockSize; i < FFTLen; ++i) {
			re2[i] = 0.0f;
			im2[i] = 0.0f;
		}
		fft_f32(re1, im1, FFTLen, 1);
		fft_f32(re2, im2, FFTLen, 1);
		for (int i = 0; i < FFTLen; ++i) {
			float re1v = re1[i];
			float im1v = im1[i];
			float re2v = re2[i];
			float im2v = -im2[i];//这个得共轭
			re1[i] = re1v * re2v - im1v * im2v;
			im1[i] = re1v * im2v + im1v * re2v;
		}
		fft_f32(re1, im1, FFTLen, -1);
		int index = 0;
		float maxv = -999999999999;
		for (int i = 0; i < range; ++i) {
			float r = re1[i];
			if (r > maxv)
			{
				maxv = r;
				index = i;
			}
		}
		return index;
	}
public:
	Bridge(SoundDB* pSdb)
	{
		this->pSdb = pSdb;
		memset(bufin, 0, sizeof(bufin));
		memset(bufout, 0, sizeof(bufout));
		memset(globalBlock, 0, sizeof(globalBlock));
		memset(searchBlock, 0, sizeof(searchBlock));
	}
	void ProcessBlock(const float* in, float* out, int numSamples)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			bufin[pos] = in[i];
			out[i] = bufout[pos];
			bufout[pos] = 0;
			pos = (pos + 1) % MaxBufSize;
			poshop++;
			if (poshop >= hopSize)
			{
				poshop = 0;
				for (int j = 0; j < MaxBlockSize; ++j)
				{
					searchBlock[j] = bufin[(pos + j) % MaxBufSize];
				}
				for (int j = 0; j < blockSize; ++j)
				{
					globalBlock[j] = bufin[(pos + j) % MaxBufSize];
				}
				if (pSdb)
				{
					pSdb->FindSimilarBlock(searchBlock, searchBlock2, MaxBlockSize);
				}
				int index = GetMaxCorrIndex2();
				//int index = 0;
				for (int j = 0; j < blockSize; ++j)
				{
					float w = 0.5 - 0.5 * cosf(2.0 * M_PI * j / blockSize);
					//float w = 1.0;
					bufout[(pos + j) % MaxBufSize] += searchBlock2[j + index] * w;
				}
			}
		}
	}
};