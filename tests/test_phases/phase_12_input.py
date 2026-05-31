import json

from .base import ToolTest, TestResult

TEST_ACTION = "test_mcp_action"



async def test_list_input_actions(ctx) -> TestResult:
    data = await ctx.session.call_tool("list_input_actions", {})
    parsed = _parse(data)
    return TestResult("list_input_actions", "PASS" if parsed else "FAIL",
                      expected="list of input actions", actual=parsed)


async def test_add_input_action(ctx) -> TestResult:
    data = await ctx.session.call_tool("add_input_action", {
        "name": TEST_ACTION,
        "deadzone": 0.5,
    })
    parsed = _parse(data)
    ctx.test_input_actions.append(TEST_ACTION)
    return TestResult("add_input_action",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected=f"action '{TEST_ACTION}' added", actual=parsed)


async def test_set_input_action_events(ctx) -> TestResult:
    events = [
        {"type": "key", "keycode": "KEY_SPACE", "physical_keycode": 32, "ctrl": False,
         "alt": False, "shift": False, "meta": False}
    ]
    data = await ctx.session.call_tool("set_input_action_events", {
        "name": TEST_ACTION,
        "mode": "replace",
        "events": events,
    })
    parsed = _parse(data)

    data2 = await ctx.session.call_tool("list_input_actions", {})
    parsed2 = _parse(data2)
    return TestResult("set_input_action_events",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected="events set", actual=parsed2)


async def test_remove_input_action(ctx) -> TestResult:
    data = await ctx.session.call_tool("remove_input_action", {
        "name": TEST_ACTION,
    })
    parsed = _parse(data)
    if TEST_ACTION in ctx.test_input_actions:
        ctx.test_input_actions.remove(TEST_ACTION)
    return TestResult("remove_input_action",
                      "PASS" if not parsed.get("error") else "FAIL",
                      expected=f"action '{TEST_ACTION}' removed", actual=parsed)


def _parse(result):

    content = result.content if hasattr(result, "content") else []
    for c in content:
        text = getattr(c, "text", str(c))
        if text and (text.startswith("{") or text.startswith("[")):
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                pass
        return {"_raw": text}
    return {}


TOOL_TESTS = [
    ToolTest("list_input_actions", run=test_list_input_actions),
    ToolTest("add_input_action", run=test_add_input_action),
    ToolTest("set_input_action_events", run=test_set_input_action_events),
    ToolTest("remove_input_action", run=test_remove_input_action),
]