// SPDX-License-Identifier: GPL-3.0-or-later
use std::ffi::OsString;
use std::os::windows::ffi::OsStringExt;
use std::path::{Path, PathBuf};
use windows::core::{w, Interface, HSTRING};
use windows::Win32::System::Com::{
    CoCreateInstance, CoInitializeEx, CoTaskMemFree, CoUninitialize, IPersistFile,
    CLSCTX_INPROC_SERVER, COINIT_APARTMENTTHREADED,
};
use windows::Win32::UI::Shell::{
    FOLDERID_Desktop, IShellLinkW, SHGetKnownFolderPath, ShellLink, KF_FLAG_DEFAULT,
};
use windows::Win32::UI::WindowsAndMessaging::{
    MessageBoxW, MB_ICONERROR, MB_OK, MB_SETFOREGROUND,
};

pub fn desktop_shortcut_exists() -> bool {
    desktop_shortcut_path().is_ok_and(|path| path.is_file())
}

pub fn set_desktop_shortcut(enabled: bool) {
    let result = (|| {
        let destination = desktop_shortcut_path()?;
        if enabled {
            save_shortcut(&std::env::current_exe()?, &destination)?;
        } else {
            remove_shortcut(&destination)?;
        }
        Ok::<_, Box<dyn std::error::Error>>(())
    })();

    if let Err(error) = result {
        let action = if enabled { "tạo" } else { "xóa" };
        let message = HSTRING::from(format!("Không thể {action} lối tắt VietType trên Desktop.\n\n{error}"));
        unsafe {
            MessageBoxW(None, &message, w!("VietType"), MB_OK | MB_SETFOREGROUND | MB_ICONERROR);
        }
    }
}

fn desktop_shortcut_path() -> windows::core::Result<PathBuf> {
    unsafe {
        let path = SHGetKnownFolderPath(&FOLDERID_Desktop, KF_FLAG_DEFAULT, None)?;
        let directory = PathBuf::from(OsString::from_wide(path.as_wide()));
        CoTaskMemFree(Some(path.0.cast()));
        Ok(directory.join("VietType.lnk"))
    }
}

fn remove_shortcut(path: &Path) -> std::io::Result<()> {
    match std::fs::remove_file(path) {
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => Ok(()),
        result => result,
    }
}

fn save_shortcut(exe: &Path, destination: &Path) -> windows::core::Result<()> {
    unsafe {
        CoInitializeEx(None, COINIT_APARTMENTTHREADED).ok()?;
        let result = (|| {
            let link: IShellLinkW = CoCreateInstance(&ShellLink, None, CLSCTX_INPROC_SERVER)?;
            let target = HSTRING::from(exe.as_os_str());
            link.SetPath(&target)?;
            if let Some(directory) = exe.parent() {
                link.SetWorkingDirectory(&HSTRING::from(directory.as_os_str()))?;
            }
            link.SetDescription(w!("VietType – Bộ gõ tiếng Việt"))?;
            link.SetIconLocation(&target, 0)?;
            let file: IPersistFile = link.cast()?;
            file.Save(&HSTRING::from(destination.as_os_str()), true)
        })();
        CoUninitialize();
        result
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use windows::Win32::System::Com::STGM_READ;

    #[test]
    fn saves_and_updates_a_shortcut_to_the_executable() {
        let exe = std::env::current_exe().unwrap();
        let destination = std::env::temp_dir()
            .join(format!("VietType shortcut test {}.lnk", std::process::id()));
        save_shortcut(&exe, &destination).unwrap();
        save_shortcut(&exe, &destination).unwrap();

        let target = unsafe {
            CoInitializeEx(None, COINIT_APARTMENTTHREADED).ok().unwrap();
            let result = (|| {
                let link: IShellLinkW = CoCreateInstance(&ShellLink, None, CLSCTX_INPROC_SERVER)?;
                let file: IPersistFile = link.cast()?;
                file.Load(&HSTRING::from(destination.as_os_str()), STGM_READ)?;
                let mut path = [0u16; 1024];
                link.GetPath(&mut path, std::ptr::null_mut(), 0)?;
                let length = path.iter().position(|&character| character == 0).unwrap();
                Ok::<_, windows::core::Error>(PathBuf::from(OsString::from_wide(&path[..length])))
            })();
            CoUninitialize();
            result
        };
        remove_shortcut(&destination).unwrap();
        assert!(!destination.exists());
        remove_shortcut(&destination).unwrap();
        assert_eq!(target.unwrap().canonicalize().unwrap(), exe.canonicalize().unwrap());
    }
}
