#!/usr/bin/python3

# ----- IMPORTS ------------------------------------------------------------------------------------

from flask import Flask, redirect, render_template, request
from subprocess import *
import RPi.GPIO as GPIO
import threading
import atexit

# ----- MISC HELPERS -------------------------------------------------------------------------------

def run_cmd(cmd):
    #print(cmd)
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=STDOUT)
    output = p.communicate()[0]
    return output

def gpio_init():
    GPIO.setmode(GPIO.BCM)
    for button in [10, 22, 27]:
        GPIO.setup(button, GPIO.IN, pull_up_down = GPIO.PUD_UP)
    for led in [17, 18, 23, 24]:
        GPIO.setup(led, GPIO.OUT)
        GPIO.output(led, GPIO.LOW)

def light_leds(vol):
    with lock:
        for led, limit in [(17, 25), (18, 50), (23, 75), (24, 100)]:
            GPIO.output(led, GPIO.HIGH if vol >= limit else GPIO.LOW)

# ----- GLOBAL MPC VARIABLES -----------------------------------------------------------------------

streams = []
streams_count = 0
with open('streams.txt', 'r') as file:
    for idx, line in enumerate(file):
        stream_info = line.split("|")
        streams.append({
            'id': idx,
            'name': stream_info[0].strip(),
            'url': stream_info[1].strip()
        })
        streams_count += 1

current_id = -1
current_volume = 75

# ----- MPC HELPERS --------------------------------------------------------------------------------

def mpc_play(stream_id):
    global streams
    with lock:
        run_cmd('mpc clear')
        run_cmd('mpc add %s' % streams[stream_id]['url'])
        run_cmd('mpc play')

def mpc_set(stream_id):
    global current_id
    with lock:
        current_id = stream_id
        mpc_play(current_id)

def mpc_next():
    global streams_count
    global current_id
    if streams_count > 0:
        with lock:
            current_id = (current_id + 1) % streams_count
            mpc_play(current_id)
    else:
        print("ERROR: no streams")

def mpc_set_vol(vol):
    with lock:
        run_cmd('mpc volume %d' % vol)
        light_leds(vol)

def mpc_vol_up():
    global current_volume
    with lock:
        current_volume = min(current_volume + 25, 100)
        mpc_set_vol(current_volume)

def mpc_vol_down():
    global current_volume
    with lock:
        current_volume = max(current_volume - 25, 0)
        mpc_set_vol(current_volume)

def mpc_stop():
    global current_id
    with lock:
        current_id = -1
        run_cmd('mpc clear')

def mpc_print():
    global streams
    global current_id
    global current_volume
    with lock:
        print('Currently playing: %s' % ('N/A' if current_id < 0 else streams[current_id]['name']))
        print('Volume: %d%%' % current_volume)

# ----- GPIO HANDLERS ------------------------------------------------------------------------------

def gpio10_handler():
    with lock:
        mpc_vol_down()
        mpc_print()

def gpio22_handler():
    with lock:
        mpc_vol_up()
        mpc_print()

def gpio27_handler():
    with lock:
        mpc_next()
        mpc_print()

# ----- GLOBAL THREADING VARIABLES -----------------------------------------------------------------

lock = threading.RLock()

# ----- ^C HANDLER ---------------------------------------------------------------------------------

def interrupt():
    for button in [10, 22, 27]:
        GPIO.remove_event_detect(button)
    GPIO.cleanup()
    mpc_stop()

# ----- FLASK APP ----------------------------------------------------------------------------------

app = Flask(__name__)

@app.route('/', methods=['GET','POST'])
def index():
    global streams
    global streams_count
    global current_id
    global current_volume
    current_name = 'N/A' if current_id < 0 else streams[current_id]['name']

    if request.method == 'POST':
        new_name = request.form['name']
        new_url = request.form['url']
        if new_name != "" and new_url != "":
            streams.append({
                'id': streams_count,
                'name': new_name.strip(),
                'url': new_url.strip()
            })
            streams_count += 1
        else:
            print('Error: wrong stream name or url')

    return render_template('main.html', title = 'Internet Radio',
                           streams = streams,
                           streams_count = streams_count,
                           current_id = current_id,
                           current_volume = current_volume,
                           current_name = current_name)

@app.route('/<int:stream_id>')
def play(stream_id):
    global streams_count
    if stream_id < streams_count: # can't be < 0, no need to check
        mpc_set(stream_id)
    else:
        print("ERROR: stream id not recognized")
    return redirect('/')

@app.route('/stop')
def stop():
    mpc_stop()
    return redirect('/')

@app.route('/vol/<action>')
def volume(action):
    if action == 'down':
        mpc_vol_down()
    elif action == 'up':
        mpc_vol_up()
    else:
        print("ERROR: volume action not recognized")
    return redirect('/')

# ----- MAIN ---------------------------------------------------------------------------------------

if __name__ == "__main__":
    gpio_init()
    mpc_set_vol(current_volume)
    GPIO.add_event_detect(10, GPIO.FALLING, callback=gpio10_handler, bouncetime=100)
    GPIO.add_event_detect(22, GPIO.FALLING, callback=gpio22_handler, bouncetime=100)
    GPIO.add_event_detect(27, GPIO.FALLING, callback=gpio27_handler, bouncetime=100)
    atexit.register(interrupt)
    app.run(host='0.0.0.0', port=8080, debug=False)
