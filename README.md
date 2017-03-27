# 说明

用于单片机的命令解析器。

## FEA
1. 支持退格删除.
2. 按下回车默认执行上一条指令.
3. 支持ctrl+c,ctrl+u,ctrl+h.
4. 支持tab键.

## Log 
**A The folling is the correct log. please check it. **
```
lorawan>help  
help    - print command description/usage
version - print monitor, compiler and linker version
printenv- print environment variables
setenv  - set environment variables
saveenv - save environment variables to persistent storage
reset   - reset cpu
port    - config lorawan port
adr     - adaptive speed switch
dr      - config speed
ch      - config sx1278 chan
power   - config sx1278 transmit power
mode    - config sx1278 node join mode
join    - lorawan join cmd
id      - lorawan generation mode


lorawan><INTERRUPT>
lorawan>ssss
Unknown command 'ssss' - try 'help'
lorawan>saveenv
Saving Environment to EEPROM..

lorawan>printenv
DEVADDR=CC:00:00:99
DEVEUI=C8:00:00:00:C8:00:00:22
APPEUI=11:22:33:44:55:66:77:89
APPKEY=CA:00:00:00:CA:00:00:00:CA:00:00:00:CA:00:00:88
NETID=CB:00:00:00
NWKSKEY=CD:00:00:00:CD:00:00:00:CD:00:00:00:CD:00:00:77
APPSKEY=CE:00:00:00:CE:00:00:00:CE:00:00:00:CE:00:00:00

Environment size: 282/4096 bytes
```

