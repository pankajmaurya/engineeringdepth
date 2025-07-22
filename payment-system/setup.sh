# setup_ecommerce.sh
#!/bin/bash

#echo "Setting up Ecommerce Flask Application..."

# Create virtual environment for ecommerce
#python3 -m venv ecommerce_venv

# Activate virtual environment
#source ecommerce_venv/bin/activate

# Upgrade pip
#pip install --upgrade pip

# Install requirements
#pip install -r ecommerce_requirements.txt

echo "Ecommerce app setup complete!"
echo ""
echo "To run both applications:"
echo ""
echo "Terminal 1 (PSP Service):"
echo "  source venv/bin/activate"
echo "  python app.py"
echo "  # Runs on http://localhost:5000"
echo ""
echo "Terminal 2 (Ecommerce App):"
echo "  source ecommerce_venv/bin/activate" 
echo "  python ecommerce_app.py"
echo "  # Runs on http://localhost:5001"
echo ""
echo "Then visit: http://localhost:5001"

# test_integration.sh
#!/bin/bash

echo "=== Testing Ecommerce + PSP Integration ==="
echo ""

echo "1. Testing Ecommerce app..."
ECOMMERCE_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:5001 2>/dev/null)
if [ "$ECOMMERCE_STATUS" = "200" ]; then
    echo "✓ Ecommerce app is running on port 5001"
else
    echo "✗ Ecommerce app is not responding on port 5001"
    echo "  Start with: python ecommerce_app.py"
fi

echo ""
echo "2. Testing PSP service..."
PSP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:5000 2>/dev/null)
if [ "$PSP_STATUS" = "200" ]; then
    echo "✓ PSP service is running on port 5000"
else
    echo "✗ PSP service is not responding on port 5000"
    echo "  Start with: python app.py"
fi

echo ""
echo "3. Testing integration..."
if [ "$ECOMMERCE_STATUS" = "200" ] && [ "$PSP_STATUS" = "200" ]; then
    echo "✓ Both services are running"
    echo ""
    echo "Complete flow test:"
    echo "1. Visit: http://localhost:5001"
    echo "2. Add products to cart"
    echo "3. Go to checkout"
    echo "4. Complete payment flow"
    echo "5. Verify payment result"
else
    echo "✗ One or both services are not running"
fi

echo ""
echo "=== Manual Test Steps ==="
echo ""
echo "1. Add to cart:"
echo "   curl -X POST http://localhost:5001/add-to-cart -d 'product_id=1&quantity=2'"
echo ""
echo "2. View products:"
echo "   curl http://localhost:5001/"
echo ""
echo "3. Check orders:"
echo "   curl http://localhost:5001/orders"

# start_both.sh
#!/bin/bash

echo "Starting both PSP and Ecommerce applications..."

# Start PSP service in background
echo "Starting PSP service on port 5000..."
cd /path/to/psp && source venv/bin/activate && python app.py &
PSP_PID=$!

sleep 2

# Start Ecommerce app in background  
echo "Starting Ecommerce app on port 5001..."
cd /path/to/ecommerce && source ecommerce_venv/bin/activate && python ecommerce_app.py &
ECOMMERCE_PID=$!

echo ""
echo "Both applications started:"
echo "PSP Service PID: $PSP_PID (http://localhost:5000)"
echo "Ecommerce App PID: $ECOMMERCE_PID (http://localhost:5001)"
echo ""
echo "Visit: http://localhost:5001 to start shopping"
echo ""
echo "To stop both applications:"
echo "kill $PSP_PID $ECOMMERCE_PID"

# Keep script running
wait
