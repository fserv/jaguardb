import {DRAW_CIRCLE} from '../contants';

export const drawCircle = (circledata) => {
    return{
        type:DRAW_CIRCLE,
        lat:circledata.lat,
        lon:circledata.lon,
        rad:circledata.rad
    }
};