import socket
import threading
import os
import json
from pymongo import MongoClient


bind_ip = "0.0.0.0"
bind_port = 19970
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((bind_ip, bind_port))
server.listen(1)
print(f"[*] Listening on {bind_ip}:{bind_port}")


def handle_client(client_socket):
	request = client_socket.recv(1024)
	print(f"[*] Received: {request.decode('utf-8')}")
	return_message = 'Temperature storage has received the package!'
	client_socket.send(return_message.encode('utf-8'))
	client_socket.close()
	# connect to the MongoDB server
	client = MongoClient('mongodb://localhost:27017/')
	# specify the database and collection
	db = client['granxa']
	readings_collection = db['readings']
	readings_collection.insert_one(json.loads(request))
	print("Reading stored in MongoDB")


while True:
	client, addr = server.accept()
	print(f"[*] Accepted connection from: {addr[0]}:{addr[1]}")
	client_handler = threading.Thread(target=handle_client, args=(client,))
	client_handler.start()
