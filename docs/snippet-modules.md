# Modula のモジュール snippets

現段階では右半分を central、左半分を peripheral とする既存の役割分担を維持します。モジュールは snippet を組み合わせて選択します。

- マウスユニット: `mouse-unit`（左の `trackball_pat9125` の置き換え）
- トラックボール: `trackball_pat9125`、`trackball_paw3222`、`pmw3610_alt`
- EC11 スロット 1: `encoder_ec11_left`（P1.13 / P1.14）
- EC11 スロット 2: `encoder_ec11_right`（P1.12 / P1.15）

`trackball_pat9125` は、モジュール上の EC11（P1.12 / P1.15）も同時に有効化します。別スロットの `encoder_ec11_left`（P1.13 / P1.14）とは GPIO が異なるため、左 peripheral では次のように同時指定できます。

```yaml
snippet: trackball_pat9125 encoder_ec11_left
```

`encoder_ec11_right` は PAT9125 モジュール内 EC11 と同じ P1.12 / P1.15 を使うため、`trackball_pat9125` と同時指定しません。また、P1.12 を chip-select に使う SPI トラックボールとも併用しません。

## mouse-unit

コネクタは左から次の順です。ENC-A/B は既存の EC11 ドライバーを使用します。

| ピン | 信号 | 動作 |
| --- | --- | --- |
| GND | GND | 共通接地 |
| VCC | VCC | 電源 |
| P0.16 | ENC-A | ホイール A |
| P1.00 | ENC-B | ホイール B |
| P1.10 | IO2 | 右クリック（SW1） |
| P1.15 | IO1 | 左クリック（SW2） |

SW3 はダイオード経由で IO1 と IO2 を同時に Low にし、ZMK Combo で中クリックを発生させます。専用ドライバーは不要です。内部プルアップ付き `zmk,kscan-gpio-direct` の同一インスタンスで両入力を読み、既存の composite kscan に追加しています。独立したキー位置 58（IO1）/59（IO2）を使うため、従来のモジュールキー位置 56/57 は維持されます。クリックは全レイヤー共通で、押し続けるとドラッグできます。

Combo は `timeout-ms = <2>`。これはデバウンス時間ではなく、central が受け取ったキーイベント間の判定時間です。デバウンスは既存設定と同じ押下8ms・解放18msです。標準ドライバーは各入力を個別にデバウンスし、BLE split はキー状態を転送するため、電気的に同時でも2ms以内のイベント到着は保証されません。SW3の実機確認が必要です。不成立時はまず到着タイミングを確認してください。

Combo の処理とホイールの割り当ては central にも必要なため、**左右両方**へ `mouse-unit` を指定し、両方のファームウェアを書き込みます。GPIO の追加は左側のみです。`build.yaml` は次の構成に更新しています。

```yaml
# 左 peripheral
snippet: mouse-unit encoder_ec11_left
# 右 central
snippet: trackball_paw3222 typing_pwm mouse-unit
```

ユニットのエンコーダーは既存の先頭センサースロット `right_encoder` を使用し、全レイヤーで縦スクロールします。EC05E1220202 は12パルス・12デテント/回転です。ZMK の EC11 ドライバーはA/B両相の全エッジを数えるため、`steps = <48>`（12 × 4）、先頭センサーの `triggers-per-rotation = <12>` とし、1デテントで1回のスクロール動作を発生させます。回路の1kΩ抵抗は直列抵抗であり、外部プルアップではないため、A/B両方に `GPIO_PULL_UP` を指定します（共通端子はGND、各入力に0.01µF）。

2番目の `encoder_ec11_left` は従来の `steps = <60>`、`triggers-per-rotation = <30>` と音量操作（Alt/GUI修飾対応）を維持します。センサーごとの回転回数設定は左右両方へ適用します。仕様出典: [Alps Alpine EC05Eカタログ](https://tech.alpsalpine.com/assets/catalog/product-catalog-ec-03.en.pdf)。別スロットの `encoder_ec11_left` は併用できます。左側では `trackball_pat9125` および `encoder_ec11_right` と併用しません（GPIO/センサースロット競合）。

`mouse-unit` を指定しないビルドでは従来の58キーとセンサー割り当てを維持します。旧 `trackball_pat9125` snippet 自体は残しています。

参考: [ZMK Combo設定](https://zmk.dev/docs/config/combos)、ローカルZMKの `app/src/combo.c`、`app/module/drivers/kscan/kscan_gpio_direct.c`、`app/src/split/bluetooth/central.c`。

### スクロール量の調整

`snippets/mouse-unit/mouse-unit.overlay` の `MOUSE_UNIT_SCROLL_MULTIPLIER` で調整します。現在は `3`（従来の3倍）です。`mouse_scrl` の速度パラメーターを±600から±1800へ変更し、`tap-ms = <20>`、12デテント/回転、回転方向は維持します。OS・アプリ上の表示移動量はスクロール設定にも依存します。

左右クリックはキーマップの `MOUSE_UNIT_BINDINGS` で入れ替え済み（IO1＝左、IO2＝右）です。Comboの `MCLK` はこの入れ替えに影響されません。SW3で右クリックが出る場合は、Comboが成立せず単独クリックが送られている可能性があります。判定時間を1msから2msに変更していますが、実機での確認が必要です。クリック・Combo・スクロールの変更は右centralの再ビルド・書き込みが必要です。
