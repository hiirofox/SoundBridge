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

	// 音频数据存储
	std::unique_ptr<juce::AudioBuffer<float>> audioBuffer;

	// 音频属性
	int numChannels;
	int sampleRate;
	int numSamples;

	// JUCE音频格式管理器
	juce::AudioFormatManager formatManager;

public:
	// 实现部分
	AudioFileReader()
		: numChannels(0), sampleRate(0), numSamples(0)
	{
		initializeFormatManager();
	}

	void initializeFormatManager()
	{
		// 注册常用的音频格式
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
		// 清除之前的数据
		clear();

		// 创建文件对象
		juce::File audioFile(filePath);

		if (!audioFile.exists())
		{
			//juce::DBG("File does not exist: " + filePath);
			return false;
		}

		// 创建音频格式读取器
		std::unique_ptr<juce::AudioFormatReader> reader(
			formatManager.createReaderFor(audioFile)
		);

		if (reader == nullptr)
		{
			//juce::DBG("Cannot create reader for file: " + filePath);
			return false;
		}

		// 获取音频文件信息
		int _numChannels = static_cast<int>(reader->numChannels);
		int _sampleRate = static_cast<int>(reader->sampleRate);
		int _numSamples = static_cast<int>(reader->lengthInSamples);

		if (_numSamples <= 0 || _numChannels <= 0)
		{
			//juce::DBG("Invalid audio file parameters");
			return false;
		}

		// 创建音频缓冲区
		audioBuffer = std::make_unique<juce::AudioBuffer<float>>(_numChannels, _numSamples);

		// 读取整个音频文件到缓冲区
		bool success = reader->read(
			audioBuffer.get(),           // 目标缓冲区
			0,                          // 缓冲区起始位置
			_numSamples,                 // 要读取的样本数
			0,                          // 文件中的起始位置
			true,                       // 使用左声道
			true             // 使用右声道（如果存在）
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
