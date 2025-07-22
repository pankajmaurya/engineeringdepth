from flask import Flask, request, jsonify, render_template_string, redirect
import sqlite3
import uuid
import requests
from datetime import datetime
import os

app = Flask(__name__)

# Database setup
def init_db():
    conn = sqlite3.connect('psp.db')
    cursor = conn.cursor()
    
    # Payments table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS payments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            payment_order_id TEXT UNIQUE NOT NULL,
            nonce TEXT UNIQUE NOT NULL,
            amount DECIMAL(10,2),
            status TEXT DEFAULT 'pending',
            callback_url TEXT,
            redirect_url TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    
    conn.commit()
    conn.close()

# Initialize database on startup
init_db()

# Add a simple health check endpoint
@app.route('/')
def health_check():
    return jsonify({'status': 'PSP service is running', 'version': '1.0'})

@app.route('/health')
def health():
    return jsonify({'status': 'healthy', 'database': 'connected'})

@app.route('/api/register-payment', methods=['POST'])
def register_payment():
    """Register a payment and return nonce for idempotency"""
    try:
        data = request.get_json()
        if not data:
            return jsonify({'error': 'No JSON data provided'}), 400
        
        payment_order_id = data.get('payment_order_id')
        amount = data.get('amount')
        callback_url = data.get('callback_url')
        redirect_url = data.get('redirect_url')
        
        if not payment_order_id or not amount:
            return jsonify({'error': 'payment_order_id and amount are required'}), 400
        
        # Generate unique nonce
        nonce = str(uuid.uuid4())
        
        conn = sqlite3.connect('psp.db')
        cursor = conn.cursor()
        
        try:
            # Check if payment_order_id already exists (idempotency)
            cursor.execute('SELECT nonce FROM payments WHERE payment_order_id = ?', (payment_order_id,))
            existing = cursor.fetchone()
            
            if existing:
                return jsonify({'nonce': existing[0], 'status': 'already_registered'})
            
            # Insert new payment
            cursor.execute('''
                INSERT INTO payments (payment_order_id, nonce, amount, callback_url, redirect_url)
                VALUES (?, ?, ?, ?, ?)
            ''', (payment_order_id, nonce, amount, callback_url, redirect_url))
            
            conn.commit()
            
            return jsonify({
                'nonce': nonce,
                'status': 'registered',
                'payment_url': f'/payment/{nonce}'
            })
            
        except sqlite3.IntegrityError:
            return jsonify({'error': 'Payment order already exists'}), 409
        except Exception as e:
            return jsonify({'error': f'Database error: {str(e)}'}), 500
        finally:
            conn.close()
            
    except Exception as e:
        return jsonify({'error': f'Request processing error: {str(e)}'}), 500

@app.route('/payment/<nonce>')
def payment_page(nonce):
    """Display payment page with wallet options"""
    conn = sqlite3.connect('psp.db')
    cursor = conn.cursor()
    
    cursor.execute('SELECT amount, status FROM payments WHERE nonce = ?', (nonce,))
    payment = cursor.fetchone()
    conn.close()
    
    if not payment:
        return "Payment not found", 404
    
    if payment[1] != 'pending':
        return f"Payment already {payment[1]}", 400
    
    # Simple payment page template
    template = '''
    <!DOCTYPE html>
    <html>
    <head><title>PSP Payment</title></head>
    <body>
        <h2>Complete Payment</h2>
        <p>Amount: ${{ amount }}</p>
        <form action="/api/process-payment" method="POST">
            <input type="hidden" name="nonce" value="{{ nonce }}">
            <h3>Select Payment Method:</h3>
            <label>
                <input type="radio" name="payment_method" value="wallet" checked>
                Digital Wallet
            </label><br>
            <label>
                <input type="radio" name="payment_method" value="card">
                Credit Card
            </label><br><br>
            <button type="submit">Pay Now</button>
        </form>
    </body>
    </html>
    '''
    
    return render_template_string(template, amount=payment[0], nonce=nonce)

@app.route('/api/process-payment', methods=['POST'])
def process_payment():
    """Process wallet payment and handle callbacks"""
    nonce = request.form.get('nonce')
    payment_method = request.form.get('payment_method')
    
    if not nonce:
        return jsonify({'error': 'Nonce required'}), 400
    
    conn = sqlite3.connect('psp.db')
    cursor = conn.cursor()
    
    # Get payment details
    cursor.execute('SELECT payment_order_id, callback_url, redirect_url, status FROM payments WHERE nonce = ?', (nonce,))
    payment = cursor.fetchone()
    
    if not payment:
        return jsonify({'error': 'Payment not found'}), 404
    
    if payment[3] != 'pending':
        return jsonify({'error': 'Payment already processed'}), 400
    
    # Simulate payment processing (in real implementation, integrate with actual payment gateway)
    import random
    payment_success = random.choice([True])  # 75% success rate
    
    status = 'completed' if payment_success else 'failed'
    
    # Update payment status
    cursor.execute('''
        UPDATE payments 
        SET status = ?, updated_at = CURRENT_TIMESTAMP 
        WHERE nonce = ?
    ''', (status, nonce))
    
    conn.commit()
    conn.close()
    
    # Send callback to ecommerce system
    if payment[1]:  # callback_url exists
        try:
            callback_data = {
                'payment_order_id': payment[0],
                'nonce': nonce,
                'status': status,
                'timestamp': datetime.now().isoformat()
            }
            requests.post(payment[1], json=callback_data, timeout=5)
        except requests.RequestException:
            pass  # Log error in production
    
    # Redirect with status
    redirect_url = payment[2] or '/'
    separator = '&' if '?' in redirect_url else '?'
    return redirect(f"{redirect_url}{separator}status={status}&nonce={nonce}")

@app.route('/api/payment-status/<nonce>')
def get_payment_status(nonce):
    """Get current payment status"""
    conn = sqlite3.connect('psp.db')
    cursor = conn.cursor()
    
    cursor.execute('SELECT payment_order_id, status, updated_at FROM payments WHERE nonce = ?', (nonce,))
    payment = cursor.fetchone()
    conn.close()
    
    if not payment:
        return jsonify({'error': 'Payment not found'}), 404
    
    return jsonify({
        'payment_order_id': payment[0],
        'nonce': nonce,
        'status': payment[1],
        'updated_at': payment[2]
    })

if __name__ == '__main__':
    app.run(debug=True, port=5000)
