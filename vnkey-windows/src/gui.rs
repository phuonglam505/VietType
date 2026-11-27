//! Cửa sổ cấu hình — tao + wry (WebView2).

use crate::webview::{self, UiEvent};
use crate::ENGINE;
use std::sync::atomic::{AtomicBool, Ordering};
use windows::Win32::UI::WindowsAndMessaging::GetForegroundWindow;

pub const VERSION: &str = "v2027.01";

static GUI_OPEN: AtomicBool = AtomicBool::new(false);

pub fn open_config_window() {
    if GUI_OPEN.swap(true, Ordering::SeqCst) { return; }
    std::thread::spawn(|| {
        let r = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| run_config()));
        if let Err(e) = r { eprintln!("[gui] panic: {e:?}"); }
        GUI_OPEN.store(false, Ordering::Relaxed);
    });
}

// ── CSS index ↔ charset id ──────────────────────────────────────────────────

use vnkey_engine::app_charset::{cs_index, cs_value, CS_NAMES};
const INPUT_METHODS: [&str; 5] = ["Telex", "Simple Telex", "VNI", "VIQR", "MS Vietnamese"];

// ── Auto-start (registry) ───────────────────────────────────────────────────

fn is_auto_start_enabled() -> bool {
    use std::os::windows::process::CommandExt;
    std::process::Command::new("reg")
        .args(["query", r"HKCU\Software\Microsoft\Windows\CurrentVersion\Run", "/v", "VietType"])
        .creation_flags(0x08000000).output()
        .map(|o| o.status.success()).unwrap_or(false)
}

fn set_auto_start(enable: bool) {
    use std::os::windows::process::CommandExt;
    if enable {
        let exe = std::env::current_exe().map(|p| p.to_string_lossy().into_owned()).unwrap_or_default();
        let _ = std::process::Command::new("reg")
            .args(["add", r"HKCU\Software\Microsoft\Windows\CurrentVersion\Run",
                   "/v", "VietType", "/t", "REG_SZ", "/d", &format!("\"{exe}\""), "/f"])
            .creation_flags(0x08000000).output();
    } else {
        let _ = std::process::Command::new("reg")
            .args(["delete", r"HKCU\Software\Microsoft\Windows\CurrentVersion\Run",
                   "/v", "VietType", "/f"])
            .creation_flags(0x08000000).output();
    }
}

fn relaunch_as_admin() {
    use windows::Win32::UI::Shell::ShellExecuteW;
    use windows::Win32::UI::WindowsAndMessaging::SW_SHOWNORMAL;
    use windows::Win32::Foundation::HWND;
    use windows::core::*;
    unsafe {
        let mut buf = [0u16; 260];
        let len = windows::Win32::System::LibraryLoader::GetModuleFileNameW(None, &mut buf);
        if len > 0 {
            ShellExecuteW(HWND::default(), w!("runas"), PCWSTR(buf.as_ptr()),
                PCWSTR::null(), PCWSTR::null(), SW_SHOWNORMAL);
            if let Ok(mw) = windows::Win32::UI::WindowsAndMessaging::FindWindowW(
                w!("VietTypeHiddenWindow"), w!("VietType")) {
                let _ = windows::Win32::UI::WindowsAndMessaging::PostMessageW(
                    mw, windows::Win32::UI::WindowsAndMessaging::WM_CLOSE,
                    windows::Win32::Foundation::WPARAM(0),
                    windows::Win32::Foundation::LPARAM(0));
            }
        }
    }
}

fn relaunch_normal() {
    use windows::core::*;
    unsafe {
        let mut buf = [0u16; 260];
        let len = windows::Win32::System::LibraryLoader::GetModuleFileNameW(None, &mut buf);
        if len > 0 {
            let path = String::from_utf16_lossy(&buf[..len as usize]);
            let mut si: windows::Win32::System::Threading::STARTUPINFOW = std::mem::zeroed();
            si.cb = std::mem::size_of_val(&si) as u32;
            let mut pi: windows::Win32::System::Threading::PROCESS_INFORMATION = std::mem::zeroed();
            let mut cmd: Vec<u16> = path.encode_utf16().chain(std::iter::once(0)).collect();
            let _ = windows::Win32::System::Threading::CreateProcessW(
                None, PWSTR(cmd.as_mut_ptr()), None, None, false,
                windows::Win32::System::Threading::PROCESS_CREATION_FLAGS(0),
                None, None, &si, &mut pi);
            let _ = windows::Win32::Foundation::CloseHandle(pi.hProcess);
            let _ = windows::Win32::Foundation::CloseHandle(pi.hThread);
            if let Ok(mw) = windows::Win32::UI::WindowsAndMessaging::FindWindowW(
                w!("VietTypeHiddenWindow"), w!("VietType")) {
                let _ = windows::Win32::UI::WindowsAndMessaging::PostMessageW(
                    mw, windows::Win32::UI::WindowsAndMessaging::WM_CLOSE,
                    windows::Win32::Foundation::WPARAM(0),
                    windows::Win32::Foundation::LPARAM(0));
            }
        }
    }
}

