import React, { Component } from 'react';
import './App.css';
import { w3cwebsocket as W3CWebSocket } from "websocket";

const client = new W3CWebSocket('ws://127.0.0.1:55248');
client.onopen = function() {
    console.log('WebSocket Client Connected');
 
    function sendName() {
        if (client.readyState === client.OPEN) {
            var name = "username:tiago";
            client.send(name);
            setTimeout(sendName, 1000);
        }
    }
    sendName();
};
client.onmessage = (message) => {
  console.log(message);
};

class App extends Component {



  componentDidMount() {
      console.log("beginning connection...");
      client.onopen();
    }


  render() {
    return (
      <div className="App">
        <header className="App-header">
          <p>
            Edit <code>src/App.js</code> and save to reload.
          </p>
          <a
            className="App-link"
            href="https://reactjs.org"
            target="_blank"
            rel="noopener noreferrer"
          >
            Learn React
          </a>
        </header>
      </div>
    );
  }


}

export default App;
