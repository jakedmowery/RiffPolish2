#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

class ShifuVirtMixAudioProcessor : public juce::AudioProcessor
{
public:
    struct FactoryPreset
    {
        juce::String name;
        float lowCutHz;
        float highCutHz;
        float lowShelfDb;
        float midScoopDb;
        float presenceDb;
        float pickAttack;
        float grit;
        float compThreshDb;
        float stereoWidth;
        float targetLoudnessDb;
        bool autoLoudness;
        float outputDb;
    };

    ShifuVirtMixAudioProcessor();
    ~ShifuVirtMixAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return static_cast<int>(factoryPresets.size()); }
    int getCurrentProgram() override { return currentProgramIndex; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void applyPreset(int index);
    const std::vector<FactoryPreset>& getFactoryPresets() const noexcept { return factoryPresets; }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float getCurrentInputDb() const noexcept { return inputLevelDb.load(std::memory_order_relaxed); }
    float getCurrentOutputDb() const noexcept { return outputLevelDb.load(std::memory_order_relaxed); }
    float getGainReductionDb() const noexcept { return gainReductionDb.load(std::memory_order_relaxed); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static std::vector<FactoryPreset> buildFactoryPresets();
    void updateFilters();

    using FilterBand = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    std::vector<FactoryPreset> factoryPresets;
    int currentProgramIndex = 0;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Virtual guitar conditioning & mix chain
    FilterBand lowCutFilter;
    FilterBand highCutFilter;
    FilterBand lowShelfFilter;
    FilterBand midCutFilter;
    FilterBand presenceBoostFilter;

    juce::dsp::WaveShaper<float> saturator {
        [](float x) noexcept
        {
            const float driven = x * 1.35f;
            return std::tanh(driven + 0.12f * driven * driven);
        }
    };

    juce::dsp::Gain<float> preDriveGain;
    juce::dsp::Gain<float> postDriveGain;
    juce::dsp::Compressor<float> busCompressor;
    juce::dsp::Gain<float> loudnessGain;
    juce::dsp::Gain<float> outputTrimGain;

    // Envelope follower for simple pick-attack / transient shaping
    std::array<float, 2> fastEnv { { 0.0f, 0.0f } };
    std::array<float, 2> slowEnv { { 0.0f, 0.0f } };

    // Smoothed loudness target state (dB)
    float loudnessTrimDbSmoothed = 0.0f;

    // Metering atomics for UI
    std::atomic<float> inputLevelDb { -60.0f };
    std::atomic<float> outputLevelDb { -60.0f };
    std::atomic<float> gainReductionDb { 0.0f };

    // Cached parameter pointers
    std::atomic<float>* lowCutHzParam = nullptr;
    std::atomic<float>* highCutHzParam = nullptr;
    std::atomic<float>* lowShelfDbParam = nullptr;
    std::atomic<float>* midScoopDbParam = nullptr;
    std::atomic<float>* presenceDbParam = nullptr;
    std::atomic<float>* pickAttackParam = nullptr;
    std::atomic<float>* gritParam = nullptr;
    std::atomic<float>* compThreshParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* targetLoudnessDbParam = nullptr;
    std::atomic<float>* autoLoudnessParam = nullptr;
    std::atomic<float>* outputDbParam = nullptr;

    // Track last filter values to avoid per-sample coefficient recalculation
    float lastLowCut = -1.0f;
    float lastHighCut = -1.0f;
    float lastLowShelf = -999.0f;
    float lastMidScoop = -999.0f;
    float lastPresence = -999.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShifuVirtMixAudioProcessor)
};
