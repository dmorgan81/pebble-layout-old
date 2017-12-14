# pebble-layout

pebble-layout provides a separation between your code and your view. Writing and maintaining a UI layout in code is tedious; pebble-layout moves all that layout code into a familiar JSON format. Standard layer types like TextLayer and BitmapLayer are provided. Custom types can be added to extend pebble-layout's functionality.

# Installation

```
pebble package install pebble-layout
```

You must be using a new-style project; install the latest pebble tool and SDK and run `pebble convert-project` on your app if you aren't.

# Usage

```c
// This is not a complete example, but should demonstrate the basic usage

#include <pebble-layout/pebble-layout.h>

...
static Layout *s_layout;
...

static void prv_window_load(Window *window) {
    s_layout = layout_create();
    layout_add_all_standard_types(s_layout);
    layout_add_system_fonts(s_layout);

    layout_parse(s_layout, RESOURCE_ID_LAYOUT);
    layer_add_child(window_get_root_layer(window), layout_get_root_layer(s_layout));
    ...
}

static void prv_window_unload(Window *window) {
  layout_destroy(s_layout);
  ...
}
```

# JSON format

The format of a layout file is pretty simple. It's just standard JSON. For example:

```json
{
    "id": "root",
    "background": "#000000",
    "layers": [
        {
            "frame": [20, 10, 124, 128],
            "background": "#FF0000",
            "layers": [
                {
                    "id": "hello",
                    "type": "TextLayer",
                    "frame": [0, 0, 124, 128],
                    "text": "Hello World!",
                    "color": "#FFFFFF",
                    "alignment": "GTextAlignmentCenter",
                    "font": "GOTHIC_28_BOLD"
                }
            ]
        },
        {
            "frame": [0, 120, 144, 40],
            "background": "#00FF00"
        }
    ] 
}
```

The layout above will be two colored rectangles of different sizes with the text "Hello World!" centered along the top of the first rectangle. Two layers have IDs; we could use `layout_find_by_id()` to get a pointer to these layers if we needed to.

The only property that is required is `frame`. Without it a layer's frame defaults to GRectZero, which isn't useful since it won't draw anything. Layers can have IDs, as shown.

Untyped layers default to basic layers. An untyped layer can have child layers (the `layers` property). a background color which defaults to GColorClear if not specified, and a `clips` boolean property which acts just like `layer_set_clips()`.

TextLayers can have the following properties:

| Property | Pebble API equivalent |
|----------|-----------------------|
| text | `text_layer_set_text()` |
| background | `text_layer_set_background_color()` |
| color | `text_layer_set_text_color()` |
| alignment | `text_layer_set_text_alignment()` |
| overflow | `text_layer_set_overflow_mode()` |

Anything that takes an enum value takes the value as a string, like GTextAlignmentCenter or GTextOverflowModeFill.

BitmapLayers can have the following properties:

| Property | Pebble API equivalent | Notes |
|----------|-----------------------|-------|
| bitmap | `bitmap_layer_set_bitmap()` | Should be a name that was registered with `layout_add_resource()`
| background | `bitmap_layer_set_background_color()` |
| alignment | `bitmap_layer_set_alignment()` |
| compositing | `bitmap_layer_set_compositing_mode()` |

# pebble-layout API

| Method | Description |
|--------|---------|
| `Layout *layout_create(void)` | Create and initialize a Layout. No parsing has been done at this point.|
| `void layout_parse(Layout *this, uint32_t resource_id)` | Parse a JSON resource into a tree of layers.|
| `void layout_parse_string(Layout *this, char *json)` | Parse a JSON string into a tree of layers.|
| `void layout_destroy(Layout *this)` | Destroy a layout, including all parsed layers.|
| `Layer *layout_get_root_layer(Layout *this)` | Get the root layer of the layout. If parsing has not been done or parsing failed, this returns `NULL`.|
| `void *layout_find_by_id(Layout *this, char *id)` | Return a layer by its ID. The caller is responsible for casting to the correct type. If no layer exists with that ID, `NULL` is returned. |
| `void layout_add_font(Layout *this, char *name, uint32_t resource_id)` | Add a custom font that can referenced during parsing. The font will be loaded and unloaded automatically. Calling this function after parsing will have no effect.|
| `GFont layout_get_font(Layout *this, char *name)` | Return a custom font that was previously added.|
| `void layout_add_resource(Layout *this, char *name, uint32_t resource_id)` | Add a resource by its ID that can be referenced during parsing. Calling this function after parsing will have no effect.|
| `uint32_t *layout_get_resource(Layout *this, char *name)` | Return a previously added resource ID.|
| `void layout_add_all_standard_types(Layout *this)` | Make all standard types available during parsing.|
| `void layout_add_standard_type(Layout *this, StandardType type)` | Make the specified standard type available during parsing.|
| `void layout_add_type(Layout *this, char *type, LayoutFuncs layout_funcs)` | Add a custom type that can be used during parsing. See the section below on [custom types](#types).|

# Custom types[](#types)

pebble-layout can be extended by adding custom types before parsing. During parsing any layer with its `type` property set to a string you specify will be constructed/destroyed using the functions you specify.

Adding a type requires implementing four functions:
* `create`: `void* (Layout *layout, Json *json, JsonToken *token)` - Anything can be returned from this function. The result will be passed around to the other custom type functions so it's a good idea to make it a struct that holds everything you might need. See the section on the [JSON API](#json) for how to use `json` and `token`.
* `destroy`: `void (void *object)` - Standard cleanup. Destroy child layers, unload resources, free allocated memory, etc.
* `get_layer`: `Layer* (void *object)` - Must return a layer to add to the layer heirarchy.
* `set_frame`: `void (void *object, GRect frame)` - Set the frame of your layer.

# JSON API[](#json)

pebble-layout includes a simple JSON API that uses [Jsmn](https://github.com/zserge/jsmn) to handle parsing/tokenizing. The API iterates through the JSON structure, converting tokens into types automatically.

You will need to use the JSON API when implementing a custom type. The custom type create function gives you a `Json` object, which you will pass along to the various API functions, and a `JsonToken`, which holds information about the current token, like its type and size. The standard template for processing fields is as follows:

```c
static void *prv_my_custom_type_create(Layout *this, Json *json, JsonToken *token) {
    // token currently points at a JSON object.

    ... // Allocate some sort of struct to hold a layer plus all the parsed properties.

    int size = token->size;
    for (int i = 0; i < size; i++) {
        token = json_next(json); // Advance token to the next object property.
        if (json_eq(json, token, "color")) {
            GColor color = json_next_color(json); // Advance and parse the token.
            ... // Do something with color.
        } else if (json_eq(json, token, "foobar")) {
            token = json_next(json); // Now we have the property value.
            if (token->type == JSON_ARRAY) {
                int nsize = token->size;
                for (int j = 0; j < nsize; j++) {
                    tok = json_next(json);
                    ... // Do something with each nested token.
                }
            }
        } else {
            json_skip_tree(json); // Skip any properties we don't care about.
            // This is important to keep iteration in sync and prevent later parsing
            // from blowing up. So always end your if-else chain with json_skip_tree().
        }
    }

    ...
}
```
