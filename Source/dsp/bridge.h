#pragma once

#include "soundDB.h"


class Bridge {
private:
	SoundDB* pSdb = nullptr;

	float HANNING_WINDOW(int i, int N) {
		return 0.5f - 0.5f * std::cosf(2.0 * M_PI * i / static_cast<float>(N));
	}
	// 缓冲区配置
	constexpr static int MaxBlockSize = 4096;
	constexpr static int MaxBufSize = MaxBlockSize; // 实际不需要更大
	constexpr static int MaxSearchLen = MaxBufSize;

	// FFT配置 (修复：扩大FFT缓冲区)
	constexpr static int MaxFFTLen = MaxBlockSize * 2; // 修复1：扩大至4096

	// 工作参数
	int blockSize = MaxBlockSize / 4;
	int hopSize = MaxBlockSize / 8;
	int range = MaxBlockSize; // 搜索半径

	// 循环缓冲区
	float bufin[MaxBufSize] = { 0 };
	float bufout[MaxBufSize] = { 0 };
	int pos = 0;          // 当前输入位置
	int poshop = 0;       // 跳数计数器
	int last_hop_pos = 0; // 上次处理位置基准

	// 处理块
	float searchBlock[MaxSearchLen] = { 0 };
	float globalBlock1[MaxBlockSize] = { 0 };

	// FFT工作区 (修复：匹配MaxFFTLen)
	float re1[MaxFFTLen] = { 0 };
	float im1[MaxFFTLen] = { 0 };
	float re2[MaxFFTLen] = { 0 };
	float im2[MaxFFTLen] = { 0 };

	// 获取最大相关位置 (返回数据库绝对位置)
	int GetMaxCorrPos(const float* audioBuf, int refPos, int numSamples) {
		// 计算搜索范围 [start, start+range+blockSize)
		int start = refPos - range / 2;
		start = std::clamp(start, 0, numSamples - range - blockSize);

		// 确定FFT长度
		const int dataLen = blockSize + range;
		const int FFTLen = 1 << static_cast<int>(ceilf(log2f(dataLen)));

		// 检查FFT长度
		if (FFTLen > MaxFFTLen) return start; // 安全保护

		// 准备数据库片段 (不加窗)
		std::memset(re1, 0, sizeof(re1));
		std::memset(im1, 0, sizeof(im1));
		for (int i = 0; i < dataLen; ++i) {
			re1[i] = audioBuf[start + i]; // 绝对位置 start+i
		}

		// 准备当前块 (加汉宁窗) - 修复3
		std::memset(re2, 0, sizeof(re2));
		std::memset(im2, 0, sizeof(im2));
		for (int i = 0; i < blockSize; ++i) {
			//re2[i] = globalBlock1[i] * HANNING_WINDOW(i, blockSize);
			re2[i] = globalBlock1[i];
		}

		// 计算互相关 (FFT加速)
		fft_f32(re1, im1, FFTLen, 1);
		fft_f32(re2, im2, FFTLen, 1);

		for (int i = 0; i < FFTLen; ++i) {
			const float ar = re1[i], ai = im1[i];
			const float br = re2[i], bi = -im2[i];  // 取共轭
			re1[i] = ar * br - ai * bi;
			im1[i] = ar * bi + ai * br;
		}

		fft_f32(re1, im1, FFTLen, -1);  // IFFT

		// 寻找最大相关点 (相对偏移)
		int maxIndex = 0;
		float maxVal = -FLT_MAX;
		for (int i = 0; i < range; ++i) {
			if (re1[i] > maxVal) {
				maxVal = re1[i];
				maxIndex = i;
			}
		}

		return start + maxIndex; // 修复2：返回绝对位置
	}

public:
	Bridge(SoundDB* pSdb) : pSdb(pSdb) {
		// 缓冲区已在定义时初始化
	}

	void ProcessBlock(const float* in, float* out, int numSamples) {
		for (int i = 0; i < numSamples; ++i) {
			// 更新输入/输出缓冲区
			bufin[pos] = in[i];
			out[i] = bufout[pos];
			bufout[pos] = 0;  // 清空当前位置

			// 更新环形缓冲区位置
			pos = (pos + 1) % MaxBufSize;
			poshop++;

			// 达到跳数时处理块
			if (poshop >= hopSize) {
				poshop = 0;
				last_hop_pos = (pos - hopSize + MaxBufSize) % MaxBufSize; // 修复4：记录基准位置

				// 准备搜索块 (大范围匹配)
				for (int j = 0; j < MaxSearchLen; ++j) {
					const int idx = (last_hop_pos + j) % MaxBufSize;
					//searchBlock[j] = bufin[idx] * HANNING_WINDOW(j, MaxSearchLen);
					searchBlock[j] = bufin[idx];
				}

				// 复制当前块 (用于精确匹配)
				for (int j = 0; j < blockSize; ++j) {
					const int idx = (last_hop_pos + j) % MaxBufSize;
					globalBlock1[j] = bufin[idx];
				}

				// 音色迁移处理
				if (pSdb) {
					// 大范围搜索
					int refPos = pSdb->FindSimilarBlock(searchBlock, MaxSearchLen);
					const float* dbBuf = pSdb->GetAudioBuffer();
					int dbSamples = pSdb->GetAudioBufferNumSamples();

					if (dbBuf && dbSamples > blockSize) {
						// 精确匹配 (返回数据库绝对位置)
						int matchPos = GetMaxCorrPos(dbBuf, refPos, dbSamples);

						// 写入匹配块 (重叠相加)
						for (int j = 0; j < blockSize; ++j) {
							const int idx = (pos + j) % MaxBufSize;
							const float w = HANNING_WINDOW(j, blockSize);
							bufout[idx] += dbBuf[matchPos + j] * w;
						}
					}
				}
			}
		}
	}
};