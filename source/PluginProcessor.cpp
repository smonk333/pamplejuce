#include "PluginProcessor.h"


//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",     juce::AudioChannelSet::stereo())
                                      .withOutput ("Output",    juce::AudioChannelSet::stereo())
                                      .withInput  ("Sidechain", juce::AudioChannelSet::stereo()))

    , parameters(*this, nullptr, "PARAMETERS", {
       std::make_unique<juce::AudioParameterFloat>("threshold", "Threshold", 0.0f, 1.0f, 0.5f),
       std::make_unique<juce::AudioParameterFloat>("alpha", "Alpha", 0.0f, 1.0f, 0.8f)
    })
    {}

PluginProcessor::~PluginProcessor()
= default;

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    lowPassCoeff = 0.0f;    // [3]
    sampleCountDown = 0;    // [4]
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
                 && ! layouts.getMainInputChannelSet().isDisabled();
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    auto mainInputOutput = getBusBuffer (buffer, true, 0);                                  // [5]
    auto sideChainInput  = getBusBuffer (buffer, true, 1);

    float alphaCopy = parameters.getRawParameterValue("alpha")->load();
    float thresholdCopy = parameters.getRawParameterValue("threshold")->load();

    for (auto j = 0; j < buffer.getNumSamples(); ++j)                                       // [7]
    {
        auto mixedSamples = 0.0f;

        for (auto i = 0; i < sideChainInput.getNumChannels(); ++i)                          // [8]
            mixedSamples += sideChainInput.getReadPointer (i) [j];

        mixedSamples /= static_cast<float> (sideChainInput.getNumChannels());
        lowPassCoeff = (alphaCopy * lowPassCoeff) + ((1.0f - alphaCopy) * mixedSamples);    // [9]

        if (lowPassCoeff >= thresholdCopy)                                                  // [10]
            sampleCountDown = (int) getSampleRate();

        // very in-effective way of doing this
        for (auto i = 0; i < mainInputOutput.getNumChannels(); ++i)                         // [11]
            *mainInputOutput.getWritePointer (i, j) = sampleCountDown > 0 ? *mainInputOutput.getReadPointer (i, j)
                                                                          : 0.0f;

        if (sampleCountDown > 0)                                                            // [12]
            --sampleCountDown;
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
