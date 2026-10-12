#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

// Shared by the plugin and the executable audio regression tests.
class VocalDSP
{
public:
    void prepare(double rate, int maximumBlock, int channels)
    {
        const juce::dsp::ProcessSpec spec { rate,
            static_cast<juce::uint32>(juce::jmax(1, maximumBlock)),
            static_cast<juce::uint32>(channels) };
        *filter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            rate, juce::jmin(80.0f, static_cast<float>(rate * 0.45)));
        filter.prepare(spec);
        compressor.prepare(spec);
        input.prepare(spec);
        output.prepare(spec);
        input.setRampDurationSeconds(0.02);
        output.setRampDurationSeconds(0.02);
        reset();
    }

    void setParameters(float inDb, float outDb, float threshold,
                       float ratio, float attack, float release)
    {
        input.setGainDecibels(inDb);
        output.setGainDecibels(outDb);
        compressor.setThreshold(threshold);
        compressor.setRatio(ratio);
        compressor.setAttack(attack);
        compressor.setRelease(release);
    }

    void reset()
    {
        filter.reset(); compressor.reset(); input.reset(); output.reset();
        inputPeak = outputPeak = reductionDb = 0.0f;
    }

    void process(juce::AudioBuffer<float>& buffer, int channels)
    {
        inputPeak = outputPeak = reductionDb = 0.0f;
        if (buffer.getNumSamples() == 0 || channels == 0) return;
        for (int ch = 0; ch < channels; ++ch)
            inputPeak = juce::jmax(inputPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
        juce::dsp::AudioBlock<float> whole(buffer);
        auto block = whole.getSubsetChannelBlock(0, static_cast<size_t>(channels));
        juce::dsp::ProcessContextReplacing<float> context(block);
        input.process(context);
        filter.process(context);
        float minimumGain = 1.0f;
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto before = samples[i];
                samples[i] = compressor.processSample(ch, before);
                if (std::abs(before) > 1.0e-9f)
                    minimumGain = juce::jmin(minimumGain, std::abs(samples[i] / before));
            }
        }
        reductionDb = -juce::Decibels::gainToDecibels(minimumGain, -100.0f);
        output.process(context);
        for (int ch = 0; ch < channels; ++ch)
            outputPeak = juce::jmax(outputPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    }

    float inputPeak = 0, outputPeak = 0, reductionDb = 0;

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> filter;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float> input, output;
};
