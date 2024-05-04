target remote :3333
set remote hardware-watchpoint-limit 2
mon reset halt
maintenance flush register-cache
b tetris_game_loop_task
thb app_main