# install.sh
#!/bin/bash

echo "Setting up Flask PSP Application..."

# Create virtual environment
python3 -m venv paymentsystem

# Activate virtual environment
source paymentsystem/bin/activate

# Upgrade pip
pip install --upgrade pip

# Install requirements
pip install -r requirements.txt

echo "Installation complete!"
echo ""
echo "To run the application:"
echo "1. Activate virtual environment: source paymentsystem/bin/activate"
echo "2. Run the app: python psp.py"
echo "3. Access at: http://localhost:5000"
echo ""
echo "API Endpoints:"
echo "- POST /api/register-payment"
echo "- GET /payment/<nonce>"
echo "- POST /api/process-payment"
echo "- GET /api/payment-status/<nonce>"


