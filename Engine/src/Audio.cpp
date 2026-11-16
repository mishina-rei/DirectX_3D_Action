#include "Engine_pch.h"

#include "Audio.h"
#include <xaudio2.h>
#include <vector>
#include <fstream>
#include <cstring>

#pragma comment(lib, "xaudio2.lib")

// 音声データ管理用構造体
struct AudioData
{
	WAVEFORMATEX wfx;
	std::vector<BYTE> buffer;
	IXAudio2SourceVoice* pSourceVoice;
};

static IXAudio2* g_pXAudio2 = nullptr;
static IXAudio2MasteringVoice* g_pMasteringVoice = nullptr;
static std::vector<AudioData*> g_audioList;

// WAVファイル読み込みヘルパー
bool LoadWav(const char* filename, WAVEFORMATEX& wfx, std::vector<BYTE>& buffer)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) return false;

	// RIFFチャンク読み込み
	char chunkId[4];
	file.read(chunkId, 4);
	if (strncmp(chunkId, "RIFF", 4) != 0) return false;

	file.seekg(4, std::ios::cur); // ファイルサイズは無視

	file.read(chunkId, 4);
	if (strncmp(chunkId, "WAVE", 4) != 0) return false;

	// チャンク探索
	while (file.read(chunkId, 4))
	{
		uint32_t chunkSize;
		file.read(reinterpret_cast<char*>(&chunkSize), 4);

		if (strncmp(chunkId, "fmt ", 4) == 0)
		{
			// fmtチャンク
			memset(&wfx, 0, sizeof(WAVEFORMATEX));
			
			// 少なくともPCMWAVEFORMAT(16byte)分は読む
			uint32_t readSize = (chunkSize < sizeof(WAVEFORMATEX)) ? chunkSize : sizeof(WAVEFORMATEX);
			file.read(reinterpret_cast<char*>(&wfx), readSize);

			// 読み残しがあればスキップ
			if (chunkSize > readSize) {
				file.seekg(chunkSize - readSize, std::ios::cur);
			}
		}
		else if (strncmp(chunkId, "data", 4) == 0)
		{
			// dataチャンク
			buffer.resize(chunkSize);
			file.read(reinterpret_cast<char*>(buffer.data()), chunkSize);
			return true; // データまで読めたら成功とする
		}
		else
		{
			// その他のチャンクはスキップ
			file.seekg(chunkSize, std::ios::cur);
		}
	}

	return false;
}

void InitAudio()
{
	HRESULT hr;
	
	// COM初期化
	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// XAudio2エンジンの作成
	hr = XAudio2Create(&g_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) return;

	// マスターボイスの作成
	hr = g_pXAudio2->CreateMasteringVoice(&g_pMasteringVoice);
	if (FAILED(hr)) return;
}

void UninitAudio()
{
	// ソースボイスの破棄とメモリ解放
	for (auto* data : g_audioList)
	{
		if (data->pSourceVoice)
		{
			data->pSourceVoice->Stop();
			data->pSourceVoice->DestroyVoice();
		}
		delete data;
	}
	g_audioList.clear();

	// マスターボイスの破棄
	if (g_pMasteringVoice)
	{
		g_pMasteringVoice->DestroyVoice();
		g_pMasteringVoice = nullptr;
	}

	// XAudio2の解放
	if (g_pXAudio2)
	{
		g_pXAudio2->Release();
		g_pXAudio2 = nullptr;
	}

	CoUninitialize();
}

int LoadSound(const char* filename)
{
	if (!g_pXAudio2) return INVALID_SOUND_HANDLE;

	AudioData* data = new AudioData();
	data->pSourceVoice = nullptr;

	// WAV読み込み
	if (!LoadWav(filename, data->wfx, data->buffer))
	{
		delete data;
		return -1;
	}

	// ソースボイス作成
	HRESULT hr = g_pXAudio2->CreateSourceVoice(&data->pSourceVoice, &data->wfx);
	if (FAILED(hr))
	{
		delete data;
		return -1;
	}

	g_audioList.push_back(data);
	return (int)g_audioList.size() - 1;
}

void Play(int index, bool loop)
{
	if (index < 0 || index >= (int)g_audioList.size()) return;

	AudioData* data = g_audioList[index];
	if (!data->pSourceVoice) return;

	// バッファの設定
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.pAudioData = data->buffer.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = (UINT32)data->buffer.size();
	buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

	// 再生中なら停止してバッファをクリア
	data->pSourceVoice->Stop();
	data->pSourceVoice->FlushSourceBuffers();

	// バッファを送信して再生
	data->pSourceVoice->SubmitSourceBuffer(&buffer);
	data->pSourceVoice->Start();
}

void StopSound(int index)
{
	if (index < 0 || index >= (int)g_audioList.size()) return;

	AudioData* data = g_audioList[index];
	if (data->pSourceVoice)
	{
		data->pSourceVoice->Stop();
		data->pSourceVoice->FlushSourceBuffers();
	}
}

void SetVolume(int index, float volume)
{
	if (index < 0 || index >= (int)g_audioList.size()) return;

	AudioData* data = g_audioList[index];
	if (data->pSourceVoice)
	{
		data->pSourceVoice->SetVolume(volume);
	}
}

bool IsSoundPlaying(int index)
{
	if (index < 0 || index >= (int)g_audioList.size()) return false;

	AudioData* data = g_audioList[index];
	if (!data->pSourceVoice) return false;

	XAUDIO2_VOICE_STATE state;
	data->pSourceVoice->GetState(&state);

	return state.BuffersQueued > 0;
}
