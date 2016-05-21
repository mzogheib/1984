function fetchWeather(latitude, longitude) {
    // Retrieve the weather data
    var req = new XMLHttpRequest();
    req.open('GET', 'http://api.openweathermap.org/data/2.5/weather?' +
    'lat=' + latitude + '&lon=' + longitude + '&cnt=1' + '&APPID=' + appid, false);
		
    // Set timeout to 4 seconds (4000 milliseconds)
    req.timeout = 4000; 
    req.ontimeout = function () {
        console.log("Timed out!!!"); 	
        // Send a URL timeout error message back to display default text
        Pebble.sendAppMessage(
            { "KEY_MSG_TYPE": 91},
            function(e) { console.log("Message sent to Pebble successfully!"); },
            function(e) { console.log("Error sending message to Pebble!"); }
        );
    };
    req.onreadystatechange = function () {
        if (req.readyState === 4 && req.status === 200) {
            console.log("Response in onreadystatechange is " + req.responseText);
            // responseText contains a JSON object with weather info
            var json = JSON.parse(req.responseText);

            // City
            var city = json.name;
            console.log("City is " + city);
            // Today's sunrise
            var sunrise = json.sys.sunrise;
            console.log("Sunrise is " + sunrise);
            // Today's sunset
            var sunset = json.sys.sunset;  
            console.log("Sunset is " + sunset);
            // Current cloud cover
            var clouds = json.weather[0].id;
            console.log("Condition is " + clouds);
            // Temperature
            var temperature = Math.round(json.main.temp - 273.15);
            console.log("Temperature is " + temperature);

            // Assemble dictionary
            var dictionary = {
                "KEY_SUNRISE": sunrise,
                "KEY_SUNSET": sunset,
                "KEY_SKY": clouds,
                "KEY_TEMPERATURE": temperature
            };
            // Send to Pebble
            Pebble.sendAppMessage(dictionary,
                function(e) { console.log("Weather info sent to Pebble successfully!"); },
                function(e) { console.log("Error sending weather info to Pebble!"); }
            );
        }
    };
    console.log("Fetching weather");
    req.send(null);
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

// If cannot get location then don't send anything back. Default values will be retained on the C side.
function locationError(err) {
    console.warn('location error (' + err.code + '): ' + err.message);
    // Send a location timeout error message back to display default text
    Pebble.sendAppMessage(
        { "KEY_MSG_TYPE": 90},
        function(e) { console.log("Message sent to Pebble successfully!"); },
        function(e) { console.log("Error sending message to Pebble!"); }
    );
}

var locationOptions = {
    'timeout': 15000,
    'maximumAge': 60000
};

Pebble.addEventListener('ready', function (e) {
    console.log('JS connected status: ' + e.ready);
    window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    console.log(e.type);
});

Pebble.addEventListener('appmessage', function (e) {
    window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    console.log('Message received from Pebble.');
});