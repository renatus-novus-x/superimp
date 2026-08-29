#include <x68k/iocs.h>

#include <stdint.h>

#define PATTERN_W 512
#define PATTERN_H 512

#define CRTC_R20 ((volatile void *)0x00E80028UL)

#define VC_R2 ((volatile void *)0x00E82600UL)

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
  uint16_t vc2;
} superimpose_state_t;

static superimpose_state_t g_state;

static uint16_t read_w16(volatile void *addr) {
  return (uint16_t)_iocs_b_wpeek((const void *)addr);
}

static void write_w16(volatile void *addr, uint16_t value) {
  _iocs_b_wpoke((void *)addr, (int)value);
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
  g_state.vc2 = read_w16(VC_R2);
}

static void restore_state(void) {
  /* TVCTRL has no query operation, so COMPUTER is the safe fallback. */
  _iocs_tvctrl(TVCTRL_COMPUTER);
  if (g_state.crt_mode >= 0) {
    _iocs_crtmod(g_state.crt_mode);
  }
  /* Never write sampled CRTC timing registers: zero reads were observed. */
  write_w16(VC_R2, g_state.vc2);
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

static void prepare_superimpose_screen(void) {
  uint16_t vc2;

  /* Equivalent screen setup to SCREEN 1,3,0,1 in the IMAGE.FNC sample. */
  _iocs_crtmod(SUPERIMPOSE_CRT_MODE);
  _iocs_g_clr_on();
  _iocs_vpage(1);

  /* IMAGE.FNC V_cut(0): allow external video while preserving other bits. */
  vc2 = read_w16(VC_R2);
  write_w16(VC_R2, (uint16_t)(vc2 & ~VC2_BIT_YS));
  draw_test_pattern();
}

static void setup_computer_15k(void) {
  _iocs_tvctrl(TVCTRL_COMPUTER);
  prepare_superimpose_screen();
}

static void setup_superimpose_request(void) {
  setup_computer_15k();
  /* IMAGE.FNC crt(2); keep the TV switch as the final operation. */
  _iocs_tvctrl(TVCTRL_SUPERIMPOSE_CONTRAST_DOWN);
}

static void setup_superimpose_graphics_enabled(void) {
  setup_computer_15k();
  /* IMAGE.FNC crt(3); V_cut(0) and Vpage(1) were prepared above. */
  _iocs_tvctrl(TVCTRL_SUPERIMPOSE_STANDARD);
}

static void setup_video_only(void) {
  setup_computer_15k();
  /* Do not draw after switching away from the computer display. */
  _iocs_tvctrl(TVCTRL_VIDEO);
}

static void setup_computer_only(void) {
  setup_computer_15k();
}

static void print_stage_plan(const char *title, const char *expectation,
                             const char *mode_details) {
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
  print_text("\r\nX68000 Superimpose Diagnostic\r\n");
  print_text("Purpose: distinguish computer/video/composite behavior.\r\n");
  print_text("Please use a real X68000 with an external composite video source.\r\n\r\n");

  snapshot_state();
  dump_raw_state("Initial state");
  print_text("Stage 0: current machine state only (no mode change).\r\n");
  print_text("Press any key to start Stage 1.\r\n");
  aborted = wait_esc_or_any_key();
  if (aborted) {
    return 0;
  }

  if (run_stage(
      "[1] 15 kHz COMPUTER",
      "white pattern only (TVCONTROL=COMPUTER).",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1D COMPUTER\r\n"
      "  VPAGE  : page 1 visible\r\n"
      "  VC R2  : YS OFF (V_cut(0)); EXON is not changed\r\n"
      "  Output : X68000 RGB pattern only\r\n",
      setup_computer_15k)) {
    restore_state();
    return 0;
  }

  if (run_timed_stage(
      "[2] REQUEST SUPERIMPOSE",
      "IMAGE.FNC crt(2) equivalent; compare video and computer layers.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1E (IMAGE.FNC crt(2))\r\n"
      "  VPAGE  : page 1 visible\r\n"
      "  VC R2  : YS OFF; EXON is not changed\r\n"
      "  Output : contrast-down superimpose selection\r\n",
      setup_superimpose_request)) {
    restore_state();
    return 0;
  }

  if (run_timed_stage(
      "[3] STANDARD SUPERIMPOSE",
      "video + white X68000 pattern (if both are visible).",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1F (IMAGE.FNC crt(3))\r\n"
      "  VPAGE  : page 1 visible\r\n"
      "  VC R2  : YS OFF; EXON is not changed\r\n"
      "  Note   : video hardware selects sync; software does not assert EXON\r\n",
      setup_superimpose_graphics_enabled)) {
    restore_state();
    return 0;
  }

  if (run_timed_stage(
      "[4] VIDEO ONLY",
      "expect source composite only; computer graphic may disappear.",
      "  CRTMOD : 0x000D (pattern remains prepared)\r\n"
      "  TVCTRL : 0x1C (IMAGE.FNC crt(0))\r\n"
      "  VC R2  : YS OFF; EXON is not changed\r\n"
      "  Output : composite VIDEO only\r\n",
      setup_video_only)) {
    restore_state();
    return 0;
  }

  if (run_stage(
      "[5] COMPUTER ONLY",
      "expect white pattern only (TV input may disappear).",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1D (IMAGE.FNC crt(1))\r\n"
      "  VPAGE  : page 1 visible\r\n"
      "  VC R2  : YS OFF; EXON is not changed\r\n"
      "  Output : X68000 RGB pattern only\r\n",
      setup_computer_only)) {
    restore_state();
    return 0;
  }

  aborted = run_timed_stage(
      "[6] SUPERIMPOSE AGAIN",
      "recheck composite result after toggling through stages.",
      "  CRTMOD : 0x000D (15 kHz, 512x512, 65536 colors)\r\n"
      "  TVCTRL : 0x1F (IMAGE.FNC crt(3))\r\n"
      "  VPAGE  : page 1 visible\r\n"
      "  VC R2  : YS OFF; EXON is not changed\r\n"
      "  Output : composite VIDEO + X68000 graphics\r\n",
      setup_superimpose_graphics_enabled);
  if (aborted) {
    restore_state();
    return 0;
  }

  restore_state();
  print_text("\r\nState restored, end.\r\n");
  print_text("If superimpose only showed VIDEO or COMPUTER in stage 2/3/6,\r\n");
  print_text("check the video input, cabling, and source synchronization.\r\n");
  return 0;
}
