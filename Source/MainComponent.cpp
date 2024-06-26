#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : audioSetupComp(deviceManager, 0, 256, 0, 256, false, false, false, false) {
    addAndMakeVisible(audioSetupComp);
    addAndMakeVisible(diagnosticsBox);

    diagnosticsBox.setMultiLine(true);
    diagnosticsBox.setReturnKeyStartsNewLine(true);
    diagnosticsBox.setReadOnly(true);
    diagnosticsBox.setScrollbarsShown(true);
    diagnosticsBox.setCaretVisible(false);
    diagnosticsBox.setPopupMenuEnabled(true);
    diagnosticsBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x32ffffff));
    diagnosticsBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x1c000000));
    diagnosticsBox.setColour(juce::TextEditor::shadowColourId, juce::Colour(0x16000000));

    cpuUsageLabel.setText("CPU Usage", juce::dontSendNotification);
    cpuUsageText.setJustificationType(juce::Justification::right);
    addAndMakeVisible(&cpuUsageLabel);
    addAndMakeVisible(&cpuUsageText);

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (760, 360);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    deviceManager.addChangeListener(this);

    startTimer(50);
}

MainComponent::~MainComponent()
{
    deviceManager.removeChangeListener(this);
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* device = deviceManager.getCurrentAudioDevice();

    auto activeInputChannels = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();

    auto maxInputChannels = activeInputChannels.countNumberOfSetBits();
    auto maxOutputChannels = activeOutputChannels.countNumberOfSetBits();

    for (auto channel = 0; channel < maxOutputChannels; ++channel) {
        if ((!activeOutputChannels[channel]) || maxInputChannels == 0) {
            bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
        } else {
            auto actualInputChannel = channel % maxInputChannels;

            if (!activeInputChannels[channel]) {
                bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
            } else {
                auto* inBuffer = bufferToFill.buffer->getReadPointer(actualInputChannel, bufferToFill.startSample);
                auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);

                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample) {
                    outBuffer[sample] = inBuffer[sample] * random.nextFloat() * 0.25f;
                }
            }
        }
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // fill black
    g.fillAll (juce::Colours::black);

    g.setColour(juce::Colours::grey);
    g.fillRect(getLocalBounds().removeFromRight(proportionOfWidth(0.4f)));
}

void MainComponent::resized()
{
    auto rect = getLocalBounds();

    audioSetupComp.setBounds(rect.removeFromLeft(proportionOfWidth(0.6f)));
    rect.reduce(10, 10);

    auto topLine(rect.removeFromTop(20));
    cpuUsageLabel.setBounds(topLine.removeFromLeft(topLine.getWidth() / 2));
    cpuUsageText.setBounds(topLine);
    rect.removeFromTop(20);

    diagnosticsBox.setBounds(rect);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster*) {
    dumpDeviceInfo();
}

juce::String MainComponent::getListOfActiveBits(const juce::BigInteger& b) {
    juce::StringArray bits;

    for (auto i = 0; i <= b.getHighestBit(); ++i) {
        if (b[i]) {
            bits.add(juce::String(i));
        }
    }

    return bits.joinIntoString(", ");
}

void MainComponent::timerCallback() {
    auto cpu = deviceManager.getCpuUsage() * 100;
    cpuUsageText.setText(juce::String(cpu, 6) + " %", juce::dontSendNotification);
}

void MainComponent::dumpDeviceInfo() {
    logMessage("--------------------------------------");
    logMessage("Current audio device type: " + (deviceManager.getCurrentDeviceTypeObject() != nullptr ? deviceManager.getCurrentDeviceTypeObject()->getTypeName() : "<none>"));

    if (auto* device = deviceManager.getCurrentAudioDevice()) {
        logMessage("Current audio device: " + device->getName().quoted());
        logMessage("Sample Rate: " + juce::String(device->getCurrentSampleRate()) + " Hz");
        logMessage("Block size: " + juce::String(device->getCurrentBufferSizeSamples()) + " samples");
        logMessage("Bit depth: " + juce::String(device->getCurrentBitDepth()));
        logMessage("Input channel names: " + device->getInputChannelNames().joinIntoString(", "));
        logMessage("Active input channels: " + getListOfActiveBits(device->getActiveInputChannels()));
        logMessage("Output channel names: " + device->getOutputChannelNames().joinIntoString(", "));
        logMessage("Active output channels: " + getListOfActiveBits(device->getActiveOutputChannels()));
    } else {
        logMessage("No audio device open");
    }
}

void MainComponent::logMessage(const juce::String& m) {
    diagnosticsBox.moveCaretToEnd();
    diagnosticsBox.insertTextAtCaret(m + juce::newLine);
}
