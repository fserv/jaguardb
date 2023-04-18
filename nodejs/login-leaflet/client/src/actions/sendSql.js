import axios from "axios";
import setAuthorizationToken from '../utils/setAuthorizationToken'
import {SEND_SQL} from '../contants'

export const setCurrentUser = (user) => {
    return{
        type:SEND_SQL,
        user
    }
}

export const userSqlRequest = (userData) => {
    return dispatch => {
        return axios.post('/api/leaflet', userData)
    //         .then(res => {
    //         console.log("re:");
    //         console.log(res.data);
    // });
    }
};