// ── HTML ─────────────────────────────────────────────────────────────────────

fn build_html(im: i32, cs: i32, spell: bool, free: bool, modern: bool,
              ede: bool, macro_en: bool, auto_start: bool, run_admin: bool) -> String {
    let cs_idx = cs_index(cs);
    let cs_opts: String = CS_NAMES.iter().enumerate().map(|(i, n)|
        if i == cs_idx { format!(r#"<option value="{i}" selected>{n}</option>"#) }
        else { format!(r#"<option value="{i}">{n}</option>"#) }
    ).collect();

    let im_opts: String = INPUT_METHODS.iter().enumerate().map(|(i, n)|
        if i == im as usize { format!(r#"<option value="{i}" selected>{n}</option>"#) }
        else { format!(r#"<option value="{i}">{n}</option>"#) }
    ).collect();

    fn ck(v: bool) -> &'static str { if v { "checked" } else { "" } }

    let body = format!(r##"
<div class="container">
  <div style="display:flex;align-items:center;gap:11px;padding:9px 0">
    <svg aria-hidden="true" width="38" height="38" viewBox="0 0 128 128" style="flex-shrink:0">
      <rect x="4" y="4" width="120" height="120" rx="28" fill="#78213f"/>
      <path d="M23 43h16l25 49 25-49h16l-33 63H56z" fill="#fff4f7"/>
      <path d="m55 38 12-17h15L68 38z" fill="#ffd257"/>
    </svg>
    <strong style="font-size:17px;line-height:1.2;color:#78213f">VietType - bộ gõ tiếng Việt</strong>
    <span style="margin-left:auto;color:var(--text-dim);white-space:nowrap">{ver}</span>
  </div>
    <div class="row" style="gap:8px">
      <div class="group">
        <label class="group-title" for="cs">Bảng mã</label>
        <select id="cs" onchange="cmd({{cmd:'cs',v:+this.value}})">{cs_opts}</select>
      </div>
      <div class="group">
        <label class="group-title" for="im">Kiểu gõ</label>
        <select id="im" onchange="cmd({{cmd:'im',v:+this.value}})">{im_opts}</select>
      </div>
    </div>
  <section>
    <div class="group-title">Tùy chọn gõ</div>
    <div class="group checkbox-grid">
      <label class="cb-item"><input type="checkbox" {spell} onchange="cmd({{cmd:'spell',v:this.checked}})">Kiểm tra chính tả</label>
      <label class="cb-item"><input type="checkbox" {free} onchange="cmd({{cmd:'free',v:this.checked}})">Bỏ dấu tự do</label>
      <label class="cb-item"><input type="checkbox" {modern} onchange="cmd({{cmd:'modern',v:this.checked}})">Kiểu mới (oà, uý)</label>
      <label class="cb-item"><input type="checkbox" {ede} onchange="cmd({{cmd:'ede',v:this.checked}})">Tiếng Tây Nguyên (Êđê)</label>
      <label class="cb-item"><input type="checkbox" {macro_en} onchange="cmd({{cmd:'macro',v:this.checked}})">Bật gõ tắt</label>
    </div>
  </section>
  <section>
    <div class="group-title">Hệ thống</div>
    <div class="group">
      <label class="cb-item"><input type="checkbox" {autostart} onchange="cmd({{cmd:'autostart',v:this.checked}})">Khởi động cùng Windows</label>
      <label class="cb-item"><input type="checkbox" {admin} onchange="cmd({{cmd:'admin',v:this.checked}})">Chạy với quyền quản trị</label>
      <label class="cb-item"><input type="checkbox" {desktop_shortcut} onchange="cmd({{cmd:'desktop_shortcut',v:this.checked}})">Tạo lối tắt trên Desktop</label>
    </div>
  </section>
  <section>
    <div class="group-title">Công cụ</div>
    <div class="checkbox-grid">
      <button onclick="cmd({{cmd:'macrotbl'}})">Quản lý gõ tắt</button>
      <button onclick="cmd({{cmd:'hotkey'}})">Gán phím</button>
      <button onclick="cmd({{cmd:'blacklist'}})">Loại trừ ứng dụng</button>
      <button onclick="cmd({{cmd:'appcs'}})">Bảng mã theo ứng dụng</button>
      <button onclick="cmd({{cmd:'converter'}})">Chuyển mã</button>
      <button onclick="cmd({{cmd:'info'}})">Giới thiệu</button>
    </div>
  </section>
  <div style="display:flex;justify-content:flex-end">
    <button onclick="cmd({{cmd:'close'}})">Đóng</button>
  </div>
</div>
"##,
        ver = VERSION,
        spell = ck(spell), free = ck(free), modern = ck(modern),
        ede = ck(ede), macro_en = ck(macro_en),
        autostart = ck(auto_start), admin = ck(run_admin),
        desktop_shortcut = ck(crate::shortcut::desktop_shortcut_exists()),
    );

    webview::html(&body, "")
}

// ── IPC handler ──────────────────────────────────────────────────────────────

fn handle_ipc(body: &str, proxy: tao::event_loop::EventLoopProxy<UiEvent>) {
    let owner = unsafe { GetForegroundWindow().0 as isize };
    handle_settings_command(body, Some(owner), || {
        let _ = proxy.send_event(UiEvent::Close);
    });
}

fn handle_settings_command(body: &str, fallback_owner: Option<isize>, close: impl FnOnce()) {
    let msg: serde_json::Value = match serde_json::from_str(body) {
        Ok(v) => v, Err(_) => return,
    };
    let owner = msg["owner"].as_i64().and_then(|handle| isize::try_from(handle).ok()).or(fallback_owner);
    let cmd = msg["cmd"].as_str().unwrap_or("");
    match cmd {
        "cs" => {
            let idx = msg["v"].as_i64().unwrap_or(0) as usize;
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.output_charset = cs_value(idx); }
            }
            crate::config::save();
        }
        "im" => {
            let idx = msg["v"].as_i64().unwrap_or(0) as i32;
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.set_input_method(idx); }
            }
            crate::config::save();
        }
        "spell" => {
            let v = msg["v"].as_bool().unwrap_or(true);
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.spell_check = v; s.sync_options(); }
            }
            crate::config::save();
        }
        "free" => {
            let v = msg["v"].as_bool().unwrap_or(true);
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.free_marking = v; s.sync_options(); }
            }
            crate::config::save();
        }
        "modern" => {
            let v = msg["v"].as_bool().unwrap_or(true);
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.modern_style = v; s.sync_options(); }
            }
            crate::config::save();
        }
        "ede" => {
            let v = msg["v"].as_bool().unwrap_or(false);
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.ede_mode = v; s.sync_options(); }
            }
            crate::config::save();
        }
        "macro" => {
            let v = msg["v"].as_bool().unwrap_or(false);
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.macro_enabled = v; s.sync_options(); }
            }
            crate::config::save();
        }
        "macrotbl" => { open_macro_window(owner); }
        "autostart" => { set_auto_start(msg["v"].as_bool().unwrap_or(false)); }
        "desktop_shortcut" => {
            if let Some(enabled) = msg["v"].as_bool() {
                crate::shortcut::set_desktop_shortcut(enabled);
            }
        }
        "admin" => {
            let v = msg["v"].as_bool().unwrap_or(false);
            crate::tray::set_run_as_admin(v);
            crate::config::save();
            if v && !crate::tray::is_elevated() { relaunch_as_admin(); }
            else if !v && crate::tray::is_elevated() { relaunch_normal(); }
        }
        "blacklist" => { crate::blacklist::open_blacklist_window(owner); }
        "converter" => { crate::converter::open_converter_window(owner); }
        "hotkey" => { crate::hotkey::open_hotkey_window(owner); }
        "appcs" => { crate::app_charset::open_app_charset_window(owner); }
        "info" => { crate::info::open_info_window(owner); }
        "close" => close(),
        _ => {}
    }
}

