// pch.h: プリコンパイル済みヘッダー ファイルです。
// 次のファイルは、その後のビルドのビルド パフォーマンスを向上させるため 1 回だけコンパイルされます。
// コード補完や多くのコード参照機能などの IntelliSense パフォーマンスにも影響します。
// ただし、ここに一覧表示されているファイルは、ビルド間でいずれかが更新されると、すべてが再コンパイルされます。
// 頻繁に更新するファイルをここに追加しないでください。追加すると、パフォーマンス上の利点がなくなります。

#ifndef PCH_H
#define PCH_H

// プリコンパイルするヘッダーをここに追加します
#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーからほとんど使用されていない部分を除外する

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3D12helper/d3dx12.h>
#include <wrl/client.h>
#include <memory>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")    // 今後 DXGI 関連のエラー（CreateDXGIFactory など）が出るのを防ぐため
#pragma comment(lib, "dxguid.lib")  // IID_ID3D12Device などのID解決エラーを防ぐため

#endif //PCH_H
