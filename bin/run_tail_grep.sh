make && sdl_player/xieps_sdl_player
sleep 2
tail -n 100 xieps_sdl_player.log
# Recommended: pipe output to something like: grep -Ei '^[a-z_]+:'
# for colorizing the output a bit (if your grep colorizes matches).
