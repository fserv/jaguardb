import { SEND_SQL } from '../contants/index';


const sql = (action) => {
    return {
        sql:action.sql,
        id:Math.random()
    }
};

const sendsql = (state = [], action = {}) => {
    console.log("reducers!!!")
    switch (action.type) {
        case DRAW_CIRCLE:
            console.log(action);
            return(
                sql(action)
            )
        default: return state;
    }
}



export default sendsql;