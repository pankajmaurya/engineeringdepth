#!/usr/bin/env python3
"""
Simple Redis connectivity test script.
Tests basic read/write operations to ensure Redis is working properly.
"""

import redis
import sys
from datetime import datetime

def test_redis_connection():
    """Test Redis connection and basic operations."""
    try:
        # Connect to Redis (default: localhost:6379, db=0)
        r = redis.Redis(host='localhost', port=6379, db=0, decode_responses=True)
        
        # Test connection
        print("Testing Redis connection...")
        r.ping()
        print("✓ Redis connection successful!")
        
        # Test writing a simple variable
        test_key = "test_variable"
        test_value = f"Hello Redis! Current time: {datetime.now()}"
        
        print(f"\nWriting to Redis:")
        print(f"Key: {test_key}")
        print(f"Value: {test_value}")
        
        r.set(test_key, test_value)
        print("✓ Write operation successful!")
        
        # Test reading the variable
        print(f"\nReading from Redis:")
        retrieved_value = r.get(test_key)
        print(f"Retrieved value: {retrieved_value}")
        
        if retrieved_value == test_value:
            print("✓ Read operation successful!")
            print("✓ Value matches what was written!")
        else:
            print("✗ Value mismatch!")
            return False
            
        # Test some additional operations
        print(f"\nTesting additional operations:")
        
        # Set with expiration
        r.setex("temp_key", 60, "This expires in 60 seconds")
        print("✓ Set key with expiration")
        
        # Check if key exists
        exists = r.exists("temp_key")
        print(f"✓ Key exists check: {bool(exists)}")
        
        # Get TTL (time to live)
        ttl = r.ttl("temp_key")
        print(f"✓ TTL for temp_key: {ttl} seconds")
        
        # Increment a counter
        counter_value = r.incr("counter")
        print(f"✓ Counter incremented to: {counter_value}")
        
        # Clean up test keys
        r.delete(test_key, "temp_key", "counter")
        print("✓ Test keys cleaned up")
        
        print(f"\n🎉 All Redis tests passed! Setup is complete.")
        return True
        
    except redis.ConnectionError:
        print("✗ Failed to connect to Redis server")
        print("Make sure Redis server is running on localhost:6379")
        return False
    except Exception as e:
        print(f"✗ An error occurred: {e}")
        return False

def main():
    """Main function to run Redis tests."""
    print("Redis Connectivity Test")
    print("=" * 30)
    
    success = test_redis_connection()
    
    if success:
        print("\n✅ Redis setup verification complete!")
        sys.exit(0)
    else:
        print("\n❌ Redis setup verification failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()
