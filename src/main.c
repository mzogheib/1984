#include <pebble.h>

#define KEY_MSG_TYPE 0
  
#define GET_WEATHER 1
#define KEY_SUNRISE 10
#define KEY_SUNSET 11
#define KEY_SKY 12
#define KEY_TEMPERATURE 13

#define GET_USER_TEMP_RANGE 5
#define KEY_TEMP_COLD_MAX 50
#define KEY_TEMP_COOL_MAX 51
#define KEY_TEMP_WARM_MAX 52

#define ERR_LOC 90
#define ERR_URL 91
  
Window *my_window;
TextLayer *text_layer;
GFont custom_font;

// Seconds since epoch of sunrise and sunset
// Weather condition code and temperature
// Temperature ranges for cold, cool, warm, hot
static int seconds_sunrise, seconds_sunset;
static int clouds, temperature;
static int temp_cold_max, temp_cool_max, temp_warm_max;

// Text labels for time
const char *hour[] = {"nought","nought one","nought two","nought three","nought four","nought five","nought six","nought seven","nought eight","nought nine",
                      "ten","eleven","twelve","thirteen","fourteen","fifteen","sixteen","seventeen","eighteen","nineteen",
                      "twenty","twenty one","twenty two","twenty three"};
const char *mins[] = {"","nought one","nought two","nought three","nought four","nought five","nought six","nought seven","nought eight","nought nine",
                      "ten","eleven","twelve","thirteen","fourteen","fifteen","sixteen","seventeen","eighteen","nineteen",
                      "twenty","twenty one","twenty two","twenty three","twenty four","twenty five","twenty six","twenty seven","twenty eight","twenty nine",
                      "thirty","thirty one","thirty two","thirty three","thirty four","thirty five","thirty six","thirty seven","thirty eight","thirty nine",
                      "forty","forty one","forty two","forty three","forty four","forty five","forty six","forty seven","forty eight","forty nine",
                      "fifty","fifty one","fifty two","fifty three","fifty four","fifty five","fifty six","fifty seven","fifty eight","fifty nine"};
    

// Returns the time in sentence format
char *write_time(struct tm tick_time) {
    static char buffer[] = "nought eight nought eight";

    strcpy(buffer, hour[tick_time.tm_hour]);
    strcat(buffer, "\n");
    strcat(buffer, mins[tick_time.tm_min]);
    return (char *)buffer;
}

// Returns weather and some indication of bright/dark day/night
char *write_day_night_weather(struct tm tick_time) {
    static char buffer[] = "drizzly cold morning";
    static char light[] = "drizzly ";
    static char temp[] = "cold ";
    static char period[] = "morning";
    int now = time(NULL);
  
    // If it is cloudy, then it is cloudy. http://bugs.openweathermap.org/projects/api/wiki/Weather_Condition_Codes
    // Otherwise if sun has risen it is bright. Otherwise, it is dark.
    // Consider some tolerance either side of sunrise/sunset, e.g. dusky
    if(clouds<=232) {
        strcpy(light, "stormy ");
    } else if(clouds<=321) {
        strcpy(light, "drizzly ");
    } else if(clouds<=531) {
        strcpy(light, "rainy ");
    } else if(clouds<=622) {
        strcpy(light, "snowy ");		
    } else if(clouds>=800 && clouds<=801) {
        if(now>=seconds_sunrise && now<seconds_sunset) {
            strcpy(light, "bright ");
        } else {
            strcpy(light, "dark ");
        }
    } else if(clouds>=802 && clouds<=804) {
        strcpy(light, "cloudy ");		
    }
  
    // Temperature
    if(temperature<=temp_cold_max) {
        // As is
    } else if(temperature<=temp_cool_max) {
        strcpy(temp, "cool ");
    } else if(temperature<=temp_warm_max) {
        strcpy(temp, "warm ");
    } else {
        strcpy(temp, "hot ");
    }
  
    // Morning:  00:00-10:59    default
    // Day:      11:00-17:59
    // Night:    18:00-23:59    or evening if before sunset
    if(tick_time.tm_hour>=11 && tick_time.tm_hour<18) {
        strcpy(period,"day");
    } else if (tick_time.tm_hour>=18) {
        if(now>=seconds_sunrise && now<seconds_sunset) {
            strcpy(period,"evening");
        } else {
            strcpy(period,"night");
        }
    }
  
    // Assemble the output
    strcpy(buffer, light);
    strcat(buffer, temp);
    strcat(buffer, period); 
  
    return (char *)buffer;
}

