# Modula のモジュール snippets

現段階では右半分を central、左半分を peripheral とする既存の役割分担を維持します。モジュールは snippet を組み合わせて選択します。

- トラックボール: `trackball_pat9125`（PAT9125 v2）、`trackball_pat9125_legacy`（PAT9125 v1）、`trackball_paw3222`、`pmw3610_alt`
- EC11 スロット 1: `encoder_ec11_left`（P1.13 / P1.14）
- EC11 スロット 2: `encoder_ec11_right`（P1.12 / P1.15）

`trackball_pat9125` は、モジュール上の EC11（P1.12 / P1.15）も同時に有効化します。別スロットの `encoder_ec11_left`（P1.13 / P1.14）とは GPIO が異なるため、左 peripheral では次のように同時指定できます。

```yaml
snippet: trackball_pat9125 encoder_ec11_left
```

`encoder_ec11_right` は PAT9125 モジュール内 EC11 と同じ P1.12 / P1.15 を使うため、`trackball_pat9125` と同時指定しません。また、P1.12 を chip-select に使う SPI トラックボールとも併用しません。

`trackball_pat9125_legacy` は PAT9125 v1 基板用です。SDA は P0.04、SCL は P1.13、MOT は P0.05 を使用します。P1.12 の ID_SEL は snippet が gpio-hog で LOW に固定するため、I2C アドレスは `0x75` です。P1.13 を SCL に使うため、`encoder_ec11_left` とは同時指定しません。v1 には v2 のモジュール内 EC11 設定を含めません。
