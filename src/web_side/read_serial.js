import express from "express";
import { WebSocketServer } from "ws";
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline";

const app = express();
const portHTTP = 8080;

let serial = null;
let parser = null;

// === SERVE FRONTEND ===
app.use(express.static("public"));
const server = app.listen(portHTTP, () =>
  console.log(`Web server running on http://localhost:${portHTTP}`)
);

// === WEBSOCKET ===
const wss = new WebSocketServer({ server });

// === SERIAL ===
function openSerialPort() {
  if (serial) {
    console.log("Serial already opened.");
    return;
  }
  broadcast({ Status: "SERIAL_OPENING" });

  serial = new SerialPort({ path: "COM8", baudRate: 115200 });

  serial.on("open", () => {
    console.log("Serial port opened");
    broadcast({ Status: "SERIAL_OPENED" });
  });

  serial.on("error", (err) => {
    console.error("Serial error:", err);
    broadcast({ Status: "SERIAL_ERROR", message: err.message });
  });

  parser = serial.pipe(new ReadlineParser({ delimiter: "\n" }));

  console.log("Serial port opened.");

  parser.on("data", (line) => {
    console.log("📥 Dari ESP32:", line);

    try {
      const json = JSON.parse(line);
      broadcast(json);
    } catch (e) {
      console.log("Bukan JSON valid:", line);
    }
  });
}

function closeSerial() {
  if (!serial) {
    console.log("Serial is not opened.");
    broadcast({ Status: "SERIAL_NOT_OPEN" });
    return;
  }

  if (!serial.isOpen) {
    console.log("Serial already closed.");
    broadcast({ Status: "SERIAL_ALREADY_CLOSED" });
    return;
  }

  serial.close((err) => {
    if (err) {
      console.log("Error closing serial:", err.message);
      broadcast({ Status: "SERIAL_CLOSE_ERROR", message: err.message });
    } else {
      console.log("Serial closed.");
      broadcast({ Status: "SERIAL_CLOSED" });

      // bersihkan parser & object
      serial = null;
      parser = null;
    }
  });
}

function broadcast(data) {
  const msg = JSON.stringify(data);
  wss.clients.forEach(ws => {
    if (ws.readyState === ws.OPEN) ws.send(msg);
  });
}

let connectionLogs = [];

function addLog(message) {
  const time = new Date().toLocaleTimeString();
  const logEntry = `[${time}] ${message}`;
  connectionLogs.push(logEntry);

  // broadcast log baru
  const json = JSON.stringify({
    Status: "LOG_EVENT",
    log: logEntry
  });

  wss.clients.forEach((ws) => {
    if (ws.readyState === ws.OPEN) ws.send(json);
  });
}

// Saat web kirim command (misal: delete)
wss.on("connection", (ws) => {
  console.log('Browser connected via WebSocket')
  addLog("Browser connected via WebSocket");

  ws.send(JSON.stringify({
    Status: "LOG_HISTORY",
    logs: connectionLogs
  }));

  ws.on("message", (msg) => {
    const command = JSON.parse(msg);
    console.log("📤 Dari Browser:", command);
    // console.log(command.type);
    if (command.type === "OPEN_SERIAL") {
      openSerialPort();
    }
    if (command.type === "CLOSE_SERIAL") {
      closeSerial();
    }
    if (command.type === "DISCONNECT") {
      const data = `DISCONNECT ${command.connID}\n`;
      console.log("DISCONNECTING...", data);
      addLog(`Browser requested DISCONNECT for ID ${command.connID}`);

      // Kirim tiap karakter satu per satu
      for (let i = 0; i < data.length; i++) {
        const char = data[i];
        serial.write(Buffer.from(char, 'ascii'), (err) => {
          if (err) {
            console.error(`Error writing char '${char}':`, err.message);
          } else {
            // console.log(`Char '${char}' sent`);
          }
        });
      }
    }

  });
});


