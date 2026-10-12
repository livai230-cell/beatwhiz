#include "../Source/VocalDSP.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>

static void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

static double toneRms(double rate, double frequency, float amplitude,
                      float threshold, float ratio, float outputDb = 0)
{
    VocalDSP dsp;
    dsp.prepare(rate, 257, 2);
    dsp.setParameters(0, outputDb, threshold, ratio, 15, 100);
    dsp.reset();
    double sum = 0; int count = 0;
    const int total = static_cast<int>(rate * 2);
    for (int offset = 0; offset < total; offset += 257)
    {
        const int n = juce::jmin(257, total-offset);
        juce::AudioBuffer<float> b(2, n);
        for (int i = 0; i < n; ++i)
        {
            const float x = amplitude * static_cast<float>(std::sin(
                2.0 * juce::MathConstants<double>::pi * frequency * (offset+i) / rate));
            b.setSample(0, i, x); b.setSample(1, i, x);
        }
        dsp.process(b, 2);
        for (int i = 0; i < n; ++i)
        {
            check(std::isfinite(b.getSample(0, i)), "Non-finite audio");
            check(b.getSample(0, i) == b.getSample(1, i), "Stereo channel mismatch");
            if (offset+i > static_cast<int>(rate))
            { sum += b.getSample(0, i) * b.getSample(0, i); ++count; }
        }
    }
    return std::sqrt(sum/count);
}

static std::vector<float> render(int blockSize)
{
    VocalDSP dsp;
    dsp.prepare(48000, blockSize, 1);
    dsp.setParameters(3, -1, -20, 4, 15, 100); dsp.reset();
    std::vector<float> result;
    for (int start = 0; start < 10000; start += blockSize)
    {
        const int n = juce::jmin(blockSize, 10000-start);
        juce::AudioBuffer<float> b(1, n);
        for (int i = 0; i < n; ++i)
            b.setSample(0, i, 0.5f * static_cast<float>(std::sin((start+i)*0.13)));
        dsp.process(b, 1);
        result.insert(result.end(), b.getReadPointer(0), b.getReadPointer(0)+n);
    }
    return result;
}

int main()
{
    try
    {
        for (double rate : {44100.0, 48000.0, 96000.0})
        {
            const auto pass = toneRms(rate, 1000, 0.1f, 0, 1);
            const auto low = toneRms(rate, 20, 0.1f, 0, 1);
            check(std::abs(pass - 0.0707107) < 0.001, "Passband gain incorrect");
            check(low/pass < 0.08, "80 Hz HPF did not suppress 20 Hz");
            const auto loud = toneRms(rate, 1000, 0.8f, -20, 4);
            check(loud < 0.3 && loud > 0.07, "Compression outside expected bounds");
            const auto boosted = toneRms(rate, 1000, 0.1f, 0, 1, 6);
            check(std::abs(boosted/pass - std::pow(10, 6.0/20)) < 0.002,
                  "Output gain incorrect");
            std::cout << "PASS " << rate << " Hz: passband, HPF, compression, gain, stereo\n";
        }

        const auto one = render(1), many = render(511);
        check(one.size() == many.size(), "Render length mismatch");
        for (size_t i = 0; i < one.size(); ++i)
            check(std::abs(one[i]-many[i]) < 1.0e-6f, "Block-size dependent DSP");
        std::cout << "PASS mono and block-size invariance (1 versus 511 samples)\n";

        VocalDSP dsp;
        dsp.prepare(48000, 512, 2);
        dsp.setParameters(0, 0, -20, 4, 15, 100); dsp.reset();
        juce::AudioBuffer<float> silence(2, 512); silence.clear();
        dsp.process(silence, 2);
        check(silence.getMagnitude(0, 512) == 0, "Silence generated noise");
        juce::AudioBuffer<float> empty(2, 0); dsp.process(empty, 2);
        std::mt19937 random(12345);
        std::uniform_real_distribution<float> noise(-1, 1);
        for (int step = 0; step < 200; ++step)
        {
            dsp.setParameters(step%2 ? 24 : -24, step%3 ? 12 : -24,
                              step%2 ? -60 : 0, step%2 ? 10 : 1,
                              step%2 ? 1 : 100, step%2 ? 10 : 1000);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i) silence.setSample(ch, i, noise(random));
            dsp.process(silence, 2);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    check(std::isfinite(silence.getSample(ch, i)), "Automation produced NaN/Inf");
        }
        dsp.reset(); silence.clear(); dsp.process(silence, 2);
        check(silence.getMagnitude(0, 512) == 0, "Reset left filter history");
        std::cout << "PASS silence, empty block, parameter extremes, reset\nALL DSP TESTS PASSED\n";
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
