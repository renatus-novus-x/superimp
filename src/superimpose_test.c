#include <x68k/iocs.h>

#include <stdint.h>

#define PATTERN_W 512
#define PATTERN_H 512

#define CRTC_R20 ((volatile void *)0x00E80028UL)

#define VC_R2 ((volatile void *)0x00E82600UL)
#define VC_R2_HI ((volatile void *)0x00E82600UL)

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xffff

/* IOCS TVCTRL codes from IOCS table (0x0c): TV control command. */
#define TVCTRL_VIDEO 0x1C
#define TVCTRL_COMPUTER 0x1D
#define TVCTRL_SUPERIMPOSE_CONTRAST_DOWN 0x1E
#define TVCTRL_SUPERIMPOSE_STANDARD 0x1F

#define VC2_BIT_YS (1u << 15)

/* IOCS CRTMOD 13: 15 kHz, 512x512, 65536 colors, one graphics page. */
#define SUPERIMPOSE_CRT_MODE 13

#define KEY_ESC 0x01

/* IOCS ONTIME counts in 1/100-second units. */
#define RISKY_STAGE_TICKS 800UL
#define ONTIME_TICKS_PER_DAY 8640000UL

typedef struct {
  int crt_mode;
  uint8_t vc2_hi;
} superimpose_state_t;

static superimpose_state_t g_state;

static uint16_t read_w16(volatile void *addr) {
  return (uint16_t)_iocs_b_wpeek((const void *)addr);
}

static uint8_t read_b8(volatile void *addr) {
  return (uint8_t)_iocs_b_bpeek((const void *)addr);
}

static void write_b8(volatile void *addr, uint8_t value) {
  _iocs_b_bpoke((void *)addr, (int)value);
}

static int wait_esc_or_any_key(void) {
  int code;
  for (;;) {
    code = _iocs_b_keyinp();
    if (code >= 0) {
      while (_iocs_b_keysns() != 0) (void)_iocs_b_keyinp();
      return ((code >> 8) & 0x7f) == KEY_ESC;
    }
  }
}

static unsigned long ontime_ticks(void) {
  struct iocs_time now = _iocs_ontime();
  return (unsigned long)now.day * ONTIME_TICKS_PER_DAY +
         (unsigned long)now.sec;
}

static int wait_esc_or_timeout(unsigned long timeout_ticks) {
  unsigned long start = ontime_ticks();

  for (;;) {
    if (_iocs_b_keysns() != 0) {
      int code = _iocs_b_keyinp();
      while (_iocs_b_keysns() != 0) (void)_iocs_b_keyinp();
      return ((code >> 8) & 0x7f) == KEY_ESC;
    }
    if ((unsigned long)(ontime_ticks() - start) >= timeout_ticks) {
      return 0;
    }
  }
}

static void print_text(const char *text) {
  _iocs_b_print(text);
}

static void hex4(char *dst, uint16_t value) {
  static const char kHex[] = "0123456789ABCDEF";
  dst[0] = '0';
  dst[1] = 'x';
  dst[2] = kHex[(value >> 12) & 0x0F];
  dst[3] = kHex[(value >> 8) & 0x0F];
  dst[4] = kHex[(value >> 4) & 0x0F];
  dst[5] = kHex[value & 0x0F];
  dst[6] = 0;
}

static void print_hex_line(const char *label, uint16_t value) {
  char value_text[8];
  hex4(value_text, value);
  print_text(label);
  print_text(value_text);
  print_text("\r\n");
}

static void snapshot_state(void) {
  g_state.crt_mode = _iocs_crtmod(-1);
  g_state.vc2_hi = read_b8(VC_R2_HI);
}

static void restore_state(void) {
  /* TVCTRL has no query operation, so COMPUTER is the safe fallback. */
  _iocs_tvctrl(TVCTRL_COMPUTER);
  if (g_state.crt_mode >= 0) {
    _iocs_crtmod(g_state.crt_mode);
  }
  write_b8(VC_R2_HI, g_state.vc2_hi);
}

static void dump_raw_state(const char *label) {
  int current_mode = _iocs_crtmod(-1);
  uint16_t vc2 = read_w16(VC_R2);

  print_text("\r\n");
  print_text(label);
  print_text("\r\n");
  print_text("Current IOCS CRT mode: ");
  if (current_mode < 0) {
    print_text("unknown\r\n");
  } else {
    print_hex_line("", (uint16_t)current_mode);
  }
  print_hex_line("VC R2 = ", vc2);
  print_text("  YS(video cut) = ");
  print_text((vc2 & VC2_BIT_YS) ? "ON\r\n" : "OFF\r\n");
  print_hex_line("CRTC R20 = ", read_w16(CRTC_R20));
  print_text("Note: R20 is diagnostic only; no raw CRTC register is restored.\r\n");
}

static void fill_rect(int x, int y, int w, int h, uint16_t color) {
  struct iocs_fillptr rect;
  if (w <= 0 || h <= 0) return;
  rect.x1 = (short)x;
  rect.y1 = (short)y;
  rect.x2 = (short)(x + w - 1);
  rect.y2 = (short)(y + h - 1);
  rect.color = color;
  _iocs_fill(&rect);
}

