import paho.mqtt.client as mqtt

#mqtt_host = 'broker.hivemq.com'
mqtt_host = 'mosquitto'
mqtt_port = 1883

def on_connect(client, userdata, flags, result_code):
    print('Conectado ', result_code)

print('Comenzando con mqtt...')

client = mqtt.Client()
client.on_connect = on_connect

client.connect(mqtt_host, mqtt_port, 60)

temperatura = int(input('Ingrese temp: '))
while temperatura > -10:

    client.publish('termometrodormitorio', str(temperatura))

    temperatura = int(input('Ingrese temp: '))
