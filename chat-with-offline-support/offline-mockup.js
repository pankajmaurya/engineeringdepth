import React, { useState, useEffect } from 'react';
import { Send, MessageCircle, User, LogIn, LogOut, Clock, Check, CheckCheck } from 'lucide-react';

// Mock API implementation
class ChatAPI {
  constructor() {
    // In-memory storage (would be database in real app)
    this.users = new Map();
    this.messages = new Map();
    this.onlineUsers = new Set();
    this.messageIdCounter = 1;
  }

  // User registration
  registerUser(userId, password) {
    if (this.users.has(userId)) {
      return { success: false, error: 'User already exists' };
    }
    this.users.set(userId, { password, registered: true });
    return { success: true };
  }

  // User login
  loginUser(userId, password) {
    const user = this.users.get(userId);
    if (!user || user.password !== password) {
      return { success: false, error: 'Invalid credentials' };
    }
    
    // Generate mock JWT token
    const token = `jwt_${userId}_${Date.now()}`;
    this.onlineUsers.add(userId);
    
    // Fetch undelivered messages for this user
    const undeliveredMessages = this.getUndeliveredMessages(userId);
    
    return { 
      success: true, 
      token,
      undeliveredMessages 
    };
  }

  // User logout
  logoutUser(userId) {
    this.onlineUsers.delete(userId);
  }

  // Send message
  sendMessage(fromUserId, toUserId, message) {
    if (!this.users.has(toUserId)) {
      return { success: false, error: 'Recipient does not exist' };
    }

    const messageId = this.messageIdCounter++;
    const messageObj = {
      id: messageId,
      from: fromUserId,
      to: toUserId,
      message,
      timestamp: new Date(),
      status: this.onlineUsers.has(toUserId) ? 'delivered' : 'pending'
    };

    this.messages.set(messageId, messageObj);
    
    return { 
      success: true, 
      messageId,
      delivered: this.onlineUsers.has(toUserId)
    };
  }

  // Poll message status
  getMessageStatus(messageId) {
    const message = this.messages.get(messageId);
    if (!message) {
      return { success: false, error: 'Message not found' };
    }
    
    return { 
      success: true, 
      status: message.status,
      timestamp: message.timestamp
    };
  }

  // Get undelivered messages for a user
  getUndeliveredMessages(userId) {
    const undelivered = [];
    for (const [messageId, message] of this.messages) {
      if (message.to === userId && message.status === 'pending') {
        undelivered.push({
          messageId,
          from: message.from,
          message: message.message,
          timestamp: message.timestamp
        });
        // Mark as delivered
        message.status = 'delivered';
      }
    }
    return undelivered;
  }

  // Check if user is online
  isUserOnline(userId) {
    return this.onlineUsers.has(userId);
  }

  // Get all registered users (for demo purposes)
  getAllUsers() {
    return Array.from(this.users.keys());
  }
}

// Initialize API
const api = new ChatAPI();

// Pre-populate with some test users
api.registerUser('alice', 'password123');
api.registerUser('bob', 'password123');
api.registerUser('charlie', 'password123');

