/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "dsp/audiofile.h"
#include "dsp/soundDB.h"
#include "dsp/bridge.h"

//==============================================================================
/**
*/
class LModelAudioProcessor : public juce::AudioProcessor
{
public:


	//==============================================================================
	LModelAudioProcessor();
	~LModelAudioProcessor() override;

	//==============================================================================
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	//==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	//==============================================================================
	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;

	//==============================================================================
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;

	//==============================================================================
	juce::AudioProcessorValueTreeState& GetParams()
	{
		return Params;
	}

	int updataFlag = 0;
	std::string audioname = "";
	AudioFileReader audioReader;
	SoundDB sdbl, sdbr;
	Bridge bl{ &sdbl };
	Bridge br{ &sdbr };

private:
	//Synth Param
	static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
	juce::AudioProcessorValueTreeState Params{ *this, nullptr, "Parameters", createParameterLayout() };
	float buf1l[8192], buf1r[8192];
	float buf2l[8192], buf2r[8192];
	float bufoutl[8192], bufoutr[8192];
	int pos = 0, posHop = 0;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LModelAudioProcessor)
};

