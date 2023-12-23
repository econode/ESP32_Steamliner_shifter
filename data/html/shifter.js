
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
}

function handleOnSubmit(event){
    document.getElementById("submitButton").disabled = true;
    document.getElementById("spinnerDiv").classList.add("spinnerDisplay");
}

window.addEventListener('load', handleOnLoad);
