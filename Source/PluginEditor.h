#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <vector>
#include <memory>

class ShifuVirtMixAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit ShifuVirtMixAudioProcessorEditor(ShifuVirtMixAudioProcessor&);
    ~ShifuVirtMixAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void addKnob(const juce::String& paramId, const juce::String& labelText);

    ShifuVirtMixAudioProcessor& audioProcessor;

    struct KnobSlot
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    std::vector<std::unique_ptr<KnobSlot>> knobs;

    juce::ComboBox presetSelector;
    juce::Label presetLabel;

    juce::ToggleButton autoLoudnessButton { "Auto Loudness" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoLoudnessAttachment;

    float meterInDb = -60.0f;
    float meterOutDb = -60.0f;
    float meterCompDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShifuVirtMixAudioProcessorEditor)
};
