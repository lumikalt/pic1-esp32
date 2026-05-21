# mock_esp32.py  —  simulates ESP32 sensor data via a local /data endpoint
# pip install flask

from flask import Flask, jsonify
import math, time, threading

app = Flask(__name__)

start = time.time()

def t():
    return time.time() - start

def sensor_data():
    """Generate smooth, realistic-looking sensor motion."""
    s = t()
    return {
        # Magnetometer (µT) — slow drift + noise
        "mx":  0.40 + 0.05 * math.sin(s * 0.3),
        "my": -0.63 + 0.04 * math.cos(s * 0.4),
        "mz": -0.67 + 0.03 * math.sin(s * 0.5),

        # Heading (degrees)
        "heading": (305 + 10 * math.sin(s * 0.2)) % 360,

        # Accelerometer (g) — gentle tilt simulation
        "ax":  0.02 * math.sin(s * 0.7),
        "ay":  0.03 * math.cos(s * 0.5),
        "az":  1.00 + 0.01 * math.sin(s * 1.1),

        # Orientation (degrees) — slow roll/pitch/yaw sweep
        "roll":  20 * math.sin(s * 0.4),
        "pitch": 15 * math.sin(s * 0.3 + 1.0),
        "yaw":   (s * 10) % 360,

        # Motor/coil state
        "motor": "Running" if int(s) % 6 < 4 else "Stopped / Off",
    }

@app.route('/data')
def data():
    return jsonify(sensor_data())

if __name__ == '__main__':
    print("Mock ESP32 running at http://localhost:5001/data")
    print("Set ESP32_IP = 'localhost' and ESP32_URL port to 5001 in dashboard.py")
    app.run(host='0.0.0.0', port=5001, debug=False)
