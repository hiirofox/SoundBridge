#pragma once

#include "soundDB.h"


class Bridge {
private:
	SoundDB* pSdb = nullptr;

	float HANNING_WINDOW(int i, int N) {
		return 0.5f - 0.5f * std::cosf(2.0 * M_PI * i / static_cast<float>(N));
	}
	// ����������
	constexpr static int MaxBlockSize = 4096;
	constexpr static int MaxBufSize = MaxBlockSize; // ʵ�ʲ���Ҫ����
	constexpr static int MaxSearchLen = MaxBufSize;

	// FFT���� (�޸�������FFT������)
	constexpr static int MaxFFTLen = MaxBlockSize * 2; // �޸�1��������4096

	// ��������
	int blockSize = MaxBlockSize / 4;
	int hopSize = MaxBlockSize / 8;
	int range = MaxBlockSize; // �����뾶

	// ѭ��������
	float bufin[MaxBufSize] = { 0 };
	float bufout[MaxBufSize] = { 0 };
	int pos = 0;          // ��ǰ����λ��
	int poshop = 0;       // ����������
	int last_hop_pos = 0; // �ϴδ���λ�û�׼

	// �����
	float searchBlock[MaxSearchLen] = { 0 };
	float globalBlock1[MaxBlockSize] = { 0 };

	// FFT������ (�޸���ƥ��MaxFFTLen)
	float re1[MaxFFTLen] = { 0 };
	float im1[MaxFFTLen] = { 0 };
	float re2[MaxFFTLen] = { 0 };
	float im2[MaxFFTLen] = { 0 };

	// ��ȡ������λ�� (�������ݿ����λ��)
	int GetMaxCorrPos(const float* audioBuf, int refPos, int numSamples) {
		// ����������Χ [start, start+range+blockSize)
		int start = refPos - range / 2;
		start = std::clamp(start, 0, numSamples - range - blockSize);

		// ȷ��FFT����
		const int dataLen = blockSize + range;
		const int FFTLen = 1 << static_cast<int>(ceilf(log2f(dataLen)));

		// ���FFT����
		if (FFTLen > MaxFFTLen) return start; // ��ȫ����

		// ׼�����ݿ�Ƭ�� (���Ӵ�)
		std::memset(re1, 0, sizeof(re1));
		std::memset(im1, 0, sizeof(im1));
		for (int i = 0; i < dataLen; ++i) {
			re1[i] = audioBuf[start + i]; // ����λ�� start+i
		}

		// ׼����ǰ�� (�Ӻ�����) - �޸�3
		std::memset(re2, 0, sizeof(re2));
		std::memset(im2, 0, sizeof(im2));
		for (int i = 0; i < blockSize; ++i) {
			//re2[i] = globalBlock1[i] * HANNING_WINDOW(i, blockSize);
			re2[i] = globalBlock1[i];
		}

		// ���㻥��� (FFT����)
		fft_f32(re1, im1, FFTLen, 1);
		fft_f32(re2, im2, FFTLen, 1);

		for (int i = 0; i < FFTLen; ++i) {
			const float ar = re1[i], ai = im1[i];
			const float br = re2[i], bi = -im2[i];  // ȡ����
			re1[i] = ar * br - ai * bi;
			im1[i] = ar * bi + ai * br;
		}

		fft_f32(re1, im1, FFTLen, -1);  // IFFT

		// Ѱ�������ص� (���ƫ��)
		int maxIndex = 0;
		float maxVal = -FLT_MAX;
		for (int i = 0; i < range; ++i) {
			if (re1[i] > maxVal) {
				maxVal = re1[i];
				maxIndex = i;
			}
		}

		return start + maxIndex; // �޸�2�����ؾ���λ��
	}

public:
	Bridge(SoundDB* pSdb) : pSdb(pSdb) {
		// ���������ڶ���ʱ��ʼ��
	}

	void ProcessBlock(const float* in, float* out, int numSamples) {
		for (int i = 0; i < numSamples; ++i) {
			// ��������/���������
			bufin[pos] = in[i];
			out[i] = bufout[pos];
			bufout[pos] = 0;  // ��յ�ǰλ��

			// ���»��λ�����λ��
			pos = (pos + 1) % MaxBufSize;
			poshop++;

			// �ﵽ����ʱ�����
			if (poshop >= hopSize) {
				poshop = 0;
				last_hop_pos = (pos - hopSize + MaxBufSize) % MaxBufSize; // �޸�4����¼��׼λ��

				// ׼�������� (��Χƥ��)
				for (int j = 0; j < MaxSearchLen; ++j) {
					const int idx = (last_hop_pos + j) % MaxBufSize;
					//searchBlock[j] = bufin[idx] * HANNING_WINDOW(j, MaxSearchLen);
					searchBlock[j] = bufin[idx];
				}

				// ���Ƶ�ǰ�� (���ھ�ȷƥ��)
				for (int j = 0; j < blockSize; ++j) {
					const int idx = (last_hop_pos + j) % MaxBufSize;
					globalBlock1[j] = bufin[idx];
				}

				// ��ɫǨ�ƴ���
				if (pSdb) {
					// ��Χ����
					int refPos = pSdb->FindSimilarBlock(searchBlock, MaxSearchLen);
					const float* dbBuf = pSdb->GetAudioBuffer();
					int dbSamples = pSdb->GetAudioBufferNumSamples();

					if (dbBuf && dbSamples > blockSize) {
						// ��ȷƥ�� (�������ݿ����λ��)
						int matchPos = GetMaxCorrPos(dbBuf, refPos, dbSamples);

						// д��ƥ��� (�ص����)
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