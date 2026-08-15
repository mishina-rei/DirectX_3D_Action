#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <assert.h>
#include <Windows.h>
#include <stdarg.h>
#include <stdio.h>

static const int FPS = 60;
static const float fFPS = static_cast<float>(FPS);

// リソースパス
#define ASSET(path)	"Assets/"path
#define SHADER(path)"Assets/Shader/"#path".cso"
#define MODEL(path)"Assets/Model/"#path".fbx"
#define SOUND(path)"Assets/Sound/"#path".wave"
#define TEXTURE(path)"Assets/Texture/"#path".png"
#define EFFECT(path)"Assets/Effect/"#path".efkefc"

// 3D空間定義
#define CMETER(value) (value * 0.01f)
#define METER(value) (value * 1.0f)
#define MSEC(value) (value / fFPS)
#define CMSEC(value) MSEC(CMETER(value))
static const float GRAVITY = 0.98f;

// タイトル
//static const char* APP_TITLE = "ENDLESS_SHOOT";

// 画面サイズ
static const int SCREEN_WIDTH	= 1920;
static const int SCREEN_HEIGHT	= 1080;

static constexpr float SCREEN_HALF_WIDTH = SCREEN_WIDTH * 0.5f;
static constexpr float SCREEN_HALF_HEIGHT = SCREEN_HEIGHT * 0.5f;

// グリッドサイズ(デバッグ用
static const int	DEBUG_GRID_NUM		= 10;			// グリッド中心から端までの線の本数
static const float	DEBUG_GRID_MARGIN	= METER(1.0f);	// グリッド配置幅


#endif // __DEFINES_H__