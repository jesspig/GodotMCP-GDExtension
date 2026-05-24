use serde_json::{Value, json};

use godot::classes::{ClassDb, Node};
use godot::meta::ToGodot;
use godot::obj::{NewGd, Singleton};
use godot::prelude::{StringName, Variant};

use super::{get_root, get_undo_redo, pipe, resolve_node, s};
use crate::dispatcher::MainThreadDispatcher;

pub const TOOL_NAMES: &[&str] = &["add_circle_collision", "add_rectangle_collision"];

pub struct CollisionCommands;

impl CollisionCommands {
    pub fn new() -> Self {
        Self
    }
}

impl Default for CollisionCommands {
    fn default() -> Self {
        Self::new()
    }
}

impl super::CommandHandler for CollisionCommands {
    fn can_handle(&self, tool: &str) -> bool {
        TOOL_NAMES.contains(&tool)
    }
    fn execute(&self, _args: &Value, _d: &MainThreadDispatcher) -> Result<Value, String> {
        Err("CollisionCommands::execute should not be called directly".into())
    }
    fn group_name(&self) -> &str {
        "collision"
    }
    fn tool_names(&self) -> &[&str] {
        TOOL_NAMES
    }
}

impl CollisionCommands {
    pub async fn handle_collision_tool(
        &self,
        tool: &str,
        args: &Value,
        d: &MainThreadDispatcher,
    ) -> Result<Value, String> {
        let a = args.clone();
        match tool {
            "add_circle_collision" => pipe(d.submit(move || cmd_add_circle_collision(&a)).await),
            "add_rectangle_collision" => {
                pipe(d.submit(move || cmd_add_rectangle_collision(&a)).await)
            }
            _ => Err(format!("Unknown collision tool: {}", tool)),
        }
    }
}

fn cmd_add_circle_collision(args: &Value) -> Value {
    let p = s(args, "node_path");
    let radius = args["radius"].as_f64().unwrap_or(10.0) as f32;
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let target = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };

    let mut circle: godot::obj::Gd<godot::classes::CircleShape2D> =
        godot::classes::CircleShape2D::new_gd();
    circle.set_radius(radius);

    if target.get_class() == "CollisionShape2D" {
        let old_shape = target.get("shape");
        let mut cs = target;
        cs.set("shape", &Variant::from(circle.clone()));
        let mut ur = get_undo_redo();
        ur.create_action(&format!("Set circle collision for {}", p));
        ur.add_do_property(
            &cs.clone(),
            &StringName::from("shape"),
            &Variant::from(circle),
        );
        ur.add_undo_property(&cs.clone(), &StringName::from("shape"), &old_shape);
        ur.commit_action_ex().execute(false).done();
        return json!({"node_path": p, "radius": radius, "shape": "CircleShape2D", "mode": "set_on_existing"});
    }

    // Check if a CollisionShape2D child already exists.
    let child_count = target.get_child_count();
    for i in 0..child_count {
        if let Some(child) = target.get_child(i)
            && child.get_class() == "CollisionShape2D"
        {
            let old_shape = child.get("shape");
            let mut cs = child;
            cs.set("shape", &Variant::from(circle.clone()));
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Set circle collision for {}", p));
            ur.add_do_property(
                &cs.clone(),
                &StringName::from("shape"),
                &Variant::from(circle),
            );
            ur.add_undo_property(&cs.clone(), &StringName::from("shape"), &old_shape);
            ur.commit_action_ex().execute(false).done();
            return json!({"node_path": p, "radius": radius, "shape": "CircleShape2D", "mode": "set_on_existing"});
        }
    }

    let parent = target;
    let mut shape_node: godot::obj::Gd<Node> = ClassDb::singleton()
        .instantiate(&StringName::from("CollisionShape2D"))
        .try_to()
        .unwrap();
    shape_node.set_name(&StringName::from("CollisionShape2D"));
    shape_node.set("shape", &Variant::from(circle));

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Add circle collision to {}", p));
    ur.add_do_method(
        &parent.clone(),
        &StringName::from("add_child"),
        &[shape_node.to_variant()],
    );
    ur.add_do_method(
        &shape_node.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_do_reference(&shape_node.clone());
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("remove_child"),
        &[shape_node.to_variant()],
    );
    ur.commit_action();

    json!({"node_path": p, "radius": radius, "shape": "CircleShape2D", "mode": "created_child"})
}

