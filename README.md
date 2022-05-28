# 1. 概要

FabGL 配下で NEC PC-8001 をエミュレーションするプログラムです。

# 2. はじめに

長い間メンテナンスされている鶏肋電算研究所 [N80の部屋](http://home1.catvmics.ne.jp/~kanemoto/n80/index.html) で公開されている
[n80pi](http://home1.catvmics.ne.jp/~kanemoto/dist/n80pi20210814.tar.gz) をベースとしました。有益なコードを無償公開されている事に感謝致します。

Disk Uint 実装において ＰＩ．様公開の [XM8](http://retropc.net/pi/xm8/index.html) [xm8_170.zip](http://retropc.net/pi/xm8/xm8_170.zip) より uPD765A(upd765a.h/upd765a.cpp)をベースとしました。有益なコードを無償公開されている事に感謝致します。

NEC PC-8001 エミュレータのため Disk Unit とのインターフェースは 8255 PIO のみです。元となるコードには DMA インターフェースがありますが移植対象外にしました。

各種 ROM ファイル、FONT ファイル、CMT ファイル、N80 ファイル、D88 ファイルの配布は致しません。手持ちが無くどうしてもと言う方は、歴史的文化的学術的な研究目的として
[internet archive](https://archive.org/)で探して下さいｗ

# 3. ハード

DEKO 様公開の[回路](https://ht-deko.com/arduino/fabgl.html) で動作確認済みです。

![DEKO回路](/img/DEKO回路.jpg)

Narya Ver 2.0 ボード [Kishima Craft Works](https://kishima.github.io/jp/family_mruby/) で動作確認済みです。

![Narya2.0ボード](/img/Narya2.0ボード.jpg)

Narya Ver 2.0 ボードでは GPIO ピン配置が異なるため FabGL 向けのパッチあてが必要です。

# 4. microSD 準備

以下の構成で準備します。

    microSD
    /
    +--PC8001
    |  +-- PC-8001.ROM or 8801-N80.ROM
    |  +-- PC-8001.FON
    |  +-- PC-8031-2W.ROM (OPTION) [2048bytes]
    |  +-- PC-80S31.ROM   (OPTION) [2048bytes=PC-8801mkIISR/FR/...]
    |  +-- DISK.ROM       (OPTION) [8192bytes=PC-8801MA/MC/MH/...]
    |
    +--MEDIA
       +-- DISK
       |   +--- *.d88     (OPTION) 5.25 inch 2D Type
       |
       +-- *.cmt
       +-- *.n80

PC-8001.ROM は 24KiB(24576byte) の物です。<br>
8801-N80.ROM は 32KiB(32768byte) の物です。無くても支障ありません。<br>
PC-8031-2W.ROM は 2KiB(2048byte) の物です。無くても支障ありません。あると Disk Unit 構成ができます。<br>
PC-80S31.ROM は 2KiB(2048byte) の物です。無くても支障ありません。あると Disk Unit 構成ができます。<br>
(PC-80S31K は 8KiB(8192byte)のマスクROMだったような気がします。)<br>
DISK.ROM は 8KiB(8192byte) の物です。無くても支障ありません。あると Disk Unit 構成ができます。<br>

フォントファイルは [n80pi20210814.tar.gz](http://home1.catvmics.ne.jp/~kanemoto/dist/n80pi20210814.tar.gz)より入手できます。
このファイルは 4096 byte で、簡易グラフックスの bit データを展開済みのデータになります。
PC-8001.FON は手持ちの 2048 byte のファイルを利用しても構いません。2048 byte のときは簡易グラフックスの bit データを生成します。

cmt ファイル、n80 ファイル、d88 ファイルはそれぞれの箇所へ配置します。

N-BASIC な cmt ファイルとして color.cmt と mono.cmt をこのリポジトリ MEDIA フォルダに入れています。動作確認の一つとして利用して下さい。

# 5. 操作

キーボードの配置はオリジナルの配置に近づけています。元を忘れた方はこちらで[キーボードレイアウト](https://www.pcmini.jp/manual/pc-8001-manual/keyboard-pc-8001/)を確認できます。<br>
(小生もHAL研PasocomMini PC-8001所有しています。d88ファイル直ロード出来なかったのがとても寂しかったですｗ)

以下、特記します。

    NEC PC-8001    106/109キーボード
        STOP           ESC
        CTRL           左右 Ctrl & Caps Lock
        GRPH           左右 Alt
        INSDEL         Back space & Insert & Delete
        カナ            カタカナ ひらがな ローマ字
                       半角/全角/漢字 (ローマ字カナ変換 IME を実装しようとしてました。需要無いよな。と思って止めました(笑))  
                       F9 メニュー表示(n80pi準拠)
                       F10 PCG-8100 ON/OFF(n80pi準拠)
                       F11 専用機として雛形ロジックあり
                       F12 リセット(n80pi準拠) (SHIFTキー併用でFDCリセット除いたソフトリセット)

# 6. cmt と n80 について

F9 で開くメニュー「Select Files」から利用します。<br>
<br>
microSD の /PC8001/MEDIA に、このリポジトリ内 MEDIA フォルダの color.cmt と mono.cmt が配置されているとして以下進みます。<br>
<br>
F9 で開くメニュー「Select Files」より color.cmt を選択します。<br>
cload"color[RETURN] します。<br>
(くどい説明書きすると [RETURN] は入力文字列では無く 106/109 キーボードの Enter キーを押す事を示します)<br>
run[RETURN]します。<br>
WIDTH 40,20/ WIDTH 40,25/ WIDTH 80,20/ WIDTH 80,25 でのカラーモード COLOR 1 〜 COLOR 7 の表示が実行されます。<br>
<br>
F9 で開くメニュー「Select Files」より mono.cmt を選択します。<br>
cload"mono[RETURN] します。<br>
run[RETURN]します。<br>
WIDTH 40,20/ WIDTH 40,25/ WIDTH 80,20/ WIDTH 80,25 での白黒モード COLOR 0 〜 COLOR 7 の表示が実行されます。<br>
<br>
マシン語ファイルロードは以下の流れになります。<br>
mon[RETURN]<br>
*L[RETURN]<br>
*CTRL+B[RETURN]<br>
<br>
BASIC セーブは以下の形式です。使用できるファイル名は ROM プログラムの制約で6文字です。<br>
csave"test01" → microSD の /PC8001/MEDIA に test01.cmt として保存されます。<br>
<br>
マシン語ファイルセーブは以下の形式です。ROMプログラムの制約でファイル名が無いです。<br>
それでは保存になりませんので開始アドレスと終了アドレスを付与したファイル名としました。<br>
mon[RETURN]<br>
*W0000,001f[RETURN] → microSD の /PC8001/MEDIA に mon0000-001F.cmt として保存されます。<br>
*WE010,E05A[RETURN] → microSD の /PC8001/MEDIA に monE010-E05A.cmt として保存されます。<br>
<br>
n80 ファイルは規定どおり RAM 8000Hよりロードし PC ← FF3DH SP ← WORD(16bit)[FF3EHの値とFF3FHの値] です。<br>

# 7. Disk Unit について

2KiB(2048byte)な ROM ファイルを利用できてる場合、フロッピーディスクは 4 基(fd0,fd1,fd2,fd3)としました。<br>
8KiB(8192byte)な ROM ファイルでも 4 基可能だと思ったのですが動作させてみると 2 基(fd0,fd1)しか出来ませんでした。<br>

F9 で開くメニューに「Disk Rom Disable」と出ているときに確定(Enter or マウスクリック)するとリセットがかかり ROM BASIC が立ち上がります。<br>
F9 で開くメニューに「Disk Rom Enable」と出ているときに確定(Enter or マウスクリック)するとリセットがかかり Disk Unit から boot しようとします。<br>

F9 で開くメニューの「Mount Disk」より fd0,fd1,(fd2,fd3) に挿入したい D88 ファイルを選択します。選択後 F12 でリセットです。場合によっては SHIFT+F12 でいけます。<br>
F9 で開くメニューの「Unmount Disk」より fd0,fd1,(fd2,fd3) に排出したい D88 ファイルを選択します。<br>

2D タイプ(2head 40track 16sector 256sectorByte = 320KiB(327680byte))の D88 ファイル(348848byte)が対象です。<br>
D88 ファイルは Read/Write されます。手持ちの D88 マスターファイルの消失には十分に注意して下さい。<br>
D88 ファイルの「書き込み禁止」ステータス(ファイル先頭より 0 基点で 26 バイト目 10H で書き込み禁止)に対応しています。<br>
2重ガードも可能にしています。n80FabGL.ino 99 行目 bool writeProtected = false; を bool writeProtected = true; にすることで可能です。<br>

Disk Unit CPU(通称サブCPU)は core #1 で動作させています。core #1 には他に ISR/other task がいます。優先順位を上げすぎると CPU 時間を食いつぶして動作不能に陥ります。<br>
タスク調整/クリティカルセクションとしてのミューテックス調整の結果、優先順位 1 としました。Main CPU 側のタスクも優先順位 1 です。<br>
このままだと Disk Unit CPU(通称サブCPU) 常時動作している事になるので ISR/other taskと資源取り合い(主に CPU 時間)が発生します。<br>
そのため Main CPU (core #0)から見た Disk Unit I/O アクセス(ポート FCH/FDH/FEH/FFH)最終より5秒後にサスペンドさせるようにしました。<br>
リジュームは Main CPU (core #0)から見た Disk Unit I/O アクセスがトリガーとなります。<br>

システム全体を安定動作に持っていかせるための弊害は Disk Unit CPU(通称サブCPU) で Disk Uint 以外の動作を行うプログラムを転送して動作待ちを行うプログラムの場合です。<br>
そのタイプのソフトは限られているのでご了承頂けると幸いです。どうしてもと言う場合は src/emu.h の #define DONT_DISKCPU_SUSPEND を有効(先頭2文字の // を消す)にして書き込んで下さい。<br>

Disk Unit CPU(通称サブCPU)を常時走らせて core #1 の資源取り合い(主に CPU 時間)が発生してキーボード入力が抜けるとか重くて画面表示が追いつかないとかを発生させるよりもマシかと思います(笑)<br>

# 8. 拡張した機能

NEC PC-8001 としてのカレンダー時計として時計設定可能です。年保持不可と言う事には変わりません。内部的には 1970 年です。<br>
プリンター出力。LLIST で Arduino IDE のシリアルポートに出てきます。それだけ(笑)<br>
BASIC TERM A,0,0,0 で Arduino IDE のシリアルポートと入出力できます。通信条件は完全無視な実装です(笑)<br>
CMT セーブの状態表示を画面右上にします。<br>
カナロックの状態表示を画面右上にします。<br>
Disk Unit アクセス中を示す状態表示を画面右上にします。<br>
Disk Unit シーク音(プランジャ動作音)を鳴動します。<br>
システム全体動作安定の為 Disk Unit CPU(通称サブCPU)はタスクサスペンド/リジュームします。<br>

# 9. Narya Ver 2.0 ボード向けパッチ

以下の流れです。ホームディレクトリでパッチを行うと言う事です。Windows なので patch.exe が無い? 探して下さい(笑) もう Micro$oft な時代は終わってるから全部完全移植すればいいのに(草)

    $ cd
    $ patch -p0 < patch1.0.8/Narya.patch

# 10. FabGL v1.0.6 を使いたい

と言う方(いわゆる安定志向)に向けたパッチも用意しています(笑)

FabGL完全準拠のハードの場合

    $ cd
    $ patch -p0 < patch1.0.6/Z80.patch
    $ patch -p0 < patch1.0.6/kbdlayouts.patch

Narya Ver 2.0 ボードの場合

    $ cd
    $ patch -p0 < patch1.0.6/Narya.patch

# 11. 問題点

Arduino ESP32 v1.0.6 & FabGL v1.0.8 を利用した場合<br>
<br>
微妙に遅い。<br>
<br>
Arduino ESP32 v2.0.3 & FabGL v1.0.8 を利用した場合<br>
<br>
より微妙に遅い。<br>
boot 後、画面最上部にファンクションキーのゴミ表示が瞬時でる。<br>
disk format(uPD765A write id)が MainCpuTask 側でタイムアウト。<br>
  SD I/O 時間が Arduino ESP32 v2.0.2 より改善されてるが、やっぱり遅いのが原因<br>
  Arduino ESP32 v1.0.6 で 80 track r/w _DEBUG モードで約 28 秒で OK<br>
  Arduino ESP32 v2.0.3 で 50 track r/w _DEBUG モードで約 56 秒で NG<br>
  n-disk basic(20-Sep-1981) format.DS は Disk I/O Error 検出<br>
  s-dos 1.1 format は異常検知で自動 reboot<br>
  CP/M 2.2 format.com は OK (55 秒内 80 track 完了判定は実装無し?ｗ)<br>
<br>
以下、Arduino ESP32 v1.0.6 & Arduino ESP32 v2.0.3 のどちらでも<br>
<br>
WIDTH 80,20 表示崩れ<br>
  なして発生するのかわがんね。単体テストはOK。このモードは放置します。<br>
  Arduino ESP32 v2.0.3 だとだいぶましだけど瞬時崩れが随時見える。<br>
  やっぱ 200LINE をフォント高さ 10 でスキャンライン描画は想定外なのかも知れない。<br>
  WIDTH 40,20 はいけてるのに。かなしすｗ<br>
<br>
左ALTキー併用でテンキー(0〜9)GRAPH文字が入力出来ない<br>
  右ALTキー併用はOK。回避策あるのでいいかとｗ<br>
<br>
beep音、seek音が微妙<br>
  実機(PC-8001/PC-8033/PC-80S31)から生録したデータを<br>
  FabGL準拠のサンプリングレート16kHz/mono/8bitデータ化で<br>
  再生してるけど元がヘタれてる?(ｗ) やっぱコンデンサ全交換要かｗ<br>

# 12. 最後に

超個人的には富士通 FM-8 で OS-9 をまた動かしたい派です。FabGL の emudevs に 6809 無いんです。ＰＩ．様のを移植って方法もあるけど...

あるあるネタですが VGA な画面でレトロゲーしてると家内が「またピロピロ言わしてるだろ!!!この骨董パソヲタが!!!」と部屋に乱入される事しばしばです(爆笑)
