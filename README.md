## FEA
1. 支持退格删除.
2. 按下回车默认执行上一条指令.
3. 支持ctrl+c,ctrl+u,ctrl+h.
4. 支持tab键.

## Log 
** The folling is the correct log. please check it. **

jz>help
Usage: help cmd
Command failed, result=-1 help - help cmd

jz>help 1
exec help cmd .....
jz>help help
exec help cmd .....
jz>help 1 2
help - help cmd

jz><INTERRUPT>
jz>ssss
Unknown command 'ssss' - try 'help'
jz>help
Usage: help cmd
Command failed, result=-1 help - help cmd

jz>
jz><INTERRUPT>
jz><INTERRUPT>
jz>