fn run_config() {
    let (im, cs, spell, free, modern, ede, macro_en) = {
        let g = ENGINE.lock().unwrap_or_else(|e| e.into_inner());
        match g.as_ref() {
            Some(s) => (s.input_method, s.output_charset, s.spell_check, s.free_marking, s.modern_style, s.ede_mode, s.macro_enabled),
            None => (0, 1, true, true, true, false, false),
        }
    };
    let auto_start = is_auto_start_enabled();
    let run_admin = crate::tray::get_run_as_admin();
    let initial = serde_json::json!({
        "version": VERSION,
        "charsets": CS_NAMES,
        "cs": cs_index(cs),
        "methods": INPUT_METHODS,
        "im": im,
        "spell": spell,
        "free": free,
        "modern": modern,
        "ede": ede,
        "macro": macro_en,
        "autostart": auto_start,
        "admin": run_admin,
        "desktop_shortcut": crate::shortcut::desktop_shortcut_exists(),
    });
    if run_qt_settings(&initial) {
        return;
    }
    let html = build_html(im, cs, spell, free, modern, ede, macro_en, auto_start, run_admin);
    webview::run_webview(&format!("VietType {VERSION}"), 520.0, 580.0, &html, None, handle_ipc);
}

fn run_qt_settings(initial: &serde_json::Value) -> bool {
    use std::io::{BufRead, BufReader};
    use std::process::{Command, Stdio};

    let Ok(exe) = std::env::current_exe() else { return false };
    let Ok(mut child) = Command::new(exe.with_file_name("VietTypeSettings.exe"))
        .arg(initial.to_string())
        .arg(std::process::id().to_string())
        .stdout(Stdio::piped())
        .spawn()
    else {
        return false;
    };
    let Some(output) = child.stdout.take() else {
        let _ = child.kill();
        let _ = child.wait();
        return false;
    };
    let mut lines = BufReader::new(output).lines();
    let ready = matches!(lines.next(), Some(Ok(line)) if line == "ready");
    if ready {
        for line in lines.map_while(Result::ok) {
            handle_settings_command(&line, None, || {});
        }
    } else {
        let _ = child.kill();
    }
    let _ = child.wait();
    ready
}

