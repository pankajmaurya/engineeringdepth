from flask import Flask, request, jsonify, render_template_string, redirect, url_for, session
import sqlite3
import uuid
import requests
from datetime import datetime
import json

app = Flask(__name__)
app.secret_key = 'ecommerce_secret_key_change_in_production'

# PSP Configuration
PSP_BASE_URL = 'http://127.0.0.1:5000'
ECOMMERCE_BASE_URL = 'http://localhost:5001'

# Database setup
def init_db():
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    
    # Products table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            price DECIMAL(10,2) NOT NULL,
            description TEXT,
            stock INTEGER DEFAULT 0
        )
    ''')
    
    # Orders table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id TEXT UNIQUE NOT NULL,
            customer_email TEXT,
            total_amount DECIMAL(10,2),
            status TEXT DEFAULT 'pending',
            psp_nonce TEXT,
            payment_status TEXT DEFAULT 'pending',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    
    # Order items table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS order_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id TEXT,
            product_id INTEGER,
            product_name TEXT,
            price DECIMAL(10,2),
            quantity INTEGER,
            FOREIGN KEY (order_id) REFERENCES orders (order_id)
        )
    ''')
    
    # Insert sample products
    cursor.execute('SELECT COUNT(*) FROM products')
    if cursor.fetchone()[0] == 0:
        sample_products = [
            ('Laptop', 999.99, 'High-performance laptop', 10),
            ('Smartphone', 599.99, 'Latest smartphone model', 25),
            ('Headphones', 149.99, 'Wireless noise-canceling headphones', 50),
            ('Tablet', 399.99, '10-inch tablet with stylus', 15),
            ('Smart Watch', 299.99, 'Fitness tracking smartwatch', 30)
        ]
        cursor.executemany('INSERT INTO products (name, price, description, stock) VALUES (?, ?, ?, ?)', sample_products)
    
    conn.commit()
    conn.close()

init_db()

