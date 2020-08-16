## Octave/Matlab Tile Decoder

Rapha�l BOICHOT contributed a decoder written in octave/matlab that can parse
the raw packet mode output of gbp_emulator_v2.

### V1 - 2020-08-16

First release. To use it, copy over the output to `New_Format.txt` and run the
octave program `Arduino_Game_Boy_Printer_decoder_compression_palette.m`.

#### Compatibility

|  **Title** | **System** | **Supported** | **TX Rate** | **Compression** | **Palette** |
| :--- | :---: | :---: | :---: | :---: | :---: |
|  1942 | GBC | Yes | Normal | No | Custom |
|  Alice in Wonderland | GBC | Yes | Normal | No | Custom |
|  Asteroids | GB/GBC |  |  |  |  |
|  Austin Powers: Oh, Behave! | GBC | Yes | Normal | No | Inverted |
|  Austin Powers: Welcome to My Underground Lair! | GBC | Yes | Normal | No | Inverted |
|  Cardcaptor Sakura: Itsumo Sakura-chan to Issho! | GB/GBC | Yes | Normal | No | Standard |
|  Cardcaptor Sakura: Tomoe Shōgakkō Daiundōkai | GBC | Yes | Normal | No | Inverted |
|  Disney's Dinosaur | GBC | Yes | Normal | No | Standard |
|  Disney's Tarzan | GBC | Yes | Normal | No | Custom |
|  Donkey Kong Country | GBC | Yes | Normal | No | Standard |
|  E.T.: Digital Companion | GBC | Yes | Normal | No | Standard |
|  Fisher-Price Rescue Heroes: Fire Frenzy | GBC | Yes | Normal | No | Standard |
|  Game Boy Camera | GB | Yes | Normal | No | Standard |
|  Harvest Moon 2 | GB/GBC | Yes | Normal | No | Standard |
|  Kakurenbo Battle Monster Tactics | GBC |  |  |  |  |
|  Klax | GBC | Yes | Normal | No | Standard |
|  The Legend of Zelda: Link's Awakening DX | GB/GBC | Yes | Double | No | Standard |
|  The Little Mermaid 2: Pinball Frenzy | GBC | Yes | Normal | No | Inverted |
|  Little Nicky | GBC | Yes | Normal | No | Custom |
|  Logical | GBC | Yes | Normal | No | Standard |
|  Magical Drop | GBC | Yes | Normal | No | Standard |
|  Mary-Kate and Ashley Pocket Planner | GB/GBC | Yes | Normal | No | Standard |
|  Mickey's Racing Adventure | GBC | Yes | Normal | No | Standard |
|  Mickey's Speedway USA | GBC | Yes | Normal | No | Standard |
|  Mission: Impossible | GBC | Yes | Normal | No | Standard |
|  NFL Blitz | GB/GBC | Yes | Normal | No | Standard |
|  Perfect Dark | GBC | Yes | Normal | No | Custom |
|  Pokémon Crystal | GBC | Yes | Normal | No | Standard |
|  Pokémon Gold and Silver (except Korean versions) | GB/GBC | Yes | Normal | No | Standard |
|  Pokémon Pinball | GB/GBC | Yes | Normal | No | Standard |
|  Pokémon Trading Card Game | GB/GBC | Yes | Normal | Yes | Standard |
|  Pokémon Card GB2: Great Rocket-Dan Sanjō! | GB/GBC | Yes | Normal | Yes | Standard |
|  Pokémon Red and Blue | GB | Yes | Normal | No | Standard |
|  Pokémon Yellow: Special Pikachu Edition | GB/GBC | Yes | Normal | No | Standard |
|  Puzzled | GBC | Yes | Normal | No | Standard |
|  Quest for Camelot | GB/GBC | Yes | Normal | No | Standard |
|  Roadsters Trophy | GBC | Yes | Normal | No | Inverted |
|  Super Mario Bros. Deluxe | GBC | Yes | Double | No | Standard |
|  Tony Hawk's Pro Skater 2 | GBC | Yes | Double | No | Inverted |
|  Trade & Battle: Card Hero | GB/GBC |  |  |  |  |
|  Tales of Phantasia: Nakiri's Dungeon (Japan) | GB/GBC |  |  |  |  |
|  Hamster Club (Japan) |  |  |  |  |  |
|  Hamster Paradise (Japan) |  |  |  |  |  |
|  Monster Race 2 (Japan) |  |  |  |  |  |
|  Pro Mahjong Tsuwamono (Japan) |  |  |  |  |  |
|  Miracle of the Zone 2 (Japan) |  |  |  |  |  |
|  Cross Hunter Treasure Hunter (Japan) |  |  |  |  |  |
|  Animal Breeder 3 (Japan) |  |  |  |  |  |
|  KAWAII KOINU Nakayoshi Pet Series 3 (Japan) |  |  |  |  |  |
|  Sanrio Time Net Future (Japan) |  |  |  |  |  |
|  Hello Kitty No Sanrio Time Net (Japan) |  |  |  |  |  |
|  Sylvanian Families 2 (Japan) |  |  |  |  |  |
|  Cake Wo Tsukurou (Japan) |  |  |  |  |  |
|  Super Black Bass Pocket 3 (Japan) |  |  |  |  |  |
|  Love Hina Pocket (Japan) |  |  |  |  |  |