// ── Macro management window ──────────────────────────────────────────────────

pub fn open_macro_window(owner: Option<isize>) {
    std::thread::spawn(move || {
        run_macro_window(owner);
    });
}

fn run_macro_window(owner: Option<isize>) {
    // Đọc macros hiện tại
    let macros_json = {
        let g = ENGINE.lock().unwrap_or_else(|e| e.into_inner());
        match g.as_ref() {
            Some(s) => s.engine.macro_table.to_json(),
            None => "[]".to_string(),
        }
    };

    let body = format!(r##"
<div class="container" style="gap:10px">
  <div style="font-weight:600;font-size:14px">Gõ tắt (Auto-text)</div>
  <div style="font-size:11px;color:var(--text-dim)">Ví dụ: bc → báo cáo, Bc → Báo Cáo, BC → BÁO CÁO</div>
  <div style="display:flex;gap:8px;align-items:center">
    <input id="mk" placeholder="Viết tắt" style="flex:1;padding:4px 8px;border:1px solid var(--border);border-radius:4px;background:var(--bg-card);color:var(--text);font-size:12px">
    <input id="mv" placeholder="Nội dung" style="flex:2;padding:4px 8px;border:1px solid var(--border);border-radius:4px;background:var(--bg-card);color:var(--text);font-size:12px">
    <button onclick="addMacro()" style="padding:4px 12px;border:1px solid var(--accent);border-radius:4px;background:var(--accent);color:#fff;cursor:pointer;font-size:12px">Thêm</button>
  </div>
  <div id="list" style="flex:1;overflow-y:auto;border:1px solid var(--border);border-radius:4px;background:var(--bg-card);font-size:12px"></div>
  <div style="display:flex;gap:8px;justify-content:flex-end">
    <button onclick="cmd({{cmd:'macro_close'}})" style="padding:4px 16px;border:1px solid var(--border);border-radius:4px;background:var(--bg-card);color:var(--text);cursor:pointer;font-size:12px">Đóng</button>
  </div>
</div>
<script>
var macros = {macros_json};
function render() {{
  var h='';
  macros.forEach(function(m,i){{
    h+='<div style="display:flex;align-items:center;padding:4px 8px;border-bottom:1px solid var(--border)">'
      +'<span style="width:100px;font-weight:600">'+esc(m.key)+'</span>'
      +'<span style="flex:1">'+esc(m.value)+'</span>'
      +'<span style="cursor:pointer;color:var(--accent);padding:0 6px" onclick="del('+i+')">✕</span>'
      +'</div>';
  }});
  document.getElementById('list').innerHTML=h||'<div style="padding:16px;text-align:center;color:var(--text-dim)">Chưa có mục nào</div>';
}}
function esc(s){{ return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }}
function addMacro(){{
  var k=document.getElementById('mk').value.trim();
  var v=document.getElementById('mv').value.trim();
  if(!k||!v) return;
  var found=false;
  macros.forEach(function(m){{ if(m.key.toLowerCase()===k.toLowerCase()){{ m.value=v; found=true; }} }});
  if(!found) macros.push({{key:k,value:v}});
  document.getElementById('mk').value='';
  document.getElementById('mv').value='';
  render();
  cmd({{cmd:'macro_save',data:JSON.stringify(macros)}});
}}
function del(i){{
  macros.splice(i,1);
  render();
  cmd({{cmd:'macro_save',data:JSON.stringify(macros)}});
}}
render();
// Vietnamese input via IPC (SendInput không hoạt động trong WebView2)
window.__vnkey_apply=function(b,t){{
  var el=document.activeElement;
  if(!el||el.selectionStart===undefined)return;
  var p=el.selectionStart,v=el.value;
  var before=v.substring(0,p-b),after=v.substring(p);
  el.value=before+t+after;
  el.selectionStart=el.selectionEnd=before.length+t.length;
  // Trigger input event for any listeners
  el.dispatchEvent(new Event('input',{{bubbles:true}}));
}};
document.addEventListener('keydown',function(e){{
  if(e.ctrlKey||e.altKey||e.metaKey)return;
  var el=document.activeElement;
  if(!el||(el.tagName!=='INPUT'&&el.tagName!=='TEXTAREA'))return;
  if(e.key==='Backspace'){{ cmd({{cmd:'vi_bs'}}); return; }}
  if(e.key==='Enter'||e.key==='Tab'||e.key==='Escape'){{ cmd({{cmd:'vi_reset'}}); return; }}
  if(e.key===' '){{ cmd({{cmd:'vi_space'}}); return; }}
  if(e.key.length!==1)return;
  var c=e.key.charCodeAt(0);
  if(c<32||c>126)return;
  e.preventDefault();
  cmd({{cmd:'vi_key',key:e.key}});
}});
</script>
"##);

    let html = webview::html(&body, "");
    webview::run_webview("Gõ tắt — VietType", 460.0, 400.0, &html, owner, handle_macro_ipc);
}

fn handle_macro_ipc(body: &str, proxy: tao::event_loop::EventLoopProxy<UiEvent>) {
    let msg: serde_json::Value = match serde_json::from_str(body) {
        Ok(v) => v, Err(_) => return,
    };
    let cmd = msg["cmd"].as_str().unwrap_or("");
    match cmd {
        "macro_save" => {
            let data = msg["data"].as_str().unwrap_or("[]");
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() {
                    s.engine.macro_table.load_from_json(data);
                }
            }
            crate::config::save();
        }
        "macro_close" => { let _ = proxy.send_event(UiEvent::Close); }
        // Vietnamese input via IPC — xử lý phím trong WebView2 input fields
        "vi_key" => {
            let key = msg["key"].as_str().unwrap_or("");
            if key.len() != 1 { return; }
            let ascii = key.as_bytes()[0];
            let (backs, text) = if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() {
                    s.sync_options();
                    let r = s.engine.process(ascii as u32);
                    let t = String::from_utf8_lossy(&r.output).to_string();
                    (r.backspaces, t)
                } else { (0, String::new()) }
            } else { (0, String::new()) };
            // If engine didn't produce output, insert the original char
            let out = if text.is_empty() { key.to_string() } else { text };
            let js = format!("window.__vnkey_apply({},{})", backs, serde_json::to_string(&out).unwrap_or_default());
            let _ = proxy.send_event(UiEvent::Eval(js));
        }
        "vi_bs" => {
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() {
                    let r = s.engine.process_backspace();
                    if r.processed && r.backspaces > 1 {
                        let t = String::from_utf8_lossy(&r.output).to_string();
                        let js = format!("window.__vnkey_apply({},{})", r.backspaces, serde_json::to_string(&t).unwrap_or_default());
                        let _ = proxy.send_event(UiEvent::Eval(js));
                    }
                }
            }
        }
        "vi_space" => {
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.engine.soft_reset(); }
            }
        }
        "vi_reset" => {
            if let Ok(mut g) = ENGINE.lock() {
                if let Some(s) = g.as_mut() { s.engine.reset(); }
            }
        }
        _ => {}
    }
}
