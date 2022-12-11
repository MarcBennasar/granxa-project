from flask import Flask, render_template, jsonify
from pymongo import MongoClient
import datetime
import time

app = Flask(__name__)

# connect to the MongoDB server
client = MongoClient('mongodb://localhost:27017/')

# specify the database and collection
db = client['granxa']
collection = db['readings']

@app.route('/')
def index():
    # retrieve the values from the MongoDB server
    values_aux = collection.find({"sensorType": "temperature"})
    value = sorted(values_aux, key=lambda x: x["timestamp"])[-1]
    iso_time = datetime.datetime.fromtimestamp(value['timestamp'])
    value['timestamp'] = iso_time.isoformat()
    value.pop("_id")
    print(" ---- this is index")
    print(value)
    # render the template, passing the values to be displayed
    return render_template('index.html', value=value)

@app.route('/update_doc')
def update_doc():
    values_aux = collection.find({"sensorType": "temperature"})
    value = sorted(values_aux, key=lambda x: x["timestamp"])[-1]
    iso_time = datetime.datetime.fromtimestamp(value['timestamp'])
    value['timestamp'] = iso_time.isoformat()
    value.pop("_id")
    print(" ---- this is update")
    print(value)
    return value
#if __name__ == '__main__':
#    app.debug = True
#    app.run()
