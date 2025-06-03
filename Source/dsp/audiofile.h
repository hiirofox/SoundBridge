#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

class AudioFileReader
{
private:
	mutable  std::mutex mtx;

	// ��Ƶ���ݴ洢
	std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;

	// ��Ƶ����
	int numChannels;
	int sampleRate;
	int numSamples;

	// JUCE��Ƶ��ʽ������
	juce::AudioFormatManager formatManager;

public:
	// ʵ�ֲ���
	AudioFileReader()
		: numChannels(0), sampleRate(0), numSamples(0)
	{
		initializeFormatManager();
	}

	void initializeFormatManager()
	{
		// ע�᳣�õ���Ƶ��ʽ
		formatManager.registerBasicFormats();
	}

	void lock()
	{
		mtx.lock();
	}
	void unlock()
	{
		mtx.unlock();
	}
	int getNumChannels() const {
		std::lock_guard<std::mutex> lock(mtx);
		return numChannels;
	}
	int getSampleRate() const {
		std::lock_guard<std::mutex> lock(mtx);
		return sampleRate;
	}
	int getNumSamples() const {
		std::lock_guard<std::mutex> lock(mtx);
		return numSamples;
	}

	bool isValid() const { return audioBuffer != nullptr && numSamples > 0; }
	float isLoadDone_ = false;
	bool isLoadDone()
	{
		return isLoadDone_;
	}
	void SetLoadDoneFlag()
	{
		isLoadDone_ = true;
	}

	bool loadFile(const std::string& filePath)
	{
		std::lock_guard<std::mutex> lock(mtx);
		isLoadDone_ = false;
		numChannels = 0;
		sampleRate = 0;
		numSamples = 0;
		// ���֮ǰ������
		clear();

		// �����ļ�����
		juce::File audioFile(filePath);

		if (!audioFile.exists())
		{
			//juce::DBG("File does not exist: " + filePath);
			return false;
		}

		// ������Ƶ��ʽ��ȡ��
		std::unique_ptr<juce::AudioFormatReader> reader(
			formatManager.createReaderFor(audioFile)
		);

		if (reader == nullptr)
		{
			//juce::DBG("Cannot create reader for file: " + filePath);
			return false;
		}

		// ��ȡ��Ƶ�ļ���Ϣ
		int _numChannels = static_cast<int>(reader->numChannels);
		int _sampleRate = static_cast<int>(reader->sampleRate);
		int _numSamples = static_cast<int>(reader->lengthInSamples);

		if (_numSamples <= 0 || _numChannels <= 0)
		{
			//juce::DBG("Invalid audio file parameters");
			return false;
		}

		// ������Ƶ������
		audioBuffer = std::make_unique<juce::AudioBuffer<float>>(_numChannels, _numSamples);

		// ��ȡ������Ƶ�ļ���������
		bool success = reader->read(
			audioBuffer.get(),           // Ŀ�껺����
			0,                          // ��������ʼλ��
			_numSamples,                 // Ҫ��ȡ��������
			0,                          // �ļ��е���ʼλ��
			true,                       // ʹ��������
			true             // ʹ����������������ڣ�
		);

		if (!success)
		{
			//juce::DBG("Failed to read audio data from file");
			clear();
			return false;
		}
		/*
		juce::DBG("Successfully loaded audio file: " + filePath);
		juce::DBG("Channels: " + juce::String(numChannels) +
			", Sample Rate: " + juce::String(sampleRate) +
			", Samples: " + juce::String(numSamples));
			*/
		numChannels = _numChannels;
		sampleRate = _sampleRate;
		numSamples = _numSamples;

		return true;
	}

	float* getBufferPointer(int channel)
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (!isValid() || channel < 0 || channel >= numChannels)
		{
			return nullptr;
		}

		return audioBuffer->getWritePointer(channel);
	}

	double getLengthInSeconds() const
	{
		if (sampleRate <= 0 || numSamples <= 0)
			return 0.0;

		return static_cast<double>(numSamples) / static_cast<double>(sampleRate);
	}

	void clear()
	{
		audioBuffer.reset();
		numChannels = 0;
		sampleRate = 0;
		numSamples = 0;
	}

};
