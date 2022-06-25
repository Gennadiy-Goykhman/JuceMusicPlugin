/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JuceMusicPluginAudioProcessor::JuceMusicPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

JuceMusicPluginAudioProcessor::~JuceMusicPluginAudioProcessor()
{
}

//==============================================================================
const juce::String JuceMusicPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JuceMusicPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JuceMusicPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JuceMusicPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JuceMusicPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JuceMusicPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JuceMusicPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JuceMusicPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String JuceMusicPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void JuceMusicPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void JuceMusicPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //инициализация и подготовка каналов перед проигрыванием
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto chainSettings = getChainSettings(apvts);

    //Создание управляющего элемента
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    
}

void JuceMusicPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JuceMusicPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void JuceMusicPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto chainSettings = getChainSettings(apvts);

    //Создание управляющего элемента
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    // Блокируем стандартный звуковой канал и перенаправляем в наши цепочки фильтров
    //А также вытаскиваем звуковые каналы из буфера, оборачиваем контекстом и передаём в фильтры
    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    //leftChannelFifo.update(buffer);
    //rightChannelFifo.update(buffer);
}

//==============================================================================
bool JuceMusicPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JuceMusicPluginAudioProcessor::createEditor()
{
    //return new JuceMusicPluginAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void JuceMusicPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void JuceMusicPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    //Извлекаем все необходимые настройки
    ChainSettings settings;
      settings.highCutFreq = apvts.getRawParameterValue(JuceMusicPluginAudioProcessor::highCutFreqstr)->load();
      settings.peakFreq = apvts.getRawParameterValue(JuceMusicPluginAudioProcessor::peakFreqstr)->load();
      settings.peakGainInDecibels = apvts.getRawParameterValue(JuceMusicPluginAudioProcessor::peakGainstr)->load();
      settings.peakQuality = apvts.getRawParameterValue(JuceMusicPluginAudioProcessor::peakQualstr)->load();
      settings.lowCutSlope = apvts.getRawParameterValue(JuceMusicPluginAudioProcessor::lowCutSlopestr)->load();
      settings.highCutSlope = apvts.getRawParameterValue(JuceMusicPluginAudioProcessor::highCutSlopestr)->load();
      //settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
      //settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    //settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f;
    //settings.peakBypassed = apvts.getRawParameterValue("Peak Bypassed")->load() > 0.5f;
   // settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")->load() > 0.5f;

    return settings;
}


juce::AudioProcessorValueTreeState::ParameterLayout JuceMusicPluginAudioProcessor::createParameterLayout() {
    //Создаём объект выкладки
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    //Добавление параметра низких частот
    layout.add(std::make_unique<juce::AudioParameterFloat>(lowCutFreqstr, lowCutFreqstr,
        juce::NormalisableRange<float>(20.f,
            20000.f,
            1.f, 0.25f),
        20.f));

    //Добавление параметра высоких частот
    layout.add(std::make_unique<juce::AudioParameterFloat>(highCutFreqstr, highCutFreqstr,
        juce::NormalisableRange<float>(20.f,
            20000.f,
            1.f, 0.25f),
        20000.f));

    //Добавление параметра пиковой музыкальной частоты
    layout.add(std::make_unique<juce::AudioParameterFloat>(peakFreqstr, peakFreqstr,
        juce::NormalisableRange<float>(20.f,
            20000.f,
            1.f, 0.25f),
        750.f));

    //Добавление параметра усиления пиковых частот
    layout.add(std::make_unique<juce::AudioParameterFloat>(peakGainstr,
        peakGainstr,
        juce::NormalisableRange<float>(-24.f,
            24.f,
            0.5f,
            1.f),
        0.0f));

    //Добавление параметра диапозона частот
    layout.add(std::make_unique<juce::AudioParameterFloat>(peakQualstr,
        peakQualstr,
        juce::NormalisableRange<float>(0.1f,
            10.f,
            0.05f,
            1.f),
        1.f));

    //Выбор децибел на октаву
    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i) {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    //Добавление параметра низкочастотного спуска
    layout.add(std::make_unique<juce::AudioParameterChoice>(lowCutSlopestr, highCutSlopestr,
        stringArray, 0));

    //Добавление параметра высокочастотного спуска
    layout.add(std::make_unique<juce::AudioParameterChoice>(highCutSlopestr, highCutSlopestr,
        stringArray, 0));

    return layout;

}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JuceMusicPluginAudioProcessor();
}
