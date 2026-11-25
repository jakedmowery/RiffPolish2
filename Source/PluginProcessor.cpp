#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace
{
constexpr const char* kParamLowCut = "low_cut_hz";
constexpr const char* kParamHighCut = "high_cut_hz";
constexpr const char* kParamLowShelf = "low_shelf_db";
constexpr const char* kParamMidScoop = "mid_scoop_db";
constexpr const char* kParamPresence = "presence_db";
constexpr const char* kParamPickAttack = "pick_attack";
constexpr const char* kParamGrit = "grit";
constexpr const char* kParamCompThresh = "comp_thresh_db";
constexpr const char* kParamWidth = "stereo_width";
constexpr const char* kParamTargetLoudness = "target_loudness_db";
constexpr const char* kParamAutoLoudness = "auto_loudness";
constexpr const char* kParamOutputDb = "output_db";

float bufferRmsDb(const juce::AudioBuffer<float>& buffer) noexcept
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numChannels <= 0 || numSamples <= 0)
        return -60.0f;

    double sumSq = 0.0;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const double s = static_cast<double>(data[i]);
            sumSq += s * s;
        }
    }

    const double meanSq = sumSq / static_cast<double>(numChannels * numSamples);
    const float db = juce::Decibels::gainToDecibels(static_cast<float>(std::sqrt(meanSq)), -60.0f);
    return std::clamp(db, -60.0f, 12.0f);
}

void setParameterNormalized(juce::AudioProcessorValueTreeState& apvts, const char* paramId, float plainValue)
{
    if (auto* param = apvts.getParameter(paramId))
        param->setValueNotifyingHost(param->convertTo0to1(plainValue));
}
} // namespace

std::vector<ShifuVirtMixAudioProcessor::FactoryPreset> ShifuVirtMixAudioProcessor::buildFactoryPresets()
{
    return {
        {
            "Default — Balanced Mix",
            85.0f, 11500.0f, -2.0f, -2.5f, 2.5f,
            15.0f, 20.0f, -14.0f, 100.0f, -14.0f, true, 0.0f
        },
        {
            "Odin III — Tight Djent / Drop",
            95.0f, 10800.0f, -3.5f, -3.5f, 3.5f,
            28.0f, 24.0f, -16.0f, 110.0f, -13.5f, true, 0.0f
        },
        {
            "Shreddage 3 — Modern Rhythm",
            85.0f, 9800.0f, -1.5f, -2.0f, 1.5f,
            10.0f, 18.0f, -14.5f, 108.0f, -14.0f, true, 0.0f
        },
        {
            "Argent — Industrial Crush",
            105.0f, 12500.0f, -4.0f, -4.5f, 4.5f,
            40.0f, 52.0f, -18.0f, 120.0f, -12.0f, true, 0.5f
        },
        {
            "Rogue — Raw Rock / Heavy",
            75.0f, 10200.0f, 0.5f, -1.0f, 2.0f,
            18.0f, 32.0f, -13.0f, 104.0f, -14.5f, true, 0.0f
        },
        {
            "Lead / Harmony — Cut Through",
            120.0f, 13000.0f, -3.0f, -2.0f, 4.0f,
            12.0f, 28.0f, -15.0f, 115.0f, -13.0f, true, 0.5f
        }
    };
}

ShifuVirtMixAudioProcessor::ShifuVirtMixAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      factoryPresets(buildFactoryPresets())
{
    lowCutHzParam = apvts.getRawParameterValue(kParamLowCut);
    highCutHzParam = apvts.getRawParameterValue(kParamHighCut);
    lowShelfDbParam = apvts.getRawParameterValue(kParamLowShelf);
    midScoopDbParam = apvts.getRawParameterValue(kParamMidScoop);
    presenceDbParam = apvts.getRawParameterValue(kParamPresence);
    pickAttackParam = apvts.getRawParameterValue(kParamPickAttack);
    gritParam = apvts.getRawParameterValue(kParamGrit);
    compThreshParam = apvts.getRawParameterValue(kParamCompThresh);
    widthParam = apvts.getRawParameterValue(kParamWidth);
    targetLoudnessDbParam = apvts.getRawParameterValue(kParamTargetLoudness);
    autoLoudnessParam = apvts.getRawParameterValue(kParamAutoLoudness);
    outputDbParam = apvts.getRawParameterValue(kParamOutputDb);
}

