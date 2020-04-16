const io = require('socket.io-client');
var socket = io.connect('http://localhost:55248');

// second attempt at implementation
class Ws {
  get newClientPromise() {
    return new Promise((resolve, reject) => {
      let wsClient = new WebSocket("ws://localhost:55248/");
      console.log(wsClient)
      wsClient.onopen = () => {
        console.log("connected");
        resolve(wsClient);
      };
      wsClient.onerror = error => reject(error);
    })
  }
  get clientPromise() {
    if (!this.promise) {
      this.promise = this.newClientPromise
    }
    return this.promise;
  }
}


 window.wsSingleton = new Ws();
      window.wsSingleton.clientPromise
      .then( wsClient =>{wsClient.send('username:tiago'); console.log('sended')})
      .catch( error => alert(error) );