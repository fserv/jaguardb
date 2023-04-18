import React, { Component } from 'react';
import { connect } from 'react-redux';
import PropTypes from 'prop-types';

import SignupForm from '../SignupForm';
import { userSignupRequest } from '../../actions/signupActions';

class SignupPage extends Component {
    static propTypes = {
        userSignupRequest: PropTypes.func.isRequired
    }

    render() {
        return (
            <div className="row">
                <div className="col-md-3"></div>
                <div className="col-md-6">
                    <SignupForm history = {this.props.history} userSignupRequest={ this.props.userSignupRequest } />
                </div>
                <div className="col-md-3"></div>
            </div>
        );
    }
}

export default connect(null, { userSignupRequest })(SignupPage);