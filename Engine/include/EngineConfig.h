#pragma once

// 通常はBuildSettings.propsから指定する。単独で使用する場合はエディタを有効にする
#ifndef WITH_EDITOR
#define WITH_EDITOR 1
#endif

#if WITH_EDITOR != 0 && WITH_EDITOR != 1
#error WITH_EDITOR must be 0 or 1.
#endif
