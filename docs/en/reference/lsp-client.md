# LSP Validation Client

> Uses Godot's built-in `StreamPeerTCP` to connect to the GDScript LSP server for syntax validation.

## Validation Flow

1. **Connect**: `StreamPeerTCP::connect("127.0.0.1", 6005)`, 2-second timeout
2. **Initialize**: Send `initialize` request and wait for response
3. **Validate**: Send `textDocument/didOpen` (with source code), read `publishDiagnostics`
4. **Cleanup**: Send `shutdown` + `exit`

## Key Details

- **Transport**: `StreamPeerTCP` (Godot's built-in TCP client)
- **LSP Frame Format**: HTTP-style `Content-Length: xxx\r\n\r\n<body>`
- **Port**: `127.0.0.1:6005` (Godot LSP default port)
- **Sync**: All I/O blocks on the main thread

## Notes

- The Language Server must be enabled in Godot Editor Settings (`Editor → Network → Language Server → Enable`)
- If the LSP server is not running, the connection will time out and return an error, falling back to basic compilation checks
- Only GDScript validation is supported (no C# validation)
