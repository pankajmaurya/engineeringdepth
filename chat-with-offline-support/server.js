const express = require('express');
const jwt = require('jsonwebtoken');
const bcrypt = require('bcrypt');
const cors = require('cors');
const { v4: uuidv4 } = require('uuid');

const app = express();
const PORT = process.env.PORT || 3000;
const JWT_SECRET = 'your-secret-key-change-in-production';

// Middleware
app.use(express.json());
app.use(cors());

// In-memory storage (use database in production)
const users = new Map(); // userId -> { password: hashedPassword, createdAt: Date }
const messages = new Map(); // messageId -> { id, from, to, message, timestamp, status }
const onlineUsers = new Set(); // Set of currently online user IDs
const userSessions = new Map(); // userId -> { token, lastSeen }

// Message status enum
const MessageStatus = {
  PENDING: 'pending',
  DELIVERED: 'delivered',
  READ: 'read'
};

// Utility function to verify JWT token
const verifyToken = (req, res, next) => {
  const authHeader = req.headers.authorization;
  if (!authHeader) {
    return res.status(401).json({ success: false, error: 'No token provided' });
  }

  const token = authHeader.startsWith('Bearer ') ? authHeader.slice(7) : authHeader;
  
  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    req.userId = decoded.userId;
    
    // Update last seen
    if (userSessions.has(decoded.userId)) {
      userSessions.get(decoded.userId).lastSeen = new Date();
    }
    
    next();
  } catch (error) {
    return res.status(401).json({ success: false, error: 'Invalid token' });
  }
};

// API Routes

/**
 * POST /api/register
 * Body: { user_id, password }
 * Returns: { success: boolean, error?: string }
 */
