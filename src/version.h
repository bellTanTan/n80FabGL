/*
  Created by tan (trinity09181718@gmail.com)
  Copyright (c) 2022 tan
  All rights reserved.

* Please contact trinity09181718@gmail.com if you need a commercial license.
* This software is available under GPL v3.

* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */

// 更新履歴
// 2022/06/11 v1.0.5 DMA OFF (I/O ポート 68H へ 0 または I/O ポート 51H へ 0)で画面表示停止忘れ。
//                   DMA OFF での速度UP忘れ。
//                   I/O ポート E2 リードでバンク選択状態を返すようにした。
//                   起動時 PCG-8100 フォント 0xFC に、ぽいフォント定義(Okの後に出る例のアレが無いとON/OFF判別できないと言う旧友の意見ｗ) 
//                   tako2 様公開の倉庫番(https://github.com/tako2/pc8001-games/tree/main/sokoban)
//                   対応(PCG-8100 有効描画と簡易グラフックス描画の両立不良改修)
// 2022/06/08        bugfire2009 様公開の DumpListEditor(http://bugfire2009.ojaru.jp/input.html)
//                   cmt 2段階ロード(cmt自動起動)できない不良改修
// 2022/06/04 v1.0.4 解説してる microSD の構造間違いを正した
// 2022/06/04 v1.0.3 24KiB(24576byte)の本体 ROM をロード出来ている場合 USER.ROM ファイルを 6000H より最大 8192 byte 可にした
//                   本体 ROM プログラムの規定どおり 6000H と 6001H が 41H('A') 42H('B') であるとき本体 ROM プログラムにより
//                   6002H へ制御移行される
// 2022/05/31 v1.0.2 version.h 追加
//                   machine.h Machine::getVersionString , m_version 追加
//                   machine.cpp Machine::Machine m_version 書式化追加 (ESP.getSdkVersion() も付与)
//                   n80FabGL.ino n80FabglMenu メニューダイアログボックス 表題へバージョン情報付与
//                   machine.cpp Machine::vkf11 t88tool.exe にて .n80 ファイル生成の明記
//                                              VK_F11 と SHIFT + VK_F11 の記述を入れ替え
//                   DISK.h DISK::softReset 追加
//                   uPD765A.h uPD765A::softReset 追加 ソフトリセット時シーク音再生停止忘れ
//                   PCG.h PCG::reset ソフトリセット時に ch #0, #1, #2 再生停止忘れ
//                   machine.cpp Machine::softReset に  m_CGA.enablePCG , m_DISK.softReset 追加
// 2022/05/29 v1.0.1 machine.cpp Machine::vkf11 専用機化雛形ロジックコメントアウトを正しくした
//                   CMT.h CMT::writeIO SIO モード時の Serial.write 忘れ
// 2022/05/28 v1.0.0 GitHub 公開
//                   

#pragma once

#define N80FABGL_VERSION_MAJOR      1
#define N80FABGL_VERSION_MINOR      0
#define N80FABGL_VERSION_REVISION   5

#define N80FABGL_VERSION            (  ((int)N80FABGL_VERSION_MAJOR) << 16 \
                                     | ((int)N80FABGL_VERSION_MINOR) << 8 \
                                     | ((int)N80FABGL_VERSION_REVISION) )

