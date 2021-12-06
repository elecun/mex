const electron = require('electron')
const fs = require('fs')
const app = electron.app 
var path = require('path')

const BrowserWindow = electron.BrowserWindow
let config_file = fs.readFileSync(path.resolve(__dirname, 'config.json'));
let config = JSON.parse(config_file);

app.on('ready', function(){
    let window = new BrowserWindow({
        fullscreen:true,
        webPreferences:{
            preload: path.join(__dirname, 'preload.js'),
        }
    })

    window.loadURL(config.config.host);
    window.once('ready-to-show', function(){
        window.show()
    })
})



// for mqtt client
const mqtt = require('mqtt')
const client_id = 'mex_viewer_'+Math.random().toString(16).substr(2,8)
const mqtt_host = 'mqtt://127.0.0.1:1883'
const options = {
    host:"127.0.0.1",
    port:1883,
    keepalive: 30,
    clientId: client_id,
    protocolId: 'MQTT',
    protocolVersion: 4,
    clean: true,
    reconnectPeriod: 1000,
    connectTimeout: 30 * 1000,
    will: {
      topic: 'WillMsg',
      payload: 'Connection Closed abnormally..!',
      qos: 2,
      retain: false
    },
    rejectUnauthorized: false
  };

  const client = mqtt.connect(options);
  client.on('connect', () => {
    console.log('Client connected:' + options.clientId);
    client.subscribe('mex/viewer', { qos: 2})
  })

  client.on('message', (topic, message, packet) => {
    console.log("on message : " + message.toString())
    if(topic=="mex/viewer"){
      const data = JSON.parse(message.toString());
      switch(data["command"]){
        case "close": { app.quit(); } break;
      }
    }
  });


  //quit
app.on('window-all-closed', function(){
    client.end();
    app.quit()
});