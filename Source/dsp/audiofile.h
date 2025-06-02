#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

class AudioFileReader
{
public:

	int getNumChannels() const { return numChannels; }
	int getSampleRate() const { return sampleRate; }
	int getNumSamples() const { return numSamples; }

	bool isValid() const { return audioBuffer != nullptr && numSamples > 0; }


private:
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

	bool loadFile(const std::string& filePath)
	{
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
		numChannels = static_cast<int>(reader->numChannels);
		sampleRate = static_cast<int>(reader->sampleRate);
		numSamples = static_cast<int>(reader->lengthInSamples);

		if (numSamples <= 0 || numChannels <= 0)
		{
			//juce::DBG("Invalid audio file parameters");
			return false;
		}

		// ������Ƶ������
		audioBuffer = std::make_unique<juce::AudioBuffer<float>>(numChannels, numSamples);

		// ��ȡ������Ƶ�ļ���������
		bool success = reader->read(
			audioBuffer.get(),           // Ŀ�껺����
			0,                          // ��������ʼλ��
			numSamples,                 // Ҫ��ȡ��������
			0,                          // �ļ��е���ʼλ��
			true,                       // ʹ��������
			numChannels > 1             // ʹ����������������ڣ�
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
		return true;
	}

	float* getBufferPointer(int channel)
	{
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