juce::AudioProcessorValueTreeState::ParameterLayout ShifuVirtMixAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamLowCut, "Low Cut",
        juce::NormalisableRange<float>(30.0f, 250.0f, 1.0f, 0.55f), 85.0f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamHighCut, "High Cut",
        juce::NormalisableRange<float>(3500.0f, 18000.0f, 10.0f, 0.55f), 11500.0f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamLowShelf, "Low Tightness",
        juce::NormalisableRange<float>(-12.0f, 9.0f, 0.1f), -2.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamMidScoop, "Low-Mid Control",
        juce::NormalisableRange<float>(-12.0f, 6.0f, 0.1f), -2.5f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamPresence, "Pick Bite",
        juce::NormalisableRange<float>(-6.0f, 12.0f, 0.1f), 2.5f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamPickAttack, "Pick Attack",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.5f), 15.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamGrit, "Tape/Tube Grit",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 20.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamCompThresh, "Glue Threshold",
        juce::NormalisableRange<float>(-36.0f, 0.0f, 0.5f), -14.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamWidth, "Stereo Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamTargetLoudness, "Target Level",
        juce::NormalisableRange<float>(-24.0f, -6.0f, 0.5f), -14.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        kParamAutoLoudness, "Auto Loudness", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        kParamOutputDb, "Output Trim",
        juce::NormalisableRange<float>(-18.0f, 12.0f, 0.1f), 0.0f, "dB"));

    return { params.begin(), params.end() };
}

void ShifuVirtMixAudioProcessor::setCurrentProgram(int index)
{
    applyPreset(index);
}

const juce::String ShifuVirtMixAudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < static_cast<int>(factoryPresets.size()))
        return factoryPresets[static_cast<size_t>(index)].name;

    return {};
}

void ShifuVirtMixAudioProcessor::applyPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(factoryPresets.size()))
        return;

    currentProgramIndex = index;
    const auto& preset = factoryPresets[static_cast<size_t>(index)];

    setParameterNormalized(apvts, kParamLowCut, preset.lowCutHz);
    setParameterNormalized(apvts, kParamHighCut, preset.highCutHz);
    setParameterNormalized(apvts, kParamLowShelf, preset.lowShelfDb);
    setParameterNormalized(apvts, kParamMidScoop, preset.midScoopDb);
    setParameterNormalized(apvts, kParamPresence, preset.presenceDb);
    setParameterNormalized(apvts, kParamPickAttack, preset.pickAttack);
    setParameterNormalized(apvts, kParamGrit, preset.grit);
    setParameterNormalized(apvts, kParamCompThresh, preset.compThreshDb);
    setParameterNormalized(apvts, kParamWidth, preset.stereoWidth);
    setParameterNormalized(apvts, kParamTargetLoudness, preset.targetLoudnessDb);
    setParameterNormalized(apvts, kParamAutoLoudness, preset.autoLoudness ? 1.0f : 0.0f);
    setParameterNormalized(apvts, kParamOutputDb, preset.outputDb);
}

void ShifuVirtMixAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = std::max(sampleRate, 8000.0);
    currentBlockSize = std::max(samplesPerBlock, 1);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(currentBlockSize);
    spec.numChannels = static_cast<juce::uint32>(std::max(getTotalNumOutputChannels(), 2));

    lowCutFilter.prepare(spec);
    highCutFilter.prepare(spec);
    lowShelfFilter.prepare(spec);
    midCutFilter.prepare(spec);
    presenceBoostFilter.prepare(spec);

    saturator.prepare(spec);
    preDriveGain.prepare(spec);
    postDriveGain.prepare(spec);

    busCompressor.prepare(spec);
    busCompressor.setRatio(3.5f);
    busCompressor.setAttack(12.0f);
    busCompressor.setRelease(90.0f);

    loudnessGain.prepare(spec);
    outputTrimGain.prepare(spec);

    preDriveGain.setRampDurationSeconds(0.02);
    postDriveGain.setRampDurationSeconds(0.02);
    loudnessGain.setRampDurationSeconds(0.05);
    outputTrimGain.setRampDurationSeconds(0.02);

    fastEnv.fill(0.0f);
    slowEnv.fill(0.0f);
    loudnessTrimDbSmoothed = 0.0f;

    lastLowCut = -1.0f;
    updateFilters();
}

void ShifuVirtMixAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ShifuVirtMixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::stereo()
        || mainOut == juce::AudioChannelSet::mono();
}
#endif

void ShifuVirtMixAudioProcessor::updateFilters()
{
    const float lc = lowCutHzParam->load();
    const float hc = highCutHzParam->load();
    const float ls = lowShelfDbParam->load();
    const float ms = midScoopDbParam->load();
    const float pr = presenceDbParam->load();

    if (lc != lastLowCut)
    {
        *lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, lc, 0.707f);
        lastLowCut = lc;
    }

    if (hc != lastHighCut)
    {
        *highCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, hc, 0.707f);
        lastHighCut = hc;
    }

    if (ls != lastLowShelf)
    {
        *lowShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            currentSampleRate, 140.0f, 0.707f, juce::Decibels::decibelsToGain(ls));
        lastLowShelf = ls;
    }

    if (ms != lastMidScoop)
    {
        *midCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            currentSampleRate, 380.0f, 1.1f, juce::Decibels::decibelsToGain(ms));
        lastMidScoop = ms;
    }

    if (pr != lastPresence)
    {
        *presenceBoostFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            currentSampleRate, 3200.0f, 1.2f, juce::Decibels::decibelsToGain(pr));
        lastPresence = pr;
    }
}

void ShifuVirtMixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    if (numSamples <= 0)
        return;

    inputLevelDb.store(bufferRmsDb(buffer), std::memory_order_relaxed);
    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // 1. Sub & fizz cleanup + guitar-voiced EQ
    lowCutFilter.process(context);
    lowShelfFilter.process(context);
    midCutFilter.process(context);
    presenceBoostFilter.process(context);
    highCutFilter.process(context);

    // 2. Pick attack / transient shaper
    const float attackAmount = pickAttackParam->load() / 100.0f;
    if (std::abs(attackAmount) > 0.001f)
    {
        const float fastCoeff = std::exp(-1.0f / (0.003f * static_cast<float>(currentSampleRate)));
        const float slowCoeff = std::exp(-1.0f / (0.065f * static_cast<float>(currentSampleRate)));

        const int channelsToProcess = std::min(buffer.getNumChannels(), 2);
        for (int ch = 0; ch < channelsToProcess; ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const float x = samples[i];
                const float absX = std::abs(x);

                fastEnv[ch] = fastCoeff * fastEnv[ch] + (1.0f - fastCoeff) * absX;
                slowEnv[ch] = slowCoeff * slowEnv[ch] + (1.0f - slowCoeff) * absX;

                const float transientDiff = std::max(0.0f, fastEnv[ch] - slowEnv[ch]);
                const float gainFactor = std::clamp(1.0f + attackAmount * transientDiff * 3.5f, 0.25f, 2.5f);
                samples[i] = x * gainFactor;
            }
        }
    }

    // 3. Harmonic grit / saturation
    const float gritNorm = std::clamp(gritParam->load() / 100.0f, 0.0f, 1.0f);
    if (gritNorm > 0.001f)
    {
        const float driveGain = 1.0f + gritNorm * 4.0f;
        preDriveGain.setGainLinear(driveGain);
        postDriveGain.setGainLinear(1.0f / std::sqrt(driveGain));

        preDriveGain.process(context);
        saturator.process(context);
        postDriveGain.process(context);
    }

    // 4. Stereo width control (mid/side matrix)
    const float widthScale = widthParam->load() / 100.0f;
    if (buffer.getNumChannels() >= 2 && std::abs(widthScale - 1.0f) > 0.001f)
    {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            const float mid = 0.5f * (left[i] + right[i]);
            const float side = 0.5f * (left[i] - right[i]) * widthScale;
            left[i] = mid + side;
            right[i] = mid - side;
        }
    }

    // 5. Glue compression
    busCompressor.setThreshold(compThreshParam->load());
    busCompressor.process(context);

    // 6. Auto loudness leveling & output trim
    const bool autoLoudness = autoLoudnessParam->load() > 0.5f;
    const float targetDb = targetLoudnessDbParam->load();

    if (autoLoudness)
    {
        const float currentRmsDb = bufferRmsDb(buffer);
        float desiredDeltaDb = 0.0f;
        if (currentRmsDb > -52.0f)
            desiredDeltaDb = std::clamp(targetDb - currentRmsDb, -12.0f, 12.0f);

        const float smoothAlpha = std::clamp(static_cast<float>(numSamples) / (0.25f * static_cast<float>(currentSampleRate)), 0.01f, 1.0f);
        loudnessTrimDbSmoothed += smoothAlpha * (desiredDeltaDb - loudnessTrimDbSmoothed);
        loudnessGain.setGainDecibels(loudnessTrimDbSmoothed);
        gainReductionDb.store(loudnessTrimDbSmoothed, std::memory_order_relaxed);
    }
    else
    {
        loudnessTrimDbSmoothed = 0.0f;
        loudnessGain.setGainDecibels(0.0f);
        gainReductionDb.store(0.0f, std::memory_order_relaxed);
    }

    loudnessGain.process(context);

    outputTrimGain.setGainDecibels(outputDbParam->load());
    outputTrimGain.process(context);

    outputLevelDb.store(bufferRmsDb(buffer), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ShifuVirtMixAudioProcessor::createEditor()
{
    return new ShifuVirtMixAudioProcessorEditor(*this);
}

void ShifuVirtMixAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("currentProgramIndex", currentProgramIndex, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ShifuVirtMixAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto restored = juce::ValueTree::fromXml(*xmlState);
        currentProgramIndex = static_cast<int>(restored.getProperty("currentProgramIndex", 0));
        apvts.replaceState(restored);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShifuVirtMixAudioProcessor();
}
