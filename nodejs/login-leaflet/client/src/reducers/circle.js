import { DRAW_CIRCLE } from '../contants/index';


const circle = (action) => {
    return {
        lat:action.lat,
        lon:action.lon,
        rad:action.rad,
        id:Math.random()
    }
};

const circles = (state = [], action = {}) => {
    console.log("reducers!!!")
    switch (action.type) {
        case DRAW_CIRCLE:
            console.log(action);
            return(
                circle(action)
            )
        default: return state;
    }
}



export default circles;