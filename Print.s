; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; SSD1306_OutChar   outputs a single 8-bit ASCII character
; SSD1306_OutString outputs a null-terminated string 

    IMPORT   SSD1306_OutChar
    IMPORT   SSD1306_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix
    PRESERVE8
    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB

N EQU 4   ;Stores N under CNT
CNT EQU 0 ;Stores CNT directly under my LR
FP RN 11 ;Sets frame pointer as the R11 register

;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutDec
	PUSH{R11} ;Allocation phase
	SUB SP, #8 
	STR R0, [SP, #N] 
	MOV FP, SP ;makes the frame pointer equal to my stack pointer

	PUSH{LR}
	MOV R0, #0 ;Store the current CNT variable as 0 so it isn't a random value
	STR R0, [FP, #CNT]
	MOV R1, #10 ;Store 10 in R1 so I can use it for the division step

read_loop
	LDR R0, [FP, #CNT] ;Loads count from hte stack and adds 1 to it
	ADD R0, #1
	STR R0, [FP, #CNT]
	
	LDR R2, [FP, #N] ;create two copies of the input N
	LDR R3, [FP, #N]
	
	UDIV R2, R2, R1  ;divide the number by 10
	
	STR R2, [FP, #N] ;Stores the remainder into N on the stack
	MUL R2, R2, R1
	SUB R2, R3, R2
	
	PUSH{R2}  ;Stores number to be used in future on stack
	
	LDR R2, [FP, #N] ;this line checks if N is 0 and if it is then it outputs to OLED
	CMP R2, #0
	BNE read_loop
	
write_loop
	POP{R2}   ;POP R2 from the stack
	AND R0, R0, #0 ;clear R0
	ADD R0, R0, R2     ;Add r2 to 0
	ADD R0, R0, #0x30  ;then add 0x30 to make it ASCII
	
	BL SSD1306_OutChar  ;branches to Outchar which should output the character
	
	LDR R0, [FP, #CNT] ;Loads CNT from stack and decrements it
	SUB R0, #1
	STR R0, [FP, #CNT]
	
	CMP R0, #0  ;CNT to 0, if it is then it moves along, if it is higher it loops
	BHI write_loop
	
	POP{LR}  ;pops LR off the stack, adds 8, and pops R11
	ADD SP, #8
	POP{R11}

      BX  LR
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.01, range 0.00 to 9.99
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.00 "
;       R0=3,    then output "0.03 "
;       R0=89,   then output "0.89 "
;       R0=123,  then output "1.23 "
;       R0=999,  then output "9.99 "
;       R0>999,  then output "*.** "
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutFix

	PUSH{R11} ;Allocation phase
	SUB SP, #8 
	STR R0, [SP, #N] 
	MOV FP, SP ;makes the frame pointer equal to my stack pointer
	
	PUSH{LR}
	MOV R0, #0 ;Store the current CNT variable as 0 so it isn't a random value
	STR R0, [FP, #CNT]
	MOV R1, #10
	
	LDR R0, [FP, #N]
	CMP R0, #1000
	BLO INRANGE
	
	MOV R0, #0x2A
	BL SSD1306_OutChar
	
	MOV R0, #0x2E
	BL SSD1306_OutChar
	
	MOV R0, #0x2A
	BL SSD1306_OutChar
	
	MOV R0, #0x2A
	BL SSD1306_OutChar
	B Done
	
INRANGE
	LDR R0, [FP, #CNT] ;Loads count from hte stack and adds 1 to it
	ADD R0, #1
	STR R0, [FP, #CNT]
	
	LDR R2, [FP, #N] ;create two copies of the input N
	LDR R3, [FP, #N]
	
	UDIV R2, R2, R1  ;divide the number by 10
	
	STR R2, [FP, #N] ;Stores the remainder into N on the stack
	MUL R2, R2, R1
	SUB R2, R3, R2
	PUSH{R2}  ;Stores number to be used in future on stack
	
	LDR R0, [FP, #CNT] ;Loads count from hte stack and adds 1 to it
	CMP R0, #3
	BLO INRANGE
	
	POP{R2}   ;POP R2 from the stack
	AND R0, R0, #0 ;clear R0
	ADD R0, R0, R2     ;Add r2 to 0
	ADD R0, R0, #0x30  ;then add 0x30 to make it ASCII
	BL SSD1306_OutChar  ;branches to Outchar which should output the character
	
	MOV R0, #0x2E
	BL SSD1306_OutChar
	
	POP{R2}   ;POP R2 from the stack
	AND R0, R0, #0 ;clear R0
	ADD R0, R0, R2     ;Add r2 to 0
	ADD R0, R0, #0x30  ;then add 0x30 to make it ASCII
	BL SSD1306_OutChar  ;branches to Outchar which should output the character
	
	POP{R2}   ;POP R2 from the stack
	AND R0, R0, #0 ;clear R0
	ADD R0, R0, R2     ;Add r2 to 0
	ADD R0, R0, #0x30  ;then add 0x30 to make it ASCII
	BL SSD1306_OutChar  ;branches to Outchar which should output the character

Done
	POP{LR}  ;pops LR off the stack, adds 8, and pops R11
	ADD SP, #8
	POP{R11}
	BX   LR
 
     ALIGN
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN          ; make sure the end of this section is aligned
     END            ; end of file
