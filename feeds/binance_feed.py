import json
import websocket

# Using Binance API
symbol = "btcusdt"  # Trading pair (lowercase)
depth_level = 100   # Top N levels (Binance supports partial book depth)
ws_url = f"wss://stream.binance.us:9443/ws/{symbol}@depth@100ms"

# Callback Functions
def on_open(ws):
    print(f"Connected to Binance WebSocket for {symbol} L3 depth!")

def on_message(ws, message):
    data = json.loads(message)

    event_time = data["E"]
    bids = data["b"]
    asks = data["a"]

    print(f"Event Time: {event_time}")
    for price, qty in bids:
        print(f"BID  {price} {qty}")
    for price, qty in asks:
        print(f"ASK  {price} {qty}")
    print("-" * 40)

def on_error(ws, error):
    print(f"Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("WebSocket closed")

# Run WebSocket
ws = websocket.WebSocketApp(
    ws_url,
    on_open=on_open,
    on_message=on_message,
    on_error=on_error,
    on_close=on_close,
)

ws.run_forever()
