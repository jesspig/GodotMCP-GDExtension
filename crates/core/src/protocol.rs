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

fn default_args() -> Value {
    Value::Object(serde_json::Map::new())
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolCallParams {
    pub tool: String,
    #[serde(default = "default_args")]
    pub args: Value,
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn test_ipc_request_roundtrip() {
        let req = IpcRequest {
            id: "test-uuid".into(),
            method: "ping".into(),
            params: json!({}),
        };
        let json_str = serde_json::to_string(&req).unwrap();
        let de: IpcRequest = serde_json::from_str(&json_str).unwrap();
        assert_eq!(de.id, "test-uuid");
        assert_eq!(de.method, "ping");
        assert_eq!(de.params, json!({}));
    }

    #[test]
    fn test_ipc_response_success() {
        let resp = IpcResponse {
            id: "abc".into(),
            result: IpcResult::Success {
                data: json!({ "message": "pong" }),
            },
        };
        let json_str = serde_json::to_string(&resp).unwrap();
        assert!(json_str.contains("\"status\":\"ok\""));
        assert!(json_str.contains("\"message\":\"pong\""));

        let de: IpcResponse = serde_json::from_str(&json_str).unwrap();
        assert_eq!(de.id, "abc");
        match de.result {
            IpcResult::Success { data } => {
                assert_eq!(data["message"], "pong");
            }
            IpcResult::Error { .. } => panic!("expected Success"),
        }
    }

    #[test]
    fn test_ipc_response_error() {
        let resp = IpcResponse {
            id: "xyz".into(),
            result: IpcResult::Error {
                code: -2,
                message: "Unknown method".into(),
            },
        };
        let json_str = serde_json::to_string(&resp).unwrap();
        assert!(json_str.contains("\"status\":\"error\""));
        assert!(json_str.contains("\"code\":-2"));

        let de: IpcResponse = serde_json::from_str(&json_str).unwrap();
        match de.result {
            IpcResult::Error { code, message } => {
                assert_eq!(code, -2);
                assert_eq!(message, "Unknown method");
            }
            IpcResult::Success { .. } => panic!("expected Error"),
        }
    }

    #[test]
    fn test_ipc_result_deserialize_from_json() {
        let ok_json = r#"{"id":"1","status":"ok","data":{"value":42}}"#;
        let resp: IpcResponse = serde_json::from_str(ok_json).unwrap();
        match resp.result {
            IpcResult::Success { data } => assert_eq!(data["value"], 42),
            IpcResult::Error { .. } => panic!("expected Success"),
        }

        let err_json = r#"{"id":"2","status":"error","code":-1,"message":"fail"}"#;
        let resp: IpcResponse = serde_json::from_str(err_json).unwrap();
        match resp.result {
            IpcResult::Error { code, message } => {
                assert_eq!(code, -1);
                assert_eq!(message, "fail");
            }
            IpcResult::Success { .. } => panic!("expected Error"),
        }
    }

    #[test]
    fn test_ipc_notification_roundtrip() {
        let notif = IpcNotification {
            msg_type: "notification".into(),
            event: "godot_ready".into(),
            data: json!({ "engine_version": "4.5.1" }),
        };
        let json_str = serde_json::to_string(&notif).unwrap();
        assert!(json_str.contains("\"type\":\"notification\""));
        assert!(!json_str.contains("\"msg_type\""));

        let de: IpcNotification = serde_json::from_str(&json_str).unwrap();
        assert_eq!(de.msg_type, "notification");
        assert_eq!(de.event, "godot_ready");
        assert_eq!(de.data["engine_version"], "4.5.1");
    }

    #[test]
    fn test_ipc_request_empty_params() {
        let json_str = r#"{"id":"a","method":"ping","params":{}}"#;
        let req: IpcRequest = serde_json::from_str(json_str).unwrap();
        assert_eq!(req.params, json!({}));
    }

    #[test]
    fn test_ipc_request_nested_params() {
        let json_str =
            r#"{"id":"b","method":"tool_invoke","params":{"tool":"create_node","args":{"x":1}}}"#;
        let req: IpcRequest = serde_json::from_str(json_str).unwrap();
        assert_eq!(req.params["tool"], "create_node");
        assert_eq!(req.params["args"]["x"], 1);
    }

    #[test]
    fn test_tool_call_params_roundtrip() {
        let params = ToolCallParams {
            tool: "create_node".into(),
            args: json!({ "parent_path": "/root", "node_type": "Node2D", "name": "Player" }),
        };
        let json_str = serde_json::to_string(&params).unwrap();
        let de: ToolCallParams = serde_json::from_str(&json_str).unwrap();
        assert_eq!(de.tool, "create_node");
        assert_eq!(de.args["parent_path"], "/root");
    }

    #[test]
    fn test_tool_call_params_empty_args() {
        let params = ToolCallParams {
            tool: "ping".into(),
            args: json!({}),
        };
        let json_str = serde_json::to_string(&params).unwrap();
        let de: ToolCallParams = serde_json::from_str(&json_str).unwrap();
        assert_eq!(de.tool, "ping");
        assert_eq!(de.args, json!({}));
    }

    #[test]
    fn test_tool_call_params_default_args() {
        let json_str = r#"{"tool":"get_scene_tree"}"#;
        let de: ToolCallParams = serde_json::from_str(json_str).unwrap();
        assert_eq!(de.tool, "get_scene_tree");
        assert_eq!(de.args, json!({}));
    }
}
