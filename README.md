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

The program uses X68000 IOCS calls and video-controller settings corresponding
to the X-BASIC IMAGE.FNC operations crt(0) through crt(3), V_cut(0), and
Vpage(1).

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

The program first displays the current video state without changing it. Press
any key to proceed, or press Escape to stop.

| Stage | Mode | Expected result |
| --- | --- | --- |
| 1 | Computer | X68000 white graphics test pattern |
| 2 | Contrast-down superimpose, equivalent to crt(2) | External video and computer layer comparison |
| 3 | Standard superimpose, equivalent to crt(3) | External video combined with the X68000 pattern |
| 4 | Video only, equivalent to crt(0) | External composite video only |
| 5 | Computer only, equivalent to crt(1) | X68000 graphics only |
| 6 | Standard superimpose again | Recheck the combined result |

Before each switch, the exact CRTMOD, TVCTRL, graphics-page, and video-cut
settings are displayed. Video and superimpose stages return to computer mode
after up to eight seconds. Any key returns early; Escape returns and stops the
test.

The test uses IOCS CRT mode 13: 15 kHz, 512 x 512, 65536 colors, with graphics
page 1 visible. It changes only the documented mode controls needed by the
test. Raw CRTC timing registers are never restored from sampled values.

At exit, the saved IOCS CRT mode and video-controller register are restored.
Because IOCS does not provide a query for the current TVCTRL selection, the
program uses computer mode as the safe final TV-control state.

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
