# Publishers: publican informacion a topic determinado (luzdormitorio) y valor (payload) (on/off)
# Consumers: recibir notificacion del topico al que estan subscriptos

import datetime
import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

db_client = InfluxDBClient(host='influxdb', port=8086)
#mqtt_host = 'broker.hivemq.com'
mqtt_host = 'mosquitto'
mqtt_port = 1883
temperature_topic = 'arduino/out/temperature'
humidity_topic = 'arduino/out/humidity'
events_topic = 'arduino/out/events'

#mqtt_host = 0
#mqtt_port = 0

def initialize_db():
    db_name = 'mqtt'
    db_client.create_database(db_name)
    db_client.switch_database(db_name)


def write_to_db(topic, measure):
    if type(measure) == str:
        try:
            measure = float(measure)
        except Exception:
            pass
    try:
        print("Gonna send to DB")
        current_time = datetime.datetime.utcnow().isoformat()
        print("This is my time", current_time)
        json_body = [{
            "measurement": topic,
            "tags": {},
            "time": current_time,
            "fields": {
                "value": measure,
            }
        }]
        print("This is my payload", json_body)
        if db_client.write_points(json_body):
            print("Sent to DB")
        else:
            print("Error sending to DB")
        print("Done sending")
    except Exception as e:
        print("Exception!")
        print(e)

        
def on_connect(client, userdata, flags, result_code):
    print('Conectado ', result_code)

    client.subscribe(temperature_topic)
    client.subscribe(humidity_topic)
    client.subscribe(events_topic)

def on_message(client, userdata, msg):
    print(msg.topic, msg.payload)
    write_to_db(msg.topic, msg.payload.decode("utf-8"))

initialize_db()

print('Comenzando con mqtt...')
write_to_db(temperature_topic, 0.0)
write_to_db(humidity_topic, 0.0)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(mqtt_host, mqtt_port, 60)

client.loop_forever()
