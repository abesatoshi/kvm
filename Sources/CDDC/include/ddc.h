#ifndef KVM_DDC_H
#define KVM_DDC_H

#include <stdbool.h>
#include <stdint.h>

/// Write a VCP value to every external display over DDC/CI (Apple Silicon).
///
/// Apple Silicon has no public DDC/CI API, so this uses the private
/// IOAVService interface (as AppleSiliconDDC / m1ddc do): it enumerates the
/// per-display DCPAVServiceProxy nodes in the IO registry, keeps the ones whose
/// Location is "External", and sends a standard DDC/CI "set VCP feature" packet
/// to each.
///
/// - vcp_code:       the VCP feature code (e.g. 0xE7 for KVM switch)
/// - value:          the 16-bit value to write (e.g. 0xFF00 for "KVM next")
/// - external_count: if non-NULL, receives the number of external displays found
///
/// Returns the number of external displays that accepted the command.
int ddc_write_vcp_external(uint8_t vcp_code, uint16_t value, int *external_count);

#endif /* KVM_DDC_H */
