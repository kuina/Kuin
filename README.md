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

「くいなちゃん」が開発するプログラミング言語「Kuin」( <http://kuina.ch/kuin/1> )のリポジトリです。  
「Kuinコンパイラ」のコンパイル済みのzipパッケージが欲しい方は、「 <http://kuina.ch/kuin/dev> 」からダウンロードしてください。

本リポジトリでは、開発中のソースコードは「develop」ブランチにマージし、リリースするタイミングでそれらの変更を「master」ブランチにマージします。  
現時点の「develop」ブランチは多くの機能が未実装な状況で、最初の「master」ブランチへのマージがあるまでは仕様変更を頻繁に行う予定です。

#### # 報告の方法
不具合の報告や機能追加を要望したい場合は、下記の方法で行うことができます。  
* 開発者(くいなちゃん)に日本語か英語で直接報告する。
* リポジトリをForkして編集した後Pull Requestをくいなちゃん宛てに送る。
* 修正したソースコードの断片をどこかにアップロードしてくいなちゃんに知らせる。  
戴いたソースコードはわたしが検査して適宜修正しますので、不具合やコーディングルールの不統一があっても問題ありません。

#### # ビルド方法
Kuinコンパイラを手元でビルドする場合には、Visual C++ 2012、2013、2015でのビルドに少なくとも対応しています。  
VC++2015の場合は/kuin.slnを開いてビルドすると完了しますが、VC++2012、VC++2013の場合はプロジェクトファイルを以下のように調整する必要があります。  
1. /src/内にある「*.vcxproj」をすべてテキストエディタで開く。  
2. 「v140」の表記を、VC++2012の場合は「v110」に、VC++2013の場合は「v120」にすべて置換して保存する。  
3. あとはVC++2015と同様に、/kuin.slnを開いてビルドする。

#### # Kuinコンパイラに対するテスト
Kuinコンパイラに対するテストプログラムを実行する場合は、下記のようにビルドします。  
1. ソリューション構成を「Debug」でビルドする。  
2. 「test」プロジェクトをスタートアッププロジェクトに指定して実行する。  
3. テストに成功した場合「Success.」が、失敗した場合は「Failure.」が表示される。

#### # 完成物の生成
Kuinコンパイラの完成物を生成する場合は、下記のようにビルドします。  
1. ソリューション構成を「Release_dbg」でビルドする。  
2. ソリューション構成を「Release_rls」でビルドする。  
3. 成功した場合は、/packageディレクトリ内に完成物がまとめて配置される。

すべてのファイルは「くいなちゃんライセンス( <http://kuina.ch/others/license> )」でご自由にお使いいただけます。  
その他、あらゆるご質問はくいなちゃんまでどうぞ。

サイト: <http://kuina.ch/>  
Twitterアカウント: <https://twitter.com/kuina_ch> (@kuina_ch)
