import React, { Component } from 'react';
import { connect } from 'react-redux';
import PropTypes from 'prop-types';

import SigninForm from '../SigninForm';
import { userSigninRequest } from '../../actions/signinActions';

class SigninPage extends Component {
    static propTypes = {
        userSigninRequest: PropTypes.func.isRequired
    }

    render() {
        return (
            <div className="row">
                <div className="col-md-3"></div>
                <div className="col-md-6">
                    <SigninForm history = {this.props.history} userSigninRequest={ this.props.userSigninRequest } />
                </div>
                <div className="col-md-3"></div>
            </div>
        );
    }
}

export default connect(null, { userSigninRequest })(SigninPage);