const ChatApp = () => {
  const [currentUser, setCurrentUser] = useState(null);
  const [token, setToken] = useState(null);
  const [loginForm, setLoginForm] = useState({ userId: '', password: '' });
  const [registerForm, setRegisterForm] = useState({ userId: '', password: '' });
  const [messageForm, setMessageForm] = useState({ toUserId: '', message: '' });
  const [sentMessages, setSentMessages] = useState([]);
  const [receivedMessages, setReceivedMessages] = useState([]);
  const [allUsers, setAllUsers] = useState([]);
  const [activeTab, setActiveTab] = useState('login');
  const [notifications, setNotifications] = useState([]);

  useEffect(() => {
    setAllUsers(api.getAllUsers());
  }, []);

  const addNotification = (message, type = 'info') => {
    const id = Date.now();
    setNotifications(prev => [...prev, { id, message, type }]);
    setTimeout(() => {
      setNotifications(prev => prev.filter(n => n.id !== id));
    }, 5000);
  };

  const handleRegister = () => {
    const result = api.registerUser(registerForm.userId, registerForm.password);
    if (result.success) {
      addNotification('Registration successful!', 'success');
      setAllUsers(api.getAllUsers());
      setRegisterForm({ userId: '', password: '' });
      setActiveTab('login');
    } else {
      addNotification(result.error, 'error');
    }
  };

  const handleLogin = () => {
    const result = api.loginUser(loginForm.userId, loginForm.password);
    if (result.success) {
      setCurrentUser(loginForm.userId);
      setToken(result.token);
      setReceivedMessages(result.undeliveredMessages);
      addNotification(`Welcome back, ${loginForm.userId}!`, 'success');
      if (result.undeliveredMessages.length > 0) {
        addNotification(`You have ${result.undeliveredMessages.length} new messages!`, 'info');
      }
    } else {
      addNotification(result.error, 'error');
    }
  };

  const handleLogout = () => {
    api.logoutUser(currentUser);
    setCurrentUser(null);
    setToken(null);
    setSentMessages([]);
    setReceivedMessages([]);
    setLoginForm({ userId: '', password: '' });
    addNotification('Logged out successfully', 'info');
  };

  const handleSendMessage = () => {
    if (!messageForm.toUserId || !messageForm.message) {
      addNotification('Please fill in all fields', 'error');
      return;
    }

    const result = api.sendMessage(currentUser, messageForm.toUserId, messageForm.message);
    if (result.success) {
      const newMessage = {
        id: result.messageId,
        to: messageForm.toUserId,
        message: messageForm.message,
        timestamp: new Date(),
        status: result.delivered ? 'delivered' : 'pending'
      };
      setSentMessages(prev => [...prev, newMessage]);
      setMessageForm({ toUserId: '', message: '' });
      addNotification(
        result.delivered ? 'Message delivered!' : 'Message queued (recipient offline)', 
        result.delivered ? 'success' : 'warning'
      );
    } else {
      addNotification(result.error, 'error');
    }
  };

  const pollMessageStatus = (messageId) => {
    const result = api.getMessageStatus(messageId);
    if (result.success) {
      setSentMessages(prev => 
        prev.map(msg => 
          msg.id === messageId 
            ? { ...msg, status: result.status }
            : msg
        )
      );
    }
  };

  const getStatusIcon = (status) => {
    switch (status) {
      case 'pending':
        return <Clock className="w-4 h-4 text-yellow-500" />;
      case 'delivered':
        return <Check className="w-4 h-4 text-green-500" />;
      case 'read':
        return <CheckCheck className="w-4 h-4 text-blue-500" />;
      default:
        return <Clock className="w-4 h-4 text-gray-400" />;
    }
  };

  if (!currentUser) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-4">
        <div className="max-w-md mx-auto">
          <div className="bg-white rounded-lg shadow-lg p-6">
            <div className="text-center mb-6">
              <MessageCircle className="w-12 h-12 mx-auto text-blue-600 mb-2" />
              <h1 className="text-2xl font-bold text-gray-800">Offline Chat App</h1>
              <p className="text-gray-600">Stay connected even when others are offline</p>
            </div>

            {/* Notifications */}
            {notifications.map(notification => (
              <div
                key={notification.id}
                className={`mb-4 p-3 rounded-lg text-sm ${
                  notification.type === 'success' ? 'bg-green-100 text-green-800' :
                  notification.type === 'error' ? 'bg-red-100 text-red-800' :
                  notification.type === 'warning' ? 'bg-yellow-100 text-yellow-800' :
                  'bg-blue-100 text-blue-800'
                }`}
              >
                {notification.message}
              </div>
            ))}

            <div className="flex mb-4">
              <button
                className={`flex-1 py-2 px-4 rounded-l-lg ${activeTab === 'login' ? 'bg-blue-600 text-white' : 'bg-gray-200'}`}
                onClick={() => setActiveTab('login')}
              >
                Login
              </button>
              <button
                className={`flex-1 py-2 px-4 rounded-r-lg ${activeTab === 'register' ? 'bg-blue-600 text-white' : 'bg-gray-200'}`}
                onClick={() => setActiveTab('register')}
              >
                Register
              </button>
            </div>

            {activeTab === 'login' ? (
              <div className="space-y-4">
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">User ID</label>
                  <input
                    type="text"
                    value={loginForm.userId}
                    onChange={(e) => setLoginForm(prev => ({ ...prev, userId: e.target.value }))}
                    className="w-full p-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    placeholder="Enter your user ID"
                  />
                </div>
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">Password</label>
                  <input
                    type="password"
                    value={loginForm.password}
                    onChange={(e) => setLoginForm(prev => ({ ...prev, password: e.target.value }))}
                    className="w-full p-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    placeholder="Enter your password"
                  />
                </div>
                <button
                  onClick={handleLogin}
                  className="w-full bg-blue-600 text-white py-3 rounded-lg hover:bg-blue-700 transition-colors flex items-center justify-center gap-2"
                >
                  <LogIn className="w-4 h-4" />
                  Login
                </button>
                <div className="text-sm text-gray-600 mt-4">
                  <p className="font-medium mb-1">Demo users (password: password123):</p>
                  <div className="flex flex-wrap gap-1">
                    {allUsers.map(user => (
                      <span key={user} className="bg-gray-100 px-2 py-1 rounded text-xs">
                        {user}
                      </span>
                    ))}
                  </div>
                </div>
              </div>
            ) : (
              <div className="space-y-4">
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">User ID</label>
                  <input
                    type="text"
                    value={registerForm.userId}
                    onChange={(e) => setRegisterForm(prev => ({ ...prev, userId: e.target.value }))}
                    className="w-full p-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    placeholder="Choose a user ID"
                  />
                </div>
                <div>
                  <label className="block text-sm font-medium text-gray-700 mb-1">Password</label>
                  <input
                    type="password"
                    value={registerForm.password}
                    onChange={(e) => setRegisterForm(prev => ({ ...prev, password: e.target.value }))}
                    className="w-full p-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                    placeholder="Choose a password"
                  />
                </div>
                <button
                  onClick={handleRegister}
                  className="w-full bg-green-600 text-white py-3 rounded-lg hover:bg-green-700 transition-colors flex items-center justify-center gap-2"
                >
                  <User className="w-4 h-4" />
                  Register
                </button>
              </div>
            )}
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-4">
      <div className="max-w-4xl mx-auto">
        {/* Header */}
        <div className="bg-white rounded-lg shadow-lg p-4 mb-6">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-3">
              <MessageCircle className="w-8 h-8 text-blue-600" />
              <div>
                <h1 className="text-xl font-bold text-gray-800">Offline Chat</h1>
                <p className="text-sm text-gray-600">Logged in as {currentUser}</p>
              </div>
            </div>
            <button
              onClick={handleLogout}
              className="flex items-center gap-2 bg-red-100 text-red-700 px-4 py-2 rounded-lg hover:bg-red-200 transition-colors"
            >
              <LogOut className="w-4 h-4" />
              Logout
            </button>
          </div>
        </div>

        {/* Notifications */}
        {notifications.map(notification => (
          <div
            key={notification.id}
            className={`mb-4 p-3 rounded-lg text-sm ${
              notification.type === 'success' ? 'bg-green-100 text-green-800' :
              notification.type === 'error' ? 'bg-red-100 text-red-800' :
              notification.type === 'warning' ? 'bg-yellow-100 text-yellow-800' :
              'bg-blue-100 text-blue-800'
            }`}
          >
            {notification.message}
          </div>
        ))}

        <div className="grid md:grid-cols-2 gap-6">
          {/* Send Message */}
          <div className="bg-white rounded-lg shadow-lg p-6">
            <h2 className="text-lg font-semibold text-gray-800 mb-4">Send Message</h2>
            <div className="space-y-4">
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">To User</label>
                <select
                  value={messageForm.toUserId}
                  onChange={(e) => setMessageForm(prev => ({ ...prev, toUserId: e.target.value }))}
                  className="w-full p-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                >
                  <option value="">Select recipient</option>
                  {allUsers.filter(user => user !== currentUser).map(user => (
                    <option key={user} value={user}>
                      {user} {api.isUserOnline(user) ? '(online)' : '(offline)'}
                    </option>
                  ))}
                </select>
              </div>
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-1">Message</label>
                <textarea
                  value={messageForm.message}
                  onChange={(e) => setMessageForm(prev => ({ ...prev, message: e.target.value }))}
                  className="w-full p-3 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                  rows="3"
                  placeholder="Type your message..."
                />
              </div>
              <button
                onClick={handleSendMessage}
                className="w-full bg-blue-600 text-white py-3 rounded-lg hover:bg-blue-700 transition-colors flex items-center justify-center gap-2"
              >
                <Send className="w-4 h-4" />
                Send Message
              </button>
            </div>
          </div>

          {/* Online Users */}
          <div className="bg-white rounded-lg shadow-lg p-6">
            <h2 className="text-lg font-semibold text-gray-800 mb-4">User Status</h2>
            <div className="space-y-2">
              {allUsers.map(user => (
                <div key={user} className="flex items-center justify-between p-2 bg-gray-50 rounded">
                  <span className="font-medium">{user}</span>
                  <span className={`px-2 py-1 rounded-full text-xs ${
                    api.isUserOnline(user) 
                      ? 'bg-green-100 text-green-800' 
                      : 'bg-gray-100 text-gray-600'
                  }`}>
                    {api.isUserOnline(user) ? 'Online' : 'Offline'}
                  </span>
                </div>
              ))}
            </div>
          </div>
        </div>

        <div className="grid md:grid-cols-2 gap-6 mt-6">
          {/* Sent Messages */}
          <div className="bg-white rounded-lg shadow-lg p-6">
            <h2 className="text-lg font-semibold text-gray-800 mb-4">Sent Messages</h2>
            <div className="space-y-3 max-h-64 overflow-y-auto">
              {sentMessages.length === 0 ? (
                <p className="text-gray-500 text-sm">No messages sent yet</p>
              ) : (
                sentMessages.map(message => (
                  <div key={message.id} className="border-l-4 border-blue-500 pl-4 py-2">
                    <div className="flex items-center justify-between">
                      <span className="font-medium text-sm">To: {message.to}</span>
                      <div className="flex items-center gap-2">
                        {getStatusIcon(message.status)}
                        <button
                          onClick={() => pollMessageStatus(message.id)}
                          className="text-xs text-blue-600 hover:underline"
                        >
                          Refresh
                        </button>
                      </div>
                    </div>
                    <p className="text-gray-800 mt-1">{message.message}</p>
                    <p className="text-xs text-gray-500 mt-1">
                      {message.timestamp.toLocaleTimeString()} • {message.status}
                    </p>
                  </div>
                ))
              )}
            </div>
          </div>

          {/* Received Messages */}
          <div className="bg-white rounded-lg shadow-lg p-6">
            <h2 className="text-lg font-semibold text-gray-800 mb-4">Received Messages</h2>
            <div className="space-y-3 max-h-64 overflow-y-auto">
              {receivedMessages.length === 0 ? (
                <p className="text-gray-500 text-sm">No messages received</p>
              ) : (
                receivedMessages.map(message => (
                  <div key={message.messageId} className="border-l-4 border-green-500 pl-4 py-2">
                    <div className="flex items-center justify-between">
                      <span className="font-medium text-sm">From: {message.from}</span>
                      <span className="text-xs text-green-600">New</span>
                    </div>
                    <p className="text-gray-800 mt-1">{message.message}</p>
                    <p className="text-xs text-gray-500 mt-1">
                      {new Date(message.timestamp).toLocaleTimeString()}
                    </p>
                  </div>
                ))
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default ChatApp;
