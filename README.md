# Kuin Programming Language
プログラミング言語「Kuin」

## [English]

***-- Welcome to the Labyrinth of Kuin Compiler,***  
***where many programmers who challenged to read the code never came back. --***

This is the git repository for the Kuin programming language, developed by Kuina-chan.  
It is still under development and the software shows only Japanese messages yet.

All the files here are provided under the Kuina-chan License.  
If you have any questions, please let me know in English or Japanese.

Kuina-chan's website: <http://kuina.ch/>  
Kuina-chan's twitter account: <https://twitter.com/kuina_ch> (@kuina_ch)

## [Japanese]

***――Kuinコンパイラの迷宮へようこそ。***  
***ここは、読解に挑んだ多くのプログラマが帰らぬこととなった場所です――。***

「くいなちゃん」が開発するプログラミング言語「Kuin」( <http://kuina.ch/kuin/a01> )のリポジトリです。  

本リポジトリでは、開発中のソースコードは「develop」ブランチにマージし、リリースするタイミングでそれらの変更を「master」ブランチにマージします。  

#### # 報告の方法
不具合の報告や機能追加を要望したい場合は、下記の方法で行うことができます。  
* 開発者(くいなちゃん)に日本語か英語で直接報告する。
* リポジトリをForkして編集した後Pull Requestをくいなちゃん宛てに送る。
* 修正したソースコードの断片をどこかにアップロードしてくいなちゃんに知らせる。  
戴いたソースコードはわたしが検査して適宜修正しますので、不具合やコーディングルールの不統一があっても問題ありません。  

#### # ビルド方法
Kuinコンパイラを手元でビルドする場合には、Visual C++ 2015でのビルドにのみ対応しており、/kuin.slnを開いてビルドすると完了します。  
他のバージョンのVisual C++でビルドするには調整する必要があるかもしれません。

#### # Kuinコンパイラに対するテスト
Kuinコンパイラに対するテストプログラムを実行する場合は、下記のようにビルドします。  
1. ソリューション構成を「Debug」でビルドする。  
2. 「test」プロジェクトをスタートアッププロジェクトに指定して実行する。  
3. テストに成功した場合「Success.」が、失敗した場合は「Failure.」が表示される。

#### # 完成物の生成
Kuinコンパイラの完成物を生成する場合は、下記のようにビルドします。  
1. ソリューション構成を「Release_dbg」でリビルドする。  
2. ソリューション構成を「Release_rls」でリビルドする。  
3. 成功した場合は、/packageディレクトリ内に完成物がまとめて配置される。

すべてのファイルは「くいなちゃんライセンス( <http://kuina.ch/others/license> )」でご自由にお使いいただけます。  
その他、あらゆるご質問はくいなちゃんまでどうぞ。

Webサイト: <http://kuina.ch/>  
Twitterアカウント: <https://twitter.com/kuina_ch> (@kuina_ch)
