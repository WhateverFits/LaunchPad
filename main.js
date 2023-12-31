var currentState = {};

function updatePage() {
	byId('#safetyon').style.display = currentState.safety ? "block" : "none";
	byId('#safetyoff').style.display = !currentState.safety ? "block" : "none";

	byId('#state').innerText = currentState.state;

	byId('#readyButton').disabled = !currentState.safety || currentState.state == 'Live';
	byId('#standbyButton').disabled = currentState.state != 'Live';
	byId('#launchButton').disabled = !currentState.safety || currentState.state != 'Live';

	byId('#launchText').style.display = currentState.launch ? "block" : "none";
	byId('#controls').style.display = !currentState.launch ? "block" : "none";

	var launchers = byId('#launchers');
	launchers.innerHTML = '';
	for (var i = 0; i < currentState.launchCount; i++) {
		const newNode = document.createElement("div");
		newNode.classList.add(currentState.launchers[i] === 0 ? 'offline' : 'online');
		newNode.classList.add('status');
		newNode.innerText = currentState.launchers[i] === 0 ? '-' : '+';
		launchers.appendChild(newNode);
	}
}
function getStatus() {
	fetch("/status.js").then(resp => resp.json()).then(json => {
		Object.assign(currentState, json);
		updatePage();
	});
}

function setReady() {
	fetch("/ready.js").then(resp => resp.json()).then(json => {
		Object.assign(currentState, json);
		updatePage();
	});
}

function setStandby() {
	fetch("/standby.js").then(resp => resp.json()).then(json => {
		Object.assign(currentState, json);
		updatePage();
	});
}

function launch() {
	fetch("/launch.js").then(resp => resp.json()).then(json => {
		Object.assign(currentState, json);
		updatePage();
	});
}

function safetyCheck() {
	fetch("/safety.js").then(resp => resp.json()).then(json => {
		Object.assign(currentState, json);
		updatePage();
	});
}

function byId(id) {
	return document.body.querySelector(id);
}

let intervalId;

document.addEventListener("DOMContentLoaded", () => {
	fetch("/startup.js").then(resp => resp.json()).then(json => {
		Object.assign(currentState, json)
		updatePage();
	});
	byId("#statusButton").addEventListener("click", (event) => { getStatus(); });
	byId("#readyButton").addEventListener("click", (event) => { setReady(); });
	byId("#standbyButton").addEventListener("click", (event) => { setStandby(); });
	byId("#launchButton").addEventListener("click", (event) => { launch(); });
	intervalId = setInterval(safetyCheck, 1000);
});

