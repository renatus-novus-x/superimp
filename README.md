# superimp

[English](README.md) | [日本語](README.ja.md)

<p align="center">
  <img src="images/teaser.png" alt="superimp running on the Sharp X68000" width="768" height="512">
</p>

<p align="center">
  <strong><a href="https://uraraworks.github.io/WebX68k/?cpu=10&ram=12&fd1=https://raw.githubusercontent.com/renatus-novus-x/superimp/main/dist/superimp.zip&run=1">Launch superimp in WebX68k</a></strong>
</p>

An interactive Human68k diagnostic and test pattern for validating computer,
external video, and superimpose display modes on a Sharp X68000.

The program uses the following control mappings compatible with Human68k 3.02
`BASIC2/IMAGE.FNC`:

| IMAGE.FNC operation | Control mapping |
| --- | --- |
| `crt(0)` | IOCS `_TVCTRL(0x1C)`: VIDEO selection |
| `crt(1)` | IOCS `_TVCTRL(0x1D)`: COMPUTER selection |
| `crt(2)` | IOCS `_TVCTRL(0x1E)`: SUPERIMPOSE, contrast down |
| `crt(3)` | IOCS `_TVCTRL(0x1F)`: SUPERIMPOSE, standard contrast |
| `V_cut(0/1)` | Clear/set bit 7 of the VICON R2 high byte at `$E82600` (16-bit R2 bit 15, YS) |

`crt()`, `V_cut()`, and `Vpage()` are independent controls. `crt()` sends an
external TV/monitor-control command, `V_cut()` controls external-video cut via
VICON YS, and `Vpage()` controls graphics-page visibility. In particular,
`crt(0)` is a TVCTRL VIDEO selection, not the same operation as producing a
video-only result with `V_cut(0)` and `Vpage(0)`.

## Requirements

For the complete test on real hardware:

- A Sharp X68000 running Human68k
- A genuine X68000 monitor or CZ-6TU for feeding external composite video into the X68000 video path
- A synchronized external composite video source

The WebX68k disk is provided for easy startup and inspection of the program.
External composite-video input and physical superimpose hardware are not
emulated, so the combined video result must be verified on real hardware.

## Usage

Run:

    superimp.x

The program first saves the original display state, then enters IOCS CRT mode
13 so that all instructions are shown at 15 kHz, 512 x 512. Press any key to
proceed, or press Escape to stop. The original CRT mode is restored at exit.

| Stage | Mode | Expected result |
| --- | --- | --- |
| 1 | TVCTRL COMPUTER, `crt(1)` | X68000 graphics visible |
| 2 | SUPERIMPOSE contrast down, `crt(2)` | External video and computer graphics, contrast-down variant |
| 3 | SUPERIMPOSE standard, `crt(3)` | External video and computer graphics, standard contrast |
| 4 | VICON VIDEO CUT, `V_cut(1)` | External video disappears; computer graphics remain |
| 5 | GRAPHICS PAGE OFF, `Vpage(0)` with `V_cut(0)` | Graphics page disappears; external video remains |
| 6 | SUPERIMPOSE restore | External video and computer graphics return |
| 7 | TVCTRL COMPUTER, `crt(1)` | Safe computer state before exit |

Before each switch, the exact CRTMOD, TVCTRL, graphics-page, and video-cut
settings are displayed. Video and superimpose stages return to computer mode
after up to eight seconds. Any key returns early; Escape returns and stops the
test.

The test keeps IOCS CRT mode 13 active throughout its instruction and test
stages: 15 kHz, 512 x 512, 65536 colors, with graphics page 1 normally visible.
This prevents instructions from being displayed in a narrow 256 x 256 mode.
It changes only the documented mode controls needed by the test.

At exit, the saved IOCS CRT mode is restored. Only the saved high byte at
`$E82600`, which is the byte modified by `V_cut()`, is written back; the full
VICON R2 word is not restored. TVCTRL state cannot be queried, so the tool
restores TV control to COMPUTER (`0x1D`) as a safe fallback.

## Build

Build under WSL or Linux with the elf2x68k toolchain installed and
m68k-xelf-gcc available in PATH.

Required host tools:

    sudo apt install python3 curl unar

From the repository root:

    cd src
    make

The Makefile downloads Human68k 3.02 and a pinned xdftool.py, then creates:

    src/superimp.x     Human68k executable
    src/superimp.xdf   Bootable Human68k disk image
    dist/superimp.zip  WebX68k-ready archive with the Human68k license

Inspect the generated disk image with:

    make check-xdf

Remove generated files with make clean, or also remove downloaded support
files with make distclean.
