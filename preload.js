
const mqtt = require('mqtt')
window.mqtt = mqtt

window.addEventListener('DOMContentLoaded', () => {
    const replacetext = (selector, text) => {
        const element = document.getElementById(selector)
        if(element) element.innerText = text
    }

    for (const type of ['chrome', 'node', 'electron']){
        replacetext('$(type)-version, process.versions[type]')
    }
});