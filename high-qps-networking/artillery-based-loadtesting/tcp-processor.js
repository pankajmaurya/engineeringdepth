// tcp-processor.js
// Custom processor for TCP socket testing with Artillery

const net = require('net');

module.exports = {
  generateTcpMessage,
  validateTcpEcho,
  customTcpTest
};

function generateTcpMessage(context, ee, next) {
  const timestamp = Date.now();
  const userId = context.vars.$uuid || Math.random().toString(36).substr(2, 9);
  
  context.vars.message = `TCP Load test from Artillery user ${userId} at ${timestamp}`;
  context.vars.originalMessage = context.vars.message;
  context.vars.startTime = timestamp;
  
  return next();
}

function validateTcpEcho(context, ee, next) {
  const originalMessage = context.vars.originalMessage;
  const response = context.vars.$response;
  
  if (response && response.includes(originalMessage)) {
    ee.emit('counter', 'tcp.echo.success', 1);
    
    if (context.vars.startTime) {
      const responseTime = Date.now() - context.vars.startTime;
      ee.emit('histogram', 'tcp.response_time', responseTime);
    }
  } else {
    ee.emit('counter', 'tcp.echo.failed', 1);
  }
  
  return next();
}

// Custom TCP test function using raw Node.js sockets
function customTcpTest(context, ee, next) {
  const host = '192.168.29.145';
  const port = 8888;
  const message = `Custom TCP test ${Date.now()}`;
  
  const startTime = Date.now();
  const client = new net.Socket();
  
  client.setTimeout(5000); // 5 second timeout
  
  client.connect(port, host, () => {
    client.write(message);
  });
  
  client.on('data', (data) => {
    const response = data.toString();
    const responseTime = Date.now() - startTime;
    
    if (response.includes(message)) {
      ee.emit('counter', 'custom.tcp.success', 1);
      ee.emit('histogram', 'custom.tcp.response_time', responseTime);
    } else {
      ee.emit('counter', 'custom.tcp.failed', 1);
    }
    
    client.destroy();
    return next();
  });
  
  client.on('error', (err) => {
    ee.emit('counter', 'custom.tcp.error', 1);
    console.error('TCP Error:', err.message);
    client.destroy();
    return next();
  });
  
  client.on('timeout', () => {
    ee.emit('counter', 'custom.tcp.timeout', 1);
    client.destroy();
    return next();
  });
}
