// swift-tools-version: 5.9
import PackageDescription

// kvm — minimal Apple-Silicon tool that switches a monitor's built-in KVM to
// the next PC via DDC/CI. Deliberately has NO external dependencies: all DDC/CI
// communication is done directly through Apple's (private) IOAVService
// interface, declared in the C target. This keeps the supply-chain surface at
// zero — only Apple system frameworks are linked.
let package = Package(
    name: "kvm",
    platforms: [.macOS(.v11)],
    products: [
        .executable(name: "kvm", targets: ["kvm"])
    ],
    targets: [
        // C shim that talks DDC/CI over the Apple Silicon IOAVService interface.
        .target(
            name: "CDDC",
            linkerSettings: [
                .linkedFramework("IOKit"),
                .linkedFramework("CoreFoundation"),
                .linkedFramework("CoreDisplay"),
            ]
        ),
        // Thin Swift CLI.
        .executableTarget(
            name: "kvm",
            dependencies: ["CDDC"]
        ),
    ]
)
