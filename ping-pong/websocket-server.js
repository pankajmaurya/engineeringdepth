const WebSocket = require('ws');
const fs = require('fs');
const readline = require('readline');

// WebSocket server
const wss = new WebSocket.Server({ port: 8080 });

console.log('WebSocket server started on ws://localhost:8080');

// Track connected clients
let clients = new Set();

wss.on('connection', (ws) => {
    console.log('Client connected');
    clients.add(ws);
    
    // Send welcome message
    ws.send('Connected to Ping-Pong Performance Monitor');
    
    ws.on('close', () => {
        console.log('Client disconnected');
        clients.delete(ws);
    });
    
    ws.on('error', (error) => {
        console.error('WebSocket error:', error);
        clients.delete(ws);
    });
});

// Function to broadcast message to all clients
function broadcast(message) {
    clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(message);
        }
    });
}

// Option 1: Read from stdin (pipe your ping-pong output)
// Usage: your_pingpong_app | node server.js
process.stdin.on('data', (data) => {
    const lines = data.toString().split('\n');
    lines.forEach(line => {
        if (line.trim()) {
            broadcast(line.trim());
        }
    });
});

// Option 2: Watch a log file
// Uncomment this section if you want to watch a file instead
/*
const logFile = 'pingpong.log'; // Change to your log file path

if (fs.existsSync(logFile)) {
    const rl = readline.createInterface({
        input: fs.createReadStream(logFile),
        crlfDelay: Infinity
    });
    
    rl.on('line', (line) => {
        broadcast(line);
    });
    
    // Watch for new lines added to the file
    fs.watchFile(logFile, (curr, prev) => {
        if (curr.size > prev.size) {
            const stream = fs.createReadStream(logFile, { start: prev.size });
            const rl = readline.createInterface({
                input: stream,
                crlfDelay: Infinity
            });
            
            rl.on('line', (line) => {
                broadcast(line);
            });
        }
    });
}
*/

// Option 3: HTTP endpoint to receive data
// POST to http://localhost:8080/data with your log lines
const http = require('http');
const url = require('url');

const server = http.createServer((req, res) => {
    const parsedUrl = url.parse(req.url, true);
    
    if (req.method === 'POST' && parsedUrl.pathname === '/data') {
        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });
        
        req.on('end', () => {
            const lines = body.split('\n');
            lines.forEach(line => {
                if (line.trim()) {
                    broadcast(line.trim());
                }
            });
            
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ status: 'success', lines: lines.length }));
        });
    } else {
        res.writeHead(404);
        res.end('Not found');
    }
});

server.listen(8081, () => {
    console.log('HTTP server listening on http://localhost:8081');
    console.log('Send POST requests to http://localhost:8081/data');
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\nShutting down servers...');
    wss.close();
    server.close();
    process.exit(0);
});
