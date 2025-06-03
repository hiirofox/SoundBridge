#pragma once

#include <vector>
#include "audiofile.h"
#include "mfcc_conv.h"

class SoundDB//音色数据库
{
private:
	MFCCConverter mfcc;
	const float* datl = NULL;
	int datlen = 0;

	constexpr static int MaxBlockSize = 4096;
	constexpr static int MfccSize = 64;//16维mfcc
	float n_mul = 8.0;
	int numBlocks = 0;
	std::vector<std::array<float, MfccSize>> dbl;

	float mfccl[MfccSize];

	int isUpdata = 1;
	int lastMaxIndex = 0;
public:
	SoundDB()
	{
		memset(mfccl, 0, sizeof(mfccl));
	}

	void SetUpdataFlag() { isUpdata = 1; }

	void Updata(const float* inl, int numSamples)//更新音色库
	{
		isUpdata = 1;

		datl = inl;
		this->datlen = numSamples;

		numBlocks = (float)numSamples / MaxBlockSize * n_mul;
		dbl.resize(numBlocks);
		for (int i = 0; i < numBlocks; ++i)
		{
			int x = (float)i / numBlocks * numSamples;
			if (x + MaxBlockSize < datlen)
			{
				mfcc.Process(&datl[x], dbl[i].data(), MaxBlockSize, MfccSize);
			}
		}

		isUpdata = 0;
	}
	int FindSimilarBlock(const float* srcl, int numSamples)//找相似的位置
	{
		if (isUpdata)return 0;
		int len = numSamples;
		if (len > MaxBlockSize)len = MaxBlockSize;
		mfcc.Process(srcl, mfccl, len, MfccSize);
		float maxCorrl = 999999999999;
		float maxIndexl = 0;
		for (int i = 0; i < numBlocks; ++i)
		{
			float suml = 0;
			for (int j = 0; j < MfccSize; ++j)
			{
				float vl = mfccl[j] - dbl[i][j];
				suml += vl * vl;
				//suml += mfccl[j] * dbl[i][j];
			}
			if (maxCorrl > suml)
			{
				maxCorrl = suml;
				maxIndexl = i;
			}
		}
		lastMaxIndex = maxIndexl;
		int startl = (float)maxIndexl / numBlocks * datlen;
		return startl;
	}
	const float* GetAudioBuffer()
	{
		return datl;
	}
	int GetAudioBufferNumSamples()
	{
		return datlen;
	}
	int GetMaxBlockSize()
	{
		return MaxBlockSize;
	}
	int GetNumBlocks()
	{
		return numBlocks;
	}
	float GetPos()
	{
		return (float)lastMaxIndex / numBlocks;
	}
};