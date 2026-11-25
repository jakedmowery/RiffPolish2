#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

ShifuVirtMixAudioProcessorEditor::ShifuVirtMixAudioProcessorEditor(ShifuVirtMixAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    presetLabel.setText("PRESET:", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(presetLabel);

    const auto& presets = audioProcessor.getFactoryPresets();
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
        presetSelector.addItem(presets[static_cast<size_t>(i)].name, i + 1);

    presetSelector.setSelectedId(audioProcessor.getCurrentProgram() + 1, juce::dontSendNotification);
    presetSelector.onChange = [this]
    {
        const int selectedIndex = presetSelector.getSelectedId() - 1;
        if (selectedIndex >= 0)
        {
            audioProcessor.applyPreset(selectedIndex);
            audioProcessor.updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
        }
    };
    addAndMakeVisible(presetSelector);

    addKnob("low_cut_hz", "Low Cut");
    addKnob("high_cut_hz", "High Cut");
    addKnob("low_shelf_db", "Low Tight");
    addKnob("mid_scoop_db", "380Hz Cut");
    addKnob("presence_db", "3.2k Bite");
    addKnob("pick_attack", "Pick Attack");
    addKnob("grit", "Grit");
    addKnob("comp_thresh_db", "Glue Thresh");
    addKnob("stereo_width", "Width");
    addKnob("target_loudness_db", "Target dB");
    addKnob("output_db", "Out Trim");

    autoLoudnessButton.setClickingTogglesState(true);
    addAndMakeVisible(autoLoudnessButton);
    autoLoudnessAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "auto_loudness", autoLoudnessButton);

    setSize(760, 420);
    startTimerHz(30);
}

void ShifuVirtMixAudioProcessorEditor::addKnob(const juce::String& paramId, const juce::String& labelText)
{
    auto slot = std::make_unique<KnobSlot>();

    slot->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slot->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    addAndMakeVisible(slot->slider);

    slot->label.setText(labelText, juce::dontSendNotification);
    slot->label.setJustificationType(juce::Justification::centred);
    slot->label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(slot->label);

    slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, paramId, slot->slider);

    knobs.push_back(std::move(slot));
}

void ShifuVirtMixAudioProcessorEditor::timerCallback()
{
    meterInDb = audioProcessor.getCurrentInputDb();
    meterOutDb = audioProcessor.getCurrentOutputDb();
    meterCompDb = audioProcessor.getGainReductionDb();

    const int hostProg = audioProcessor.getCurrentProgram();
    if (presetSelector.getSelectedId() != hostProg + 1)
        presetSelector.setSelectedId(hostProg + 1, juce::dontSendNotification);

    repaint(0, getHeight() - 48, getWidth(), 48);
}

void ShifuVirtMixAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff14161a));

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("SHIFU VIRTMIX — VIRTUAL GUITAR MIXER", 20, 12, 430, 28, juce::Justification::centredLeft);

    g.setColour(juce::Colours::silver);
    g.setFont(juce::Font(12.0f));
    g.drawText("EQ • Transient • Grit • Width • Loudness", 20, 38, 420, 18, juce::Justification::centredLeft);

    // Bottom status & level strip
    const int footerY = getHeight() - 44;
    g.setColour(juce::Colour(0xff1d2026));
    g.fillRect(16, footerY, getWidth() - 32, 32);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(13.0f));
    const juce::String statusText = juce::String::formatted(
        "IN: %.1f dB   |   OUT: %.1f dB   |   LOUDNESS ADJ: %+.1f dB",
        meterInDb, meterOutDb, meterCompDb);
    g.drawText(statusText, 28, footerY, getWidth() - 220, 32, juce::Justification::centredLeft);
}

void ShifuVirtMixAudioProcessorEditor::resized()
{
    const int marginX = 20;

    // Top-right preset dropdown
    presetLabel.setBounds(getWidth() - 310, 16, 62, 26);
    presetSelector.setBounds(getWidth() - 242, 16, 222, 26);

    const int topOffset = 66;
    const int bottomReserve = 56;
    const int usableWidth = getWidth() - marginX * 2;
    const int usableHeight = getHeight() - topOffset - bottomReserve;

    const int cols = 4;
    const int rows = 3;
    const int cellW = usableWidth / cols;
    const int cellH = usableHeight / rows;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int col = static_cast<int>(i) % cols;
        const int row = static_cast<int>(i) / cols;
        const int x = marginX + col * cellW;
        const int y = topOffset + row * cellH;

        knobs[i]->label.setBounds(x, y, cellW, 20);
        knobs[i]->slider.setBounds(x + 8, y + 20, cellW - 16, cellH - 24);
    }

    // Place Auto Loudness toggle in the 12th grid slot (bottom-right)
    const int toggleX = marginX + 3 * cellW + 14;
    const int toggleY = topOffset + 2 * cellH + 28;
    autoLoudnessButton.setBounds(toggleX, toggleY, cellW - 28, 36);
}
