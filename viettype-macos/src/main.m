/*
 * viettype-macos — Bộ gõ tiếng Việt cho macOS
 * Sử dụng CGEventTap + viettype-engine (Rust FFI)
 *
 * Kiến trúc: CGEventTap hook keyboard → viettype-engine xử lý →
 * CGEvent backspace + Unicode text output. Không dùng IMKit
 * → không gạch chân, tab/autocomplete hoạt động bình thường.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#import <Cocoa/Cocoa.h>
#import "VietTypeAppDelegate.h"

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        [NSApplication sharedApplication];

        VietTypeAppDelegate *delegate = [[VietTypeAppDelegate alloc] init];
        [NSApp setDelegate:delegate];

        NSLog(@"VietType: Khởi động (CGEventTap mode)");

        [NSApp run];
    }
    return 0;
}
