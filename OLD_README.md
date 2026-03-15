
# Old readme

This code declares a webserver to run on your computer. Open the page on your mobile device to stream IMU data back to the host. Use this data to calculate the device relative position, orientation, etc.

## Install and run

1. Create a python environment, with Anaconda or Virtual Env. With Anaconda, use for example `conda create -n flask37 python=3.7`
1. Source your env. With Anaconda, use `conda activate flask37`
1. Verify you are using Python 3 : `python --version`
1. Install dependencies `pip install -r requirements.txt`, read the output to check everything went well
1. Note your local IP address, such as `192.168.1.64` for me. On linux, use `ifconfig`
1. Run server with `python server.py`. Pro tip: you can stop the script with `Ctrl-C`
1. On your device, open the address `192.168.1.64:5000` to visit the server homepage
1. On some browser, the timestamp has a reduced precision ([explanation](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/now)), on Firefox you need for example to disable `privacy.reduceTimerPrecision` in `about:config`

### Common issues

- `socket.error: [Errno 98] Address already in use` means another flask instance is still running. Restart your terminal or type `fuser 5000/tcp -k` (might need a sudo) for linux ([source](https://stackoverflow.com/questions/41393384/flask-running-issue-socket-error-errno-98-address-already-in-use))

## Sources

- FlaskIO documentation and [example](https://github.com/miguelgrinberg/Flask-SocketIO/tree/master/example)
- Device motion capture [reference](https://whatwebcando.today/device-motion.html)
