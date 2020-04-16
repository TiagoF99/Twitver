import React, { Component } from 'react';
import './App.css';


class App extends Component {

    constructor(props) {
        super(props);
        this.state = {message: "before"};
      }


    componentDidMount() {
        console.log("starting...")
        fetch('/test', {
            method: 'get',
            headers: {
              'Accept': 'application/json, text/plain, */*',
              'Content-Type': 'application/json'
            }}).then(response => {
                return response.json();
            }).then(text => {
                console.log(text);
                console.log(text["message"]);
                this.setState({message: text["message"]});
            });
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
          <p> message: {this.state.message} </p>
        </header>
      </div>
    );
  }


}

export default App;
