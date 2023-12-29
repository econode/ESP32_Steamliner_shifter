window.addEventListener('load', handleOnLoad);

var websocket;
var buttonState = {};

function handleOnLoad(event) {
    if( shifterConfig.hasFormUpdated == "true"){
        const oUpdatedMessage = document.getElementById("updatedMessage");
        oUpdatedMessage.style.display = "block";
        setTimeout( function(){oUpdatedMessage.style.display = "none";},1500);
    }
    if( shifterConfig.hasFromDefaults == "true" ){
        const oDefaultsMessage = document.getElementById("defaultsMessage");
        oDefaultsMessage.style.display = "block";
        setTimeout( function(){oDefaultsMessage.style.display = "none";},1500);
    }
    document.getElementById("submitButton").addEventListener("submit",handleOnSubmit);
    // buttonGearUp
    document.getElementById("buttonGearUp").addEventListener("pointerdown",function(){buttonState.upGearStart = new Date().getTime()} );
    document.getElementById("buttonGearUp").addEventListener("pointerup",handleBtnGearUp);
    // buttonGearDown
    document.getElementById("buttonGearDown").addEventListener("pointerdown",function(){ buttonState.downGearStart = new Date().getTime()} );
    document.getElementById("buttonGearDown").addEventListener("pointerup",handleBtnGearDown);

    

    let wsGatewayAddr = shifterConfig.wsGatewayAddr == "%wsGatewayAddr%" ? "ws://192.168.4.1/ws":shifterConfig.wsGatewayAddr;
    initWebSocket( wsGatewayAddr );
}

function handleBtnGearUp(event){
    var now = new Date().getTime();
    var pressedTime = now - buttonState.upGearStart;
    var jsonData = JSON.stringify({message:'gearUp',pressedTime:pressedTime});
    console.log(jsonData);
    websocket.send(jsonData);
}

function handleBtnGearDown(event){
    var now = new Date().getTime();
    var pressedTime = now - buttonState.downGearStart;
    var jsonData = JSON.stringify({message:'gearDown',pressedTime:pressedTime});
    console.log(jsonData);
    websocket.send(jsonData);
}

function handleOnSubmit(event){
    document.getElementById("submitButton").disabled = true;
    document.getElementById("spinnerDiv").classList.add("spinnerDisplay");
    websocket.close();
}

function initWebSocket(wsGatewayAddr){
    console.log('WS: Trying to open a WebSocket connection to '+ wsGatewayAddr );
    websocket = new WebSocket(wsGatewayAddr);
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
    console.log(jsonData);
    messageType = jsonData.messageType;
    var gearPositon = jsonData.payload.currentGearPosition;
    var gearPosId = jsonData.payload.gearPosId;
    var pressedTime = jsonData.payload.pressedTime;
    changeGearBoxIndicator(gearPositon);
}

function changeGearBoxIndicator(gearPositon){
    var elements = document.querySelectorAll(".gbSelected");
    elements.forEach((element) => {element.classList.remove('gbSelected');});
    if( document.getElementById('gb'+gearPositon) ){
        document.getElementById('gb'+gearPositon).classList.add('gbSelected');
    }
}

