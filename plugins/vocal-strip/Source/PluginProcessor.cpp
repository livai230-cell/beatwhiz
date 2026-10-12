#include <juce_audio_processors/juce_audio_processors.h>
#include "VocalDSP.h"
#include <atomic>
#include <cstdint>
#include <memory>

class SimpleVocalStripAudioProcessor final : public juce::AudioProcessor
{
public:
    SimpleVocalStripAudioProcessor()
        : AudioProcessor(BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters(*this, nullptr, "SimpleVocalStripState", createLayout())
    {
        inputDb = parameters.getRawParameterValue("inputDb");
        outputDb = parameters.getRawParameterValue("outputDb");
        thresholdDb = parameters.getRawParameterValue("thresholdDb");
        ratio = parameters.getRawParameterValue("ratio");
        attackMs = parameters.getRawParameterValue("attackMs");
        releaseMs = parameters.getRawParameterValue("releaseMs");
    }

    ~SimpleVocalStripAudioProcessor() override = default;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        const auto& output = layouts.getMainOutputChannelSet();
        return (output == juce::AudioChannelSet::mono()
                || output == juce::AudioChannelSet::stereo())
            && layouts.getMainInputChannelSet() == output;
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        engine.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        updateParameters();
        reset();
    }

    void reset() override
    {
        engine.reset();
        meterIn.store(0); meterOut.store(0); meterReduction.store(0);
    }

    void releaseResources() override { reset(); }

    using juce::AudioProcessor::processBlock;

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override
    {
        juce::ScopedNoDenormals noDenormals;
        juce::ignoreUnused(midiMessages);

        const auto inputs = getTotalNumInputChannels();
        const auto outputs = getTotalNumOutputChannels();
        for (int channel = inputs; channel < outputs; ++channel)
            buffer.clear(channel, 0, buffer.getNumSamples());

        if (buffer.getNumSamples() == 0 || outputs == 0)
            return;

        updateParameters();
        engine.process(buffer, outputs);
        publishPeak(meterIn, engine.inputPeak);
        publishPeak(meterOut, engine.outputPeak);
        publishPeak(meterReduction, engine.reductionDb);
    }

    std::atomic<float> meterIn { 0 }, meterOut { 0 }, meterReduction { 0 };

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    const juce::String getName() const override { return "Beatwhiz Vocal Strip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destination) override
    {
        if (auto xml = parameters.copyState().createXml())
            copyXmlToBinary(*xml, destination);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        if (data == nullptr || sizeInBytes <= 0)
            return;

        if (auto xml = getXmlFromBinary(data, sizeInBytes))
            if (xml->hasTagName(parameters.state.getType().toString()))
            {
                const auto restored = juce::ValueTree::fromXml(*xml);
                if (restored.isValid())
                    parameters.replaceState(restored);
            }
        // DSP is updated on the next audio block, not on the state-loading thread.
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        const auto add = [&layout](const char* id, const char* name,
                                   float minimum, float maximum,
                                   float step, float initial)
        {
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { id, 1 }, name,
                juce::NormalisableRange<float>(minimum, maximum, step), initial));
        };

        add("inputDb", "Input (dB)", -24.0f, 24.0f, 0.1f, 0.0f);
        add("thresholdDb", "Threshold (dB)", -60.0f, 0.0f, 0.1f, -20.0f);
        add("ratio", "Ratio", 1.0f, 10.0f, 0.1f, 4.0f);
        add("attackMs", "Attack (ms)", 1.0f, 100.0f, 0.1f, 15.0f);
        add("releaseMs", "Release (ms)", 10.0f, 1000.0f, 1.0f, 100.0f);
        add("outputDb", "Output (dB)", -24.0f, 24.0f, 0.1f, 0.0f);
        return layout;
    }

    static float read(const std::atomic<float>* value) noexcept
    {
        return value->load(std::memory_order_relaxed);
    }

    void updateParameters()
    {
        engine.setParameters(read(inputDb), read(outputDb), read(thresholdDb),
                             read(ratio), read(attackMs), read(releaseMs));
    }

    static void publishPeak(std::atomic<float>& target, float value) noexcept
    {
        // One bounded atomic exchange. Peaks persist until the next UI refresh.
        const auto previous = target.exchange(value, std::memory_order_relaxed);
        if (previous > value) target.store(previous, std::memory_order_relaxed);
    }

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* inputDb = nullptr;
    std::atomic<float>* outputDb = nullptr;
    std::atomic<float>* thresholdDb = nullptr;
    std::atomic<float>* ratio = nullptr;
    std::atomic<float>* attackMs = nullptr;
    std::atomic<float>* releaseMs = nullptr;

    VocalDSP engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleVocalStripAudioProcessor)
};

class VocalStripEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VocalStripEditor(SimpleVocalStripAudioProcessor& p)
        : AudioProcessorEditor(p), processor(p), controls(p)
    {
        addAndMakeVisible(controls);
        setSize(580, 390);
        startTimerHz(20);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff141416));
        g.setColour(juce::Colour(0xffed645d));
        g.setFont(22.0f);
        g.drawText("BEATWHIZ / VOCAL STRIP", 20, 12, getWidth()-40, 32,
                   juce::Justification::centredLeft);
        g.setColour(juce::Colours::lightgrey);
        g.setFont(13.0f);
        g.drawText("80 Hz cleanup  /  Compression  /  Output", 20, 47,
                   getWidth()-40, 20, juce::Justification::centredLeft);
        g.setFont(15.0f);
        g.drawText("IN  " + level(in) + " dBFS", 20, getHeight()-64, 170, 24,
                   juce::Justification::centredLeft);
        g.drawText("GR  " + juce::String(gr, 1) + " dB", 205, getHeight()-64, 150, 24,
                   juce::Justification::centredLeft);
        g.setColour(out >= 1.0f ? juce::Colours::orangered : juce::Colours::lightgrey);
        g.drawText("OUT  " + level(out) + " dBFS", 380, getHeight()-64, 180, 24,
                   juce::Justification::centredLeft);
        g.setColour(juce::Colours::grey);
        g.setFont(12.0f);
        g.drawText("Match output loudness when comparing with host bypass.",
                   20, getHeight()-32, getWidth()-40, 20, juce::Justification::centredLeft);
    }

    void resized() override { controls.setBounds(12, 82, getWidth()-24, getHeight()-158); }

private:
    static juce::String level(float v)
    {
        return v < 0.00001f ? juce::String("-inf")
                           : juce::String(juce::Decibels::gainToDecibels(v), 1);
    }
    void timerCallback() override
    {
        in = juce::jmax(in * 0.8f, processor.meterIn.exchange(0));
        out = juce::jmax(out * 0.8f, processor.meterOut.exchange(0));
        gr = juce::jmax(gr * 0.8f, processor.meterReduction.exchange(0));
        repaint();
    }
    SimpleVocalStripAudioProcessor& processor;
    juce::GenericAudioProcessorEditor controls;
    float in = 0, out = 0, gr = 0;
};

juce::AudioProcessorEditor* SimpleVocalStripAudioProcessor::createEditor()
{
    return new VocalStripEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleVocalStripAudioProcessor();
}
