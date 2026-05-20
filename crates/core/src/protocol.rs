use serde::{Deserialize, Serialize};
use serde_json::Value;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct IpcRequest {
    pub id: String,
    pub method: String,
    pub params: Value,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct IpcResponse {
    pub id: String,
    #[serde(flatten)]
    pub result: IpcResult,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "status")]
pub enum IpcResult {
    #[serde(rename = "ok")]
    Success { data: Value },
    #[serde(rename = "error")]
    Error { code: i32, message: String },
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct IpcNotification {
    #[serde(rename = "type")]
    pub msg_type: String,
    pub event: String,
    pub data: Value,
}
