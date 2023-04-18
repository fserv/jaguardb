import { combineReducers } from 'redux'

import auth from './auth';

import circle from './circle'

export default combineReducers({
    auth,
    circle
});