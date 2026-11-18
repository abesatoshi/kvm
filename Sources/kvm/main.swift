import CDDC
import Foundation

let version = "0.2.0"

// VCP 0xE7 with value 0xFF00 triggers "switch KVM to next PC" on monitors that
// implement a built-in KVM (e.g. Dell Pxxxx / Uxxxx series).
let kvmVCPCode: UInt8 = 0xE7
let kvmNextValue: UInt16 = 0xFF00

func printError(_ message: String) {
    FileHandle.standardError.write(Data("Error: \(message)\n".utf8))
}

/// Send the "KVM next" command to every external display and report the result.
func runKVMNext() -> Int32 {
    var externalCount: Int32 = 0
    let succeeded = ddc_write_vcp_external(kvmVCPCode, kvmNextValue, &externalCount)

    guard externalCount > 0 else {
        printError("no external displays found")
        return 1
    }

    guard succeeded > 0 else {
        printError("failed to send KVM switch command "
            + "(display may not support DDC/CI or a built-in KVM)")
        return 1
    }

    return 0
}

func usage() {
    print("""
    kvm \(version) — switch a monitor's built-in KVM to the next PC (Apple Silicon)

    USAGE:
      kvm next       Switch the KVM to the next connected PC
      kvm            Same as 'kvm next'
      kvm --version  Print the version
      kvm --help     Show this help

    Sends DDC/CI VCP 0xE7 = 0xFF00 to every external display.
    """)
}

// Minimal argument handling — no third-party parser needed.
let args = Array(CommandLine.arguments.dropFirst())
switch args {
case [], ["next"]:
    exit(runKVMNext())
case ["--version"], ["-v"]:
    print(version)
    exit(0)
case ["--help"], ["-h"], ["help"]:
    usage()
    exit(0)
default:
    printError("unknown command '\(args.joined(separator: " "))'. Try 'kvm --help'.")
    exit(2)
}