@app.route('/')
def home():
    """Home page with product catalog"""
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    cursor.execute('SELECT id, name, price, description, stock FROM products WHERE stock > 0')
    products = cursor.fetchall()
    conn.close()
    
    template = '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Ecommerce Store</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; }
            .product { border: 1px solid #ddd; padding: 20px; margin: 10px 0; border-radius: 5px; }
            .cart-info { background: #f0f0f0; padding: 10px; margin: 20px 0; border-radius: 5px; }
            button { background: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 3px; cursor: pointer; }
            button:hover { background: #0056b3; }
            .cart-button { background: #28a745; }
            .checkout-button { background: #fd7e14; font-size: 16px; padding: 15px 25px; }
        </style>
    </head>
    <body>
        <h1>Ecommerce Store</h1>
        
        <div class="cart-info">
            <strong>Cart: {{ cart_count }} items | Total: ${{ "%.2f"|format(cart_total) }}</strong>
            {% if cart_count > 0 %}
                <a href="/cart"><button class="cart-button">View Cart</button></a>
                <a href="/checkout"><button class="checkout-button">Checkout</button></a>
            {% endif %}
        </div>
        
        <h2>Products</h2>
        {% for product in products %}
        <div class="product">
            <h3>{{ product[1] }} - ${{ "%.2f"|format(product[2]) }}</h3>
            <p>{{ product[3] }}</p>
            <p>Stock: {{ product[4] }}</p>
            <form action="/add-to-cart" method="POST" style="display: inline;">
                <input type="hidden" name="product_id" value="{{ product[0] }}">
                <input type="number" name="quantity" value="1" min="1" max="{{ product[4] }}" style="width: 60px;">
                <button type="submit">Add to Cart</button>
            </form>
        </div>
        {% endfor %}
        
        <div style="margin-top: 40px;">
            <a href="/admin">Admin Panel</a> | 
            <a href="/orders">View Orders</a>
        </div>
    </body>
    </html>
    '''
    
    # Calculate cart totals
    cart = session.get('cart', {})
    cart_count = sum(cart.values()) if cart else 0
    cart_total = calculate_cart_total(cart)
    
    return render_template_string(template, products=products, cart_count=cart_count, cart_total=cart_total)

def calculate_cart_total(cart):
    """Calculate total price for items in cart"""
    if not cart:
        return 0.0
    
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    
    total = 0.0
    for product_id, quantity in cart.items():
        cursor.execute('SELECT price FROM products WHERE id = ?', (product_id,))
        result = cursor.fetchone()
        if result:
            total += result[0] * quantity
    
    conn.close()
    return total

@app.route('/add-to-cart', methods=['POST'])
def add_to_cart():
    """Add product to cart"""
    product_id = request.form.get('product_id')
    quantity = int(request.form.get('quantity', 1))
    
    if 'cart' not in session:
        session['cart'] = {}
    
    if product_id in session['cart']:
        session['cart'][product_id] += quantity
    else:
        session['cart'][product_id] = quantity
    
    session.modified = True
    return redirect('/')

@app.route('/cart')
def view_cart():
    """View cart contents"""
    cart = session.get('cart', {})
    if not cart:
        return redirect('/')
    
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    
    cart_items = []
    total = 0.0
    
    for product_id, quantity in cart.items():
        cursor.execute('SELECT id, name, price FROM products WHERE id = ?', (product_id,))
        product = cursor.fetchone()
        if product:
            item_total = product[2] * quantity
            cart_items.append({
                'id': product[0],
                'name': product[1],
                'price': product[2],
                'quantity': quantity,
                'total': item_total
            })
            total += item_total
    
    conn.close()
    
    template = '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Shopping Cart</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; }
            table { width: 100%; border-collapse: collapse; }
            th, td { padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }
            th { background-color: #f2f2f2; }
            .total-row { font-weight: bold; background-color: #f9f9f9; }
            button { background: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 3px; cursor: pointer; margin: 5px; }
            .checkout-btn { background: #28a745; font-size: 16px; padding: 15px 25px; }
            .clear-btn { background: #dc3545; }
        </style>
    </head>
    <body>
        <h1>Shopping Cart</h1>
        
        <table>
            <tr>
                <th>Product</th>
                <th>Price</th>
                <th>Quantity</th>
                <th>Total</th>
            </tr>
            {% for item in cart_items %}
            <tr>
                <td>{{ item.name }}</td>
                <td>${{ "%.2f"|format(item.price) }}</td>
                <td>{{ item.quantity }}</td>
                <td>${{ "%.2f"|format(item.total) }}</td>
            </tr>
            {% endfor %}
            <tr class="total-row">
                <td colspan="3">Total</td>
                <td>${{ "%.2f"|format(total) }}</td>
            </tr>
        </table>
        
        <div style="margin-top: 20px;">
            <a href="/"><button>Continue Shopping</button></a>
            <a href="/checkout"><button class="checkout-btn">Proceed to Checkout</button></a>
            <a href="/clear-cart"><button class="clear-btn">Clear Cart</button></a>
        </div>
    </body>
    </html>
    '''
    
    return render_template_string(template, cart_items=cart_items, total=total)

@app.route('/clear-cart')
def clear_cart():
    """Clear the shopping cart"""
    session.pop('cart', None)
    return redirect('/')

@app.route('/checkout')
def checkout():
    """Checkout page"""
    cart = session.get('cart', {})
    if not cart:
        return redirect('/')
    
    total = calculate_cart_total(cart)
    
    template = '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Checkout</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; }
            .form-group { margin: 15px 0; }
            label { display: block; margin-bottom: 5px; font-weight: bold; }
            input[type="text"], input[type="email"] { width: 300px; padding: 8px; border: 1px solid #ddd; border-radius: 3px; }
            button { background: #28a745; color: white; padding: 15px 25px; border: none; border-radius: 3px; cursor: pointer; font-size: 16px; }
            .order-summary { background: #f8f9fa; padding: 20px; border-radius: 5px; margin: 20px 0; }
        </style>
    </head>
    <body>
        <h1>Checkout</h1>
        
        <div class="order-summary">
            <h3>Order Summary</h3>
            <p><strong>Total Amount: ${{ "%.2f"|format(total) }}</strong></p>
            <p>Items in cart: {{ cart_count }}</p>
        </div>
        
        <form action="/process-checkout" method="POST">
            <div class="form-group">
                <label for="email">Email Address:</label>
                <input type="email" id="email" name="email" required>
            </div>
            
            <div class="form-group">
                <label for="name">Full Name:</label>
                <input type="text" id="name" name="name" required>
            </div>
            
            <button type="submit">Place Order & Pay</button>
        </form>
        
        <div style="margin-top: 20px;">
            <a href="/cart">← Back to Cart</a>
        </div>
    </body>
    </html>
    '''
    
    cart_count = sum(cart.values())
    return render_template_string(template, total=total, cart_count=cart_count)

@app.route('/process-checkout', methods=['POST'])
def process_checkout():
    """Process checkout and integrate with PSP"""
    cart = session.get('cart', {})
    if not cart:
        return redirect('/')
    
    email = request.form.get('email')
    name = request.form.get('name')
    
    # Generate unique order ID
    order_id = f"ORD_{uuid.uuid4().hex[:8].upper()}"
    
    # Calculate total
    total_amount = calculate_cart_total(cart)
    
    # Save order to database
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    
    cursor.execute('''
        INSERT INTO orders (order_id, customer_email, total_amount, status)
        VALUES (?, ?, ?, 'pending')
    ''', (order_id, email, total_amount))
    
    # Save order items
    for product_id, quantity in cart.items():
        cursor.execute('SELECT name, price FROM products WHERE id = ?', (product_id,))
        product = cursor.fetchone()
        if product:
            cursor.execute('''
                INSERT INTO order_items (order_id, product_id, product_name, price, quantity)
                VALUES (?, ?, ?, ?, ?)
            ''', (order_id, product_id, product[0], product[1], quantity))
    
    conn.commit()
    conn.close()
    
    # Register payment with PSP
    try:
        psp_data = {
            'payment_order_id': order_id,
            'amount': float(total_amount),
            'callback_url': f'{ECOMMERCE_BASE_URL}/payment-callback',
            'redirect_url': f'{ECOMMERCE_BASE_URL}/payment-result'
        }
        
        response = requests.post(f'{PSP_BASE_URL}/api/register-payment', json=psp_data, timeout=10)
        
        if response.status_code == 200:
            psp_result = response.json()
            nonce = psp_result.get('nonce')
            
            # Update order with nonce
            conn = sqlite3.connect('ecommerce.db')
            cursor = conn.cursor()
            cursor.execute('UPDATE orders SET psp_nonce = ? WHERE order_id = ?', (nonce, order_id))
            conn.commit()
            conn.close()
            
            # Store order info in session for result page
            session['current_order'] = {
                'order_id': order_id,
                'nonce': nonce,
                'total': total_amount,
                'email': email
            }
            
            # Redirect to PSP payment page
            return redirect(f'{PSP_BASE_URL}/payment/{nonce}')
        else:
            return f"PSP Registration Error: {response.text}", 500
            
    except requests.RequestException as e:
        return f"PSP Connection Error: {str(e)}", 500

@app.route('/payment-callback', methods=['POST'])
def payment_callback():
    """Receive payment status callback from PSP"""
    try:
        data = request.get_json()
        payment_order_id = data.get('payment_order_id')
        nonce = data.get('nonce')
        status = data.get('status')
        
        # Update order status
        conn = sqlite3.connect('ecommerce.db')
        cursor = conn.cursor()
        cursor.execute('''
            UPDATE orders 
            SET payment_status = ?, status = ?, updated_at = CURRENT_TIMESTAMP 
            WHERE order_id = ? AND psp_nonce = ?
        ''', (status, 'completed' if status == 'completed' else 'failed', payment_order_id, nonce))
        conn.commit()
        conn.close()
        
        return jsonify({'status': 'received'})
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/payment-result')
def payment_result():
    """Handle payment result redirect from PSP"""
    status = request.args.get('status', 'unknown')
    nonce = request.args.get('nonce')
    
    current_order = session.get('current_order', {})
    
    if not current_order:
        return "No order information found", 400
    
    # Update order status based on redirect
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    cursor.execute('''
        UPDATE orders 
        SET payment_status = ?, status = ?, updated_at = CURRENT_TIMESTAMP 
        WHERE order_id = ?
    ''', (status, 'completed' if status == 'completed' else 'failed', current_order['order_id']))
    conn.commit()
    conn.close()
    
    # Clear cart if payment successful
    if status == 'completed':
        session.pop('cart', None)
    
    template = '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Payment Result</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; text-align: center; }
            .success { color: #28a745; }
            .failure { color: #dc3545; }
            .order-details { background: #f8f9fa; padding: 20px; border-radius: 5px; margin: 20px auto; max-width: 500px; }
            button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 3px; cursor: pointer; margin: 10px; }
        </style>
    </head>
    <body>
        {% if status == 'completed' %}
            <h1 class="success">✓ Payment Successful!</h1>
            <p>Thank you for your purchase. Your order has been confirmed.</p>
        {% else %}
            <h1 class="failure">✗ Payment Failed</h1>
            <p>Unfortunately, your payment could not be processed. Please try again.</p>
        {% endif %}
        
        <div class="order-details">
            <h3>Order Details</h3>
            <p><strong>Order ID:</strong> {{ order_id }}</p>
            <p><strong>Total Amount:</strong> ${{ "%.2f"|format(total) }}</p>
            <p><strong>Email:</strong> {{ email }}</p>
            <p><strong>Payment Status:</strong> {{ status.title() }}</p>
            <p><strong>Transaction ID:</strong> {{ nonce[:16] }}...</p>
        </div>
        
        <div>
            <a href="/"><button>Continue Shopping</button></a>
            <a href="/orders"><button>View Orders</button></a>
            {% if status != 'completed' %}
                <a href="/checkout"><button style="background: #28a745;">Try Again</button></a>
            {% endif %}
        </div>
    </body>
    </html>
    '''
    
    # Clear current order from session
    session.pop('current_order', None)
    
    return render_template_string(template, 
                                status=status, 
                                order_id=current_order.get('order_id', 'Unknown'),
                                total=current_order.get('total', 0),
                                email=current_order.get('email', 'Unknown'),
                                nonce=nonce or 'Unknown')

@app.route('/orders')
def view_orders():
    """View all orders"""
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    cursor.execute('''
        SELECT order_id, customer_email, total_amount, status, payment_status, created_at 
        FROM orders ORDER BY created_at DESC
    ''')
    orders = cursor.fetchall()
    conn.close()
    
    template = '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Orders</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; }
            table { width: 100%; border-collapse: collapse; }
            th, td { padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }
            th { background-color: #f2f2f2; }
            .completed { color: #28a745; font-weight: bold; }
            .failed { color: #dc3545; font-weight: bold; }
            .pending { color: #ffc107; font-weight: bold; }
        </style>
    </head>
    <body>
        <h1>Order History</h1>
        
        <table>
            <tr>
                <th>Order ID</th>
                <th>Customer Email</th>
                <th>Amount</th>
                <th>Order Status</th>
                <th>Payment Status</th>
                <th>Created</th>
            </tr>
            {% for order in orders %}
            <tr>
                <td>{{ order[0] }}</td>
                <td>{{ order[1] }}</td>
                <td>${{ "%.2f"|format(order[2]) }}</td>
                <td class="{{ order[3] }}">{{ order[3].title() }}</td>
                <td class="{{ order[4] }}">{{ order[4].title() }}</td>
                <td>{{ order[5] }}</td>
            </tr>
            {% endfor %}
        </table>
        
        <div style="margin-top: 20px;">
            <a href="/">← Back to Store</a>
        </div>
    </body>
    </html>
    '''
    
    return render_template_string(template, orders=orders)

@app.route('/admin')
def admin():
    """Simple admin panel"""
    template = '''
    <!DOCTYPE html>
    <html>
    <head>
        <title>Admin Panel</title>
        <style>
            body { font-family: Arial, sans-serif; margin: 40px; }
            .admin-section { margin: 30px 0; padding: 20px; border: 1px solid #ddd; border-radius: 5px; }
            button { background: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 3px; cursor: pointer; margin: 5px; }
        </style>
    </head>
    <body>
        <h1>Admin Panel</h1>
        
        <div class="admin-section">
            <h3>Quick Actions</h3>
            <a href="/orders"><button>View All Orders</button></a>
            <a href="/admin/clear-all-orders"><button style="background: #dc3545;">Clear All Orders</button></a>
        </div>
        
        <div class="admin-section">
            <h3>Database Info</h3>
            <p>This is a demo admin panel. In production, add proper authentication.</p>
        </div>
        
        <div style="margin-top: 20px;">
            <a href="/">← Back to Store</a>
        </div>
    </body>
    </html>
    '''
    
    return render_template_string(template)

@app.route('/admin/clear-all-orders')
def clear_all_orders():
    """Clear all orders (admin function)"""
    conn = sqlite3.connect('ecommerce.db')
    cursor = conn.cursor()
    cursor.execute('DELETE FROM order_items')
    cursor.execute('DELETE FROM orders')
    conn.commit()
    conn.close()
    return redirect('/admin')

if __name__ == '__main__':
    app.run(debug=True, port=5001)
