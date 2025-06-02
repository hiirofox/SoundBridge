#pragma once

#include <JuceHeader.h>
#include "../dsp/audiofile.h"
class SoundUI :public juce::Component, public juce::FileDragAndDropTarget
{
private:
	int isFileDragOver = false;
	int hasAudioData = 0;
	juce::String filename = "";
	AudioFileReader* pReader = NULL;

	constexpr static int MaxDisplayDataLen = 8192;
	float displayData[MaxDisplayDataLen];
	void UpdataDisplayData()
	{
		if (!pReader)return;
		juce::Rectangle<int> rect = getBounds();
		int w = rect.getWidth(), h = rect.getHeight();

		int numSamples = pReader->getNumSamples();
		float* buf = pReader->getBufferPointer(0);
		if (!buf)return;

		float sum2 = 0;
		for (int i = 0; i < w; ++i)
		{
			float sum = 0;
			int len = numSamples / w;
			int start = len * i;
			int end = len * (i + 1);
			for (int j = start; j < end; ++j)
			{
				float v = buf[j];
				sum += v * v;
			}
			sum = sqrtf(sum);
			sum2 += sum;
			displayData[i] = sum;
		}
		float lpfv = 0;
		for (int i = 0; i < w; ++i)
		{
			float v = displayData[i] / sum2 * w / 12.0;
			if (lpfv < v)lpfv += 0.85 * (v - lpfv);
			else lpfv += 0.5 * (v - lpfv);
			displayData[i] = lpfv;
		}
	}
public:
	SoundUI()
	{

	}
	bool isInterestedInFileDrag(const juce::StringArray& files) override
	{
		if (files.size() != 1)
			return false;
		juce::File file(files[0]);
		juce::String extension = file.getFileExtension().toLowerCase();
		juce::StringArray supportedFormats;
		supportedFormats.add(".wv");
		supportedFormats.add(".wav");
		supportedFormats.add(".mp3");
		supportedFormats.add(".ogg");
		supportedFormats.add(".flac");
		supportedFormats.add(".aiff");
		supportedFormats.add(".aif");
		supportedFormats.add(".m4a");
		supportedFormats.add(".wma");  // Windows Media Audio (ÔÚWindowsÉÏ)
		return supportedFormats.contains(extension);
	}
	void fileDragEnter(const juce::StringArray&, int, int) override
	{
		isFileDragOver = true;
		repaint();
	}
	void fileDragExit(const juce::StringArray&) override
	{
		isFileDragOver = false;
		repaint();
	}
	void filesDropped(const juce::StringArray& files, int x, int y) override
	{
		isFileDragOver = false;
		filename = files[0];
		if (pReader)
		{
			if (pReader->loadFile(filename.toStdString()))
			{
				hasAudioData = 1;
			}
		}
		UpdataDisplayData();
	}
	void resized() override
	{
		Component::resized();
		UpdataDisplayData();
	}
	void paint(juce::Graphics& g) override
	{
		juce::Rectangle<int> rect = getBounds();
		int w = rect.getWidth(), h = rect.getHeight();

		if (!hasAudioData)
		{
			g.setColour(juce::Colour(0xFFFFFFFF));
			g.setFont(juce::Font("FIXEDSYS", 16.0, 1));
			g.drawFittedText("- Drop File Here -\n*.wav *.mp3 *.ogg *.flac", getLocalBounds().reduced(10, 0), juce::Justification::centred, 4);
		}
		else
		{
			g.setColour(juce::Colour(0xff004444));
			for (int i = 0; i < w; ++i)
			{
				float v = displayData[i] * h;
				g.drawLine(i, h / 2 - v, i, h / 2 + v, 1.0);
			}
			g.setColour(juce::Colour(0xff00ffff));
			for (int i = 1; i < w; ++i)
			{
				float v1 = displayData[i - 1] * h;
				float v2 = displayData[i] * h;
				g.drawLine(i - 1, h / 2 - v1, i, h / 2 - v2, 2.0);
				g.drawLine(i - 1, h / 2 + v1, i, h / 2 + v2, 2.0);
			}
		}

		if (isFileDragOver) g.setColour(juce::Colour(0xFF00FFFF));
		else g.setColour(juce::Colour(0xFF00FF00));
		g.drawLine(0, 0, w, 0, 3);
		g.drawLine(0, 0, 0, h, 3);
		g.drawLine(w, 0, w, h, 3);
		g.drawLine(0, h, w, h, 3);
	}
	/////////////////////////
	void SetAudioFileReader(AudioFileReader* pReader)
	{
		this->pReader = pReader;
	}
};