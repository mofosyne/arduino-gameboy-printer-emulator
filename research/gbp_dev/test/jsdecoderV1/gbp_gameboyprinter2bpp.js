/*
Gameboy Printer Render via "Gameboy 2BPP Graphics Format". 2017-4-16

Original Tile Decoder From: http://www.huderlem.com/demos/gameboy2bpp.html By Marcus

Heavily adapted to work with gameboy camera and gameboy printer data By Brian Khuu

*/

window.onload = function() {
    // Tile Constants
    TILE_PIXEL_WIDTH = 8;
    TILE_PIXEL_HEIGHT = 8;
    TILES_PER_LINE = 20; // Gameboy Printer Tile Constant

    var canvas = document.getElementById("demo_canvas");

    /* Determine size of each pixel in canvas */
    square_width = canvas.width / (TILE_PIXEL_WIDTH * TILES_PER_LINE);
    square_height = square_width;

    // Change below for other palettes
    colors = new Array("#ffffff", "#aaaaaa", "#555555", "#000000");
    // Very green
    //          colors = new Array("#9BBC0F", "#77A112", "#306230", "#0F380F");
    // GBP greenscale
    //          colors = new Array("#e0f8d0", "#88c070", "#346856", "#081820");
    // Dirtyboy palette
    //          colors = new Array("#c4cfa1", "#8b956d", "#4d533c", "#1f1f1f");
    // Grafxkid GBP gray
    //          colors = new Array("#e0dbcd", "#a89f94", "#706b66", "#2b2b26");
    // Grafxkid GBP green
    //          colors = new Array("#dbf4b4", "#abc396", "#7b9278", "#4c625a");
    // PJ Gameboy palette
    //          colors = new Array("#c4cfa1", "#8b956d", "#4d533c", "#1f1f1f");


    button = document.getElementById("submit_button");
    data = document.getElementById("data_text")

    button.addEventListener("click", function(evt){
        refresh(canvas, data.value);
    }, false);

    // Initial Render
    refresh(canvas, data.value);
}

function refresh(canvas, rawBytes)
{
    data.removeAttribute("style");  // Clear Error Indicator

    if (!render_gbp(canvas, rawBytes)) {
        data.style.backgroundColor = "red"; // Trigger error status
    }

}

function render_gbp(canvas, rawBytes)
{   // Returns false on error
    var status = true;

    // Clear Screen
    var ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // rawBytes is a string of hex where each line represents a gameboy tile
    var tiles_rawBytes_array = rawBytes.split(/\n/);

    /* Dry run, to find height */

    var total_tile_count = 0;

    for (var tile_i = 0; tile_i < tiles_rawBytes_array.length; tile_i++)
    {   // Process each gameboy tile
        tile_element = tiles_rawBytes_array[tile_i];

        // Check for invalid raw lines
        if (tile_element.length == 0)
        {   // Skip lines with no bytes (can happen with .split() )
            continue;
        }
        else if (/[^0-9a-z]/i.test(tile_element[0]) == true)
        {   // Skip lines used for comments
            continue;
        }

        // Increment Tile Count Tracker
        total_tile_count++;
    }

    var tile_height_count = Math.floor(total_tile_count / TILES_PER_LINE);

    // Resize height (Setting new canvas size will reset canvas)
    canvas.width = square_width * TILE_PIXEL_WIDTH * TILES_PER_LINE;
    canvas.height = square_height * TILE_PIXEL_HEIGHT * tile_height_count;

    console.log("lol"+canvas.height);

    /* Render Screen Tile by Tile */

    var tile_count = 0, tile_x_offset = 0, tile_y_offset = 0;

    for (var tile_i = 0; tile_i < tiles_rawBytes_array.length; tile_i++)
    {   // Process each gameboy tile
        tile_element = tiles_rawBytes_array[tile_i];

        // Check for invalid raw lines
        if (tile_element.length == 0)
        {   // Skip lines with no bytes (can happen with .split() )
            continue;
        }
        else if (/[^0-9a-z]/i.test(tile_element[0]) == true)
        {   // Skip lines used for comments
            console.log(tile_element)
            continue;
        }

        // Gameboy Tile Offset
        tile_x_offset = tile_count % TILES_PER_LINE;
        tile_y_offset = Math.floor(tile_count / TILES_PER_LINE);

        pixels = decode(tile_element);

        if (pixels)
        {
            paint(canvas, pixels, square_width, square_height, tile_x_offset, tile_y_offset);
        }
        else
        {
            status = false;
        }

        // Increment Tile Count Tracker
        tile_count++;
    }

    return status;
}

function decode(rawBytes)
{   // Gameboy tile decoder function from http://www.huderlem.com/demos/gameboy2bpp.html
    var bytes = rawBytes.replace(/ /g, "");
    if (bytes.length != 32) return false;

    var byteArray = new Array(16);
    for (var i = 0; i < byteArray.length; i++) {
        byteArray[i] = parseInt(bytes.substr(i*2, 2), 16);
    }

    var pixels = new Array(TILE_PIXEL_WIDTH*TILE_PIXEL_HEIGHT);
    for (var j = 0; j < TILE_PIXEL_HEIGHT; j++) {
        for (var i = 0; i < TILE_PIXEL_WIDTH; i++) {
            var hiBit = (byteArray[j*2 + 1] >> (7-i)) & 1;
            var loBit = (byteArray[j*2] >> (7-i)) & 1;
            pixels[j*TILE_PIXEL_WIDTH + i] = (hiBit << 1) | loBit;
        }
    }
    return pixels;
}

function paint(canvas, pixels, pixel_width, pixel_height, tile_x_offset, tile_y_offset )
{   // This paints the tile with a specified offset and pixel width

    tile_offset     = tile_x_offset * tile_y_offset;
    pixel_x_offset  = TILE_PIXEL_WIDTH   * tile_x_offset * pixel_width;
    pixel_y_offset  = TILE_PIXEL_HEIGHT  * tile_y_offset * pixel_height;

    var ctx = canvas.getContext("2d");

    for (var i = 0; i < TILE_PIXEL_WIDTH; i++)
    {   // pixels along the tile's x axis
        for (var j = 0; j < TILE_PIXEL_HEIGHT; j++)
        {   // pixels along the tile's y axis

            // Pixel Color
            ctx.fillStyle = colors[pixels[j*TILE_PIXEL_WIDTH + i]];

            // Pixel Position (Needed to add +1 to pixel width and height to fill in a gap)
            ctx.fillRect(
                    pixel_x_offset + i*pixel_width,
                    pixel_y_offset + j*pixel_height,
                    pixel_width + 1 ,
                    pixel_height + 1
                );
        }
    }
}
