#!/bin/bash

echo "=== Testing Ecommerce + PSP Integration ==="
echo ""

echo "1. Testing Ecommerce app..."
ECOMMERCE_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5001 2>/dev/null)
if [ "$ECOMMERCE_STATUS" = "200" ]; then
    echo "✓ Ecommerce app is running on port 5001"
else
    echo "✗ Ecommerce app is not responding on port 5001"
    echo "  Start with: python ecommerce_app.py"
fi

echo ""
echo "2. Testing PSP service..."
PSP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5000 2>/dev/null)
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
    echo "1. Visit: http://127.0.0.1:5001"
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


