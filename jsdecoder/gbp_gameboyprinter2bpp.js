/*
Gameboy Printer Render via "Gameboy 2BPP Graphics Format". 2017-4-16

Original Tile Decoder From: http://www.huderlem.com/demos/gameboy2bpp.html By Marcus

Heavily adapted to work with gameboy camera and gameboy printer data By Brian Khuu

*/

// Tile Constants
TILE_PIXEL_WIDTH = 8;
TILE_PIXEL_HEIGHT = 8;
TILES_PER_LINE = 20; // Gameboy Printer Tile Constant

document.addEventListener('DOMContentLoaded', function ()
{

    var canvas = document.getElementById("demo_canvas");

    var button = document.getElementById("submit_button");

    button.addEventListener("click", function (evt)
    {
        refresh();
    }, false);

    // Initial Render
    refresh();
});

function refresh()
{
    var data = document.getElementById("data_text");
    data.classList.remove('error');  // Clear Error Indicator

    try {
        render_gbp(data.value)
    } catch (error)
    {
        data.classList.add('error');// Trigger error status
    }
}

function newCanvas()
{
    var container = document.createElement('div');
    var canvas = document.createElement('canvas');
    var downloadButton = document.createElement('button');

    container.className = 'image-container';

    canvas.width = 480;
    downloadButton.innerText = 'Download';

    downloadButton.addEventListener('click', download);

    document.getElementById('images').appendChild(container);
    container.appendChild(canvas);
    container.appendChild(downloadButton);
    return canvas;
}

function renderImage(tiles)
{
    var tile_height_count = Math.floor(tiles.length / TILES_PER_LINE);
    var canvas = newCanvas();

    // /* Determine size of each pixel in canvas */
    var square_width = canvas.width / (TILE_PIXEL_WIDTH * TILES_PER_LINE);
    var square_height = square_width;

    var ctx = canvas.getContext("2d");

    // Resize height (Setting new canvas size will reset canvas)
    canvas.width = square_width * TILE_PIXEL_WIDTH * TILES_PER_LINE;
    canvas.height = square_height * TILE_PIXEL_HEIGHT * tile_height_count;

    tiles.forEach(function (pixels, index)
    {
        // Gameboy Tile Offset
        var tile_x_offset = index % TILES_PER_LINE;
        var tile_y_offset = Math.floor(index / TILES_PER_LINE);
        paint(canvas, pixels, square_width, square_height, tile_x_offset, tile_y_offset);
    })
}

function render_gbp(rawBytes)
{
    // clear all images
    document.getElementById('images').innerHTML = '';

    // rawBytes is a string of hex where each line represents a gameboy tile
    var tiles_rawBytes_array = rawBytes.split(/\n/);

    var tile_count = 0

    var images = [];
    var currentImage = null;

    tiles_rawBytes_array
        .map(function (raw_line)
        {
            if ((raw_line.charAt(0) === '#')) return null;

            if ((raw_line.charAt(0) === '!'))
            {
                var command = JSON.parse(raw_line.slice(1).trim());
                if (command.command === 'INIT')
                {
                    return 'INIT'
                }
            }
            return (decode(raw_line));
        })
        .filter(Boolean)
        .forEach(function (tile_element)
        {
            if ((tile_element === 'INIT'))
            {
                currentImage = [];
                images.push(currentImage);
            } else {
                currentImage.push(tile_element);
            }
        })

    images.forEach(renderImage);
}

// Gameboy tile decoder function from http://www.huderlem.com/demos/gameboy2bpp.html
function decode(rawBytes)
{
    var bytes = rawBytes.replace(/[^0-9A-F]/ig, '');
    if (bytes.length != 32) return false;

    var byteArray = new Array(16);
    for (var i = 0; i < byteArray.length; i++)
    {
        byteArray[i] = parseInt(bytes.substr(i * 2, 2), 16);
    }

    var pixels = new Array(TILE_PIXEL_WIDTH * TILE_PIXEL_HEIGHT);
    for (var j = 0; j < TILE_PIXEL_HEIGHT; j++)
    {
        for (var i = 0; i < TILE_PIXEL_WIDTH; i++)
        {
            var hiBit = (byteArray[j * 2 + 1] >> (7 - i)) & 1;
            var loBit = (byteArray[j * 2] >> (7 - i)) & 1;
            pixels[j * TILE_PIXEL_WIDTH + i] = (hiBit << 1) | loBit;
        }
    }
    return pixels;
}

// This paints the tile with a specified offset and pixel width
function paint(canvas, pixels, pixel_width, pixel_height, tile_x_offset, tile_y_offset)
{

    var e = document.getElementById("palette");
    var palette = e.options[e.selectedIndex].value;

    switch (palette)
    {
        case "grayscale":
            colors = new Array("#ffffff", "#aaaaaa", "#555555", "#000000");
            break;
        case "dmg":
            colors = new Array("#9BBC0F", "#77A112", "#306230", "#0F380F");
            break;
        case "gameboypocket":
            colors = new Array("#c4cfa1", "#8b956d", "#4d533c", "#1f1f1f");
            break;
        case "gameboycoloreuus":
            colors = new Array("#ffffff", "#7bff30", "#0163c6", "#000000");
            break;
        case "gameboycolorjp":
            colors = new Array("#ffffff", "#ffad63", "#37332f", "#000000");
            break;
        case "bgb":
            colors = new Array("#e0f8d0", "#88c070", "#346856", "#081820");
            break;
        case "grafixkidgray":
            colors = new Array("#e0dbcd", "#a89f94", "#706b66", "#2b2b26");
            break;
        case "grafixkidgreen":
            colors = new Array("#dbf4b4", "#abc396", "#7b9278", "#4c625a");
            break;
        case "blackzero":
            colors = new Array("#7e8416", "#577b46", "#385d49", "#2e463d");
            break;
        default:
            colors = new Array("#ffffff", "#aaaaaa", "#555555", "#000000");
    }
    tile_offset = tile_x_offset * tile_y_offset;
    pixel_x_offset = TILE_PIXEL_WIDTH * tile_x_offset * pixel_width;
    pixel_y_offset = TILE_PIXEL_HEIGHT * tile_y_offset * pixel_height;

    var ctx = canvas.getContext("2d");

    // pixels along the tile's x axis
    for (var i = 0; i < TILE_PIXEL_WIDTH; i++)
    {
        for (var j = 0; j < TILE_PIXEL_HEIGHT; j++)
        {
            // pixels along the tile's y axis

            // Pixel Color
            ctx.fillStyle = colors[pixels[j * TILE_PIXEL_WIDTH + i]];

            // Pixel Position (Needed to add +1 to pixel width and height to fill in a gap)
            ctx.fillRect(
                pixel_x_offset + i * pixel_width,
                pixel_y_offset + j * pixel_height,
                pixel_width + 1,
                pixel_height + 1
            );
        }
    }
}

function download(event)
{

    var canvas = event.target.previousSibling;

    var currentdate = new Date();
    var filename = "Game Boy Photo "
        + currentdate.getFullYear()
        + addZero((currentdate.getMonth() + 1))
        + addZero(currentdate.getDate()) + " - "
        + addZero(currentdate.getHours()) + ""
        + addZero(currentdate.getMinutes()) + ""
        + addZero(currentdate.getSeconds())
        + ".jpg";
    var download = document.getElementById("download");
    var image = canvas.toDataURL("image/jpeg")
        .replace("image/jpeg", "image/octet-stream");
    download.setAttribute("href", image);
    download.setAttribute("download", filename);
    download.click();
}

function addZero(i)
{
    if (i < 10)
    {
        i = "0" + i;
    }
    return i;
}
