#pragma once

using soundHandle = int;

static constexpr int INVALID_SOUND_HANDLE = -1;

// 初期化
void InitAudio();

// 終了処理
void UninitAudio();

// サウンド読み込み (戻り値: サウンドハンドル / 失敗時 -1)
int LoadSound(const char* filename);

// 再生
void Play(int index, bool loop = false);

// 停止
void StopSound(int index);

// ボリューム設定 (0.0f ~ 1.0f)
void SetVolume(int index, float volume);

// 再生中かどうか確認 (true: 再生中, false: 停止中)
bool IsSoundPlaying(int index);