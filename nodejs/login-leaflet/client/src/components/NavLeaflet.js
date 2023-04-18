import React, {Component} from 'react';
import PropTypes from 'prop-types';
import {Link} from "react-router-dom";

class NavLeaflet extends Component {
    render() {
        return (
            <div>
                <nav className="navbar navbar-expand-lg navbar-light bg-light mb-3">
                    <div className="container">
                        <button className="navbar-toggler" type="button" data-toggle="collapse" data-target="#navbarsExample05" aria-controls="navbarsExample05" aria-expanded="false" aria-label="Toggle navigation">
                            <span className="navbar-toggler-icon"></span>
                        </button>

                        <div className="collapse navbar-collapse" id="navbarsExample05">
                            <ul className="navbar-nav mr-auto">
                                <li className="nav-item">
                                    <Link className="nav-link" to="/circle">Circle</Link>
                                </li>

                                <li className="nav-item">
                                    <Link className="nav-link" to="/rectangle">Ractangle</Link>
                                </li>

                                <li className="nav-item">
                                    <Link className="nav-link" to="/polygon">Polygon</Link>
                                </li>

                                <li className="nav-item">
                                    <Link className="nav-link" to="/sql">Sql</Link>
                                </li>

                            </ul>
                        </div>
                    </div>
                </nav>
            </div>
        );
    }
}

NavLeaflet.propTypes = {};

export default NavLeaflet;
