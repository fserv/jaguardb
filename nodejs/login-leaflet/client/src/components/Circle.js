import React, { Component } from 'react';
import Leaflet from './Leaflet';
import {connect} from 'react-redux';
import {drawCircle} from '../actions/drawCircle';


class Circle extends Component {
    constructor(props) {
        super(props)
        this.state = {
            lat: '',
            lon: '',
            rad: ''
        }
    }

    drawCircle(){
        this.props.drawCircle(this.state)
    }

    onChange = (e) => {
        this.setState({ [e.target.name]: e.target.value });
    }

    onSubmit = (e) => {
        // e.target.data = this.state;
        // console.log(this.props.reminders);
        this.drawCircle(this.state);
        this.props.history.push('/leaflet');
        // this.props.history.push('/leaflet',{state: this.state});
        // console.log(this.state);
    }

    render() {
        return (
            <div>
                <form onSubmit={ this.onSubmit }>
                    <h1>Draw a circle!</h1>

                    <div className="form-group">
                        <label className="control-label">Latitude</label>
                        <input
                            value={ this.state.lat }
                            onChange={ this.onChange }
                            type="text"
                            name="lat"
                            className="form-control"
                        />
                    </div>


                    <div className="form-group">
                        <label className="control-label">Longitude</label>
                        <input
                            value={ this.state.lon }
                            onChange={ this.onChange }
                            type="text"
                            name="lon"
                            className="form-control"
                        />
                    </div>

                    <div className="form-group">
                        <label className="control-label">Radius</label>
                        <input
                            value={ this.state.rad }
                            onChange={ this.onChange }
                            type="text"
                            name="rad"
                            className="form-control"
                        />
                    </div>


                    <div className="form-group">
                        <button
                            // type="button"
                            className="btn btn-primary btn-lg"
                        >
                            Draw
                        </button>
                    </div>
                </form>
            </div>
        );
    }
}

const mapStateToProps = (state) => {
    return {
        reminders: state
    };
};

export default connect(mapStateToProps,{drawCircle})(Circle);