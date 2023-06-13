import multiprocessing
import asyncio
import time
from websockets import client

# Constants
NUM_WORKERS = 2  # Number of workers to create
NUM_CONNECTIONS_PER_WORKER = 100  # Number of connections per worker
DURATION = 3  # Duration of the stress test in seconds
TILE_X = 1000

# WebSocket server URL
SERVER_URL = "ws://localhost:8081/tile"

# Performance measurement variables
stop = False

msg_dict = []

async def worker(worker_id, result, msg):
    import random
    import datetime
    start = datetime.datetime.now()
    connections = []
    for i in range(NUM_CONNECTIONS_PER_WORKER):
        connections.append(await client.connect(SERVER_URL))

    while True:
        for ws in connections:
            send = msg[random.randint(0, len(msg) - 1)]
            await ws.send(send)
            await ws.recv()
            # if if we passed the duration
            result[worker_id] += 1

        if (datetime.datetime.now() - start).total_seconds() * 1000 >= DURATION * 1000:
            return




def random_msg():
    """adds random messages to the msg_dict in JSON {x: int, y: int, color: int} format"""
    import random
    import json
    import sys
    global msg_dict
    msg_dict = []
    for _i in range(10000):
        x = random.randint(0, 999)
        y = random.randint(0, 999)
        color = random.randint(0, 2**32 - 1)
        msg_dict.append(
            (json.dumps({"x": x, "y": y,
                         "color": color}),
             {"x": x, "y": y, "color": color}
             )
        )

def coroutine(i, result, msg):
    asyncio.run(worker(i, result, msg))

def main():
    global msg_dict
    random_msg()
    # Create workers
    print(f"Creating {NUM_WORKERS} workers with {NUM_CONNECTIONS_PER_WORKER} connections each")
    process = []
    result = multiprocessing.Array('i', [0] * NUM_WORKERS )
    for i in range(NUM_WORKERS):
        p = multiprocessing.Process(target=coroutine, args=(i,result,msg_dict))
        process.append(p)
        p.start()

    print(f"Running for {DURATION} seconds")


    # Stop workers
    for p in process:
        p.join(DURATION)

    # Calculate total messages sent
    result = list(result)
    total_messages_sent = sum(result)

    # Print performance results
    print(f"Total messages sent: {total_messages_sent}")
    print(f"Average messages per second: {total_messages_sent / DURATION}")


if __name__ == "__main__":
    main()
