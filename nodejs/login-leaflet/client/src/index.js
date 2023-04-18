import React from 'react';
import ReactDOM from 'react-dom';
import App from './components/App';
import NavigationBar from './components/NavigationBar';
import registerServiceWorker from './registerServiceWorker';

import logger from 'redux-logger';
import { composeWithDevTools } from 'redux-devtools-extension';
import thunk from 'redux-thunk';

import { BrowserRouter as Router } from 'react-router-dom';
import routes from './routes';

import { createStore, applyMiddleware } from 'redux';

import rootReducer from './reducers';

import { Provider } from 'react-redux';
import setAuthorizationToken from './utils/setAuthorizationToken'
import jwtDecode from "jwt-decode";
import {setCurrentUser} from "./actions/signinActions";

const store = createStore(
    rootReducer,
    composeWithDevTools(
        applyMiddleware(thunk, logger)
    )
);

if(localStorage.jwtToken){
    setAuthorizationToken(localStorage.jwtToken);
    store.dispatch(setCurrentUser(jwtDecode(localStorage.jwtToken)))
}


ReactDOM.render(
    <Provider store={ store }>
        <Router routes={ routes }>
            <div>
                <NavigationBar />
                { routes }
            </div>
        </Router>
    </Provider>,
    document.getElementById('root')
);
registerServiceWorker();