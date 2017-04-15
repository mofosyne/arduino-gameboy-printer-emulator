window.onload = function() {
    width = 8;
    height = 8;
    square_width = document.getElementById("demo_canvas").width / (width);
    square_height = document.getElementById("demo_canvas").height / (height);

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

function refresh(canvas, rawBytes) {
    data.removeAttribute("style");  // Clear Error Indicator

    var tiles_rawBytes_array = rawBytes.split(/\n/);

    console.log(tiles_rawBytes_array);

    for (var i = 0; i < tiles_rawBytes_array.length; i++) 
    {
        pixels = decode(tiles_rawBytes_array[i]);
        //console.log(pixels);
        if (pixels) {
            paint(canvas, pixels);
        } else {
            data.style.backgroundColor = "red";
        }
    }


}

function decode(rawBytes) {
    var bytes = rawBytes.replace(/ /g, "");
    if (bytes.length != 32) return false;
    
    var byteArray = new Array(16);
    for (var i = 0; i < byteArray.length; i++) {
        byteArray[i] = parseInt(bytes.substr(i*2, 2), 16);
    }

    var pixels = new Array(width*height);
    for (var j = 0; j < height; j++) {
        for (var i = 0; i < width; i++) {
            var hiBit = (byteArray[j*2 + 1] >> (7-i)) & 1;
            var loBit = (byteArray[j*2] >> (7-i)) & 1;
            pixels[j*width + i] = (hiBit << 1) | loBit;
        }
    }
    return pixels;
}

function paint(canvas, pixels) {
    var ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (var i = 0; i < width; i++) {
        for (var j = 0; j < height; j++) {
            ctx.fillStyle = colors[pixels[j*width + i]];
            ctx.fillRect(i*square_width, j*square_height, square_width, square_height);
        }
    }
}
