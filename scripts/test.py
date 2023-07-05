import asyncio
import time
import json
import random
from websockets.client import connect


async def hello():
    connections = []
    diff = 10000
    sent = [0] * 1
    for i in range(1):
        websocket = await connect("ws://localhost:8081/rplace")
        connections.append(websocket)

    start = time.time()
    while True:
        what = {"x": random.randint(0, 1000), "y": random.randint(0, 1000),
                "color": random.randint(0, 1_000_000)}
        for index, websocket in enumerate(connections):
            await websocket.send(json.dumps(what))
            msg = await websocket.recv()
            obj = json.loads(msg)
            if what["x"] != obj["x"] or what["y"] != obj["y"] or what["color"] != obj["color"]:
                print("ERROR", what, obj)
            sent[index] += 1
        diff = time.time() - start
        if diff > 10:
            break
    print("Sent", sum(sent), "messages")
    print("Throughput ", sum(sent) / diff, "msg/s")
    for websocket in connections:
        await websocket.close()

asyncio.run(hello())
