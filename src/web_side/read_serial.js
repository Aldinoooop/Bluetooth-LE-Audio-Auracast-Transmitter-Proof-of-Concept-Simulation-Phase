import express from "express";
import { WebSocketServer } from "ws";
import { SerialPort } from "serialport";
import { ReadlineParser } from "@serialport/parser-readline";

const app = express();
const portHTTP = 8080;

// === SERVE FRONTEND ===
app.use(express.static("public"));
const server = app.listen(portHTTP, () =>
  console.log(`Web server running on http://localhost:${portHTTP}`)
);

// === WEBSOCKET ===
const wss = new WebSocketServer({ server });

// === SERIAL ===
const serial = new SerialPort({ path: "COM8", baudRate: 115200 });
const parser = serial.pipe(new ReadlineParser({ delimiter: "\n" }));

// Saat ESP32 kirim JSON lewat Serial
parser.on("data", (line) => {
  console.log("📥 Dari ESP32:", line);
  try {
    const json = JSON.parse(line);
    // kirim ke semua client WebSocket
    wss.clients.forEach((ws) => {
      if (ws.readyState === ws.OPEN) ws.send(JSON.stringify(json));
    });
  } catch (e) {
    console.log("Bukan JSON valid:", line);
  }
});

// Saat web kirim command (misal: delete)
wss.on("connection", (ws) => {
  ws.on("message", (msg) => {
    const command = JSON.parse(msg);
    console.log("📤 Dari Browser:", command);
    // console.log(command.type);


    if (command.type === "DISCONNECT") {
      const data = `DISCONNECT:${command.connID}\n`;
      console.log("DISCONNECTING...", data);

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
