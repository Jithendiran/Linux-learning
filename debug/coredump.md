# Core dump

## Enable core dump
> \# ulimit -c 
If the previous command return 0, no core dump enabled
> \# ulimit -c unlimited 
set max core dump file size to unlimited
> \# ulimit -c 
Verify by above command
> \# sudo nano /etc/security/limits.conf, append with file with below values
```
* soft core unlimited
* hard core unlimited
```

## set custom path Dump to /tmp with filename pattern
sudo sysctl -w kernel.core_pattern='/tmp/core.%e.%p.%t'
echo 'kernel.core_pattern=/tmp/core.%e.%p.%t' | sudo tee -a /etc/sysctl.conf

## To core dump a program
> \# ./program_name  
> \# ctrl + \

## find core dump in /tmp
`core.program_name.7111.175008357` is the sample file name

## debug with gdb
gdb program_name core.program_name.7111.1750083570 