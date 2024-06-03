from collections import deque
import threading
from flask import Flask
import websocket
import _thread
import time
import rel
import requests
import json

app = Flask(__name__)

buffer_size = 375
buffer = deque(maxlen=buffer_size)
i = 0

figs = {'circle': 0, 'oval': 0, 'rectangle': 0, 'triangle': 0, 'noise':0}
lastTime = 0

def on_message(ws, message):
    global i  
    numbers = list(map(float, message.split(',')))
    buffer.extend(numbers)
    if len(buffer) == buffer_size:
        i += 1
    if len(buffer) == buffer_size and i == 12: # количество точек разница между буферами
        i = 0
        send_buffer()

def send_buffer():
    global figs
    global lastTime
    # Формируем JSON-объект для отправки
    payload = {"features": list(buffer)}
    # Отправляем запрос по указанному URL
    url = "http://localhost:1337/api/features" # адрес модели
    headers = {"Content-Type": "application/json"}
    response = requests.post(url, data=json.dumps(payload), headers=headers)
    try:
        json_response = json.loads(response.text)
        fig, v = find_largest_shape(json_response["result"]["classification"])
        print(fig, v)
        if v > 0.98: 
            figs[fig] += 1
            print(fig)
        if time.time()  - lastTime > 2: # время за которое вычисляется средняя фигура 
            figs = {'circle': 0, 'oval': 0, 'rectangle': 0, 'triangle': 0, 'noise':0}
            lastTime = time.time()
            print("clear")
    except json.JSONDecodeError as e:
        print("Failed to parse JSON response:", e)

def find_largest_shape(shapes):
    if not shapes:
        return None, None

    max_value = float('-inf')
    max_shape = None

    for shape, value in shapes.items():
        if value > max_value and value > 0:
            max_value = value
            max_shape = shape

    return max_shape, max_value

def on_error(ws, error):
    print(error)

def on_close(ws, close_status_code, close_msg):
    print("### closed ###")

def on_open(ws):
    print("Opened connection")

@app.route('/fig', methods=['GET'])
def get_fig():
    fig, v = find_largest_shape(figs)
    if fig:
        return fig
    return "none"

@app.after_request
def add_cors_headers(response):
    response.headers['Access-Control-Allow-Origin'] = '*'
    response.headers['Access-Control-Allow-Headers'] = 'Content-Type'
    response.headers['Access-Control-Allow-Methods'] = 'GET, POST, PUT'
    return response

def websocket_thread():
    app.run(host="localhost", port=9901)
   
if __name__ == "__main__":
    websocket_thread = threading.Thread(target=websocket_thread)
    websocket_thread.start()
    ws = websocket.WebSocketApp("ws://192.168.0.102:80", # ip палки
                                on_open=on_open,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)
    ws.run_forever()