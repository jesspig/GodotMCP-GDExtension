#pragma once

#include "built_in/cmd_utils/schema_builder.hpp"
#include "built_in/tool_base.hpp"
#include "pipeline/workflow_parser.hpp"
#include "pipeline/workflow_runner.hpp"
#include "pipeline/yaml_parser.hpp"
#include "server/registry/handler_registry.hpp"

namespace godot_mcp {

class ExecuteWorkflowTool : public ITool {
    HandlerRegistry *registry_ = nullptr;
public:
    void set_registry(HandlerRegistry *reg) override { registry_ = reg; }

    String name() const noexcept override { return "execute_workflow"; }
    String category() const noexcept override { return "meta_tools"; }
    String brief() const noexcept override { return String("Execute a multi-step workflow"); }
    String description() const override {
        return String("Executes a sequence of tool calls defined as a workflow. "
                      "Accepts either workflow_json (object) or workflow_yaml (string). "
                      "Supports ${vars.*} for variable substitution and "
                      "${steps.<id>.result.<path>} for step result chaining. "
                      "Use dry_run=true to validate without executing.");
    }
    Dictionary build_input_schema() const override {
        return SchemaBuilder()
            .prop("workflow_json", "object", "Workflow definition as JSON object (stages, steps, vars, timeout)")
            .prop("workflow_yaml", "string", "Workflow definition as YAML string")
            .prop("dry_run", "boolean", "Validate workflow without executing", false)
            .build();
    }
    bool is_meta() const noexcept override { return true; }

protected:
    Dictionary execute_impl(const ToolContext &ctx) override {
        if (!registry_) {
            return ToolResult::err("NO_REGISTRY", "HandlerRegistry not available");
        }

        Dictionary input;
        bool has_json = ctx.args.has("workflow_json");
        bool has_yaml = ctx.args.has("workflow_yaml");

        if (!has_json && !has_yaml) {
            return ToolResult::err("MISSING_INPUT",
                "Provide either workflow_json or workflow_yaml");
        }

        pipeline::WorkflowParseResult parsed;
        if (has_json) {
            parsed = pipeline::WorkflowParser::parse(ctx.args["workflow_json"]);
        } else {
            parsed = pipeline::WorkflowParser::parse_yaml(ctx.args["workflow_yaml"]);
        }

        if (parsed.error.has_value()) {
            return ToolResult::err("PARSE_ERROR", parsed.error->message);
        }

        bool dry_run = ctx.args.get("dry_run", false);
        if (dry_run) {
            Dictionary data;
            data["valid"] = true;
            data["total_steps"] = 0;
            int count = 0;
            for (const auto &stage : parsed.pipeline->stages) {
                count += static_cast<int>(stage.steps.size());
            }
            data["total_steps"] = count;
            return ToolResult::ok(data);
        }

        pipeline::WorkflowRunner runner(registry_);
        Dictionary result = runner.run_workflow(
            *parsed.pipeline,
            parsed.vars,
            parsed.timeout_ms,
            parsed.max_steps);

        if (result.has("error")) {
            return ToolResult::err("WORKFLOW_ERROR", String(result["error"]));
        }

        return ToolResult::ok(result);
    }
};

} // namespace godot_mcp
