import React, { Component } from 'react';
import './App.css';
import 'bootstrap/dist/css/bootstrap.min.css';

class App extends Component {

   
    constructor(props) {
        super(props);
        this.state = {messages: [], input: "", count: 0};

        this.handleChange = this.handleChange.bind(this);
        this.handleSubmit = this.handleSubmit.bind(this);
      }


    componentDidMount() {
        console.log("starting...")
        fetch('/connect', {
            method: 'get',
            headers: {
              'Accept': 'application/json, text/plain, */*',
              'Content-Type': 'application/json'
            }}).then(response => {
                return response.json();
            }).then(text => {
                console.log(text);
                console.log(text["message"]);
                this.add_message("server message:", text["message"]);
                console.log(this.state.messages);
            });
    }

    add_message = (title, message) => {
      let arr = [];
      arr.push(title);
      arr.push(message);
      this.state.messages.push(arr)
      let new_count = this.state.count + 1;
      this.setState({count: new_count});
    } 

    handleChange(event) {
        this.setState({input: event.target.value});
    }

    handleSubmit(event) {
        console.log(this.state.input)
        fetch('/write', {
            method: 'post',
            headers: {
              'Accept': 'application/json, text/plain, */*',
              'Content-Type': 'application/json'
            },
            body: JSON.stringify({input: this.state.input})
            }).then(response => {
                return response.json();
            }).then(text => {
                console.log(text);
                console.log(text["message"]);
                this.add_message("User Input:", this.state.input);
                this.add_message("server message:", text["message"]);
                console.log(this.state.messages);
            });
        event.preventDefault();
    }




  render() {
    return (
      <div className="App">
        <header className="App-header">
          <div className="container-fluif">
            <div className="row">
              <div className="col-6">
                <img id="twitter_image" src="/assets/twitter_icon.png" className="card-img-top" alt="..." />
              </div>
              <div className="col-6">
                <h2 id="title"> Welcome to Twitver! </h2>
              </div>
            </div>
          </div>

          <div className="list-group">
            <ul>
              {this.state.messages.map((value, index) => {
                return (
                  <a href="#" className="list-group-item list-group-item-action">
                    <div className="d-flex w-100 justify-content-between">
                      <h5 className="mb-1">{value[0]}</h5>
                    </div>
                    <p className="mb-1">{value[1]}</p>
                  </a>
                )})}
            </ul>
          </div>

          <form onSubmit={this.handleSubmit}>
            <div className="form-group">
              <label htmlFor="exampleInputEmail1">Your Input:</label>
              <input value={this.state.input} onChange={this.handleChange} type="text" className="form-control" id="exampleInputEmail1" aria-describedby="emailHelp"/>
            </div>
            <button type="submit" className="btn btn-primary">Submit</button>
          </form>
        </header>
      </div>
    );
  }


}

export default App;
