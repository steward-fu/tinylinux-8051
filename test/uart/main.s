    uart_tx .equ p3.1
 
    .org 0h
    jmp _start
     
    .org 100h
_start:
    setb uart_tx
main:
    mov a, #'A'
    call uart_send_byte
    jmp main
  
    ; 115200,N,8,1
uart_send_byte:
    mov r6, #8
    clr uart_tx
    call delay_16us
u0:
    rrc a
    jc u1
    clr uart_tx
    sjmp u2
u1:
    setb uart_tx
u2:
    call delay_16us
    djnz r6, u0
    setb uart_tx
    call delay_16us
    ret
  
    ; (1 / 35000) * (1 + (4 * 72)) = 0.008257us
    ; 115200bps = 0.00868us
delay_16us:
    mov r5, #75
d3:
    djnz r5, d3
    ret
    .end
