pub mod meta;

use serde_json::Value;

use crate::dispatcher::MainThreadDispatcher;

pub trait CommandHandler: Send + Sync {
    fn can_handle(&self, tool: &str) -> bool;
    fn execute(&self, args: &Value, dispatcher: &MainThreadDispatcher) -> Result<Value, String>;
    fn group_name(&self) -> &str;
    fn tool_names(&self) -> &[&str];
}

pub fn create_registry() -> Vec<Box<dyn CommandHandler>> {
    vec![Box::new(meta::MetaCommands::new())]
}
