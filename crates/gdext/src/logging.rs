//! Cross-thread logging facility.
//!
//! Tool routing runs entirely on tokio worker threads. Godot's
//! `godot_print!`/`godot_warn!`/`godot_error!` macros must be invoked from
//! the main thread (calling them off-thread triggers
//! `attempted to access binding from different thread than main thread` UB
//! and crashes the editor).
//!
//! To get tool-call logs into the Godot editor's "Output" panel, we:
//! 1. Worker threads push `LogRecord`s onto a global MPSC channel via
//!    `log_info/log_warn/log_error`.
//! 2. The main-thread pump (registered against `SceneTree::process_frame`
//!    in `editor_plugin::enter_tree`) calls `drain_to_console()` every frame,
//!    which dequeues records and forwards them to the appropriate Godot
//!    macro.
//!
//! Every record is *also* mirrored to `eprintln!` so the messages still show
//! up in the terminal stderr stream when Godot is launched from a shell or
//! the addon is run during tests.

use std::sync::Mutex;
use std::sync::OnceLock;
use std::sync::mpsc::{Receiver, Sender, channel};

use godot::global::{godot_error, godot_print, godot_warn};

const MAX_PAYLOAD_LEN: usize = 512;
const FORMAT_PREFIX: &str = "[Godot MCP]";

#[derive(Clone, Copy, Debug)]
pub enum LogLevel {
    Info,
    Warn,
    Error,
}

impl LogLevel {
    fn as_str(self) -> &'static str {
        match self {
            LogLevel::Info => "INFO",
            LogLevel::Warn => "WARN",
            LogLevel::Error => "ERROR",
        }
    }
}

struct LogRecord {
    level: LogLevel,
    tool: String,
    msg: String,
}

static SENDER: OnceLock<Sender<LogRecord>> = OnceLock::new();
static RECEIVER: OnceLock<Mutex<Receiver<LogRecord>>> = OnceLock::new();

fn channels() -> &'static Sender<LogRecord> {
    SENDER.get_or_init(|| {
        let (tx, rx) = channel::<LogRecord>();
        // First initialiser also stashes the receiver. `set` is infallible here
        // because we are inside `get_or_init` which guarantees single execution.
        let _ = RECEIVER.set(Mutex::new(rx));
        tx
    })
}

fn truncate(text: &str) -> String {
    if text.len() <= MAX_PAYLOAD_LEN {
        text.to_string()
    } else {
        let head: String = text.chars().take(MAX_PAYLOAD_LEN).collect();
        format!("{}… ({} bytes total)", head, text.len())
    }
}

fn emit(level: LogLevel, tool: &str, msg: &str) {
    let truncated = truncate(msg);
    // Mirror to stderr so the log is captured even before the main-thread
    // pump is installed (very early plugin boot) or in headless test runs.
    eprintln!(
        "{}[{}][{}] {}",
        FORMAT_PREFIX,
        tool,
        level.as_str(),
        truncated
    );
    let record = LogRecord {
        level,
        tool: tool.to_string(),
        msg: truncated,
    };
    let _ = channels().send(record);
}

pub fn log_info(tool: &str, msg: &str) {
    emit(LogLevel::Info, tool, msg);
}

#[allow(dead_code)]
pub fn log_warn(tool: &str, msg: &str) {
    emit(LogLevel::Warn, tool, msg);
}

pub fn log_error(tool: &str, msg: &str) {
    emit(LogLevel::Error, tool, msg);
}

/// Drain every queued log record and forward it to the matching Godot macro.
///
/// **Must only be called from the main thread.** Designed to be invoked
/// from a `SceneTree::process_frame` signal handler so the call site never
/// holds a binding on any of our own `GodotClass` instances; that prevents
/// the `bind_mut() failed, already bound` panic that occurs when a Godot
/// engine API re-enters our class while we still hold an exclusive borrow.
pub fn drain_to_console() {
    let Some(receiver_lock) = RECEIVER.get() else {
        return;
    };
    let Ok(receiver) = receiver_lock.lock() else {
        return;
    };
    // `try_iter` yields every pending record without blocking and without
    // disconnecting the sender; once the queue is empty the loop exits.
    for record in receiver.try_iter() {
        let line = format!(
            "{}[{}][{}] {}",
            FORMAT_PREFIX,
            record.tool,
            record.level.as_str(),
            record.msg
        );
        match record.level {
            LogLevel::Info => godot_print!("{}", line),
            LogLevel::Warn => godot_warn!("{}", line),
            LogLevel::Error => godot_error!("{}", line),
        }
    }
}
