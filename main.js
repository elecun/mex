const electron = require('electron')
const fs = require('fs')
const app = electron.app 
var path = require('path')

const BrowserWindow = electron.BrowserWindow
let config_file = fs.readFileSync(path.resolve(__dirname, 'config.json'));
let config = JSON.parse(config_file);
console.log("confgure:", config);

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

//quit
app.on('window-all-closed', function(){
    app.quit()
})