app.post('/api/register', async (req, res) => {
  try {
    const { user_id, password } = req.body;
    
    if (!user_id || !password) {
      return res.status(400).json({ 
        success: false, 
        error: 'user_id and password are required' 
      });
    }
    
    if (users.has(user_id)) {
      return res.status(400).json({ 
        success: false, 
        error: 'User already exists' 
      });
    }
    
    // Hash password
    const hashedPassword = await bcrypt.hash(password, 10);
    
    // Store user
    users.set(user_id, {
      password: hashedPassword,
      createdAt: new Date()
    });
    
    console.log(`User registered: ${user_id}`);
    res.json({ success: true });
    
  } catch (error) {
    console.error('Registration error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

/**
 * POST /api/login
 * Body: { user_id, password }
 * Returns: { success: boolean, token?: string, undelivered_messages?: array, error?: string }
 */
app.post('/api/login', async (req, res) => {
  try {
    const { user_id, password } = req.body;
    
    if (!user_id || !password) {
      return res.status(400).json({ 
        success: false, 
        error: 'user_id and password are required' 
      });
    }
    
    const user = users.get(user_id);
    if (!user) {
      return res.status(401).json({ 
        success: false, 
        error: 'Invalid credentials' 
      });
    }
    
    // Verify password
    const isValidPassword = await bcrypt.compare(password, user.password);
    if (!isValidPassword) {
      return res.status(401).json({ 
        success: false, 
        error: 'Invalid credentials' 
      });
    }
    
    // Generate JWT token
    const token = jwt.sign({ userId: user_id }, JWT_SECRET, { expiresIn: '24h' });
    
    // Mark user as online
    onlineUsers.add(user_id);
    userSessions.set(user_id, { token, lastSeen: new Date() });
    
    // Get undelivered messages
    const undeliveredMessages = [];
    for (const [messageId, message] of messages) {
      if (message.to === user_id && message.status === MessageStatus.PENDING) {
        undeliveredMessages.push({
          message_id: messageId,
          from: message.from,
          message: message.message,
          timestamp: message.timestamp
        });
        // Mark as delivered
        message.status = MessageStatus.DELIVERED;
      }
    }
    
    console.log(`User logged in: ${user_id}, undelivered messages: ${undeliveredMessages.length}`);
    
    res.json({ 
      success: true, 
      token,
      undelivered_messages: undeliveredMessages
    });
    
  } catch (error) {
    console.error('Login error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

/**
 * POST /api/logout
 * Headers: Authorization: Bearer <token>
 * Returns: { success: boolean }
 */
app.post('/api/logout', verifyToken, (req, res) => {
  try {
    onlineUsers.delete(req.userId);
    userSessions.delete(req.userId);
    
    console.log(`User logged out: ${req.userId}`);
    res.json({ success: true });
    
  } catch (error) {
    console.error('Logout error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

/**
 * POST /api/send-message
 * Headers: Authorization: Bearer <token>
 * Body: { to_user_id, message }
 * Returns: { success: boolean, message_id?: string, delivered?: boolean, error?: string }
 */
app.post('/api/send-message', verifyToken, (req, res) => {
  try {
    const { to_user_id, message } = req.body;
    
    if (!to_user_id || !message) {
      return res.status(400).json({ 
        success: false, 
        error: 'to_user_id and message are required' 
      });
    }
    
    // Check if recipient exists
    if (!users.has(to_user_id)) {
      return res.status(400).json({ 
        success: false, 
        error: 'Recipient does not exist' 
      });
    }
    
    // Create message
    const messageId = uuidv4();
    const isRecipientOnline = onlineUsers.has(to_user_id);
    
    const messageObj = {
      id: messageId,
      from: req.userId,
      to: to_user_id,
      message,
      timestamp: new Date(),
      status: isRecipientOnline ? MessageStatus.DELIVERED : MessageStatus.PENDING
    };
    
    messages.set(messageId, messageObj);
    
    console.log(`Message sent from ${req.userId} to ${to_user_id} (${messageObj.status}): ${message}`);
    
    res.json({ 
      success: true, 
      message_id: messageId,
      delivered: isRecipientOnline
    });
    
  } catch (error) {
    console.error('Send message error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

/**
 * GET /api/message-status/:messageId
 * Headers: Authorization: Bearer <token>
 * Returns: { success: boolean, status?: string, timestamp?: Date, error?: string }
 */
app.get('/api/message-status/:messageId', verifyToken, (req, res) => {
  try {
    const { messageId } = req.params;
    
    const message = messages.get(messageId);
    if (!message) {
      return res.status(404).json({ 
        success: false, 
        error: 'Message not found' 
      });
    }
    
    // Check if user is authorized to view this message status
    if (message.from !== req.userId) {
      return res.status(403).json({ 
        success: false, 
        error: 'Unauthorized to view this message status' 
      });
    }
    
    res.json({ 
      success: true, 
      status: message.status,
      timestamp: message.timestamp
    });
    
  } catch (error) {
    console.error('Message status error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

/**
 * GET /api/users
 * Headers: Authorization: Bearer <token>
 * Returns: { success: boolean, users: array }
 */
app.get('/api/users', verifyToken, (req, res) => {
  try {
    const userList = Array.from(users.keys()).map(userId => ({
      user_id: userId,
      online: onlineUsers.has(userId)
    }));
    
    res.json({ 
      success: true, 
      users: userList 
    });
    
  } catch (error) {
    console.error('Get users error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

/**
 * GET /api/messages/received
 * Headers: Authorization: Bearer <token>
 * Returns: { success: boolean, messages: array }
 */
app.get('/api/messages/received', verifyToken, (req, res) => {
  try {
    const receivedMessages = [];
    
    for (const [messageId, message] of messages) {
      if (message.to === req.userId) {
        receivedMessages.push({
          message_id: messageId,
          from: message.from,
          message: message.message,
          timestamp: message.timestamp,
          status: message.status
        });
      }
    }
    
    // Sort by timestamp (newest first)
    receivedMessages.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));
    
    res.json({ 
      success: true, 
      messages: receivedMessages 
    });
    
  } catch (error) {
    console.error('Get received messages error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

/**
 * GET /api/messages/sent
 * Headers: Authorization: Bearer <token>
 * Returns: { success: boolean, messages: array }
 */
app.get('/api/messages/sent', verifyToken, (req, res) => {
  try {
    const sentMessages = [];
    
    for (const [messageId, message] of messages) {
      if (message.from === req.userId) {
        sentMessages.push({
          message_id: messageId,
          to: message.to,
          message: message.message,
          timestamp: message.timestamp,
          status: message.status
        });
      }
    }
    
    // Sort by timestamp (newest first)
    sentMessages.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));
    
    res.json({ 
      success: true, 
      messages: sentMessages 
    });
    
  } catch (error) {
    console.error('Get sent messages error:', error);
    res.status(500).json({ success: false, error: 'Internal server error' });
  }
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ 
    status: 'OK', 
    timestamp: new Date(),
    online_users: onlineUsers.size,
    total_users: users.size,
    total_messages: messages.size
  });
});

// Error handling middleware
app.use((error, req, res, next) => {
  console.error('Unhandled error:', error);
  res.status(500).json({ success: false, error: 'Internal server error' });
});

// 404 handler
app.use((req, res) => {
  res.status(404).json({ success: false, error: 'Endpoint not found' });
});

// Clean up offline users periodically (remove users inactive for more than 1 hour)
setInterval(() => {
  const oneHourAgo = new Date(Date.now() - 60 * 60 * 1000);
  
  for (const [userId, session] of userSessions) {
    if (session.lastSeen < oneHourAgo) {
      onlineUsers.delete(userId);
      userSessions.delete(userId);
      console.log(`Cleaned up inactive user: ${userId}`);
    }
  }
}, 5 * 60 * 1000); // Run every 5 minutes

// Start server
app.listen(PORT, () => {
  console.log(`Chat server running on port ${PORT}`);
  console.log(`Health check: http://localhost:${PORT}/health`);
  console.log('\nAPI Endpoints:');
  console.log('POST /api/register - Register new user');
  console.log('POST /api/login - User login');
  console.log('POST /api/logout - User logout');
  console.log('POST /api/send-message - Send message');
  console.log('GET /api/message-status/:messageId - Get message status');
  console.log('GET /api/users - Get all users');
  console.log('GET /api/messages/received - Get received messages');
  console.log('GET /api/messages/sent - Get sent messages');
});

module.exports = app;
