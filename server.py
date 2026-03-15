#!/usr/bin/env python

# python base modules
from threading import Lock
from queue import Queue, Empty

# dependencies
from flask import Flask, render_template, copy_current_request_context
from flask_json import json_response
from flask_socketio import SocketIO, emit

# Set this variable to "threading", "eventlet" or "gevent" to test the
# different async modes, or leave it set to None for the application to choose
# the best option based on installed packages.
async_mode = None

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
app.config['JSON_ADD_STATUS'] = False
socketio = SocketIO(app, async_mode=async_mode)

is_paused = True
data = None


@app.route('/')
def index():
    return render_template('index.html', async_mode=socketio.async_mode)

@app.route('/data')
def report_data():
    if data:
        return json_response(alpha=data[1], beta=data[2], gamma=data[3])
    else:
        return json_response()

@socketio.on('incoming_data')
def test_message(message):
    global data
    data = message['data']
    print(data)
    #emit('server_response', {'text': 'Got {}'.format(message['data'])})


@socketio.on('pause')
def action_request():
    print('Pause')
    global is_paused
    is_paused = not is_paused
    emit('server_response', {
        'text': 'Paused: {}'.format(str(is_paused)),
    })


@socketio.on('connect')
def test_connect():
    print('Client connected')
    emit('server_response', {'text': 'Client is connected'})


@socketio.on('disconnect')
def test_disconnect():
    print('Client disconnected')


if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', debug=True)