// Return the complete text to display
char *write_text(struct tm tick_time) {
    // Confirm why static is needed for the buffer
    static char buffer[] = "It was a drizzly cold morning in September, and the clocks were striking nought eight nought eight";
    static char start[] = "It was a ";
    static char month[] = " in September";
    static char mid[] = ", and the clocks were striking\n";
  
    // Assemble the output
    strcpy(buffer, start);
    strcat(buffer, write_day_night_weather(tick_time));
    strftime(month, sizeof(month), " in %B", &tick_time);
    strcat(buffer, month);
    strcat(buffer, mid);
    strcat(buffer, write_time(tick_time));

    return (char *)buffer;     
}

// Send a message to the phone to request weather data to be sent
void update_weather() {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, KEY_MSG_TYPE, GET_WEATHER);
  
    // Send the message!
    app_message_outbox_send();
}

// Run this function at every tick of the clock, i.e. second or minute
static void handle_tick(struct tm *tick_time, TimeUnits units){  
	// Use bluetooth connection status to determine if can update weather...
  
    if(bluetooth_connection_service_peek()) {
        // Do online things
        APP_LOG(APP_LOG_LEVEL_INFO, "BT connected. Update weather.");
        update_weather();
    } else {
        // Do default things
        APP_LOG(APP_LOG_LEVEL_INFO, "BT disconnected. Use default text.");
        text_layer_set_text(text_layer, write_text(*tick_time));
    }

}

// Bluetooth handler
void handle_bluetooth(bool connected) {
	// nothing for now
}

// Process the dictionary sent from the phone
// e.g. save to variables
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Message recieved!");
	
    // Read first item
    Tuple *t = dict_read_first(iterator);

    // For all items
    while(t != NULL) {
        // Which key was received?
        switch(t->key) {
            case KEY_SUNRISE:
            seconds_sunrise = t->value->int32;
            break;
            case KEY_SUNSET:
            seconds_sunset = t->value->int32;
            break;
            case KEY_SKY:
            clouds = t->value->int32;
            APP_LOG(APP_LOG_LEVEL_INFO, "Sky in JS message: %d", clouds);
            break;
            case KEY_TEMPERATURE:
            temperature = t->value->int32;
            break;
            case KEY_MSG_TYPE:
            switch(t->value->int32) {
                case ERR_LOC:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Location timeout.");
                case ERR_URL:
                APP_LOG(APP_LOG_LEVEL_ERROR, "URL timeout");	
            }
            break;
            default:
            APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
            break;
        }

        // Look for next item
        t = dict_read_next(iterator);
    }	
	
    time_t temp_time = time(NULL); 
    struct tm *tick_time = localtime(&temp_time);
	
    text_layer_set_text(text_layer, write_text(*tick_time));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// Convenience function to create a text layer
static TextLayer *init_text_layer(GRect location, GColor colour, GColor background, GFont font, GTextAlignment alignment) {
  TextLayer *layer = text_layer_create(location);
  text_layer_set_text_color(layer, colour);
  text_layer_set_background_color(layer, background);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);
 
  return layer;
}

static void main_window_load() {
  Layer *window_layer = window_get_root_layer(my_window);

  // Init the layers and font. 
  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_18));
  text_layer = init_text_layer(GRect(5, 2, 134, 164), GColorBlack, GColorClear, custom_font, GTextAlignmentLeft);
  
  text_layer_set_text(text_layer, "It was a ...");

  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

void main_window_unload() {
  text_layer_destroy(text_layer);
  fonts_unload_custom_font(custom_font);
}

// Init function
void handle_init(void) {
    my_window = window_create();
  
    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(my_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
  
    window_stack_push(my_window, true);

    // Time for display on start up and default sunrise (6:00) & sunset (19:00)
    time_t temp_time = time(NULL); 
    struct tm *tick_time = localtime(&temp_time);
  
 
    // Set all the default values for the variables that affect the text
    seconds_sunrise = (int)temp_time - tick_time->tm_hour*60*60 - tick_time->tm_min*60 - tick_time->tm_sec + 6*60*60;
    seconds_sunset = (int)temp_time - tick_time->tm_hour*60*60 - tick_time->tm_min*60 - tick_time->tm_sec + 19*60*60;
    clouds = 800;
    temperature = 0;
    temp_cold_max = 15;
    temp_cool_max = 20;
    temp_warm_max = 25;
    // Any temp above warm is hot

    // Do default things
    text_layer_set_text(text_layer, write_text(*tick_time));

    // // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
 
    // On app load, weather will be grabbed from the event listner 'ready' on the JS side 
  
    // Subcribe to ticker 
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  
}

// Deinit function
void handle_deinit(void) {
    window_destroy(my_window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
