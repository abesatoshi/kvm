#include "ddc.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <string.h>
#include <unistd.h>

// --- Private Apple Silicon DDC/CI API -------------------------------------
// There is no public DDC/CI API on Apple Silicon. Displays are driven through
// the private IOAVService interface (the same one AppleSiliconDDC and m1ddc
// use). We declare the symbols ourselves; they are exported by IOKit.
typedef CFTypeRef IOAVServiceRef;
extern IOAVServiceRef IOAVServiceCreateWithService(CFAllocatorRef allocator,
                                                   io_service_t service);
extern IOReturn IOAVServiceWriteI2C(IOAVServiceRef service, uint32_t chipAddress,
                                    uint32_t offset, void *inputBuffer,
                                    uint32_t inputBufferSize);
// --------------------------------------------------------------------------

// DDC/CI framing constants.
static const uint32_t kDDCChipAddress = 0x37; // monitor's DDC/CI I2C address
static const uint32_t kDDCDataOffset = 0x51;  // DDC/CI source/offset address
static const uint8_t kDDCDestWriteAddr = 0x6E; // 8-bit write address (for xor)

// A write is retried a couple of times after a short settle delay; some
// monitors drop the first transaction. Matches m1ddc's defaults.
static const int kDDCWriteIterations = 2;
static const useconds_t kDDCWriteWait = 10000; // 10 ms

/// Send a single DDC/CI "set VCP feature" packet to one AV service.
static bool write_vcp(IOAVServiceRef av, uint8_t vcp_code, uint16_t value) {
    uint8_t data[6];
    data[0] = 0x80 | 0x04; // 0x84: length byte (4 data bytes follow)
    data[1] = 0x03;        // "set VCP feature" opcode
    data[2] = vcp_code;
    data[3] = (uint8_t)(value >> 8);   // high byte
    data[4] = (uint8_t)(value & 0xFF); // low byte
    data[5] = kDDCDestWriteAddr ^ (uint8_t)kDDCDataOffset ^ data[0] ^ data[1] ^
              data[2] ^ data[3] ^ data[4]; // XOR checksum

    bool ok = false;
    for (int i = 0; i < kDDCWriteIterations; i++) {
        usleep(kDDCWriteWait);
        if (IOAVServiceWriteI2C(av, kDDCChipAddress, kDDCDataOffset, data,
                                sizeof(data)) == KERN_SUCCESS) {
            ok = true;
        }
    }
    return ok;
}

/// Read the "Location" string property of a registry entry.
/// Returns true if the entry represents an external (non-built-in) display.
static bool is_external(io_service_t service) {
    CFTypeRef location = IORegistryEntryCreateCFProperty(
        service, CFSTR("Location"), kCFAllocatorDefault, kNilOptions);
    bool external = location != NULL &&
                    CFGetTypeID(location) == CFStringGetTypeID() &&
                    CFStringCompare(CFSTR("External"), (CFStringRef)location, 0) ==
                        kCFCompareEqualTo;
    if (location != NULL) {
        CFRelease(location);
    }
    return external;
}

int ddc_write_vcp_external(uint8_t vcp_code, uint16_t value, int *external_count) {
    if (external_count != NULL) {
        *external_count = 0;
    }

    // Walk the whole IOService plane looking for DCPAVServiceProxy nodes, which
    // represent per-display DDC/CI endpoints on Apple Silicon.
    io_registry_entry_t root = IORegistryGetRootEntry((mach_port_t)0);
    io_iterator_t iter = 0;
    if (IORegistryEntryCreateIterator(root, kIOServicePlane,
                                      kIORegistryIterateRecursively,
                                      &iter) != KERN_SUCCESS) {
        return 0;
    }

    int externals = 0;
    int succeeded = 0;
    io_service_t service = 0;
    while ((service = IOIteratorNext(iter)) != 0) {
        io_name_t name;
        if (IORegistryEntryGetName(service, name) != KERN_SUCCESS ||
            strcmp(name, "DCPAVServiceProxy") != 0) {
            IOObjectRelease(service);
            continue;
        }

        if (!is_external(service)) {
            IOObjectRelease(service);
            continue;
        }

        IOAVServiceRef av =
            IOAVServiceCreateWithService(kCFAllocatorDefault, service);
        if (av != NULL) {
            externals++;
            if (write_vcp(av, vcp_code, value)) {
                succeeded++;
            }
            CFRelease(av);
        }

        IOObjectRelease(service);
    }

    IOObjectRelease(iter);

    if (external_count != NULL) {
        *external_count = externals;
    }
    return succeeded;
}
