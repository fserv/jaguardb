import React, { Component } from 'react';

class SignupForm extends Component {
    constructor(props) {
        super(props)
        this.state = {
            username: '',
            email: '',
            password: '',
            passwordConfirmation: ''
        }
    }

    onChange = (e) => {
        this.setState({ [e.target.name]: e.target.value });
    }

    onSubmit = (e) => {
        e.preventDefault();
        this.props.userSignupRequest(this.state).then(
            (response) =>{},
            (error) => {console.dir(error)}
        );
    }

    render() {
        return (
            <form onSubmit={ this.onSubmit }>
                <h1>Join our community!</h1>

                <div className="form-group">
                    <label className="control-label">Username</label>

                    <input
                        defaultValue="1"
                        // value={ this.state.username }
                        onChange={ this.onChange }
                        type="text"
                        name="username"
                        className="form-control"
                    />
                </div>

                <div className="form-group">
                    <label className="control-label">Email</label>

                    <input
                        defaultValue="1@1.com"

                        // value={ this.state.email }
                        onChange={ this.onChange }
                        type="email"
                        name="email"
                        className="form-control"
                    />
                </div>

                <div className="form-group">
                    <label className="control-label">Password</label>

                    <input
                        defaultValue="1"
                        // value={ this.state.password }
                        onChange={ this.onChange }
                        type="password"
                        name="password"
                        className="form-control"
                    />
                </div>

                <div className="form-group">
                    <label className="control-label">Password Confirmation</label>

                    <input
                        defaultValue="1"
                        // value={ this.state.passwordConfirmation }
                        onChange={ this.onChange }
                        type="password"
                        name="passwordConfirmation"
                        className="form-control"
                    />
                </div>

                <div className="form-group">
                    <button className="btn btn-primary btn-lg">
                        Sign up
                    </button>
                </div>
            </form>
        );
    }
}

export default SignupForm;