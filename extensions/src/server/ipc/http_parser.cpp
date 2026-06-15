#include "http_server.hpp"
#include "logging.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <charconv>
#include <system_error>

using namespace godot;

namespace godot_mcp {

HttpServer::ParseResult HttpServer::parse_headers(Connection &conn) {
    const Vector<uint8_t> &buf = conn.read_buf;

    int header_end = -1;
    for (int i = 0; i + 3 < buf.size(); ++i) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            header_end = i;
            break;
        }
    }
    if (header_end < 0) return NEED_MORE;

    conn.header_end_pos = header_end + 4;

    String header_section;
    {
        Error utf8_err = header_section.parse_utf8((const char *)buf.ptr(), header_end);
        if (utf8_err != OK) {
            return ERROR_PARSE;
        }
    }

    const int first_lf = header_section.find("\r\n");
    if (first_lf < 0) return ERROR_PARSE;
    const String status_line = header_section.substr(0, first_lf);

    const int sp1 = status_line.find(" ");
    if (sp1 < 0) return ERROR_PARSE;
    conn.method = status_line.substr(0, sp1);

    const int sp2 = status_line.find(" ", sp1 + 1);
    if (sp2 < 0) return ERROR_PARSE;
    conn.path = status_line.substr(sp1 + 1, sp2 - sp1 - 1);
    int qpos = conn.path.find("?");
    if (qpos != -1) conn.path = conn.path.substr(0, qpos);

    int line_start = first_lf + 2;
    int header_count = 0;
    bool seen_content_length = false;
    while (line_start < header_end) {
        const int line_end = header_section.find("\r\n", line_start);
        if (line_end < 0) break;
        const String line = header_section.substr(line_start, line_end - line_start);
        const int colon = line.find(":");
        if (colon > 0) {
            if (++header_count > kMaxHeaders) {
                log_warn("http", "Too many headers (> " + String::num_int64(kMaxHeaders) + ")");
                return ERROR_PARSE;
            }
            String key = line.substr(0, colon).to_lower().strip_edges();
            String value = line.substr(colon + 1).strip_edges();
            if (key == "content-length") {
                if (seen_content_length) {
                    log_warn("http", "Duplicate Content-Length header rejected");
                    return ERROR_PARSE;
                }
                seen_content_length = true;
            }
            conn.headers[key] = value;
        }
        line_start = line_end + 2;
    }

    auto cl_it = conn.headers.find("content-length");
    if (cl_it != conn.headers.end()) {
        // Parse Content-Length with strict bounds validation. The previous
        // `static_cast<int>(to_int())` truncated an attacker-controlled int64
        // header (e.g. 2147483648 -> negative), which silently bypassed the
        // kMaxBodyLength cap below. Use std::from_chars on the raw UTF-8 bytes
        // and reject anything negative or out of range *before* narrowing to int.
        const CharString cl_utf8 = cl_it->value.utf8();
        int64_t cl_value = 0;
        const auto [ptr, ec] = std::from_chars(cl_utf8.ptr(), cl_utf8.ptr() + cl_utf8.length(), cl_value);
        if (ec != std::errc{} || ptr != cl_utf8.ptr() + cl_utf8.length() ||
            cl_value < 0 || cl_value > kMaxBodyLength) {
            log_warn("http", String("Invalid or oversized Content-Length header"));
            return ERROR_PARSE;
        }
        conn.content_length = static_cast<int>(cl_value);
    }

    // Infer body length from remaining buffer (RFC 7230 non-compliant but works for single-request scenarios)
    if (conn.content_length <= 0 && conn.header_end_pos < conn.read_buf.size()) {
        conn.content_length = conn.read_buf.size() - conn.header_end_pos;
        if (conn.content_length > kMaxBodyLength) {
            log_warn("http", String("Body without Content-Length exceeds max ") +
                                 String::num_int64(kMaxBodyLength));
            return ERROR_PARSE;
        }
    }

    auto c_it = conn.headers.find("connection");
    if (c_it != conn.headers.end() && c_it->value.to_lower().find("close") != -1) {
        conn.keep_alive = false;
    }

    auto sid_it = conn.headers.find("mcp-session-id");
    if (sid_it != conn.headers.end()) {
        conn.session_id = sid_it->value;
    }

    conn.headers_done = true;
    return COMPLETE;
}

void HttpServer::try_read_body(Connection &conn) {
    if (conn.content_length <= 0) return;
    const int remaining = conn.content_length - conn.body.size();
    if (remaining <= 0) return;

    if (conn.body.size() >= kMaxBodyLength) return;

    conn.tcp->poll();
    const int64_t avail = conn.tcp->get_available_bytes();
    if (avail <= 0) return;

    const int max_read = kMaxBodyLength - conn.body.size();
    const int to_read = static_cast<int>(avail) < remaining ? static_cast<int>(avail) : remaining;
    const int safe_read = to_read < max_read ? to_read : max_read;
    if (safe_read <= 0) return;

    const Array read_result = conn.tcp->get_data(safe_read);
    if (static_cast<Error>(static_cast<int>(read_result[0])) != OK) return;

    const PackedByteArray chunk = read_result[1];
    for (int i = 0; i < chunk.size(); ++i) conn.body.push_back(chunk[i]);
    conn.read_buf.clear();
    conn.last_activity_msec = Time::get_singleton()->get_ticks_msec();
}

} // namespace godot_mcp
