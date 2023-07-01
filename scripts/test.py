import asyncio
import time
import json
import random
from websockets.client import connect


async def hello():
    async with connect("ws://localhost:8081/rplace") as websocket:
        while True:
            what = {"x": random.randint(0, 1000), "y": random.randint(0, 1000),
                    "color": random.randint(0, 1_000_000)}
            await websocket.send(json.dumps(what))
            msg = await websocket.recv()
            obj = json.loads(msg)
            if what["x"] != obj["x"] or what["y"] != obj["y"] or what["color"] != obj["color"]:
                print("ERROR", what, obj)
            time.sleep(1)

asyncio.run(hello())
