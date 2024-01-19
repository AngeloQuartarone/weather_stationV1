import socket
import sys
import time
import telepot
from telepot.loop import MessageLoop

HOST = '127.0.0.1'  # Indirizzo IP del server
PORT = 8080         # Porta a cui connettersi

def handle(msg):
    chat_id = msg['chat']['id']
    command = msg['text']

    print ('Got command: %s' % command)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect((HOST, PORT))
            print('Connected')
            
            if command == '/forecast':
                message = "FORECAST"
                s.sendall(message.encode("utf-8"))
                response = s.recv(128).decode()
                bot.sendMessage(chat_id, response)
                
            elif command == '/sensor_value':
                message = "S_VALUE"
                s.sendall(message.encode("utf-8"))
                response = s.recv(128).decode()
                bot.sendMessage(chat_id, response)

        except Exception as e:
            print("An error occurred:", e)

bot = telepot.Bot('6905003409:AAFHaVP2BGZGsf7SXt5TNc1nyCLKiPx4RNI')

MessageLoop(bot, handle).run_as_thread()
print('I am listening ...')

while True:
    time.sleep(10)
