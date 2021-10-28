# Kuin Programming Language
プログラミング言語「Kuin」

## [English]  
  
***-- Welcome to the Labyrinth of Kuin Compiler,***  
***where many programmers who challenged to read the code never came back. --***  
  
This is the git repository for the Kuin programming language, developed by Kuina-chan.  
  
All the files here are provided under the Kuina-chan License.  
If you have any questions, please let me know in English or Japanese.  
  
Kuina-chan's website: <https://kuina.ch>  
Kuina-chan's twitter account: <https://twitter.com/kuina_ch> (@kuina_ch)  
  
## [Japanese]  
  
***――Kuinコンパイラの迷宮へようこそ。***  
***ここは、読解に挑んだ多くのプログラマが帰らぬこととなった場所です――。***  
  
「くいなちゃん」が開発するプログラミング言語「Kuin」( <https://kuina.ch/kuin> )のリポジトリです。  
  
#### # 報告の方法  
不具合の報告や機能追加を要望したい場合は、下記の方法で行うことができます。  
* 開発者(くいなちゃん)に日本語か英語で直接報告する。  
* リポジトリをForkして編集した後Pull Requestをくいなちゃん宛てに送る。  
* 修正したソースコードの断片をどこかにアップロードしてくいなちゃんに知らせる。  
戴いたソースコードはわたしが検査して適宜修正しますので、不具合やコーディングルールの不統一があっても問題ありません。  
  
#### # ビルド方法  
Kuinコンパイラを手元でビルドする場合には、Visual C++ 2019が必要です。  
  
/build/deploy_for_exe.batを実行すると、以下の段階を経て/build/deploy_exe_ja内にKuinコンパイラ及びKuinエディタが生成されます。  
1. 仮のKuinコンパイラ(/build/kuincl.exe)を使って、/src/compilerのソースからKuinコンパイラが/build/output/kuin.exeに生成されます。  
2. 生成されたコンパイラを使って、/src/compilerのソースからC++のKuinコンパイラ(/build/output/kuin_cpp_ja.cppおよび/build/output/kuin_dll_ja.cpp)が生成されます。  
3. Visual C++を使って、C++のKuinコンパイラから最終的なKuinコンパイラ/build/deploy_exe_ja/kuincl.exeが生成されます。  
C++を経ているのは、Visual C++の強力な最適化によってパフォーマンスを高めるためです。  
4. 最終的なKuinコンパイラを使って、/src/kuin_editorのソースからKuinエディタが/build/deploy_exe_ja/kuin.exeに生成されます。  
  
その後、/build/deploy_exe_ja/内の全ファイルを/build/package_ja/内にコピーすると、パッケージが完成します。  
  
すべてのファイルは「くいなちゃんライセンス( <https://kuina.ch/others/license> )」でご自由にお使いいただけます。  
その他、あらゆるご質問はくいなちゃんまでどうぞ。  
  
Webサイト: <https://kuina.ch>  
Twitterアカウント: <https://twitter.com/kuina_ch> (@kuina_ch)  
