A simple puzzle game inspired by Sokoban

Build and run:
```bash
# For web
emcmake cmake -B build -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Debug
cd build && make && cd sokoban
sudo npm install -g live-server
live-server --entry-file=sokoban.html

# For desktop
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cd build && make && cd sokoban
./sokoban
```

Credits:
- Levels from [here](https://sokoban.dk/levels/levels-the-download-page/)
- 3D models from [here](https://sona-sar.itch.io/voxel-animals-items-pack-free-assets)
- Font from [here](https://www.fontspace.com/super-playful-font-f118059)
- Background music from [here](https://silentswimmer.itch.io/toymaker)
- Sound effects from [here](https://opengameart.org/content/12-player-movement-sfx)
  and [here](https://pixabay.com/sound-effects/search/pop/)

Just for fun :)
