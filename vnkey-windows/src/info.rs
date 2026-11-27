//! Cửa sổ giới thiệu — tao + wry.

use crate::webview::{self, UiEvent};
use std::sync::atomic::{AtomicBool, Ordering};
use windows::core::{w, PCWSTR};
use windows::Win32::UI::Shell::ShellExecuteW;
use windows::Win32::UI::WindowsAndMessaging::SW_SHOWNORMAL;

static INFO_OPEN: AtomicBool = AtomicBool::new(false);

pub fn open_info_window(owner: Option<isize>) {
    if INFO_OPEN.swap(true, Ordering::SeqCst) { return; }
    std::thread::spawn(move || {
        let r = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| run_info(owner)));
        if let Err(e) = r { eprintln!("[info] panic: {e:?}"); }
        INFO_OPEN.store(false, Ordering::Relaxed);
    });
}

fn build_html() -> String {
    let body = format!(r#"
<div class="container">
  <h1 style="font-size:18px">VietType - gõ tiếng Việt</h1>
  <p>Phiên bản: {version}</p>
  <div class="group" style="display:grid;gap:8px">
    <p>Email: <a href="mailto:hophuonglam@outlook.com" onclick="cmd({{cmd:'email'}}); return false">hophuonglam@outlook.com</a></p>
    <p>Website: <a href="https://github.com/phuonglam505/VietType" onclick="cmd({{cmd:'website'}}); return false">https://github.com/phuonglam505/VietType</a></p>
    <p>Dự án gốc: <a href="https://vnkey.app/" onclick="cmd({{cmd:'upstream'}}); return false">vnkey</a></p>
  </div>
  <div style="text-align:right"><button onclick="cmd({{cmd:'close'}})">Đóng</button></div>
</div>
"#, version = crate::gui::VERSION);
    webview::html(&body, "")
}

fn handle_ipc(body: &str, proxy: tao::event_loop::EventLoopProxy<UiEvent>) {
    let msg: serde_json::Value = match serde_json::from_str(body) {
        Ok(v) => v, Err(_) => return,
    };
    let target = match msg["cmd"].as_str() {
        Some("close") => {
            let _ = proxy.send_event(UiEvent::Close);
            return;
        }
        Some("email") => w!("mailto:hophuonglam@outlook.com"),
        Some("website") => w!("https://github.com/phuonglam505/VietType"),
        Some("upstream") => w!("https://vnkey.app/"),
        _ => return,
    };
    unsafe {
        ShellExecuteW(None, w!("open"), target, PCWSTR::null(), PCWSTR::null(), SW_SHOWNORMAL);
    }
}

fn run_info(owner: Option<isize>) {
    let html = build_html();
    webview::run_webview("Giới thiệu VietType", 410.0, 230.0, &html, owner, handle_ipc);
}
