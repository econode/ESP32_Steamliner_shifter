window.addEventListener('load', handleOnLoad);

var gateway = 'ws://192.168.4.1/ws';
var websocket;

function handleOnLoad(event) {
    if( hasFormUpdated ){
        const oUpdatedMessage = document.getElementById("updatedMessage");
        oUpdatedMessage.style.display = "block";
        setTimeout( function(){oUpdatedMessage.style.display = "none";},1500);
    }
    if( hasFromDefaults ){
        const oDefaultsMessage = document.getElementById("defaultsMessage");
        oDefaultsMessage.style.display = "block";
        setTimeout( function(){oDefaultsMessage.style.display = "none";},1500);
    }
    document.getElementById("shifterForm").addEventListener("submit",handleOnSubmit);
    initWebSocket();
}

function handleOnSubmit(event){
    document.getElementById("submitButton").disabled = true;
    document.getElementById("spinnerDiv").classList.add("spinnerDisplay");
    websocket.close();
}

function initWebSocket() {
    console.log('WS: Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}
function onOpen(event) {
    console.log('WS: Connection opened');
}

function onClose(event) {
    console.log('WS: Connection closed');
    setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
    var messageType = false
    var jsonData = false
    // Make sure we have valid JSON
    try {
        jsonData = JSON.parse(event.data)
    } catch (e) {
    console.log("Websocket JSON parse error")
    console.log(e)
    }

    messageType = jsonData.messageType;
    var gearPositon = jsonData.payload.currentGearPosition;
    changeGearBoxIndicator(gearPositon);
}

function changeGearBoxIndicator(gearPositon){
    var elements = document.querySelectorAll(".gbSelected");
    elements.forEach((element) => {element.classList.remove('gbSelected');});
    document.getElementById('gb'+gearPositon).classList.add('gbSelected');
}

