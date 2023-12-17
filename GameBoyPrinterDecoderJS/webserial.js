/*
  WebSerial wrapper
  Simplifies WebSerial

  created 15 May 2022
  modified 16 May 2022
  by Tom Igoe

  https://github.com/tigoe/html-for-conndev/blob/ace1750022c08cc141a4f5bcd0396cb2a6531e9f/webSerial/webserial.js

  Implemented for use with the GameBoyPrinterDecoderJS project:
  created 16 December 2023
  by Jim Valentine <github.com/f13dev>
  
  MIT License

  Copyright (c) 2020 tigoe

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// need self = this for connect/disconnect functions
let self;

class WebSerialPort {
  constructor() {
    // if webserial doesn't exist, return false:
    if (!navigator.serial) {
      alert("WebSerial is not enabled in this browser");
      return false;
    }
    // TODO: make this an option.
    this.autoOpen = true;
    // copy this to a global variable so that
    // connect/disconnect can access it:
    self = this;

    // basic WebSerial properties:
    this.port;
    this.reader;
    this.serialReadPromise;
    // add an incoming data event:
    // TODO: data should probably be an ArrayBuffer or Stream
    this.incoming = {
      data: null
    }
    // incoming serial data event:
    this.dataEvent = new CustomEvent('data', {
      detail: this.incoming,
      bubbles: true
    });

    // TODO: bubble these up to calling script
    // so that you can change button names on 
    // connect or disconnect:
    navigator.serial.addEventListener("connect", this.serialConnect);
    navigator.serial.addEventListener("disconnect", this.serialDisconnect);

    // if the calling script passes in a message
    // and handler, add them as event listeners:
    this.on = (message, handler) => {
      parent.addEventListener(message, handler);
    };
  }

  async openPort(thisPort) {
    try {
      // if no port is passed to this function, 
      if (thisPort == null) {
        // pop up window to select port:
        this.port = await navigator.serial.requestPort();
      } else {
        // open the port that was passed:
        this.port = thisPort;
      }
      // set port settings and open it:
      // TODO: make port settings configurable
      // from calling script:
      await this.port.open({ baudRate: 115200 });
      // start the listenForSerial function:
      this.serialReadPromise = this.listenForSerial();

    } catch (err) {
      // if there's an error opening the port:
      console.error("There was an error opening the serial port:", err);
    }
  }

  async closePort() {
    if (this.port) {
      // stop the reader, so you can close the port:
      this.reader.cancel();
      // wait for the listenForSerial function to stop:
      await this.serialReadPromise;
      // close the serial port itself:
      await this.port.close();
      // clear the port variable:
      this.port = null;
    }
  }

  async sendSerial(data) {
    // if there's no port open, skip this function:
    if (!this.port) return;
    // if the port's writable: 
    if (this.port.writable) {
      // initialize the writer:
      const writer = this.port.writable.getWriter();
      // convert the data to be sent to an array:
      // TODO: make it possible to send as binary:
      var output = new TextEncoder().encode(data);
      // send it, then release the writer:
      writer.write(output).then(writer.releaseLock());
    }
  }

  async listenForSerial() {
    // if there's no serial port, return:
    if (!this.port) return;
    // while the port is open:
    while (this.port.readable) {
      // initialize the reader:
      this.reader = this.port.readable.getReader();
      try {
        // read incoming serial buffer:
        const { value, done } = await this.reader.read();
        if (value) {
          // convert the input to a text string:
          // TODO: make it possible to receive as binary:
          this.incoming.data = new TextDecoder().decode(value);

          // fire the event:
          parent.dispatchEvent(this.dataEvent);
        }
        if (done) {
          break;
        }
      } catch (error) {
        // if there's an error reading the port:
        console.log(error);
      } finally {
        this.reader.releaseLock();
      }
    }
  }

  // this event occurs every time a new serial device
  // connects via USB:
  serialConnect(event) {
    console.log(event.target);
    // TODO: make autoOpen configurable
    if (self.autoOpen) {
      self.openPort(event.target);
    }
  }

  // this event occurs every time a new serial device
  // disconnects via USB:
  serialDisconnect(event) {
    console.log(event.target);
  }
}

// GameBoyPrinterDecoderJS specific code:

let webserial;
let serialButton = document.getElementById("selectSerial");
let readContainer = document.getElementById('data_text');
let autoClear = true;

function setup() {
    webserial = new WebSerialPort();
    if (webserial) {
        webserial.on('data', serialRead);
        serialButton.addEventListener("click", openClosePort);
    }
}

async function openClosePort() {
    let buttonLabel = "Open port";
    if (webserial.port) {
        await webserial.closePort();
    } else {
        await webserial.openPort();
        buttonLabel = "Close port";        
    }
    serialButton.innerHTML = buttonLabel;
}

function serialRead(event) {
    let new_data = event.detail.data;

    if ((autoClear && !new_data.includes('650B')) || new_data.includes('GAMEBOY PRINTER')) {
        readContainer.value = '';
        autoClear = false;
    }

    readContainer.value += new_data;

    readContainer.scrollTop = readContainer.scrollHeight;

    if (new_data.includes('Completed')) {
        document.getElementById('submit_button').click();
        autoClear = true;
    }
}

document.addEventListener('DOMContentLoaded', setup);