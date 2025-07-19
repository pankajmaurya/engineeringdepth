# Chat Messaging Server

A Node.js server implementing offline chat messaging functionality with REST API endpoints. Messages are queued when users are offline and delivered when they come online.

## Features

- ✅ User registration and authentication with JWT tokens
- ✅ Send messages to online/offline users
- ✅ Message queueing for offline users
- ✅ Message status tracking (pending/delivered/read)
- ✅ Message history (sent/received)
- ✅ User online/offline status
- ✅ Secure password hashing with bcrypt
- ✅ Token-based authentication
- ✅ Automatic cleanup of inactive sessions

## Quick Start

### Prerequisites
- Node.js (v14 or higher)
- npm or yarn

### Installation

1. **Install dependencies:**
```bash
npm install
```

2. **Start the server:**
```bash
npm start
```

Or for development with auto-restart:
```bash
npm run dev
```

3. **Test the server:**
```bash
node test-client.js
```

The server will run on `http://localhost:3000` by default.

## API Endpoints

### Authentication

#### Register User
```http
POST /api/register
Content-Type: application/json

{
  "user_id": "alice",
  "password": "password123"
}
```

**Response:**
```json
{
  "success": true
}
```

#### Login User
```http
POST /api/login
Content-Type: application/json

{
  "user_id": "alice",
  "password": "password123"
}
```

**Response:**
```json
{
  "success": true,
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "undelivered_messages": [
    {
      "message_id": "uuid-here",
      "from": "bob",
      "message": "Hello Alice!",
      "timestamp": "2024-01-01T12:00:00.000Z"
    }
  ]
}
```

#### Logout User
```http
POST /api/logout
Authorization: Bearer <token>
```

### Messaging

#### Send Message
```http
POST /api/send-message
Authorization: Bearer <token>
Content-Type: application/json

{
  "to_user_id": "bob",
  "message": "Hello Bob!"
}
```

**Response:**
```json
{
  "success": true,
  "message_id": "uuid-here",
  "delivered": true
}
```

#### Get Message Status
```http
GET /api/message-status/:messageId
Authorization: Bearer <token>
```

**Response:**
```json
{
  "success": true,
  "status": "delivered",
  "timestamp": "2024-01-01T12:00:00.000Z"
}
```

### User Management

#### Get All Users
```http
GET /api/users
Authorization: Bearer <token>
```

**Response:**
```json
{
  "success": true,
  "users": [
    {
      "user_id": "alice",
      "online": true
    },
    {
      "user_id": "bob",
      "online": false
    }
  ]
}
```

### Message History

#### Get Received Messages
```http
GET /api/messages/received
Authorization: Bearer <token>
```

#### Get Sent Messages
```http
GET /api/messages/sent
Authorization: Bearer <token>
```

### Health Check
```http
GET /health
```

## Message Status Types

- **pending**: Message sent to offline user, waiting for delivery
- **delivered**: Message delivered to recipient
- **read**: Message read by recipient (future enhancement)

## Example Usage

### Using cURL

1. **Register a user:**
```bash
curl -X POST http://localhost:3000/api/register \
  -H "Content-Type: application/json" \
  -d '{"user_id": "alice", "password": "password123"}'
```

2. **Login:**
```bash
curl -X POST http://localhost:3000/api/login \
  -H "Content-Type: application/json" \
  -d '{"user_id": "alice", "password": "password123"}'
```

3. **Send message:**
```bash
curl -X POST http://localhost:3000/api/send-message \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_TOKEN_HERE" \
  -d '{"to_user_id": "bob", "message": "Hello Bob!"}'
```

### Using the Test Client

The included test client demonstrates all API functionality:

```bash
node test-client.js
```

This will:
1. Register three users (alice, bob, charlie)
2. Demonstrate offline messaging
3. Show message delivery when users come online
4. Display message history and status

## Project Structure

```
chat-messaging-server/
├── server.js           # Main server file
├── test-client.js      # API test client
├── package.json        # Dependencies
└── README.md          # This file
```

## Security Features

- ✅ Password hashing with bcrypt
- ✅ JWT token authentication
- ✅ Token expiration (24 hours)
- ✅ Input validation
- ✅ CORS
