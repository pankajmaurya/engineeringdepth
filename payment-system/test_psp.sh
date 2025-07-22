# PSP Sample API Requests
# Make sure the Flask app is running on http://127.0.0.1:5000

echo "=== PSP Payment Flow Test ==="
echo ""

# Step 1: Register a payment (called by ecommerce system)
echo "1. Registering payment..."
RESPONSE=$(curl -s -X POST http://127.0.0.1:5000/api/register-payment \
  -H "Content-Type: application/json" \
  -d '{
    "payment_order_id": "ORDER_12345",
    "amount": 99.99,
    "callback_url": "https://ecommerce.example.com/payment-callback",
    "redirect_url": "https://ecommerce.example.com/payment-complete"
  }')

echo "Response: $RESPONSE"
echo ""

# Extract nonce from response (requires jq)
NONCE=$(echo $RESPONSE | grep -o '"nonce": "[^"]*' | cut -d'"' -f4)
echo "Extracted nonce: $NONCE"
echo ""

# Step 2: Show payment page URL (user would visit this)
echo "2. Payment page URL:"
echo "http://127.0.0.1:5000/payment/$NONCE"
echo ""

# Step 3: Simulate payment processing (what happens when user submits form)
echo "3. Processing payment..."
curl -s -X POST http://127.0.0.1:5000/api/process-payment \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "nonce=$NONCE&payment_method=wallet" \
  -w "\nHTTP Status: %{http_code}\n"
echo ""

# Step 4: Check payment status
echo "4. Checking payment status..."
curl -s -X GET http://127.0.0.1:5000/api/payment-status/$NONCE | jq '.'
echo ""

echo "=== Test Idempotency ==="
echo "5. Trying to register same payment again..."
curl -s -X POST http://127.0.0.1:5000/api/register-payment \
  -H "Content-Type: application/json" \
  -d '{
    "payment_order_id": "ORDER_12345",
    "amount": 99.99,
    "callback_url": "https://ecommerce.example.com/payment-callback",
    "redirect_url": "https://ecommerce.example.com/payment-complete"
  }' | jq '.'
echo ""

echo "=== Manual Testing Commands ==="
echo ""
echo "# Register Payment"
echo 'curl -X POST http://127.0.0.1:5000/api/register-payment \'
echo '  -H "Content-Type: application/json" \'
echo '  -d "{\"payment_order_id\": \"ORDER_67890\", \"amount\": 149.99, \"callback_url\": \"https://shop.example.com/callback\", \"redirect_url\": \"https://shop.example.com/success\"}"'
echo ""
echo "# Check Status (replace NONCE with actual nonce)"
echo 'curl -X GET http://127.0.0.1:5000/api/payment-status/YOUR_NONCE_HERE'
echo ""
echo "# Process Payment (replace NONCE with actual nonce)"
echo 'curl -X POST http://127.0.0.1:5000/api/process-payment \'
echo '  -H "Content-Type: application/x-www-form-urlencoded" \'
echo '  -d "nonce=YOUR_NONCE_HERE&payment_method=wallet"'
echo ""

echo "=== Test Complete ==="
