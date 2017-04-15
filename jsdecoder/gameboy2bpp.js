window.onload = function() {
    // Tile Constants
    TILE_PIXEL_WIDTH = 8;
    TILE_PIXEL_HEIGHT = 8;

    // Gameboy Printer Tile Constant
    TILES_PER_LINE = 20;
    LINES_PER_IMAGE = 40;

    square_width = document.getElementById("demo_canvas").width / (TILE_PIXEL_WIDTH * TILES_PER_LINE);
    square_height = document.getElementById("demo_canvas").height / (TILE_PIXEL_HEIGHT * LINES_PER_IMAGE);

    colors = new Array("#ffffff", "#aaaaaa", "#555555", "#000000");

    var canvas = document.getElementById("demo_canvas");

    button = document.getElementById("submit_button");
    data = document.getElementById("data_text")

    button.addEventListener("click", function(evt){
        refresh(canvas, data.value);
    }, false);

    //var bytes = "FF 00 7E FF 85 81 89 83 93 85 A5 8B C9 97 7E FF";
    //refresh(canvas, bytes);
}

function refresh(canvas, rawBytes) 
{
    data.removeAttribute("style");  // Clear Error Indicator

    var tiles_rawBytes_array = rawBytes.split(/\n/);

    console.log(tiles_rawBytes_array);

    for (var tile_i = 0; tile_i < tiles_rawBytes_array.length; tile_i++) 
    {   // Process each gameboy tile

        tile_x_offset = tile_i % TILES_PER_LINE;
        tile_y_offset = Math.floor(tile_i / TILES_PER_LINE);

        pixels = decode(tiles_rawBytes_array[tile_i]);
        // console.log(pixels);
        if (pixels) {
            paint(canvas, pixels, square_width, square_height, tile_x_offset, tile_y_offset);
        } else {
            data.style.backgroundColor = "red"; // Trigger error status
        }
    }
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
{   // This paints the tile

    tile_offset     = tile_x_offset * tile_y_offset;
    pixel_x_offset  = TILE_PIXEL_WIDTH   * tile_x_offset * pixel_width;
    pixel_y_offset  = TILE_PIXEL_HEIGHT  * tile_y_offset * pixel_height;

    var ctx = canvas.getContext("2d");
    //ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (var i = 0; i < TILE_PIXEL_WIDTH; i++) 
    {   // pixels along the tile's x axis
        for (var j = 0; j < TILE_PIXEL_HEIGHT; j++) 
        {   // pixels along the tile's y axis

            // Pixel Color
            ctx.fillStyle = colors[pixels[j*TILE_PIXEL_WIDTH + i]];

            // Pixel Position
            ctx.fillRect(
                    pixel_x_offset + i*pixel_width, 
                    pixel_y_offset + j*pixel_height, 
                    pixel_width, 
                    pixel_height
                );
        }
    }
}
