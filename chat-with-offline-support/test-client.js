const axios = require('axios');

const API_BASE = 'http://localhost:3000/api';

class ChatClient {
  constructor(baseUrl = API_BASE) {
    this.baseUrl = baseUrl;
    this.token = null;
    this.userId = null;
  }

  async register(userId, password) {
    try {
      const response = await axios.post(`${this.baseUrl}/register`, {
        user_id: userId,
        password: password
      });
      
      console.log(`✅ Registration successful for ${userId}:`, response.data);
      return response.data;
    } catch (error) {
      console.log(`❌ Registration failed for ${userId}:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }

  async login(userId, password) {
    try {
      const response = await axios.post(`${this.baseUrl}/login`, {
        user_id: userId,
        password: password
      });
      
      if (response.data.success) {
        this.token = response.data.token;
        this.userId = userId;
        console.log(`✅ Login successful for ${userId}`);
        console.log(`📨 Undelivered messages: ${response.data.undelivered_messages.length}`);
        
        if (response.data.undelivered_messages.length > 0) {
          response.data.undelivered_messages.forEach(msg => {
            console.log(`  📩 From ${msg.from}: ${msg.message} (${new Date(msg.timestamp).toLocaleString()})`);
          });
        }
      }
      
      return response.data;
    } catch (error) {
      console.log(`❌ Login failed for ${userId}:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }

  async logout() {
    try {
      const response = await axios.post(`${this.baseUrl}/logout`, {}, {
        headers: { Authorization: `Bearer ${this.token}` }
      });
      
      console.log(`✅ Logout successful for ${this.userId}`);
      this.token = null;
      this.userId = null;
      return response.data;
    } catch (error) {
      console.log(`❌ Logout failed:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }

  async sendMessage(toUserId, message) {
    try {
      const response = await axios.post(`${this.baseUrl}/send-message`, {
        to_user_id: toUserId,
        message: message
      }, {
        headers: { Authorization: `Bearer ${this.token}` }
      });
      
      console.log(`✅ Message sent to ${toUserId}: "${message}"`);
      console.log(`📊 Status: ${response.data.delivered ? 'Delivered' : 'Pending'} (ID: ${response.data.message_id})`);
      
      return response.data;
    } catch (error) {
      console.log(`❌ Send message failed:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }

  async getMessageStatus(messageId) {
    try {
      const response = await axios.get(`${this.baseUrl}/message-status/${messageId}`, {
        headers: { Authorization: `Bearer ${this.token}` }
      });
      
      console.log(`📊 Message ${messageId} status: ${response.data.status}`);
      return response.data;
    } catch (error) {
      console.log(`❌ Get message status failed:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }

  async getUsers() {
    try {
      const response = await axios.get(`${this.baseUrl}/users`, {
        headers: { Authorization: `Bearer ${this.token}` }
      });
      
      console.log(`👥 Users:`);
      response.data.users.forEach(user => {
        console.log(`  ${user.online ? '🟢' : '🔴'} ${user.user_id}`);
      });
      
      return response.data;
    } catch (error) {
      console.log(`❌ Get users failed:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }

  async getReceivedMessages() {
    try {
      const response = await axios.get(`${this.baseUrl}/messages/received`, {
        headers: { Authorization: `Bearer ${this.token}` }
      });
      
      console.log(`📨 Received messages (${response.data.messages.length}):`);
      response.data.messages.forEach(msg => {
        console.log(`  📩 From ${msg.from}: "${msg.message}" (${new Date(msg.timestamp).toLocaleString()})`);
      });
      
      return response.data;
    } catch (error) {
      console.log(`❌ Get received messages failed:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }

  async getSentMessages() {
    try {
      const response = await axios.get(`${this.baseUrl}/messages/sent`, {
        headers: { Authorization: `Bearer ${this.token}` }
      });
      
      console.log(`📤 Sent messages (${response.data.messages.length}):`);
      response.data.messages.forEach(msg => {
        console.log(`  📤 To ${msg.to}: "${msg.message}" [${msg.status}] (${new Date(msg.timestamp).toLocaleString()})`);
      });
      
      return response.data;
    } catch (error) {
      console.log(`❌ Get sent messages failed:`, error.response?.data || error.message);
      return error.response?.data;
    }
  }
}

// Test function to demonstrate the API
async function runDemo() {
  console.log('🚀 Starting Chat API Demo\n');

  const alice = new ChatClient();
  const bob = new ChatClient();
  const charlie = new ChatClient();

  try {
    // Step 1: Register users
    console.log('📝 Step 1: Registering users...');
    await alice.register('alice', 'password123');
    await bob.register('bob', 'password123');
    await charlie.register('charlie', 'password123');
    console.log('');

    // Step 2: Alice logs in and sends message to offline Bob
    console.log('🔐 Step 2: Alice logs in and sends message to offline Bob...');
    await alice.login('alice', 'password123');
    await alice.sendMessage('bob', 'Hello Bob! How are you?');
    await alice.sendMessage('charlie', 'Hi Charlie, are you there?');
    console.log('');

    // Step 3: Check users status
    console.log('👥 Step 3: Checking user status...');
    await alice.getUsers();
    console.log('');

    // Step 4: Bob logs in (should receive pending message)
    console.log('🔐 Step 4: Bob logs in (should receive pending messages)...');
    await bob.login('bob', 'password123');
    console.log('');

    // Step 5: Bob sends reply to Alice (both online now)
    console.log('💬 Step 5: Bob sends reply to Alice...');
    await bob.sendMessage('alice', 'Hi Alice! I\'m doing great, thanks for asking!');
    console.log('');

    // Step 6: Check message history
    console.log('📋 Step 6: Checking message history...');
    console.log('\n--- Alice\'s messages ---');
    await alice.getSentMessages();
    await alice.getReceivedMessages();
    
    console.log('\n--- Bob\'s messages ---');
    await bob.getSentMessages();
    await bob.getReceivedMessages();
    console.log('');

    // Step 7: Charlie logs in late
    console.log('⏰ Step 7: Charlie logs in late...');
    await charlie.login('charlie', 'password123');
    await charlie.getReceivedMessages();
    console.log('');

    // Step 8: Test message status polling
    console.log('📊 Step 8: Testing message status polling...');
    const messageResult = await alice.sendMessage('bob', 'Testing status polling');
    if (messageResult.success) {
      await alice.getMessageStatus(messageResult.message_id);
    }
    console.log('');

    // Step 9: Logout users
    console.log('👋 Step 9: Logging out users...');
    await alice.logout();
    await bob.logout();
    await charlie.logout();

  } catch (error) {
    console.error('Demo failed:', error.message);
  }
}

// Health check function
async function checkHealth() {
  try {
    const response = await axios.get('http://localhost:3000/health');
    console.log('🏥 Server Health:', response.data);
  } catch (error) {
    console.log('❌ Server health check failed:', error.message);
  }
}

// Run the demo if this file is executed directly
if (require.main === module) {
  console.log('Checking server health first...\n');
  checkHealth().then(() => {
    console.log('\nStarting demo in 2 seconds...\n');
    setTimeout(runDemo, 2000);
  });
}

module.exports = ChatClient;
