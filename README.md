# nm-wifi-wifi-handover

Acquire the strength of the AP and automatically switch the AP to connect to.

## AP switching mechanism

1. When scanning is completed, determine the AP to which the NIC is connected according to the following procedure.
  1.1. main NIC If is not connected, connect to the one with strong signal strength
  1.2. Determine the threshold of signal strength
  1.3. 対象NICがAPに接続済みの場合，より電波強度の高い別のAPがあれば，そのAPを選択
  1.4. 対象NICがAPに未接続の場合，電波強度が閾値以上かつ，main NICが接続していないものがあれば選択
  1.5. この時点でAP選択が選択されておらず，NICがAPに未接続の場合，電波強度が一番高いものを選択
  1.6. 接続済みAPと異なるAPが選択されている場合，接続を行う．
2. main NICの接続済みAPの電波強度を監視し，ある程度の変化があった場合，以下の処理を行う
  2.1. NICのロールチェックを行う
  2.2. 閾値(現在は固定値)以上の変化があった場合，スキャン・APの選択・接続を行う
3. APへの接続が完了した際，NICのロールチェックを行う

接続AP選択時の，電波強度の閾値はmain NICの電波強度から決定する．
現在は単に `main_strength * 0.8` としている．


## ロールチェックの手順

main NIC以外のNICそれぞれに対し，以下の処理を行う
1. main NICがAPに未接続の場合，main NICに昇格
3. main NICと接続先が異なり，かつ電波強度がmain NICの接続先より強い場合，main NICへ昇格

