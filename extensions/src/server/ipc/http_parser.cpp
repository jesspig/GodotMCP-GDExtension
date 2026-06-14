#include "http_server.hpp"
#include "logging.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

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
    header_section.parse_utf8((const char *)buf.ptr(), header_end);

    const int first_lf = header_section.find("\r\n");
    if (first_lf < 0) return ERROR_PARSE;
    const String status_line = header_section.substr(0, first_lf);

    const int sp1 = status_line.find(" ");
    if (sp1 < 0) return ERROR_PARSE;
    conn.method = status_line.substr(0, sp1);

    const int sp2 = status_line.find(" ", sp1 + 1);
    if (sp2 < 0) return ERROR_PARSE;
    conn.path = status_line.substr(sp1 + 1, sp2 - sp1 - 1);

    int line_start = first_lf + 2;
    int header_count = 0;
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
            conn.headers[key] = value;
        }
        line_start = line_end + 2;
    }

    auto cl_it = conn.headers.find("content-length");
    if (cl_it != conn.headers.end()) {
        conn.content_length = (int)cl_it->value.to_int();
        if (conn.content_length > kMaxBodyLength) {
            log_warn("http", String("Content-Length ") + String::num_int64(conn.content_length) +
                                 String(" exceeds max ") + String::num_int64(kMaxBodyLength));
            return ERROR_PARSE;
        }
    }

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
    const int to_read = (int)avail < remaining ? (int)avail : remaining;
    const int safe_read = to_read < max_read ? to_read : max_read;
    if (safe_read <= 0) return;

    const Array read_result = conn.tcp->get_data(safe_read);
    if ((Error)(int)read_result[0] != OK) return;

    const PackedByteArray chunk = read_result[1];
    for (int i = 0; i < chunk.size(); ++i) conn.body.push_back(chunk[i]);
    conn.last_activity_msec = Time::get_singleton()->get_ticks_msec();
}

} // namespace godot_mcp
