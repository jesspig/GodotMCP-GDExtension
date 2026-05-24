use serde_json::Value;
use std::collections::VecDeque;
use std::sync::{Arc, Mutex};
use tokio::sync::oneshot;

struct DispatcherJob {
    closure: Box<dyn FnOnce() -> Value + Send>,
    response: oneshot::Sender<Value>,
}

pub struct MainThreadDispatcher {
    queue: Arc<Mutex<VecDeque<DispatcherJob>>>,
}

impl Clone for MainThreadDispatcher {
    fn clone(&self) -> Self {
        Self {
            queue: self.queue.clone(),
        }
    }
}

impl MainThreadDispatcher {
    pub fn new() -> Self {
        Self {
            queue: Arc::new(Mutex::new(VecDeque::new())),
        }
    }

    pub async fn submit<F>(&self, f: F) -> Value
    where
        F: FnOnce() -> Value + Send + 'static,
    {
        let (tx, rx) = oneshot::channel();
        let job = DispatcherJob {
            closure: Box::new(f),
            response: tx,
        };
        self.queue.lock().unwrap().push_back(job);
        rx.await.unwrap_or(Value::Null)
    }

    pub fn process_pending(&self) {
        let jobs: VecDeque<DispatcherJob> = {
            let mut guard = self.queue.lock().unwrap();
            std::mem::take(&mut *guard)
        };

        for job in jobs {
            let result = (job.closure)();
            let _ = job.response.send(result);
        }
    }
}

impl Default for MainThreadDispatcher {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[tokio::test]
    async fn submit_and_process_returns_value() {
        let dispatcher = MainThreadDispatcher::new();

        let dispatcher_clone = dispatcher.clone();
        let handle =
            tokio::spawn(
                async move { dispatcher_clone.submit(|| json!({ "result": "ok" })).await },
            );

        tokio::task::yield_now().await;
        dispatcher.process_pending();

        let result = handle.await.unwrap();
        assert_eq!(result, json!({ "result": "ok" }));
    }

    #[tokio::test]
    async fn multiple_jobs_processed_in_order() {
        let dispatcher = MainThreadDispatcher::new();

        let d1 = dispatcher.clone();
        let h1 = tokio::spawn(async move { d1.submit(|| json!("first")).await });

        let d2 = dispatcher.clone();
        let h2 = tokio::spawn(async move { d2.submit(|| json!("second")).await });

        let d3 = dispatcher.clone();
        let h3 = tokio::spawn(async move { d3.submit(|| json!("third")).await });

        tokio::task::yield_now().await;
        tokio::task::yield_now().await;
        tokio::task::yield_now().await;
        dispatcher.process_pending();

        assert_eq!(h1.await.unwrap(), json!("first"));
        assert_eq!(h2.await.unwrap(), json!("second"));
        assert_eq!(h3.await.unwrap(), json!("third"));
    }

    #[tokio::test]
    async fn process_empty_queue_is_noop() {
        let dispatcher = MainThreadDispatcher::new();
        dispatcher.process_pending();
    }

    #[tokio::test]
    async fn submit_captures_owned_data() {
        let dispatcher = MainThreadDispatcher::new();
        let name = "test_node".to_string();

        let d = dispatcher.clone();
        let handle = tokio::spawn(async move { d.submit(move || json!({ "name": name })).await });

        tokio::task::yield_now().await;
        dispatcher.process_pending();

        let result = handle.await.unwrap();
        assert_eq!(result["name"], "test_node");
    }
}
