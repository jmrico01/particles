#include "win32_audio.h"

internal bool Win32InitAudio(Win32Audio* winAudio,
    int sampleRate, int channels, int bufferSizeSamples)
{
    // Only support stereo for now
    DEBUG_ASSERT(channels == 2);

    // Try to load Windows 10 version
	HMODULE xAudio2Lib = LoadLibrary("xaudio2_9.dll");
	if (!xAudio2Lib) {
        // Fall back to Windows 8 version
		xAudio2Lib = LoadLibrary("xaudio2_8.dll");
    }
	if (!xAudio2Lib) {
        // Fall back to Windows 7 version
		xAudio2Lib = LoadLibrary("xaudio2_7.dll");
	}
    if (!xAudio2Lib) {
        // TODO load earlier versions?
        DEBUG_PRINT("Could not find a valid XAudio2 DLL\n");
        return false;
    }

    XAudio2Create = (XAudio2CreateFunc*)GetProcAddress(
        xAudio2Lib, "XAudio2Create");
    if (!XAudio2Create) {
        DEBUG_PRINT("Failed to load XAudio2Create function\n");
        return false;
    }

    HRESULT hr;
    IXAudio2* xAudio2;
    hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        DEBUG_PRINT("Failed to create XAudio2 instance, HRESULT %x\n", hr);
        return false;
    }

    IXAudio2MasteringVoice* masterVoice;
    hr = xAudio2->CreateMasteringVoice(&masterVoice,
        channels,
        sampleRate,
        0,
        NULL
    );
    if (FAILED(hr)) {
        DEBUG_PRINT("Failed to create mastering voice, HRESULT %x\n", hr);
        return false;
    }

    WAVEFORMATEXTENSIBLE format;
    format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    format.Format.nChannels = (WORD)channels;
    format.Format.nSamplesPerSec = (DWORD)sampleRate;
    format.Format.nAvgBytesPerSec = (DWORD)(
        sampleRate * channels * sizeof(int16));
    format.Format.nBlockAlign = (WORD)(channels * sizeof(int16));
    format.Format.wBitsPerSample = (WORD)(sizeof(int16) * 8);
    format.Format.cbSize = (WORD)(
        sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX));
    format.Samples.wValidBitsPerSample =
        format.Format.wBitsPerSample;
    format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    hr = xAudio2->CreateSourceVoice(&winAudio->sourceVoice,
        (const WAVEFORMATEX*)&format);
    if (FAILED(hr)) {
        DEBUG_PRINT("Failed to create source voice, HRESULT %x\n", hr);
        return false;
    }

    winAudio->channels = channels;
    winAudio->sampleRate = sampleRate;
    winAudio->bufferSizeSamples = bufferSizeSamples;
	winAudio->buffer = (int16*)VirtualAlloc(0,
        bufferSizeSamples * channels * sizeof(int16),
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    for (int i = 0; i < bufferSizeSamples; i++) {
        winAudio->buffer[i * channels] = 0;
        winAudio->buffer[i * channels + 1] = 0;
    }

    XAUDIO2_BUFFER buffer;
    buffer.Flags = 0;
    buffer.AudioBytes = bufferSizeSamples * channels * sizeof(int16);
    buffer.pAudioData = (const BYTE*)winAudio->buffer;
    buffer.PlayBegin = 0;
    buffer.PlayLength = 0;
    buffer.LoopBegin = 0;
    buffer.LoopLength = 0;
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    buffer.pContext = NULL;

    hr = winAudio->sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        DEBUG_PRINT("Failed to submit buffer, HRESULT %x\n", hr);
        return false;
    }

    hr = winAudio->sourceVoice->Start(0);
    if (FAILED(hr)) {
        DEBUG_PRINT("Failed to start source voice, HRESULT %x\n", hr);
        return false;
    }

    return true;
}
