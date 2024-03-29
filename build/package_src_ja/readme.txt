-------------------------------------------------------------------------------
Kuin Programming Language
v.2021.8.17
(C) Kuina-chan
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
1. はじめに
-------------------------------------------------------------------------------

    「くいなちゃん」が制作するプログラミング言語「Kuin」へようこそ。
    Kuinは、簡単で高速な実用プログラミング言語です。

    このzipにはソースコードのみが含まれます。
    Windows用のコンパイラおよびエディタやサンプル一式は、
    「Kuinのダウンロードと紹介( https://kuina.ch/kuin/download )」から
    ダウンロードしてください。

    また、Kuinの詳細については「Kuinドキュメント( https://kuina.ch/kuin )」を
    ご覧ください。

-------------------------------------------------------------------------------
2. 動作環境
-------------------------------------------------------------------------------

    Kuinは、64bitのWindowsで動作する実行ファイルのほか、C++やJavaScriptの
    ソースコードを生成することができ、様々な環境で動作します。

-------------------------------------------------------------------------------
3. 連絡先
-------------------------------------------------------------------------------

    あらゆるご質問、ご要望は「くいなちゃん」までどうぞ。

    Webサイト: https://kuina.ch/
    Twitterアカウント: https://twitter.com/kuina_ch (@kuina_ch)

    下記の投稿フォームからは、くいなちゃんに匿名で気軽にメッセージが送れますので
    ご活用ください。
    
    https://kuina.ch/others/contact

-------------------------------------------------------------------------------
4. 変更履歴
-------------------------------------------------------------------------------

v.2021.8.17
    - KuinEditorにコマンドライン引数が指定できる機能の追加
    - math@permutationNext、math@permutationPrev、math@searchBreadthFirst、
      math@searchDepthFirst、math@searchPermutation関数の追加
    - .joinメソッドを[]int、[]floatに拡張
    - file@Reader.readStrメソッドが先頭の全角文字を読み飛ばすことがある不具合の
      修正
    - KuinEditorの操作性の向上
v.2021.7.17
    - file@copyDir関数が正しく動作しないことがある問題の修正
    - 環境がcppのときのint型の比較結果が正しくないことがある不具合の修正
v.2021.6.17
    - 英語版の対応
v.2021.4.17
    - 細かな不具合修正
        - コンパイルが終わらないことがある不具合の修正
        - file@やinput@の入力処理の速度改善
v.2020.9.17
    - web版の機能を本格化、「くいんべーだー」が動作
v.2020.8.17
    - 内部でファイルを開く関数は、ファイルが開けなかったときに例外を発生させる
      ように変更
    - 使用頻度が低めな機能の見直し
        - dict型の.toArrayKey、.toArrayValueメソッドの廃止
    - 細かな不具合修正
        - mathライブラリを使うとコンパイルエラーが発生することがあった問題の
          修正
        - lib@hermite関数が正しい結果を返さなかった問題の修正
        - drawライブラリを参照せずにwnd@makeDraw関数を呼び出すとエラーになる問題
          の修正
v.2020.7.17
    - コンパイル時の、処理時間とメモリ消費量の全体的な削減
    - 細かな機能追加
        - float.nan、file@Reader.readPart、file@Writer.writePartメソッドの追加
    - 細かな不具合修正
        - 環境が「cpp」のときにビルドできないコードを生成することがある不具合の
          修正
        - .toStrFmtメソッドが使えなかった問題の修正
        - パスに全角文字が含まれていたときに、リリースビルド時のresファイルが
          正しく生成されない問題の修正
        - resフォルダに巨大なファイルが存在したときに、リリースビルド時にメモリ
          不足になる問題の修正
        - クラスのメンバ関数の前にメンバ変数を定義するとコンパイルに失敗する
          ことがある不具合の修正
v.2020.6.27
    - コンパイラをKuinで実装し(セルフホスティング)、全体の大幅な見直し
    - Windows向けexeファイルの出力以外にも、C++やJavaScriptのソースコードで出力
      できるように対応
    - 様々な不具合修正、改善、機能追加
    - 使用頻度が低めな機能の見直し
        - (以下、C++やJavaScript対応の事情によりやむを得ず変更)
        - コンパイラのオプションで「-e cui」を「-e exe」へ変更、「-e wnd」を
          「-e exe -x wnd」へ変更
        - オーバーライドの引数や戻り値や、型名の中でクラスを使ったときに、
          今までは継承関係にあるクラスが使えたが、継承関係のクラスは使えないよう
          に型チェックを厳密化
        - :$(スワップ)演算子の廃止
        - file@exeDir、file@sysDir関数を、wnd@exeDir、wnd@sysDirへ移動
        - .clampメソッドを、lib@clamp、lib@clampFloat関数へ移動
        - list<>.sortメソッドの廃止。 配列に変換して配列の.sortメソッドを使う
          ことを推奨
        - その他、既存の関数の引数が微妙に変わっているところがいくつか
v.2020.5.17
    - エディタの補完機能でエディタが落ちることがあった不具合の修正
    - nullをキャストしたときに実行時エラーにならないように変更
    - 細かな機能追加
        - []char.toBin64、list.idx、list.getPtr、list.setPtr、
          file@Writer.flushメソッドの追加
        - lib@addr、lib@toBit64Forcibly、lib@toFloatForcibly、
          cui@flush関数の追加
        - sql@Sqlクラスにいくつかメソッドを追加
    - 細かな不具合修正
        - [][]char.joinメソッドの要素数が空のときに正しい結果が返らない
          不具合の修正
        - エディタの補完機能の不具合の修正
v.2019.9.17
    - 3D描画の環境光の計算の不具合の修正
    - 影を描画する機能と、フラットに描画する機能の追加
    - 細かな機能追加
        - sql@Sql.getBlob、sql@Sql.errMsgメソッドの追加

v.2019.8.17
    - 画面を視覚的に作成できる機能の操作性の向上

v.2019.6.17
    - 任意のファイルをdataフォルダ以下に自動コピーできる機能をエディタに追加
    - sndライブラリで、ストリーミング再生時にもsetPosやgetPosが行えるように拡張
    - 画面を視覚的に作成できる機能を大幅に改善
    - スニペット機能を大幅に改善
    - 細かな機能追加
        - テキストの拡縮描画が行えるdraw@Font.drawScaleメソッドの追加
        - draw@Blendに%exclusion(除外)の追加
        - BGM再生に特化したbgmライブラリ、カーソル処理を行うcursorライブラリの
          追加

v.2019.5.17
    - 細かな機能追加
        - エディタの操作感の改善
        - テクスチャや画像のサイズを取得するdraw@Tex.width、draw@Tex.height、
          draw@Tex.imgWidth、draw@Tex.imgHeightメソッドの追加
        - cui@delimiter、cui@inputLetter、cui@inputInt、cui@inputFloat、
          cui@inputChar、cui@inputStr、math@fibonacci関数の追加
        - num@BigInt、num@BigFloat、num@Complexクラスの追加
    - 細かな不具合の修正
        - 「条件式?(null,参照型)」とすると、参照型の値がまれに壊れることがある
          不具合の修正
        - 継承元クラスにaliasを指定するとコンパイルエラーが発生する不具合の修正

v.2019.4.17
    - 細かな機能追加
        - エディタの操作感の改善
        - エディタの関数のヒント表示を分かりやすく改善
        - エディタのメニューに「最近使ったファイル」の追加
        - 小さいバッファを作成して荒いドットで描画できるwnd@makeDrawReduced関数
          の追加
    - 細かな不具合の修正
        - エディタで変更していないときに保存を促すメッセージが出ることがある
          不具合の修正
        - エディタのスクロールバー上でマウスカーソルがちらつく問題の修正
        - :+演算子等の左辺値が2回評価される不具合の修正

v.2019.3.17
    - 一部のビデオカードを使用した環境で、文字の描画が崩れる問題の修正
    - 細かな機能追加
        - draw@makePlane、draw@makeBox、draw@makeSphere関数の追加
        - 3D描画の大幅な改善
        - サンプルの修正

v.2019.2.17
    - 細かな機能追加
        - kuincl.exeに「-a」オプションが指定できない不具合の修正
        - dbg@printで出力したテキストに合わせてスクロールバーを自動でスクロール
          するように改善

v.2019.1.17
    - ウインドウを視覚的に作成する機能の操作性の改善
    - 2D描画を視覚的に作成する機能の追加
    - file@Readerがリリースビルド時にresフォルダ内のファイルを読み込めない不具合
      の修正
    - 細かな機能追加
        - draw@Font.setHeight、draw@Font.getHeight、draw@Font.calcSize、
          draw@Font.alignメソッドの追加
        - draw@Font.drawメソッドで、文字列に'\n'が含まれていたときに改行する
          ように改善
        - []char.toIntメソッドで、「0x」から始まる場合には16進数として変換する
          ように改善

v.2018.12.17
    - ウインドウを視覚的に作成する機能を全面的に作り直し
    - エディタ上でファイルを保存することなくファイルの追加ができるように改善
    - 互換性が失われる変更
        - wnd@ListViewに画像を設定できるようにし、それに伴い引数等の変更

v.2018.11.17
    - ローカルなデータベース(SQLite)が構築できるsqlライブラリの追加
    - 高度な2D図形が描画できるdraw2dライブラリの追加
    - 互換性が失われる変更
        - dict.getの引数で存在の有無が取得できるように、引数を変更
        - []char.toInt、[]char.toFloatの引数をdict.getと同じ形に変更
    - 細かな機能追加
        - file@setCurDir、file@getCurDir、lib@countUp、draw@capture、
          file@moveDir、file@fullPath、zip@unzip関数の追加
        - dict.delメソッドの追加
    - 細かな不具合の修正
        - カレントディレクトリがexeの位置に書き換わっていたのを、書き換えない
          ように修正
        - file@makeDir関数が相対パスでは正しく動作しない不具合の修正
        - int型の^演算子で、結果がintの範囲内になるにもかかわらず
          オーバーフローの例外が発生することがある不具合の修正
        - game@Map.setとgame@Map.findが正しく動作していなかった不具合の修正
        - 配列.find、配列.findLastの第2引数に-1以外を指定したときの動作が
          おかしくなっていた不具合の修正

v.2018.10.17
    - ピクセル単位で色の読み書きができるwnd@DrawEditableクラスと、使い方を示す
      0014_edit_pixelsサンプルの追加
    - エディタの補完時に強制終了することがある不具合の修正
    - エディタの補完時の挙動を改善
    - 互換性が失われる変更
        - file@Reader.delimiterに'\n'を登録しない場合に、改行を区切り文字と
          みなさないように変更
    - 細かな機能追加
        - 「#」「##」「$>」「$<」演算子が使えないクラスにこれらを使うと、
          コンパイル時にエラーにするように改善
        - デバック実行の終了後にエディタウインドウをアクティブにするように改善
        - wnd@Drawクラスのクリアが手動で行える、draw@autoClear、draw@clear関数
          の追加
        - wnd@fileDialogDir、draw@circleLine、draw@poly、draw@polyLine関数の
          追加
        - draw@circle関数の描画結果にアンチエイリアスがかかるように改善
    - 細かな不具合の修正
        - 要素数が0のdict型のtoBinメソッドを呼ぶと例外が発生する不具合の修正
        - resフォルダが存在しないときにkuincl.exeがログに「Failure.」を出力
          する不具合の修正
        - クラスの参照が巡回しているときにコンパイルに失敗することがある
          不具合の修正

v.2018.9.17
    - エディタの補完機能を改善
    - エディタのカーソル表示が消えたり残像が残ったりする問題の改善
    - エディタにブレークポイント機能を追加
    - 例外が発生したときの位置を行単位で特定するように改善
    - 細かな不具合の修正
        - エディタで検索ウインドウを表示せずにF3を押すと終了する不具合の修正
        - class内にclassを定義するとエディタが終了する不具合の修正
        - リリースビルド時にresフォルダ内のxmlファイルがxml@makeXml関数で
          読み込めない不具合の修正

v.2018.8.17
    - エディタにすべてのドキュメントや選択範囲から検索や置換ができる機能を追加
    - エディタに「res」フォルダを開く機能を追加
    - エディタにファイルをドロップして開く機能を追加
    - game@ライブラリの追加
    - game@ライブラリに2Dマップチップと衝突判定を扱うクラスの追加
    - game@ライブラリにシューティングゲームのステージやスタッフロールを扱う
      クラスの追加
    - 粒子を描画するdraw@Particleクラスの追加
    - 2Dゲーム用のフリー素材を添付

v.2018.7.17
    - よく使うコードを簡単に挿入できる、スニペット機能の追加
    - 互換性が失われる変更
        - wnd@WndBase.enableをwnd@WndBase.setEnabledに変更
    - 細かな機能追加
        - wnd@WndBase.getEnabled、wnd@WndBase.getVisible、
          wnd@ListView.clearColumnメソッドの追加
        - wnd@ListView.onSel、wnd@ListView.onMouseClickイベントの追加
    - 細かな不具合の修正
        - math@lcm関数に渡す値が大きいとオーバーフローする不具合の修正

v.2018.6.17
    - 互換性が失われる変更
        - 「+**」構文の廃止と、オーバーライド元メソッドを参照する「super」構文の
          追加
        - 16進数リテラルの構文を「16#」から「0x」に変更
    - 細かな機能追加
        - list.find、list.findLastメソッドの追加
        - math@knapsack、math@dijkstra、math@bellmanFord、math@floydWarshall、
          draw@filterNone、draw@filterMonotone関数の追加

v.2018.5.17
    - 輪郭線を描画するメソッドdraw@Obj.drawOutlineの追加
    - トゥーンレンダリングで描画するメソッドdraw@Obj.drawToonの追加
    - 互換性が失われる変更
        - draw@Obj.drawの引数の順序の変更

v.2018.3.17
    - 細かな不具合の修正
        - aliasの定義が循環していた場合に不明なエラーが発生する不具合の修正
        - 型定義が不正な場合にエディタが終了する不具合の修正

v.2018.2.17
    - ソースコードの自動整形機能の追加
    - 補完機能の挙動を全面的に改良
    - elif、else、case、default、catch、finallyがスコープ扱いになるように変更

v.2018.1.17
    - ウインドウを視覚的にデザインできる機能の追加
    - Kuinエディタが強制終了することがある不具合の修正
    - draw@render(0)を実行すると垂直同期を待ち、実際には60FPS程度になっていた
      不具合の修正
    - include文の追加
    - 不透明度(アルファ)が本来よりも透明に描画されていた不具合の修正
    - 互換性が失われる変更
        - wnd@Keyの要素mouseCをmouseMに変更
        - draw@Obj.mtxをdraw@Obj.matに変更
        - regex@Regex.matchの引数の変更
        - wnd@WndBase.getPosScrをwnd@WndBase.getPosScreenに変更
        - draw@makeFontの引数の変更
    - 細かな機能追加
        - draw@argbToColor、draw@colorToArgb、input@mousePos関数の追加
        - draw@Font.maxWidth、draw@Font.maxHeight、draw@Font.calcWidthメソッドの追加

v.2017.12.17
    - リリースビルド時に、resフォルダをres.kndファイルにアーカイブする機能の
      追加
    - HTTP通信を行うnet@Httpクラスの追加
    - Kuinエディタの操作感の改善
    - 不透明度(アルファ)が本来よりも不透明に描画されていた不具合の修正
    - 互換性が失われる変更
        - draw@makeFontの引数の変更
    - 細かな機能追加
        - lib@lerp、lib@qerp、lib@cerp、lib@hermite、file@ext関数の追加
    - 細かな不具合の修正
        - draw@Font.drawで座標が小数の場合に線が描画されることがある不具合の修正

v.2017.11.17
    - ヘッダ情報が多すぎる.pngファイルの読み込みに失敗する不具合の修正
    - Kuinエディタに補完機能を追加
    - XMLライブラリ「xml」の追加
    - ネットワークライブラリ「net」の追加
    - 「excpt@invalidDataFormat」を「excpt@invalidDataFmt」に改名
    - 「excpt@noMemory」を「excpt@noMem」に改名
    - 「wnd@Wnd.setActive」を「wnd@Wnd.activate」に改名
    - 細かな機能追加
        - Kuinエディタの検索ダイアログの操作感の改善
        - lib@levenshtein関数の追加
        - list.sort、list.sortDesc、dict.toArrayKey、
          dict.toArrayValue、int.toStrFmt、float.toStrFmtメソッドの追加
    - 細かな不具合の修正
        - listの要素がcharの場合、toArrayの戻り値の末端に不正な文字が付加される
          不具合の修正
        - file@fileName関数の終端に文字コード0の文字が入る不具合の修正
        - []char.replaceメソッドのoldに空文字列を指定すると無限ループになる
          不具合の修正
        - kuin@Class.toStr()の戻り値をnullではなく空文字列が返るように修正
        - ドローコントロールのサイズを変更するとクリアカラーがリセットされる
          不具合の修正
        - float型を要素に持つ配列、list、stack、queue、dictの組み込みメソッドの
          うち、戻り値がfloatのメソッドが正しく動作しない不具合の修正
        - ダブルクリック時にwnd@Draw.onMouseDownLイベントが発生しない不具合の
          修正
        - file@Readerのファイル読み込みメソッドで2byte以上の文字が正しく読み込め
          ない不具合の修正
        - regex@Regex.replaceの書式がPerl互換になっていなかった不具合の修正

v.2017.10.17
    - 音が鳴らない環境でKuinエディタの起動に失敗する不具合の修正
    - 一部の環境やKuinエディタを多重起動したときにexeファイルの生成に失敗する
      不具合の修正
    - 数学・科学・アルゴリズムライブラリ「math」の追加
    - 正規表現ライブラリ「regex」の追加
    - 解像度の低い環境で大きいウインドウサイズを指定したときに、ウインドウが画面
      外に出ないように改善
    - Kuinエディタに「検索・置換」機能の追加
    - dbg@print関数の出力結果がKuinエディタ上で自動改行される不具合の修正
    - クラスのプロパティが関数型だった場合、関数呼び出しではなくメソッド呼び出し
      と解釈されてアクセス違反になる不具合の修正
    - 一部のライブラリで、デバッグ実行時にのみ発生する例外が発生しないことがある
      不具合の修正
    - 細かな機能追加
        - file@forEach関数の追加
        - 型の不一致のエラーメッセージで参照渡しを表示するように改善
        - lib@toRad、lib@toDegree、lib@makeBmSearch、lib@rndUuid、
          lib@cmpEx、lib@min、lib@minFloat、lib@max、lib@maxFloat関数の追加
        - int.sign、float.sign、lib@Rnd.rndUuid、[]char.findStr、
          []char.findStrLast、[]char.findStrEx、[].min、[].max、[].repeat、
          wnd@WndBase.getPosScrメソッドの追加
        - 「excpt@」以下に例外コードの定数を追加
        - kuin@ClassにtoStrメソッドを定義
    - 細かな不具合の修正
        - 実行直後の例外をKuinエディタで検知できない不具合の修正
        - Windows内部で発生する不明な例外をKuinエディタが検知してしまわないよう
          に修正
        - Kuinエディタ上でマウスホイールのスクロールを行ったときにスクロールバー
          が更新されない不具合の修正
        - 関数を超えてブロック変数にアクセスしたときに予期せぬエラーが発生する
          不具合の修正
        - input@setCfgKey関数で正しくキーが割り当てられない不具合の修正
        - []char.toInt、[]char.toFloatメソッドで変換の失敗が正しく検出できない
          不具合の修正

v.2017.9.17
    - 完成版としてリリース
