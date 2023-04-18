import axios from "axios";

export const userSignupRequest = (userData) => {
    return dispatch => {
        return axios.post('/api/users', userData);
    }
};