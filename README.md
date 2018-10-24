# Kuin言語 / Direct2D導入検討ブランチ

管理者：若草春男(twitter: @HaruoWakakusa)

このブランチはkuina/Kuinのdevelopブランチからの派生です。

## 検討内容

### ライブラリ名：dc

dcはDevice Contextの略です。Windowsプログラミング経験者であれば
この言葉を知らない人はいないでしょう。Device ContextはDirect3D 11でも
導入されていますが、Win32 APIのGDI、もしくは.netのWindowsフォームのような
使いやすさを提供します。しかもGPUアクセラレートされた美しい画面描画を
容易に実現させるということを目標としております。

### 実装方法

lib_wnd(d0001.knd)にウィンドウハンドル、またはDirectX系オブジェクト(検討中)を
渡すためのインターフェースを用意します。dcライブラリの本体である
lib_dc(lib_dc.dll)にそれらを入力することでKuinのウィンドウシステムとの
共存を行います。lib_wnd本体を大きく改変することなくDirect2D系ラッパーの
実装を行うことができます。

また、Direct3D系のインターフェースから実装を切り離すことによって
ユーザーがDirect3DとDirect2Dのどちらかを選んだり、両方を用いたりすることが
できます。これはアプリ起動時間を遅くしないようにすることを意図しています。

## 進捗

* Kuin上でDirect2Dを動作させた
* 複数画面でdcライブラリによる描画に成功

### その他

KuinのDLLではcommon.h, common.cを用いていますが、common.hをC/C++ファイルで
共有させるための記述(extern "C")を追加しています。

Direct2DのほかにDirectWriteも実装検討予定です。
