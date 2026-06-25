#pragma once

#include "../pipeline/pipeline_runner_base.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>

namespace godot_mcp {
namespace pipeline {

class TestRunner : public PipelineRunnerBase {
public:
    TestRunner(HandlerRegistry *registry);

    Dictionary run_test(const std::shared_ptr<PipelineDef> &pipeline);

protected:
    Dictionary execute_step(const StepDef &step_def,
                             PipelineContext &ctx,
                             CallStats &stats) override;

private:
    struct FileSnapshot {
        godot::PackedStringArray paths;
    };

    FileSnapshot take_snapshot();
    Array track_paths(const Dictionary &result);
    void cleanup(const Array &tracked,
                 const FileSnapshot &before,
                 const FileSnapshot &after,
                 Array &out_deleted,
                 Array &out_skipped);
    void record_setting_before_call(const String &tool_name, const Dictionary &args);
    void restore_settings();
    void backup_project_godot();
    void restore_project_godot();

    Dictionary settings_snapshot_;
    String project_godot_backup_;
};

} // namespace pipeline
} // namespace godot_mcp
