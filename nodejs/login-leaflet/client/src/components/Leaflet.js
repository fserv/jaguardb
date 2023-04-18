import React, { Component } from 'react';
import { userSqlRequest } from '../actions/sendSql';
import { connect } from 'react-redux';



class Leaflet extends Component {
    constructor(props) {
        super(props)
        this.state = {
            sql: ''
        }
    }

    onChange = (e) => {
    this.setState({ [e.target.name]: e.target.value });
}

    onSubmit = (e) => {
        e.preventDefault();
        this.props.userSqlRequest(this.state).then(

            (response) => {
                const result = JSON.parse(response.data.split("=")[1]);
                // console.log("_____:", typeof (result.geometry.coordinates[0]));
                // console.log("_____:", result.geometry.coordinates[0]);
                this.props.history.push('/leaflet',{state: result.geometry.coordinates});
            }
            // (response) => console.log("_____:",response),
                // this.props.history.push('/leaflet')

    );
}


    // onSubmit = (e) => {
    //     e.preventDefault();
    //     this.props.userSigninRequest(this.state).then(
    //         (response) =>this.props.history.push('/navleaflet'),
    //     );
    // }

render() {
    return (
        <form onSubmit={ this.onSubmit }>
    <h1>Input a sql!</h1>
    <div className="form-group">
        <label className="control-label">Username</label>
        <input
            value={ this.state.username }
            onChange={ this.onChange }
            type="text"
            name="sql"
            className="form-control"
        />
        </div>



        <div className="form-group">
        <button className="btn btn-primary btn-lg">
        Search
    </button>
    </div>
    </form>
);
}
}

export default connect(null, { userSqlRequest })(Leaflet);