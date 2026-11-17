// SPDX-License-Identifier: MIT
// macOS's own Bluetooth MIDI window (the one in Audio MIDI Setup's MIDI Studio), opened by itself.
// The daemon builds this once with swiftc (host/conductor/blelink.py) and opens it when it finds no MIDI port:
// one click on Connect next to "FM-1 XXXX". It quits when the window closes, or after 10 minutes.
import Cocoa
import CoreAudioKit

let app = NSApplication.shared
app.setActivationPolicy(.accessory)
let wc = CABTLEMIDIWindowController()
wc.showWindow(nil)
NSApp.activate(ignoringOtherApps: true)
NotificationCenter.default.addObserver(forName: NSWindow.willCloseNotification, object: wc.window, queue: nil) { _ in exit(0) }
DispatchQueue.main.asyncAfter(deadline: .now() + 600) { exit(0) }
app.run()
