import axios from "axios";
import setAuthorizationToken from '../utils/setAuthorizationToken'
import jwtDecode from 'jwt-decode'
import {SET_CURRENT_USER} from '../contants'

export const setCurrentUser = (user) => {
    return{
        type:SET_CURRENT_USER,
        user
    }
}

export const logout = (id) => {
    return dispatch => {
        localStorage.removeItem('jwtToken');
        setAuthorizationToken(false);
        dispatch(setCurrentUser());

    }
}

export const userSigninRequest = (userData) => {
    return dispatch => {
        return axios.post('/api/login', userData).then(res => {
            const token = res.data.token;
            localStorage.setItem("jwtToken", token);
            setAuthorizationToken(token);
            // console.log();
            dispatch(setCurrentUser(jwtDecode(token)))
        });
    }
};