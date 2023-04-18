import React from 'react';

import { Route } from 'react-router-dom';

import App from './components/App';
import requireAuth from './utils/requireAuth'

import SignupPage from './components/signup/SignupPage';
import SigninPage from './components/signin/SigninPage';
import Home from './components/Home';
import Map from './components/Map';
import NavLeaflet from './components/NavLeaflet';
import Circle from './components/Circle';
import Leaflet from './components/Leaflet';

export default (
    <div className="container">
        <Route exact path="/" component={ App } />
        <Route path="/signup" component={ SignupPage } />
        <Route path="/signin" component={ SigninPage } />
        <Route path="/home" component={ Home } />
        <Route path="/leaflet" component={ requireAuth(Map) } />
        <Route path="/navleaflet" component={requireAuth( NavLeaflet) } />
        <Route path="/circle" component={ requireAuth(Circle) } />
        <Route path="/sql" component={ requireAuth(Leaflet) } />
    </div>
)