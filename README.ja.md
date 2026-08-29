# superimp

[English](README.md) | [日本語](README.ja.md)

<p align="center">
  <img src="images/teaser.png" alt="Sharp X68000で動作するsuperimp" width="768" height="512">
</p>

<p align="center">
  <strong><a href="https://uraraworks.github.io/WebX68k/?cpu=10&ram=12&fd1=https://raw.githubusercontent.com/renatus-novus-x/superimp/main/dist/superimp.zip&run=1">WebX68kでsuperimpを起動</a></strong>
</p>

Sharp X68000でコンピュータ画面、外部ビデオ画面、スーパーインポーズ画面を
確認するための、Human68k用対話型診断・テストパターンプログラムです。

X68000 IOCSと、X-BASICのIMAGE.FNCにあるcrt(0)からcrt(3)、V_cut(0)、
Vpage(1)に相当するビデオ制御を使用します。

## 必要な環境

実機ですべてのテストを行うには、次の環境が必要です。

- Human68kが動作するSharp X68000
- 外部コンポジット映像を入力できるX68000純正ディスプレイまたはCZ-6TU
- 同期した外部コンポジット映像ソース

WebX68k用ディスクは、プログラムを簡単に起動して画面や操作を確認するために
用意しています。外部コンポジット入力と物理的なスーパーインポーズ機器は
エミュレートされないため、実際の合成結果は実機で確認してください。

## 使用方法

次のコマンドで起動します。

    superimp.x

最初に、画面モードを変更せず現在のビデオ状態を表示します。任意のキーで
次へ進み、ESC キーでテストを終了します。

| ステージ | モード | 期待される表示 |
| --- | --- | --- |
| 1 | コンピュータ | X68000の白いグラフィックテストパターン |
| 2 | crt(2)相当のコントラストダウン合成 | 外部映像とコンピュータ面の比較 |
| 3 | crt(3)相当の標準スーパーインポーズ | 外部映像とX68000パターンの合成 |
| 4 | crt(0)相当のビデオのみ | 外部コンポジット映像のみ |
| 5 | crt(1)相当のコンピュータのみ | X68000グラフィックのみ |
| 6 | 標準スーパーインポーズを再実行 | 合成結果の再確認 |

各切り替え前に、使用するCRTMOD、TVCTRL、グラフィックページ、ビデオカット
設定を表示します。ビデオおよびスーパーインポーズのステージは最大8秒で
コンピュータ画面へ戻ります。待機中に任意のキーを押すとすぐに戻り、
ESC キーでは戻った後にテストを終了します。

テストではIOCS CRTモード13、15kHz、512×512ドット、65536色を使用し、
グラフィックページ1を表示します。必要な正規のモード制御のみを変更します。

終了時には保存したIOCS CRTモードとビデオコントローラーレジスタを復元します。
IOCSには現在のTVCTRL選択を取得する機能がないため、最終的なTV制御状態には
安全側としてコンピュータモードを使用します。

## ビルド

elf2x68kを導入し、m68k-xelf-gccにPATHが通ったWSLまたはLinux環境で
ビルドします。

必要なホストツールをインストールします。

    sudo apt install python3 curl unar

リポジトリ直下からsrcへ移動して実行します。

    cd src
    make

MakefileはHuman68k 3.02とバージョンを固定したxdftool.pyをダウンロードし、
次のファイルを生成します。

    src/superimp.x     Human68k実行ファイル
    src/superimp.xdf   起動可能なHuman68kディスクイメージ
    dist/superimp.zip  Human68k許諾条件を含むWebX68k対応配布アーカイブ

生成したディスクイメージの内容を確認する場合は次を実行します。

    make check-xdf

生成ファイルはmake cleanで削除できます。ダウンロードした補助ファイルも
削除する場合はmake distcleanを使用します。
