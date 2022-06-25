/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//Структура содержащая параметры музыкального фильтра
struct ChainSettings {
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    int lowCutSlope{ 0 }, highCutSlope{ 0 };
   // Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

//Получаем данные звукового фильтра
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);
//

//==============================================================================
/**
*/
class JuceMusicPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    static constexpr auto highCutFreqstr = "HighCut Freq";
    static constexpr auto lowCutFreqstr = "Lowcut freq";
    static constexpr auto peakFreqstr = "Peak Freq";
    static constexpr auto peakGainstr = "Peak Gain";
    static constexpr auto peakQualstr = "Peak Quality";
    static constexpr auto lowCutSlopestr = "LowCut Slope";
    static constexpr auto highCutSlopestr = "HighCut Slope";
    //==============================================================================
    JuceMusicPluginAudioProcessor();
    ~JuceMusicPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //Создаёт выкладку(отображение, лейаут) параметров звука
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this , nullptr, "Параметры", createParameterLayout()};



private:
    //
    //Звуковые фильтры для звуковых спусков
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    //монопотоки
    MonoChain rightChain, leftChain;

    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    //
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceMusicPluginAudioProcessor)
};