static void draw_test_pattern(void) {
  fill_rect(0, 0, PATTERN_W, PATTERN_H, COLOR_BLACK);
  fill_rect(0, 0, PATTERN_W, 2, COLOR_WHITE);
  fill_rect(0, PATTERN_H - 2, PATTERN_W, 2, COLOR_WHITE);
  fill_rect(0, 0, 2, PATTERN_H, COLOR_WHITE);
  fill_rect(PATTERN_W - 2, 0, 2, PATTERN_H, COLOR_WHITE);

  fill_rect(PATTERN_W / 2 - 1, 10, 3, PATTERN_H - 20, COLOR_WHITE);
  fill_rect(10, PATTERN_H / 2 - 1, PATTERN_W - 20, 3, COLOR_WHITE);

  fill_rect(12, 12, 16, 16, COLOR_WHITE);
  fill_rect(PATTERN_W - 28, 12, 16, 16, COLOR_WHITE);
  fill_rect(12, PATTERN_H - 28, 16, 16, COLOR_WHITE);
  fill_rect(PATTERN_W - 28, PATTERN_H - 28, 16, 16, COLOR_WHITE);

  /* The text-plane pattern remains useful when the graphics page is hidden. */
  print_text("\r\n########################################\r\n");
  print_text("##                                    ##\r\n");
  print_text("##     X68000 SUPERIMPOSE PATTERN     ##\r\n");
  print_text("##             ++++++                 ##\r\n");
  print_text("##          ++++++++++++              ##\r\n");
  print_text("##             ++++++                 ##\r\n");
  print_text("##                                    ##\r\n");
  print_text("########################################\r\n");
}

/*
 * SHARP IMAGE.FNC V_cut(ch):
 * read/modify/write the high byte at $E82600.
 * Bit 7 of this byte is VICON R2 bit 15 (YS).
 *
 * Compatible with Human68k 3.02 BASIC2/IMAGE.FNC.
 */
static void video_cut(int cut) {
  uint8_t r2_hi = read_b8(VC_R2_HI);

  r2_hi &= 0x7f;
  if (cut) {
    r2_hi |= 0x80;
  }

  write_b8(VC_R2_HI, r2_hi);
}

/* Vpage controls graphics-page visibility independently of TVCTRL and YS. */
static void graphics_page(int visible) {
  _iocs_vpage(visible ? 1 : 0);
}

/*
 * SHARP IMAGE.FNC crt(ch):
 *   _iocs_tvctrl(0x1c + ch)
 *
 * Compatible with Human68k 3.02 BASIC2/IMAGE.FNC.
 */
static void image_crt(int channel) {
  _iocs_tvctrl(TVCTRL_VIDEO + channel);
}

static void prepare_superimpose_screen(void) {
  /* Equivalent screen setup to SCREEN 1,3,0,1 in the IMAGE.FNC sample. */
  _iocs_crtmod(SUPERIMPOSE_CRT_MODE);
  _iocs_g_clr_on();
  video_cut(0);
  graphics_page(1);
  draw_test_pattern();
}

static void setup_computer_15k(void) {
  prepare_superimpose_screen();
  image_crt(1);
}

static void setup_superimpose_contrast_down(void) {
  prepare_superimpose_screen();
  image_crt(2);
}

static void setup_superimpose_standard(void) {
  prepare_superimpose_screen();
  image_crt(3);
}

static void setup_video_cut_test(void) {
  prepare_superimpose_screen();
  image_crt(3);
  video_cut(1);
  graphics_page(1);
}

static void setup_graphics_page_off_test(void) {
  prepare_superimpose_screen();
  image_crt(3);
  video_cut(0);
  graphics_page(0);
}

static void ensure_diagnostic_display(void) {
  _iocs_crtmod(SUPERIMPOSE_CRT_MODE);
  video_cut(0);
  graphics_page(1);
  image_crt(1);
}

static void print_stage_plan(const char *title, const char *expectation,
                             const char *mode_details) {
  /* Keep every instruction screen readable even if the previous mode was
     256x256 or a test stage hid the graphics page. */
  ensure_diagnostic_display();
  print_text("\r\n");
  print_text(title);
  print_text("\r\nExpect: ");
  print_text(expectation);
  print_text("\r\nNext mode settings:\r\n");
  print_text(mode_details);
}

static int run_stage(const char *title, const char *expectation,
                     const char *mode_details, void (*setup)(void)) {
  print_stage_plan(title, expectation, mode_details);
  print_text("Press any key to switch, ESC to stop.\r\n");
  if (wait_esc_or_any_key()) {
    return 1;
  }

  if (setup != 0) {
    setup();
  }

  dump_raw_state("Registers");
  print_text("Press any key to continue, ESC to stop.\r\n");
  return wait_esc_or_any_key();
}

