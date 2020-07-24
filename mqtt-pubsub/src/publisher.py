import paho.mqtt.client as mqtt

#mqtt_host = 'broker.hivemq.com'
mqtt_host = 'mosquitto'
mqtt_port = 1883
publish_topic_name = 'arduino/in/actions'

def on_connect(client, userdata, flags, result_code):
    print('Conectado ', result_code)

print('Comenzando con mqtt...')

client = mqtt.Client()
client.on_connect = on_connect

client.connect(mqtt_host, mqtt_port, 60)

temperatura = None #int(input('Ingrese temp: '))
while temperatura != "q":

    client.publish(publish_topic_name, str(temperatura))

    #temperatura = int(input('Ingrese temp: '))
    temperatura = input('Ingrese payload: ')
