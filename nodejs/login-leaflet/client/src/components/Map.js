import { Map as LeafletMap, TileLayer, Marker, Popup, Circle,Polygon,Polyline,Rectangle,GeoJSON} from 'react-leaflet';
import React, { Component } from 'react';
import { connect } from 'react-redux';


const style = {
    width: "100%",
    height: "700px"
};

class Map extends React.Component {

    render() {
        let tt = '';
        if (this.props.reminders.circle.lat != null) {
            console.log("test:", this.props.reminders.circle.lat);
            const latlen = this.props.reminders.circle || [0, 0];
            // console.log(this.props.location.state.state.lat)
            // const center = [this.props.location.state.state.lat,this.props.location.state.state.lon]
            // const radius = parseInt(this.props.location.state.state.rad)
            const center = [latlen.lat, latlen.lon];
            const radius = parseInt(latlen.rad);
            // console.log(typeof (radius))
            const multiPolygon = [
                [[51.51, -0.12], [51.51, -0.13], [51.53, -0.13]],
                [[51.51, -0.05], [51.51, -0.07], [51.53, -0.07]],
            ]
            const polygon = []
            const plyLine = []
            const rectAngle = []
            const geoJson = []

            tt = (<LeafletMap
                center={center}
                zoom={12}
                maxZoom={30}
                attributionControl={true}
                zoomControl={true}
                doubleClickZoom={true}
                scrollWheelZoom={true}
                dragging={true}
                animate={true}
                easeLinearity={0.35}
                style={style}
            >
                <TileLayer
                    url='http://{s}.tile.osm.org/{z}/{x}/{y}.png'
                />
                <Marker position={[50, -0.01]}>
                    <Popup>
                        Popup for any custom information.
                    </Popup>
                </Marker>
                <Circle center={center} radius={radius}/>
                <Polygon color="purple" positions={multiPolygon}/>
            </LeafletMap>)
        }
        return (
            <div>
                {tt}
            </div>
        );
    }
}

const mapStateToProps = (state) => {
    return {
        reminders: state
    };
};

export default connect(mapStateToProps)(Map);
