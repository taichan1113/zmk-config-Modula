# Modula ユーザーガイド

Modula は ZMK を使う左右分割キーボードです。トラックボール、EC11 エンコーダ、PWM フィードバックは `build.yaml` の snippet で選択します。この README はファームウェアを選んで書き込む利用者向けの案内です。

## 現在の左右の役割

現段階では、**右半分（`Modula_R`）が central、左半分（`Modula_L`）が peripheral** です。USB を PC に接続するのは右半分です。この役割は Modula の shield 定義で固定されています。

## モジュールを選ぶ

`build.yaml` の `snippet` は空白区切りで複数指定できます。左右の物理配線と一致するものだけを選んでください。

| 分類 | snippet | 用途・注意 |
| --- | --- | --- |
| トラックボール | `trackball_paw3222` | PAW3222 モジュール。 |
| トラックボール | `trackball_pat9125` | PAT9125 v2 モジュール。モジュール内 EC11（P1.12 / P1.15）も有効になります。 |
| トラックボール | `trackball_pat9125_legacy` | PAT9125 v1 基板。P1.12 を ID_SEL 用 LOW に固定し、I2C アドレス `0x75` を使用します。 |
| トラックボール | `pmw3610_alt` | PMW3610-alt モジュール。 |
| エンコーダ | `encoder_ec11_left` | EC11 スロット 1（P1.13 / P1.14）。PAT9125 と併用できます。 |
| エンコーダ | `encoder_ec11_right` | EC11 スロット 2（P1.12 / P1.15）。PAT9125 内蔵 EC11 と競合するため併用しません。 |
| PWM | `typing_pwm` | 打鍵速度に連動する PWM フィードバック。 |
| PWM | `pwm_backlight_runtime` | ZMK backlight が PWM を管理する構成。`typing_pwm` と同時に指定しません。 |

PAT9125 と `encoder_ec11_left` の組み合わせは次のように書けます。

```yaml
snippet: trackball_pat9125 encoder_ec11_left
```

PAT9125 v1 では SCL が P1.13 のため、`encoder_ec11_left` と同時には指定しません。

現在の有効な構成は [build.yaml](build.yaml) を参照してください。モジュールのピン競合については [snippet-modules.md](docs/snippet-modules.md) も参照してください。

## エンコーダの操作

PAT9125 モジュールの内蔵エンコーダは、通常は画面輝度の上下です。追加 EC11 は、通常は音量の上下です。Alt または GUI と同時に回すと、キーマップで定義された別操作になります。キー割り当てを変更する場合は [config/Modula.keymap](config/Modula.keymap) を編集します。

## GitHub Actions で firmware を取得する

1. `build.yaml` を編集して commit・push します。GitHub の **Actions** タブから **Build ZMK firmware** を手動実行しても構いません。
2. workflow が成功したら、該当 run の下部にある **Artifacts** から `firmware` をダウンロードします。
3. zip を展開し、`artifact-name` に対応する `.uf2` ファイルを選びます。`Modula_R` と `Modula_L` を取り違えないでください。

Actions が失敗した場合は、まず build 定義の snippet 名、shield 名、モジュールの競合を確認してください。Actions は新規環境で依存モジュールを取得するため、ローカルで以前に成功した build と結果が異なる場合があります。

## Seeed XIAO BLE への書き込み

1. 対象の半分を USB 接続します。
2. リセットボタンを素早く2回押し、USB ストレージとして認識させます。
3. 対象の `.uf2` ファイルを、そのドライブの直下へコピーします。
4. コピー後に自動で再起動します。認識されない場合は USB を抜き差しして確認してください。

左右には同じ build 世代の firmware を書き込んでください。split 接続や Bluetooth 接続が不安定なときは、左右の artifact、役割、電源状態を確認します。設定初期化が必要な場合は `settings_reset` artifact を**対象の半分だけ**に書き込めますが、Bluetooth の保存情報などが消えるため、必要な場合だけ使用してください。

## 追加情報

- snippet は配線・センサー・PWM を選ぶためのもので、通常のキー配列は変更しません。
- peripheral 側の USB 接続は書き込みや充電に使えますが、通常の PC 接続は central である右半分を使用します。
- firmware の書き込み中は、外部モジュールを不用意に抜き差ししないでください。
