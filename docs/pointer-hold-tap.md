# Pointer-motion-aware hold-tap

`&pht_lt` and `&pht_mt` are local ZMK behaviors for a mouse button that is
also a hold-tap key.

They work like the equivalent `&lt` / `&mt` while the key is held normally.
If a relative pointer event (`INPUT_REL_X` or `INPUT_REL_Y`) arrives before
the tapping term expires, they instead immediately press the tap-side mouse
button and keep it pressed until the key is released. This makes the sequence
"hold key → move trackball → release key" a drag-and-drop.

Use only on the bindings that need this behavior; ordinary `&lt` and `&mt`
are unchanged.

```dts
/* &lt MOUSE MB1 becomes: */
&pht_lt MOUSE MB1

/* &mt LCTRL MB1 becomes: */
&pht_mt LCTRL MB1
```

The definitions are in `dts/behaviors/pointer_hold_tap.dtsi` and are included
by `config/Modula.keymap`. To change the 200 ms tapping term or flavor, add a
local behavior node with `compatible = "modula,behavior-pointer-hold-tap"`
and the desired `bindings`, following the two supplied definitions.

The behavior reacts to any connected ZMK pointing device's X/Y relative
motion, including pointer events forwarded from the split peripheral.

Both supplied definitions use `hold-while-undecided;`. Consequently their
hold binding (the layer or modifier) is active immediately on key down, just
as with ZMK's standard hold-tap option. Pointer movement before the tapping
term releases that hold binding and presses the mouse button instead.
