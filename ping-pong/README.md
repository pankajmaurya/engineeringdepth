# Compile pingpong program
```bash
gcc -o pingpong_stopbillion pingpong_1Mround_madebuggy.c
```

# Load up the visualizer_ws.html in your browser

# start ping server and pipe out websocket server
```bash
./pingpong_stopbillion ping | node websocket-server.js
```

# connect visualizer to websocket server
Use the UI.

# Start pong server
```bash
./pingpong_stopbillion pong
```