fn cmd_add_rectangle_collision(args: &Value) -> Value {
    let p = s(args, "node_path");
    let width = args["width"].as_f64().unwrap_or(20.0) as f32;
    let height = args["height"].as_f64().unwrap_or(100.0) as f32;
    let root = match get_root() {
        Ok(r) => r,
        Err(e) => return e,
    };
    let target = match resolve_node(&root, p.as_str()) {
        Some(n) => n,
        None => return json!({"error": format!("Node not found: {}", p)}),
    };

    let mut rect: godot::obj::Gd<godot::classes::RectangleShape2D> =
        godot::classes::RectangleShape2D::new_gd();
    rect.set_size(godot::builtin::Vector2::new(width, height));

    if target.get_class() == "CollisionShape2D" {
        let old_shape = target.get("shape");
        let mut cs = target;
        cs.set("shape", &Variant::from(rect.clone()));
        let mut ur = get_undo_redo();
        ur.create_action(&format!("Set rectangle collision for {}", p));
        ur.add_do_property(
            &cs.clone(),
            &StringName::from("shape"),
            &Variant::from(rect),
        );
        ur.add_undo_property(&cs.clone(), &StringName::from("shape"), &old_shape);
        ur.commit_action_ex().execute(false).done();
        return json!({
            "node_path": p,
            "width": width,
            "height": height,
            "shape": "RectangleShape2D",
            "mode": "set_on_existing"
        });
    }

    // Check if a CollisionShape2D child already exists.
    let child_count = target.get_child_count();
    for i in 0..child_count {
        if let Some(child) = target.get_child(i)
            && child.get_class() == "CollisionShape2D"
        {
            let old_shape = child.get("shape");
            let mut cs = child;
            cs.set("shape", &Variant::from(rect.clone()));
            let mut ur = get_undo_redo();
            ur.create_action(&format!("Set rectangle collision for {}", p));
            ur.add_do_property(
                &cs.clone(),
                &StringName::from("shape"),
                &Variant::from(rect),
            );
            ur.add_undo_property(&cs.clone(), &StringName::from("shape"), &old_shape);
            ur.commit_action_ex().execute(false).done();
            return json!({
                "node_path": p,
                "width": width,
                "height": height,
                "shape": "RectangleShape2D",
                "mode": "set_on_existing"
            });
        }
    }

    let parent = target;
    let mut shape_node: godot::obj::Gd<Node> = ClassDb::singleton()
        .instantiate(&StringName::from("CollisionShape2D"))
        .try_to()
        .unwrap();
    shape_node.set_name(&StringName::from("CollisionShape2D"));
    shape_node.set("shape", &Variant::from(rect));

    let mut ur = get_undo_redo();
    ur.create_action(&format!("Add rectangle collision to {}", p));
    ur.add_do_method(
        &parent.clone(),
        &StringName::from("add_child"),
        &[shape_node.to_variant()],
    );
    ur.add_do_method(
        &shape_node.clone(),
        &StringName::from("set_owner"),
        &[root.to_variant()],
    );
    ur.add_do_reference(&shape_node.clone());
    ur.add_undo_method(
        &parent.clone(),
        &StringName::from("remove_child"),
        &[shape_node.to_variant()],
    );
    ur.commit_action();

    json!({
        "node_path": p,
        "width": width,
        "height": height,
        "shape": "RectangleShape2D",
        "mode": "created_child"
    })
}
