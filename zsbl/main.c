/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#include "board.h"

#include <sifive/barrier.h>
#include <sifive/platform.h>
#include <sifive/smp.h>
#include <ux00boot/ux00boot.h>
#include <gpt/gpt.h>
#include "encoding.h"

#ifdef vc707
  #include "tl_clock.h"
#endif

static Barrier barrier;
extern const gpt_guid gpt_guid_sifive_fsbl;

#ifndef vc707
  #define CORE_CLK_KHZ 33000
#else
  #ifdef TL_CLK
    #define CORE_CLK_KHZ TL_CLK/1000
  #else
    #error Must define TL_CLK
  #endif
#endif

void handle_trap(void) {
  ux00boot_fail((long) read_csr(mcause), 1);
}

void init_uart(unsigned int peripheral_input_khz) {
  unsigned long long uart_target_hz = 115200ULL;
  UART0_REG(UART_REG_DIV) = uart_min_clk_divisor(peripheral_input_khz * 1000ULL, uart_target_hz);
}

/* no-op */
int puts(const char* str) {
	return 1;
}

int main() {
  if (read_csr(mhartid) == NONSMP_HART) {
    unsigned int peripheral_input_khz;
#ifndef vc707
    if (UX00PRCI_REG(UX00PRCI_CLKMUXSTATUSREG) & CLKMUX_STATUS_TLCLKSEL) {
      peripheral_input_khz = CORE_CLK_KHZ; // perpheral_clk = tlclk
    } else {
      peripheral_input_khz = (CORE_CLK_KHZ / 2);
    }
#else
    peripheral_input_khz = CORE_CLK_KHZ;
#endif
    init_uart(peripheral_input_khz);
    ux00boot_load_gpt_partition((void*) CCACHE_SIDEBAND_ADDR, &gpt_guid_sifive_fsbl, peripheral_input_khz);
  }

  Barrier_Wait(&barrier, NUM_CORES);

  return 0;
}
