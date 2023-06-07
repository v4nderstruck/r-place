import multiprocessing
import asyncio
import time
from websockets import client

# Constants
NUM_WORKERS = 5  # Number of workers to create
NUM_CONNECTIONS_PER_WORKER = 1  # Number of connections per worker
DURATION = 3  # Duration of the stress test in seconds

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
        if (datetime.datetime.now() - start).total_seconds() * 1000 >= DURATION * 1000:
            return
        for ws in connections:
            send = msg[random.randint(0, len(msg) - 1)]
            await ws.send(send)
            # await ws.recv()
            # if if we passed the duration
            result[worker_id] += 1




def random_msg():
    """adds random messages to the msg_dict in JSON {x: int, y: int, color: int} format"""
    import random
    import json
    global msg_dict
    msg_dict = []
    for i in range(10000):
        msg_dict.append(json.dumps({"x": random.randint(0, 999), "y": random.randint(0, 999),
                         "color": random.randint(0, 10000)}))

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