static int run_timed_stage(const char *title, const char *expectation,
                           const char *mode_details, void (*setup)(void)) {
  int aborted;

  print_stage_plan(title, expectation, mode_details);
  print_text("\r\nThis test lasts up to 8 seconds, then returns to COMPUTER.\r\n");
  print_text("Any key returns early; ESC returns and stops the test.\r\n");
  print_text("Press any key to start, ESC to stop.\r\n");
  if (wait_esc_or_any_key()) {
    return 1;
  }

  setup();
  aborted = wait_esc_or_timeout(RISKY_STAGE_TICKS);

  /* Recover to the display mode already confirmed by Stage 1. */
  setup_computer_15k();
  print_text("\r\nReturned safely to 15 kHz COMPUTER.\r\n");
  dump_raw_state("Safe COMPUTER state");

  if (aborted) {
    return 1;
  }
  print_text("Press any key to continue, ESC to stop.\r\n");
  return wait_esc_or_any_key();
}

int main(void) {
  int aborted;

  snapshot_state();
  setup_computer_15k();

  print_text("\r\nX68000 Superimpose Diagnostic\r\n");
  print_text("Purpose: distinguish computer/video/composite behavior.\r\n");
  print_text("Please use a real X68000 with an external\r\n");
  print_text("composite video source.\r\n\r\n");

  dump_raw_state("Diagnostic display state");
  print_text("Stage 0: 15 kHz 512x512 COMPUTER display ready.\r\n");
  print_text("The original mode is saved and restored at exit.\r\n");
  print_text("Press any key to start Stage 1.\r\n");
  aborted = wait_esc_or_any_key();
  if (aborted) {
    restore_state();
    return 0;
  }

  if (run_stage(
      "[1] TVCTRL COMPUTER (crt(1))",
      "X68000 graphics visible.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1D COMPUTER\r\n"
      "  V_cut  : 0 (YS OFF; external video allowed)\r\n"
      "  Vpage  : 1 (graphics page visible)\r\n"
      "  Output : X68000 RGB pattern only\r\n",
      setup_computer_15k)) {
    restore_state();
    return 0;
  }

  if (run_timed_stage(
      "[2] SUPERIMPOSE CONTRAST DOWN (crt(2))",
      "external video + computer graphics;\r\n"
      "contrast-down variant.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1E SUPERIMPOSE / contrast down\r\n"
      "  V_cut  : 0 (YS OFF; external video allowed)\r\n"
      "  Vpage  : 1 (graphics page visible)\r\n"
      "  Output : contrast-down superimpose selection\r\n",
      setup_superimpose_contrast_down)) {
    restore_state();
    return 0;
  }

  if (run_timed_stage(
      "[3] SUPERIMPOSE STANDARD (crt(3))",
      "external video + computer graphics;\r\n"
      "standard contrast.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1F SUPERIMPOSE / standard contrast\r\n"
      "  V_cut  : 0 (YS OFF; external video allowed)\r\n"
      "  Vpage  : 1 (graphics page visible)\r\n",
      setup_superimpose_standard)) {
    restore_state();
    return 0;
  }

  if (run_timed_stage(
      "[4] VICON VIDEO CUT (V_cut(1))",
      "external video disappears;\r\n"
      "computer graphics remain visible.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1F SUPERIMPOSE / standard contrast\r\n"
      "  V_cut  : 1 (YS ON; external video cut)\r\n"
      "  Vpage  : 1 (graphics page visible)\r\n",
      setup_video_cut_test)) {
    restore_state();
    return 0;
  }

  if (run_timed_stage(
      "[5] GRAPHICS PAGE OFF (Vpage(0))",
      "graphics-page pattern disappears;\r\n"
      "external video remains visible.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1F SUPERIMPOSE / standard contrast\r\n"
      "  V_cut  : 0 (YS OFF; external video allowed)\r\n"
      "  Vpage  : 0 (graphics page hidden)\r\n"
      "  Note   : text-plane diagnostics may remain visible\r\n",
      setup_graphics_page_off_test)) {
    restore_state();
    return 0;
  }

  aborted = run_timed_stage(
      "[6] SUPERIMPOSE RESTORE",
      "external video + computer graphics restored\r\n"
      "after Stages 4 and 5.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1F SUPERIMPOSE / standard contrast\r\n"
      "  V_cut  : 0 (YS OFF; external video allowed)\r\n"
      "  Vpage  : 1 (graphics page visible)\r\n"
      "  Output : composite VIDEO + X68000 graphics\r\n",
      setup_superimpose_standard);
  if (aborted) {
    restore_state();
    return 0;
  }

  if (run_stage(
      "[7] TVCTRL COMPUTER (crt(1))",
      "X68000 graphics visible in the safe\r\n"
      "COMPUTER state.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1D COMPUTER\r\n"
      "  V_cut  : 0 (YS OFF; external video allowed)\r\n"
      "  Vpage  : 1 (graphics page visible)\r\n",
      setup_computer_15k)) {
    restore_state();
    return 0;
  }

  restore_state();
  print_text("\r\nState restored, end.\r\n");
  print_text("If superimpose only showed VIDEO or COMPUTER in stage 2/3/6,\r\n");
  print_text("check the video input, cabling, and source synchronization.\r\n");
  return 0;
}
